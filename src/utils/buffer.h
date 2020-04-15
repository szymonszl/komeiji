#ifndef KMJ_BUFFER_H
#define KMJ_BUFFER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KMJ_BUFFER_NODE_SIZE 512

#define MAX(A,B) (A > B ? A : B)
#define MIN(A,B) (A < B ? A : B)

typedef struct buffer_t {
    char data[KMJ_BUFFER_NODE_SIZE];
    int length;

    struct buffer_t* next;
} buffer_t;

buffer_t* buffer_create();

void buffer_write(buffer_t*, char* data, int length);

char* buffer_read(buffer_t*, int* length);
int buffer_length(buffer_t*);

buffer_t* buffer_free(buffer_t*);

#endif
