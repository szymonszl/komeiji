#ifndef KMJ_STRING_H
#define KMJ_STRING_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

char* str_lower(char*);
char* str_upper(char*);

char* base64_encode(const char*, int length);
char* base64_encode_str(const char*);
char* base64_decode(const char*);

#endif
