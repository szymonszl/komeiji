#include "map.h"

/* this data structure is called a hash map. hash maps are somewhat
 * similar to dictionaries in c# but unlike in c# it is difficult to
 * meaningfully abstract this data structure to accomodate any data
 * type as a key in c. the way this data structure works is that it
 * creates a series of 'buckets' in a linear array, each bucket
 * containing several nodes within it. when a value is queried in the
 * map, the key is run through the hash function and produces a
 * repeatable value that can be constricted to the bounds of the
 * array to allow quick node lookup. it is typical to resize the
 * bucket count in a hashmap once the number of items in it reaches
 * a certain threshold relative to the number of buckets, or
 * KMJ_MAP_GROW_THRESHOLD * bucket_cnt. This prevents the nodes within
 * each bucket from increasing to the point that the time complexity
 * of a lookup goes from O(1) to O(n).
 */

uint32_t _hash_str(const char* str) {
    /* HASH FORMULA:
     * assume char array starts at 1
     * n: string length
     * c[i]: char as number at i
     *
     *  _n_
     *  \
     *   | 31^(n-i) * c[i]
     *  /__
     * i = 1
     */

    uint32_t out = 0;
    uint32_t scaler = 1;

    for(size_t i = strlen(str) - 1; i >= 0; --i) {
        out += scaler * str[i];
        scaler *= 31;
    }

    return out;
}

map_node_t* _bucket_resize(map_node_t* bucket) {
    // function abstracted to handle both initializing new buckets
    // and resizing existing buckets. logic begins by assuming a
    // new bucket and deviates if the passed bucket ptr is not null,
    // which indicates that there is an existing bucket to be
    // resized. when an existing bucket is resized, the work is done
    // on the existing bucket and so the return value is ignored;
    // however, on a new bucket the memory is both allocated and
    // initialized from nothing and must be assigned from the return
    // value to the bucket's memory location in the map

    int start_size = 0;
    int new_size = KMJ_MAP_BUCKET_START_SIZE;

    if(bucket != NULL) {
        start_size = 1;
        for(; !bucket->end; ++bucket) ++start_size;
        new_size = start_size * 2;
        bucket[start_size - 1].end = 0;
    }

    // realloc will behave the same as malloc when given a nullptr
    realloc(bucket, new_size);
    for(int i = start_size; i < new_size; ++i) {
        bucket[i].key = bucket[i].value = NULL;
        bucket[i].end = 0;
    }

    bucket[new_size - 1].end = 1;
    return bucket;
}

void _map_resize(map_t* map) {
    // function abstracted to handle both initializing new maps
    // and resizing existing maps. logic checks around old_bucket
    // are done to skip logic for readding existing key-value
    // pairs as a new map will have no key-value pairs

    int old_bucket_cnt = map->bucket_cnt;
    map->bucket_cnt =
        map->buckets == NULL
            ? map->bucket_cnt
            : map->bucket_cnt * map->grow_base;

    map_node_t** old_buckets = map->buckets;
    map->buckets = malloc(sizeof(map_node_t*) * map->bucket_cnt);
    for(int i = 0; i < map->bucket_cnt; ++i)
        map->buckets[i] = _bucket_resize(NULL);

    if(old_buckets == NULL)
        return;

    map->node_cnt = 0;
    for(int i = 0; i < old_bucket_cnt; ++i) {
        for(int j = 0;; ++j) {
            if(old_buckets[i][j].key != NULL) {
                map_set(map, old_buckets[i][j].key, old_buckets[i][j].value);

                // keys in old_buckets are freed as the logic
                // in map_set will behave as if no keys yet exist
                // as such the strdup in there will create new
                // memory space for the strings. this is required
                // to avoid memory leaks
                free(old_buckets[i][j].key);
            }

            if(old_buckets[i][j].end)
                break;
        }

        free(old_buckets[i]);
    }

    free(old_buckets);
}

map_t* map_create() {
    return map_create_ex(KMJ_MAP_DEFAULT_BUCKETS, KMJ_MAP_DEFAULT_GROW_BASE, 1);
}

map_t* map_create_i() {
    return map_create_ex(KMJ_MAP_DEFAULT_BUCKETS, KMJ_MAP_DEFAULT_GROW_BASE, 0);
}

