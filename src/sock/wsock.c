#include "wsock.h"

wsock_t* wsock_open(const char* url) {
    wsock_t* conn = malloc(sizeof(wsock_t));

    if(url_parse(url, &(conn->url)) < 0) {
        free(conn);
        return NULL;
    }

    if(conn->url.proto != KMJ_URL_PROTO_WS &&
       conn->url.proto != KMJ_URL_PROTO_WSS)
    {
        free(conn);
        return NULL;
    }

    if((conn->conn = tcp_open(conn->url.host, conn->url.port,
        conn->url.proto == KMJ_URL_PROTO_WSS)) == NULL)
    {
        free(conn);
        return NULL;
    }

    buffer_t* buffer = buffer_create();
    buffer_write_str(buffer, "GET ");
    buffer_write_str(buffer, conn->url.path);
    buffer_write_str(buffer, " HTTP/1.1\r\n");

    char port[6];
    sprintf(port, "%i", conn->url.port);
    buffer_write_str(buffer, "Host: ");
    buffer_write_str(buffer, conn->url.host);
    buffer_write_str(buffer, ":");
    buffer_write_str(buffer, port);
    buffer_write_str(buffer, "\r\n");

    buffer_write_str(buffer, "Cache-Control: no-cache\r\n");
    buffer_write_str(buffer, "Accept: */*\r\n");

    buffer_write_str(buffer, "Connection: keep-alive, Upgrade\r\n");
    buffer_write_str(buffer, "Upgrade: websocket\r\n");

    char nonce[16];
    random_bytes(nonce, 16);
    char* nonce_code = base64_encode(nonce, 16);
    buffer_write_str(buffer, "Sec-Websocket-Version: 13\r\n");
    buffer_write_str(buffer, "Sec-Websocket-Key: ");
    buffer_write_str(buffer, nonce_code);
    buffer_write_str(buffer, "\r\n");
    free(nonce_code);

    buffer_write_str(buffer, "\r\n");

    tcp_send(conn->conn, buffer);
    buffer_truncate(buffer);

    int header_length;
    char* header;

    for(;;) {
        tcp_recv(conn->conn, buffer, KMJ_TCP_NONE);

        int i, found = 0;
        header = buffer_read(buffer, &header_length);
        for(i = 0; i < header_length - 3; ++i) {
            if(strncmp("\r\n\r\n", header + i, 4) == 0) {
                found = 1;
                break;
            }
        }

        if(found) {
            *(header + i + 2) = '\0';
            buffer_truncate_to(buffer, i + 4);
            break;
        } else {
            free(header);

            if(header_length > 1024) {
                buffer_free(buffer);
                tcp_close(conn->conn);
                free(conn);

                return NULL;
            }
        }
    }

    // TODO check returned nonce but i really dont care right now

    char accept[] = "HTTP/1.1 101";
    if(strncmp(accept, header, sizeof(accept) - 1) != 0) {
        buffer_free(buffer);
        tcp_close(conn->conn);
        free(header);
        free(conn);

        return NULL;
    }

    free(header);
    conn->buffer = buffer;
    return conn;
}

int wsock_send(wsock_t* conn, buffer_t* buffer) {
    char header[14], *body;
    int header_length = 2, body_length;
    body = buffer_read(buffer, &body_length);

    header[0] = 0x81;
    header[1] = 0x80;
    if(body_length < 126)
        header[1] |= body_length;
    else if(body_length <= 0xFFFF) {
        header[1] |= 126;
        num_write(header + 2, body_length, 2);
        header_length += 2;
    } else {
        header[1] |= 127;
        num_write(header + 2, body_length, 8);
        header_length += 8;
    }

    random_bytes(header + header_length, 4);
    header_length += 4;
    for(int i = 0; i < body_length; ++i)
        body[i] ^= header[header_length - 4 + (i % 4)];

    int sent = 0;
    if(tcp_send_raw(conn->conn, header, header_length) > 0)
        sent = tcp_send_raw(conn->conn, body, body_length);

    free(body);
    return sent;
}

int _wsock_recv(wsock_t* conn, buffer_t* data) {
    char header[14], pong[131], *ptr = header, *mask = NULL;
    int header_length, opcode, final;
    uint64_t body_length;

    enum {
        OP_CONTINUE = 0x0,
        OP_TEXT     = 0x1,
        OP_BINARY   = 0x2,

        OP_CLOSE    = 0x8,
        OP_PING     = 0x9,
        OP_PONG     = 0xA
    };

    if(tcp_recv_raw(conn->conn, ptr, 2, KMJ_TCP_WHOLE) < 0)
        return -1;

    final = (header[0] & 0x80) > 0;
    opcode = header[0] & 0x0F;
    ptr += 2;

    if((header[1] & 0x7F) < 126) {
        body_length = (uint64_t) (header[1] & 0x7F);
        ++ptr;
    } else if((header[1] & 0x7F) == 126) {
        if(tcp_recv_raw(conn->conn, ptr, 2, KMJ_TCP_WHOLE) < 0)
            return -1;

        body_length = num_read(ptr, 2);
        ptr += 2;
    } else {
        if(tcp_recv_raw(conn->conn, ptr, 8, KMJ_TCP_WHOLE) < 0)
            return -1;

        body_length = num_read(ptr, 8);
        if(body_length > INT32_MAX) {
            wsock_close(conn);
            return -1;
        }

        ptr += 8;
    }

    if((header[1] & 0x80) > 0) {
        mask = ptr;
        if(tcp_recv_raw(conn->conn, ptr, 4, KMJ_TCP_WHOLE) < 0)
            return -1;
    }

    switch(opcode) {
        case OP_CONTINUE:
        case OP_BINARY:
        case OP_TEXT:
            tcp_recv_to(conn->conn, data, body_length, 0);
            if(mask != NULL)
                buffer_mask(data, mask, 4);
            break;

        case OP_PING:
        case OP_PONG:
            if(!(header[0] & 0x80) || body_length > 125) {
                wsock_close(conn);
                return -1;
            }

            if(opcode == OP_PING) {
                pong[0] = 0x80 | OP_PONG;
                pong[1] = 0x80 | body_length;
                random_bytes(pong + 2, 4);

                if(body_length > 0)
                    tcp_recv_raw(conn->conn, pong + 6,
                        body_length, KMJ_TCP_WHOLE);

                if(mask != NULL)
                    for(int i = 0; i < body_length; ++i)
                        pong[i + 6] ^= mask[i % 4];

                for(int i = 0; i < body_length; ++i)
                    pong[i + 6] ^= pong[2 + (i % 4)];

                tcp_send_raw(conn->conn, pong, body_length + 6);
            } else if(body_length > 0)
                tcp_recv_raw(conn->conn, pong + 6, body_length, KMJ_TCP_WHOLE);

            return _wsock_recv(conn, data);

        case OP_CLOSE:
        default:
            wsock_close(conn);
            return -1;
    }

    return final;
}

int wsock_recv(wsock_t* conn, buffer_t* data, int blocking) {
    if (!blocking && !tcp_is_data_ready(conn->conn)) return 0;
    int status;
    while((status = _wsock_recv(conn, data)) == 0);
    
    return status;
}

void wsock_close(wsock_t* conn) {
    const char close[] = { 0x88, 0x00 };
    
    tcp_send_raw(conn->conn, close, sizeof(close));
    tcp_free(conn->conn);
    conn->conn = NULL;
}

void wsock_free(wsock_t* conn) {
    if (conn->conn)
        wsock_close(conn);
    free(conn);
}
