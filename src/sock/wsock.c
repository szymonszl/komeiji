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


}