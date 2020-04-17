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

#define B64_PADDING 0x00
#define B64_IGNORE  0xFF

char* base64_encode(const char* str, int length) {
    const char table[]
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    int out_length = 1 + (1 + (length / 3)) * 4;
    out_length += 2 * (out_length / 76);

    char *out = malloc(out_length * sizeof(char)),
         *ptr = out;

    for(int i = 0; i < length; i += 3) {
        if(i > 0 && ((i / 3) * 4) % 76 == 0) {
            *(ptr++) = '\r';
            *(ptr++) = '\n';
        }

        int part = (str[i] << 16)
            | ((i + 1 >= length) ? 0 : (str[i + 1] << 8))
            | ((i + 2 >= length) ? 0 : str[i + 2]);

        *(ptr++) = table[(part >> 18) & 0x3F];
        *(ptr++) = table[(part >> 12) & 0x3F];
        *(ptr++) = (i + 1 >= length)
            ? '=' : table[(part >> 6) & 0x3F];
        *(ptr++) = (i + 2 >= length)
            ? '=' : table[part & 0x3F];
    }
    *ptr = '\0';

    return out;
}

char* base64_encode_str(const char* str) {
    return base64_encode(str, strlen(str));
}

int _base64_decode_char(char c) {
    if(c >= 'A' && c <= 'Z')
        return c - 'A';
    else if(c >= 'a' && c <= 'z')
        return c - 'a' + 26;
    else if(c >= '0' && c <= '9')
        return c - '0' + 52;
    else if(c == '+')
        return 62;
    else if(c == '/')
        return 63;
    else if(c == '=')
        return B64_PADDING;
    else
        return B64_IGNORE;
}

char* base64_decode(const char* str) {
    char* code = malloc((strlen(str) + 1) * sizeof(char)),
        * ptr_out = code;
    const char* ptr_in = str;

    do {
        if(_base64_decode_char(*ptr_in) != B64_IGNORE || *ptr_in == '\0')
            *(ptr_out++) = *ptr_in;
    } while(*(ptr_in++) != '\0');

    int code_length = strlen(code);
    if(code_length % 4 != 0) {
        free(code);
        return NULL;
    }

    char* out = malloc(((code_length / 4) * 3 + 1) * sizeof(char));
    ptr_out = out;
    for(int i = 0; i < code_length; i += 4) {
        if(code[i] == '=' || code[i + 1] == '='
            || (code[i + 2] == '=' && code[i + 3] != '=')
            || (i + 4 < code_length &&
                (code[i + 2] == '=' || code[i + 3] == '=')))
        {
            free(out);
            free(code);
            return NULL;
        }

        int part =
              (_base64_decode_char(code[i])     << 18)
            | (_base64_decode_char(code[i + 1]) << 12)
            | (_base64_decode_char(code[i + 2]) << 6)
            | (_base64_decode_char(code[i + 3]));

        *(ptr_out++) = (part >> 16) & 0xFF;
        if(code[i + 2] != '=')
            *(ptr_out++) = (part >> 8) & 0xFF;
        if(code[i + 3] != '=')
            *(ptr_out++) = part & 0xFF;
    }

    *ptr_out = '\0';
    free(code);
    return out;
}