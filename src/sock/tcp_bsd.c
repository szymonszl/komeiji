#ifndef _WIN32

#include "tcp.h"
#include "tcp_ssl.i"

tcp_t* tcp_open(const char* host, uint16_t port, int secure) {
    if(secure && !_ssl_init())
        return NULL;

    struct addrinfo hints, *results, *ptr;
    bzero((char*)&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char port_str[6];
    sprintf(port_str, "%i", port);

    if(getaddrinfo(host, port_str, &hints, &results) != 0)
        return NULL;

    __SOCK_T sock = -1;
    for(ptr = results; ptr != NULL; ptr = ptr->ai_next) {
        sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if(sock < 0) {
            freeaddrinfo(results);
            return NULL;
        }

        if(connect(sock, ptr->ai_addr, ptr->ai_addrlen) == 0)
            break;

        close(sock);
        sock = -1;
    }

    freeaddrinfo(results);
    if(sock < 0)
        return NULL;

    tcp_t* conn = malloc(sizeof(tcp_t));
    conn->sock = sock;
    conn->sock_open = 1;
    bzero(&conn->addr, sizeof(__ADDR_T));
    conn->addr_len = 0;

    if(!secure)
        conn->ssl = NULL;
    else {
        conn->ssl = SSL_new(_ssl_ctx.client);
        SSL_set_fd(conn->ssl, conn->sock);
        if(SSL_connect(conn->ssl) != 1) {
            tcp_close(conn);
            return NULL;
        }
    }

    return conn;
}

int tcp_send(tcp_t* conn, buffer_t* buffer) {
    if(!conn->sock_open)
        return -1;

    int length;
    char* data = buffer_read(buffer, &length);
    length = tcp_send_raw(conn, data, length);

    free(data);
    return length;
}

int tcp_send_raw(tcp_t* conn, const char* data, int length) {
    if(!conn->sock_open)
        return -1;

    int total_sent = 0;
    while(total_sent < length) {
        int sent = (conn->ssl == NULL)
            ? (int)send(conn->sock, data + total_sent, length - total_sent, 0)
            : (int)SSL_write(conn->ssl, data + total_sent, length - total_sent);

        if(sent <= 0) {
            tcp_close(conn);
            return -1;
        } else
            total_sent += sent;
    }

    return total_sent;
}

int tcp_recv(tcp_t* conn, buffer_t* buffer, int flags) {
    if(!conn->sock_open)
        return -1;

    char chunk[KMJ_TCP_CHUNK];

    do {
        int read = tcp_recv_raw(conn, chunk, KMJ_TCP_CHUNK, flags);

        if(read < 0)
            return read;
        else if(read > 0)
            buffer_write(buffer, chunk, read);
    } while(!(flags & KMJ_TCP_ONE_CHUNK) && tcp_is_data_ready(conn));

    return 0;
}

// if called with KMJ_TCP_NO_BLOCK, this relies on the buffer length
// to keep track of what was received between calls. if you use this
// with a buffer that doesn't start out empty you will not get the
// entire message you're expecting. if this flag is not set this will
// ignore any data in the buffer and block until the entire packet recvs
int tcp_recv_to(tcp_t* conn, buffer_t* buffer, int length, int flags) {
    if(!conn->sock_open)
        return -1;
    char chunk[KMJ_TCP_CHUNK];

    if(flags & KMJ_TCP_NO_BLOCK)
        length -= buffer_length(buffer);
    if(length <= 0)
        return 0;

    do {
        int read = tcp_recv_raw(conn, chunk,
            MIN(KMJ_TCP_CHUNK, length), flags | KMJ_TCP_WHOLE);

        if(read < 0)
            return read;
        else if(read > 0)
            buffer_write(buffer, chunk, read);

        length -= read;
    } while(length > 0);

    return 0;
}

int tcp_recv_raw(tcp_t* conn, char* data, int length, int flags) {
    if(!conn->sock_open)
        return -1;

    // nonblocking request takes precedence over whole request
    // whole request will break nonblocking order so clear it
    if(flags & KMJ_TCP_NO_BLOCK)
        flags &= ~KMJ_TCP_WHOLE;

    if((flags & KMJ_TCP_NO_BLOCK) && !tcp_is_data_ready(conn))
        return 0;

    int read;
    if(conn->ssl == NULL)
        read = (int)recv(conn->sock, data, length,
            (flags & KMJ_TCP_WHOLE) ? MSG_WAITALL : 0);
    else {
        if(flags & KMJ_TCP_WHOLE) {
            char* ptr = data;

            while(length > 0) {
                read = (int)SSL_read(conn->ssl, ptr, length);
                if(read <= 0)
                    break;

                ptr += read;
                length -= read;
            }

            read = (read > 0) ? length : read;
        } else
            read = (int)SSL_read(conn->ssl, data, length);
    }

    if(read <= 0) {
        tcp_close(conn);
        return -1;
    }

    return read;
}

int tcp_is_open(tcp_t* conn) {
    return conn->sock_open;
}

int tcp_is_secure(tcp_t* conn) {
    return conn->ssl != NULL;
}

int tcp_is_data_ready(tcp_t* conn) {
    if(!conn->sock_open)
        return 1; // i think "the sock died" is important information
                  // and should be classified as data
                  // also, keeps all if (data_ready()) read(); loops working

    char dummy;
    int check = (int)recv(conn->sock, &dummy, 1, MSG_PEEK | MSG_DONTWAIT),
        error = errno;

    if(check <= 0) {
        if(check != 0 && (error == EWOULDBLOCK || error == EAGAIN))
            return 0;
        else {
            tcp_close(conn);
            return 0;
        }
    } else {
        return 1;
    }
}

void tcp_close(tcp_t* conn) {
    if(!conn->sock_open)
        return;

    shutdown(conn->sock, SHUT_RDWR);
    close(conn->sock);
    if(conn->ssl != NULL)
        SSL_free(conn->ssl); // this one is safe to close early i think?

    conn->sock_open = 0;
}

void tcp_free(tcp_t *conn) {
    tcp_close(conn);
    free(conn);
}

#endif