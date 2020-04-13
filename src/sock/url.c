#include "url.h"

typedef struct {
    int id;
    char* prefix;
    uint16_t port;
} proto_t;

int url_parse(const char* uri, url_t* out) {
    const char* ptr = uri;

    proto_t protos[] = {
        {KMJ_URL_PROTO_HTTP,    "http://",  80},
        {KMJ_URL_PROTO_HTTPS,   "https://", 443},
        {KMJ_URL_PROTO_WS,      "ws://",    80},
        {KMJ_URL_PROTO_WSS,     "wss://",   443},
    };

    out->proto = KMJ_URL_PROTO_UNKNOWN;
    char* uri_lc = str_lower(strdup(uri));
    for(int i = 0; i < (sizeof(protos) / sizeof(*protos)); ++i) {
        if(strncmp(uri_lc, protos[i].prefix, strlen(protos[i].prefix)) == 0) {
            out->proto = protos[i].id;
            out->port = protos[i].port;
            ptr += strlen(protos[i].prefix);

            break;
        }
    }
    free(uri_lc);

    if(out->proto == KMJ_URL_PROTO_UNKNOWN)
        return KMJ_URL_ERR_BAD_PROTO;

    out->addr[0] = '\0';
    strcpy(out->path, "/");
    out->query = NULL;
    out->fragment = NULL;

    enum { ADDR, PORT, PATH, QUERY, FRAGMENT };
    int state = ADDR;

    for(; ptr != '\0'; ++ptr) {

    }

    return KMJ_URL_OK;
}

map_t* url_parse_query(url_t* url) {


    return NULL;
}