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
    conn->append_ptr = conn->append_loc = NULL;

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



void tcp_close(tcp_t* conn) {
    if(!conn->sock_open)
        return;

    shutdown(conn->sock, SHUT_RDWR);
    close(conn->sock);
    if(conn->ssl != NULL)
        SSL_free(conn->ssl);

    free(conn);
}

#endif