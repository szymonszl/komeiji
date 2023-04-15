#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "ts3/ts3.h"
#include "sock/tcp.h"
#include "utils/string.h"

#define ARENA_LEN 1024

struct ts3__arena {
    struct ts3__arena *next;
    unsigned int usage;
    char buf[ARENA_LEN];
};

static void *
mallocA(struct ts3__arena *arena, size_t sz)
{
    if (arena->usage + sz > ARENA_LEN) {
        if (!arena->next)
            arena->next = calloc(1, sizeof(struct ts3__arena));
        return mallocA(arena->next, sz);
    }
    void *r = arena->buf + arena->usage;
    arena->usage += (sz+7) & (~7);
    return r;
}
static char *
strdupA(struct ts3__arena *a, const char *s)
{
    size_t len = strlen(s);
    char *s2 = mallocA(a, len+1);
    strcpy(s2, s);
    return s2;
}

void
ts3_freeresp(ts3_resp *resp)
{
    struct ts3__arena *a = resp->__arena;
    while (a) {
        struct ts3__arena *n = a->next;
        free(a);
        a = n;
    }
}

int
ts3_issuccess(ts3_resp *resp)
{
    return resp->errid == 0;
}

char *
unescape(struct ts3__arena *a, const char *s)
{
    char *out = mallocA(a, strlen(s)+1);
    // unescaped will be shorter than original
    char *cur = out;
    for (;;) {
        char c = *s++;
        if (c == '\\') {
            switch (*s++) {
                case '\\': c = '\\'; break;
                case '/': c = '/'; break;
                case 's': c = ' '; break;
                case 'p': c = '|'; break;
                case 'a': c = '\a'; break;
                case 'b': c = '\b'; break;
                case 'f': c = '\f'; break;
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                case 'v': c = '\v'; break;
            }
        }
        *cur++ = c;
        if (c == '\0')
            return out;
    }
}

ts3_record *
parse_params(struct ts3__arena *a, char *params)
{
    char *srecord, *ssrecord;
    ts3_record *recs = NULL;
    for (srecord = strtok_r(params, "|", &ssrecord); srecord; srecord = strtok_r(NULL, "|", &ssrecord)) {
        ts3_record *rec = mallocA(a, sizeof(ts3_record));
        char *skv, *sskv;
        ts3_kv *kvs = NULL;
        for (skv = strtok_r(srecord, " ", &sskv); skv; skv = strtok_r(NULL, " ", &sskv)) {
            ts3_kv *kv = mallocA(a, sizeof(ts3_kv));
            char *eq = strchr(skv, '=');
            if (eq) {
                *eq = 0;
                kv->k = strdupA(a, skv);
                kv->v = unescape(a, eq+1);
            } else {
                kv->k = strdupA(a, skv);
                kv->v = strdupA(a, "");
            }
            kv->next = kvs;
            kvs = kv;
        }
        rec->params = kvs;
        rec->next = recs;
        recs = rec;
    }
    return recs;
}

static ts3_resp *
newresp(void)
{
    struct ts3__arena *a = calloc(1, sizeof(struct ts3__arena));
    ts3_resp *r = mallocA(a, sizeof(ts3_resp));
    r->__arena = a;
    return r;
}

ts3_resp *
_ts3_read_resp(ts3_t *conn)
{
    ts3_resp *r = newresp();
    struct ts3__arena *a = r->__arena;
    for (;;) {
        char *line = _ts3_read_line(conn);
        if (str_prefix(line, "error")) {
            char *part, *s;
            for (part = strtok_r(line, " ", &s); part; part = strtok_r(NULL, " ", &s)) {
                if (sscanf(part, "id=%u", &r->errid) == 1)
                    break;
            }
            free(line);
            break;
        }
        if (str_prefix(line, "notify")) {
            conn->stash = newresp();
            char *space = strchr(line, ' ');
            *space = 0;
            conn->stash->desc = strdupA(conn->stash->__arena, line+6);
            conn->stash->records = parse_params(conn->stash->__arena, space+1);
        }
        r->records = parse_params(a, line);
        free(line);
    }
    conn->ts = ts3_ts();
    return r;
}

ts3_resp *
_ts3_read_push(ts3_t *conn)
{
    char *line = _ts3_read_line(conn);
    ts3_resp *r = NULL;
    if (str_prefix(line, "notify")) {
        r = newresp();
        char *space = strchr(line, ' ');
        *space = 0;
        r->desc = strdupA(r->__arena, line+6);
        r->records = parse_params(r->__arena, space+1);
    } else {
        fprintf(stderr, "[T] unexpected line [%s]\n", line);
    }
    free(line);
    conn->ts = ts3_ts();
    return r;
}

char *
ts3_getval(ts3_record *rec, const char *key)
{
    for (ts3_kv *kv = rec->params; kv; kv = kv->next) {
        if (0 == strcmp(kv->k, key))
            return kv->v;
    }
    return NULL;
}