#ifndef KMJ_BUFFER_H
#define KMJ_BUFFER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KMJ_BUFFER_NODE_SIZE 256

#define MAX(A,B) (A > B ? A : B)
#define MIN(A,B) (A < B ? A : B)

typedef struct buffer_t {
    char data[KMJ_BUFFER_NODE_SIZE];
    int length;

    struct buffer_t* next;
} buffer_t;

buffer_t* buffer_create();

void buffer_write(buffer_t*, const char* data, int length);
void buffer_write_str(buffer_t*, const char* data);

char* buffer_read(buffer_t*, int* length);
char* buffer_read_str(buffer_t*);

int buffer_length(buffer_t*);
void buffer_mask(buffer_t*, char* mask, int length);

void buffer_truncate(buffer_t*);
void buffer_truncate_to(buffer_t*, int to);

void buffer_free(buffer_t*);

#endif
