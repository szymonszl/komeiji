#include "wsock.h"

struct {
    SSL_CTX* client;
} _ssl_ctx;

int ssl_init(void) {
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
