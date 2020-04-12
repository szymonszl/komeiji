#ifndef KMJ_URL_H
#define KMJ_URL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define KMJ_URL_PROTO_NA    0
#define KMJ_URL_PROTO_HTTP  1
#define KMJ_URL_PROTO_HTTPS 2
#define KMJ_URL_PROTO_WS    3
#define KMJ_URL_PROTO_WSS   4

typedef struct {
    int proto;
    char addr[256];
    uint16_t port;
    char path[2048];
    char* query;
    char* fragment;
} url_t;

url_t* url_parse(const char* url, url_t* out);

#endif
