#ifndef KMJ_MAP_H
#define KMJ_MAP_H

#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    char* name;
    void* value;
} map_node_t;

typedef struct {
    map_node_t** buckets;
    int bucket_cnt;
    int node_cnt;
    int check_case;
} map_t;

map_t* map_create();
map_t* map_create_i();
map_t* map_create_ex(int bucket_cnt, int check_case);

void map_free(map_t*);
void map_free_ex(map_t*, )

#endif
