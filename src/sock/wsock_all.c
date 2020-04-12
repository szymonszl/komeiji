#include "wsock.h"

struct {
    SSL_CTX* client;
} _ssl_ctx;

int _ssl_init(void) {
    static int is_ready = 0;
    if(is_ready)
        return 0;

    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    _ssl_ctx.client = SSL_CTX_new(SSLv23_client_method());
    if(!_ssl_ctx.client)
        return -1;

    SSL_CTX_set_ecdh_auto(_ssl_ctx.client, 1);
    is_ready = 1;
    return 0;
}

#define KMJ_SOCK_PROTO_WS   0
#define KMJ_SOCK_PROTO_WSS  1


url_t* _ws_read_url(const char* url, url_t *out) {

}