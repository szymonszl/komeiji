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
        for(i = 0; i < header_length - 4; ++i) {
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

    char accept[] = "HTTP/1.1 101 Switching Protocols\r\n";
    if(strncmp(accept, header, sizeof(accept)) != 0) {
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
    int header_length = 6, body_length;
    body = buffer_read(buffer, &body_length);

    header[0] = 0x2;
    
}

void wsock_close(wsock_t* conn) {

}