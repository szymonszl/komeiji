#ifndef KMJ_STRING_H
#define KMJ_STRING_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>

char* str_lower(char*);
char* str_upper(char*);

int str_prefix(const char*, const char*);

char* base64_encode(const char*, int length);
char* base64_encode_str(const char*);
char* base64_decode(const char*);

char* random_bytes(char*, int length);

void num_write(char*, uint64_t, int bytes);
uint64_t num_read(const char*, int bytes);

#endif
