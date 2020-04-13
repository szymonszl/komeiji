#include "string.h"

char* str_lower(char* str) {
    for(int i = 0; i < strlen(str); ++i)
        str[i] = (char)tolower(str[i]);
    return str;
}

char* str_upper(char* str) {
    for(int i = 0; i < strlen(str); ++i)
        str[i] = (char)toupper(str[i]);
    return str;
}