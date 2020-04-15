#ifndef KMJ_MAP_H
#define KMJ_MAP_H

#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include "utils/string.h"

#define KMJ_MAP_GROW_THRESHOLD      1.25
#define KMJ_MAP_BUCKET_START_SIZE   4
#define KMJ_MAP_DEFAULT_GROW_BASE   2
#define KMJ_MAP_DEFAULT_BUCKETS     16

typedef struct {
    char* key;
    void* value;
    int end;
} map_node_t;

typedef struct {
    map_node_t** buckets;
    int bucket_cnt;
    int node_cnt;
    int grow_base;
    int check_case;
} map_t;

typedef void(*map_free_func)(void*);

map_t* map_create();
map_t* map_create_i();
map_t* map_create_ex(int bucket_cnt, int grow_base, int check_case);

void* map_set(map_t*, const char* key, void* value);
void* map_get(map_t*, const char* key);
void* map_remove(map_t*, const char* key);
int   map_has_key(map_t*, const char* key);

void map_free(map_t*);
void map_free_ex(map_t*, map_free_func);

#endif
