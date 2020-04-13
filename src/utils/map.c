#include "map.h"

uint32_t _hash_str(const char* str) {
    uint32_t out = 0;
    uint32_t scaler = 0;

    for(size_t i = strlen(str) - 1; i >= 0; --i) {
        out += (scaler == 0 ? 1 : scaler) * str[i];
        scaler = scaler == 0 ? 31 : scaler * 31;
    }

    return out;
}

