#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ts3/ts3.h"
#include "sock/tcp.h"

char *
_ts3_read_line(ts3_t *conn)
{
    for (;;) {
        char *line = buffer_read_line(conn->buf);
        if (line)
            return line;
        tcp_recv(conn->conn, conn->buf, KMJ_TCP_NONE);
    }
}

static int
ensure_open(ts3_t *conn)
{
    if (conn->conn) {
        if (tcp_is_open(conn->conn))
            return 1;
        tcp_free(conn->conn);
    }
    conn->conn = tcp_open(conn->host, 10011, 0);
    if (!conn->conn)
        return 0;
    char *line = _ts3_read_line(conn);
    if (!line)
        goto fail;
    if (0 != strcmp(line, "TS3")) {
        free(line);
        goto fail;
    }
    free(line);
    if (!(line = _ts3_read_line(conn))) // "motd"
        goto fail;
    free(line);
    char login[256];
    snprintf(login, 256, "login %s %s", conn->user, conn->pass);
    ts3_resp *r = _ts3_query(conn, login);
    if (!ts3_issuccess(r)) {
        ts3_freeresp(r);
        goto fail;
    }
    ts3_freeresp(r);
    r = _ts3_query(conn, "use 1");
    if (!ts3_issuccess(r)) {
        ts3_freeresp(r);
        goto fail;
    }
    ts3_freeresp(r);
    return 1;
fail:
    tcp_send_raw(conn->conn, "quit\n", 5);
    tcp_close(conn->conn);
    return 0;
}

ts3_t *
ts3_open(const char *host, const char *user, const char *pass)
{
    ts3_t *conn = malloc(sizeof(ts3_t));
    conn->host = strdup(host);
    conn->user = strdup(user);
    conn->pass = strdup(pass);
    conn->conn = NULL;
    conn->buf = buffer_create();
    conn->ts = 0;
    ensure_open(conn);
    return conn;
}

void
ts3_close(ts3_t *conn) {
    tcp_send_raw(conn->conn, "quit\n", 5);
    tcp_close(conn->conn);
    free(conn->host);
    free(conn->user);
    free(conn->pass);
    buffer_free(conn->buf);
    free(conn);
}

ts3_resp *
_ts3_query(ts3_t *conn, const char *query)
{
    tcp_send_raw(conn->conn, query, strlen(query));
    char nl = '\n';
    tcp_send_raw(conn->conn, &nl, 1);
    ts3_resp *r = _ts3_read_resp(conn);
    return r;
}

ts3_resp *
ts3_query(ts3_t *conn, const char *query)
{
    if (!ensure_open(conn))
        return NULL;
    return _ts3_query(conn, query);
}
