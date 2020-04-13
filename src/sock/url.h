#ifndef KMJ_URI_H
#define KMJ_URI_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "utils/string.h"
#include "utils/map.h"

#define KMJ_URL_PROTO_UNKNOWN   0
#define KMJ_URL_PROTO_HTTP      1
#define KMJ_URL_PROTO_HTTPS     2
#define KMJ_URL_PROTO_WS        3
#define KMJ_URL_PROTO_WSS       4

typedef struct {
    int proto;
    char addr[256];
    uint16_t port;
    char path[2048];
    char* query;
    char* fragment;
} url_t;

#define KMJ_URL_OK               0
#define KMJ_URL_ERR_BAD_PROTO   -1
#define KMJ_URL_ERR_BAD_ADDR    -2
#define KMJ_URL_ERR_BAD_PORT    -3
#define KMJ_URL_ERR_BAD_PATH    -4

int url_parse(const char* url, url_t* out);
map_t* url_parse_query(url_t*);

#endif
