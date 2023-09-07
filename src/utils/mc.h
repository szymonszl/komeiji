#ifndef KMJ_MC_H
#define KMJ_MC_H

#include <stdint.h>
#include "sock/tcp.h"

typedef struct {
    char *host;
    uint16_t port;
    double ts;

    unsigned online;
    unsigned max;
    char *motd;
} mc_t;

mc_t *mc_open(const char *host, uint16_t port);
int mc_query(mc_t *mc);
int mc_poll(mc_t *mc);

double ts3_ts(void); // used in ts3 AND mc, but defined in main

#endif