#include <string.h>
#include <iconv.h>
#include "utils/mc.h"
#include "sock/tcp.h"

mc_t *
mc_open(const char *host, uint16_t port)
{
    mc_t *mc = malloc(sizeof(mc_t));
    mc->host = strdup(host);
    mc->port = port;
    mc->ts = 0;
    mc->online = 0;
    mc->max = 0;
    mc->motd = NULL;
    mc_poll(mc);
    return mc;
}
int
mc_query(mc_t *mc)
{
    // do not go here without this open https://wiki.vg/Server_List_Ping#1.6
    int success = 0;
    tcp_t *conn = tcp_open(mc->host, mc->port, 0);
    if (!conn) goto mc_done;
    if (tcp_send_raw(conn, "\xfe", 1) < 1) goto mc_done;
    uint8_t tmp[3];
    if (tcp_recv_raw(conn, (char*)tmp, 3, KMJ_TCP_NONE) < 3) goto mc_done;
    int length = (tmp[1] << 8) | tmp[2];
    if (length > 256) length = 256;
    /* note: length is in fucking utf16 chars, which might take 2 or 4 bytes
    gonna force read length*2 bytes, assuming no BMP, but also doing a non-
    blocking read to hoover up emoji leftovers (or whatever flash puts there) */
    char raw[1024];
    int r = tcp_recv_raw(conn, raw, length*2, KMJ_TCP_WHOLE);
    if (r < 0) goto mc_done;
    r += tcp_recv_raw(conn, raw+r, 1024-r, KMJ_TCP_NO_BLOCK);
    tcp_close(conn);
    success = 1;
    // HACK time
    for (int i = 0; i < r; i++) {
        if (raw[i] == '\xa7') raw[i] = '\x1e'; // much easier to parse !
    }
    // would be neat to have this as a buffer_iconv() function except not really
    iconv_t ic = iconv_open("utf8", "utf16be"); // meme
    char resp[1024]; // eh fuck it
    char *cur = resp;
    char *in = raw;
    size_t inl = r;
    size_t outl = 1024;
    iconv(ic, &in, &inl, &cur, &outl);
    iconv(ic, 0, 0, &cur, &outl);
    *cur++ = 0; // null term
    iconv_close(ic);
    char *motd = strtok(resp, "\x1e");
    char *online = strtok(NULL, "\x1e");
    char *max = strtok(NULL, "\x1e");
    if (motd && online && max) {
        success = 2;
    }
mc_done:
    mc->ts = ts3_ts();
    if (conn) tcp_free(conn);
    if (success == 2) {
        mc->online = strtol(online, NULL, 10);
        mc->max = strtol(max, NULL, 10);
        if (!mc->motd || strcmp(motd, mc->motd) != 0) {
            if (mc->motd)
                free(mc->motd);
            mc->motd = strdup(motd);
        }
        return 0;
    } else if (success) {
        return -2;
    } else {
        return -1;
    }
}

int mc_poll(mc_t *mc) {
    if (ts3_ts() > mc->ts+30)
        return mc_query(mc);
    return 0;
}