#ifndef KMJ_TS3_H
#define KMJ_TS3_H

#include <stdint.h>
#include "sock/tcp.h"

typedef struct {
    char *host;
    char *user;
    char *pass;

    tcp_t *conn;
    buffer_t *buf;
    double ts;
} ts3_t;

typedef struct _ts3_kv {
    char *k;
    char *v;
    struct _ts3_kv *next;
} ts3_kv;

typedef struct _ts3_record {
    ts3_kv *params;
    struct _ts3_record *next;
} ts3_record;

typedef struct {
    struct ts3__arena *__arena;
    int errid;
    ts3_record *records;
} ts3_resp;

ts3_t *ts3_open(const char *host, const char *user, const char *pass);
void ts3_close(ts3_t *conn);

ts3_resp *ts3_query(ts3_t *conn, const char *query);
int ts3_issuccess(ts3_resp *resp);
void ts3_freeresp(ts3_resp *resp);

ts3_resp *ts3_idlepoll(ts3_t *conn);

ts3_resp *_ts3_query(ts3_t *conn, const char *query);
ts3_resp *_ts3_read_resp(ts3_t *conn);
char *_ts3_read_line(ts3_t *conn);

double ts3_ts(void); // used in ts3, but defined in main

#endif