#include <string.h>
#include <stdlib.h>
#include "ts3/ts3.h"

void
tsn_push(struct tsn_cache *cache, const char *id, const char *nick)
{
    for (struct tsn_cache *c = cache; c; c = c->next) {
        for (int i = 0; i < TSN_N; i++) {
            if (!c->ids[i]) {
                c->ids[i] = strdup(id);
                c->nicks[i] = strdup(nick);
                return;
            }
        }
    }
    while (cache->next)
        cache = cache->next;
    cache->next = calloc(1, sizeof(struct tsn_cache));
    cache->next->ids[0] = strdup(id);
    cache->next->nicks[0] = strdup(nick);
}
char *
tsn_pull(struct tsn_cache *cache, const char *id)
{
    char *nick = NULL;
    for (struct tsn_cache *c = cache; c; c = c->next) {
        for (int i = 0; i < TSN_N; i++) {
            if (c->ids[i] && 0 == strcmp(c->ids[i], id)) {
                nick = c->nicks[i];
                free(c->ids[i]);
                c->ids[i] = NULL;
                c->nicks[i] = NULL;
                goto chk;
            }
        }
    }
chk:
    ;
    struct tsn_cache *last = cache;
    while (last->next)
        last = last->next;
    for (int i = 0; i < TSN_N; i++) {
        if (last->ids[i])
            return nick;
    }
    if (last != cache) free(last);
    return nick;
}