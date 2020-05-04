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

    out->host[0] = '\0';
    strcpy(out->path, "/");
    out->query = NULL;
    out->fragment = NULL;

    enum { HOST, PORT, PATH, QUERY, FRAGMENT };
    int state = HOST;
    char* to = out->host;
    char port[6] = {0, 0, 0, 0, 0, 1};

    for(; *ptr != '\0'; ++ptr) {
        if(state == HOST) {
            if(*ptr == ':') {
                *to = '\0';
                to = port;
                state = PORT;
            } else if(*ptr == '/' || *ptr == '?' || *ptr == '#') {
                *to = '\0';
                to = out->path + 1;
                state = PATH;

                if(*ptr == '?') {
                    state = QUERY;
                    out->query = to;
                    *(to++) = *ptr;
                } else if(*ptr == '#') {
                    state = FRAGMENT;
                    out->fragment = to;
                    *(to++) = *ptr;
                }
            } else
                *(to++) = *ptr;
        } else if(state == PORT) {
            if(!isdigit(*ptr) && *ptr != '/' && *ptr != '?' && *ptr != '#')
                return KMJ_URL_ERR_BAD_PORT;
            else if(*ptr == '/' || *ptr == '?' || *ptr == '#') {
                *to = '\0';
                to = out->path + 1;
                state = PATH;

                if(*ptr == '?') {
                    state = QUERY;
                    out->query = to;
                    *(to++) = *ptr;
                } else if(*ptr == '#') {
                    state = FRAGMENT;
                    out->fragment = to;
                    *(to++) = *ptr;
                }
            } else {
                if(*to == 1)
                    return KMJ_URL_ERR_BAD_PORT;
                *(to++) = *ptr;
            }
        } else {
            if(*ptr == ':')
                return KMJ_URL_ERR_BAD_PATH;

            if(state == PATH) {
                *(to++) = *ptr;

                if(*ptr == '?') {
                    state = QUERY;
                    out->query = to - 1;
                } else if(*ptr == '#') {
                    state = FRAGMENT;
                    out->fragment = to - 1;
                }
            } else {
                if(*ptr == '/' || *ptr == '?')
                    return KMJ_URL_ERR_BAD_PATH;

                if(state == QUERY) {
                    *(to++) = *ptr;
                    if(*ptr == '#') {
                        state = FRAGMENT;
                        out->fragment = to - 1;
                    }
                } else if(state == FRAGMENT) {
                    if(*ptr == '#')
                        return KMJ_URL_ERR_BAD_PATH;

                    *(to++) = *ptr;
                }
            }
        }
    }

    if(state == PATH || state == QUERY || state == FRAGMENT)
        *to = '\0';
    if(port[0] != '\0')
        out->port = (uint16_t)atoi(port);

    return KMJ_URL_OK;
}

map_t* url_parse_query(url_t* url) {


    return NULL;
}