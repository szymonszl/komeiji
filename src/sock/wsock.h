#ifndef KMJ_WSOCK_H
#define KMJ_WSOCK_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "sock/tcp.h"
#include "sock/url.h"
#include "utils/buffer.h"

typedef struct {
    tcp_t* conn;
    url_t url;
    buffer_t* buffer;
} wsock_t;

wsock_t* wsock_open(const char* url);

int wsock_send(wsock_t*, buffer_t*);
int wsock_recv(wsock_t*, buffer_t*);

void wsock_close(wsock_t*);

#endif