map_t* map_create_ex(int bucket_cnt, int grow_base, int check_case) {
    map_t* map = malloc(sizeof(map_t));
    map->buckets = NULL;
    map->bucket_cnt = bucket_cnt;
    map->node_cnt = 0;
    map->grow_base = grow_base;
    map->check_case = check_case;

    _map_resize(map);
    return map;
}

void* map_set(map_t* map, const char* key, void* value) {
    char* key_cmp =
        (map->check_case)
            ? strdup(key)
            : str_lower(strdup(key));
    int n = _hash_str(key_cmp) % map->bucket_cnt;
    map_node_t* write_node = NULL;

    // technically a resize isn't necessary unless the key does not
    // yet exist, however the added instructions to check for this
    // when a false growth will only occur on the call to this
    // function right before it would grow from a new entry anyways
    // is not worth it. if we're within one entry from growing,
    // the map might as well grow. in environments where memory is
    // extremely scarce (<=1kb) you may want to consider performing
    // the check as at that point computational time is less expensive
    // than memory space but we will run under the assumption this is
    // run on a system with >1kb memory
    if(map->node_cnt + 1 >= map->bucket_cnt * KMJ_MAP_GROW_THRESHOLD)
        _map_resize(map);

    int i;
    for(i = 0;; ++i) {
        // because removed elements have their keys set to null, we
        // locate the first empty node to write to in case the key
        // does not yet exist; however, because within the bucket
        // items are not ordered in any way we must look through each
        // node in the bucket to determine if the key exists yet. on
        // a properly implemented hash map this will still result in
        // O(1) time for setting as the standard node count in any
        // given bucket should be close to constant

        if(map->buckets[n][i].key == NULL)
            if(write_node == NULL)
                write_node = &(map->buckets[n][i]);
        else if(strcmp(key_cmp, map->buckets[n][i].key) == 0) {
            write_node = &(map->buckets[n][i]);
            break;
        }

        if(map->buckets[n][i].end)
            break;
    }

    if(write_node == NULL) {
        _bucket_resize(map->buckets[n]);
        write_node = &(map->buckets[n][i + 1]);
    }

    void* old_data = write_node->value;
    write_node->value = value;

    if(write_node->key != NULL)
        free(key_cmp);
    else {
        // at this point we're certain this is a new node if the key
        // is null, so it is safe to increment the node count

        ++map->node_cnt;
        write_node->key = key_cmp;
    }

    return old_data;
}

map_node_t* _map_get(map_t* map, const char* key) {
    // abstracted true lookup of an individual node. because this
    // logic is repeated verbatim needlessly i consolidated it into
    // a implementation-local function for use when necessary

    char* key_cmp =
        (map->check_case)
        ? strdup(key)
        : str_lower(strdup(key));
    int n = _hash_str(key_cmp) % map->bucket_cnt;

    for(int i = 0;; ++i) {
        if(map->buckets[n][i].key != NULL
           && strcmp(key_cmp, map->buckets[n][i].key) == 0)
        {
            free(key_cmp);
            return &(map->buckets[n][i]);
        }

        if(map->buckets[n][i].end)
            break;
    }

    free(key_cmp);
    return NULL;
}

void* map_get(map_t* map, const char* key) {
    map_node_t* node;
    if((node = _map_get(map, key)) != NULL)
        return node->value;

    return NULL;
}

void* map_remove(map_t* map, const char* key) {
    map_node_t* node;
    if((node = _map_get(map, key)) != NULL) {
        free(node->key);
        node->key = NULL;
        --map->node_cnt;

        // the node's value is not set to NULL as this is never checked
        // anywhere and to set it to NULL and return the value prior to
        // this being set would require additional space on the stack
        // which is not necessary. the user is returned the contents of
        // the node so that they may free it in whatever method is required
        return node->value;
    }

    return NULL;
}

int map_has_key(map_t* map, const char* key) {
    return map_get(map, key) != NULL;
}

void map_free(map_t* map) {
    map_free_ex(map, free);
}

void map_free_ex(map_t* map, map_free_func func) {
    // called when map holds pointers that must be freed using a
    // function specific to that data type rather than free()

    for(int i = 0; i < map->bucket_cnt; ++i) {
        for(int j = 0;; ++j) {
            free(map->buckets[i][j].key);
            func(map->buckets[i][j].value);

            if(map->buckets[i][j].end)
                break;
        }

        free(map->buckets[i]);
    }

    free(map->buckets);
}