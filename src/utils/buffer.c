#include "buffer.h"

buffer_t* buffer_create() {
    buffer_t* buffer = malloc(sizeof(buffer_t));
    buffer->length = 0;
    buffer->next = NULL;

    return buffer;
}

void buffer_write(buffer_t* buffer, const char* data, int length) {
    if(buffer->next != NULL) {
        buffer_write(buffer->next, data, length);
        return;
    }

    int written = MIN(length, KMJ_BUFFER_NODE_SIZE - buffer->length);
    memcpy(buffer->data + buffer->length, data, written);
    buffer->length += written;
    length -= written;

    if(length > 0)
        buffer_write(buffer->next = buffer_create(), data + written, length);
}

void buffer_write_str(buffer_t* buffer, const char* data) {
    buffer_write(buffer, data, strlen(data));
}

void __buffer_read(buffer_t* buffer, char* data) {
    memcpy(data, buffer->data, buffer->length);
    if(buffer->next != NULL)
        __buffer_read(buffer->next, data + buffer->length);
}

char* _buffer_read(buffer_t* buffer, int* length, int null_term) {
    int _length = buffer_length(buffer) + (null_term ? 1 : 0);
    if(length != NULL)
        *length = _length;

    char* data = malloc(sizeof(char) * _length);
    __buffer_read(buffer, data);
    if(null_term)
        data[_length - 1] = '\0';

    return data;
}

char* buffer_read(buffer_t* buffer, int* length) {
    return _buffer_read(buffer, length, 0);
}

char* buffer_read_str(buffer_t* buffer) {
    return _buffer_read(buffer, NULL, 1);
}

char* buffer_read_line(buffer_t* buffer) {
    char *nl = NULL;
    buffer_t *lastbuf = NULL;
    int len = 0;
    for (buffer_t* cur = buffer; cur != NULL; cur = cur->next) {
        nl = memchr(cur->data, '\n', cur->length);
        if (nl) {
            lastbuf = cur;
            len += nl - cur->data;
            break;
        } else {
            len += cur->length;
        }
    }
    if (nl == NULL)
        return NULL;

    char *out = malloc(len+1);
    int outw = 0;
    for (buffer_t* cur = buffer; cur != NULL; cur = cur->next) {
        if (cur != lastbuf) {
            memcpy(out+outw, buffer->data, buffer->length);
            outw += buffer->length;
        } else {
            memcpy(out+outw, buffer->data, nl - buffer->data);
            out[len] = '\0';
            buffer_truncate_to(buffer, len+1);
            return out;
        }
    }
}

int buffer_length(buffer_t* buffer) {
    return buffer->length +
        ((buffer->next == NULL) ? 0 : buffer_length(buffer->next));
}

void buffer_mask(buffer_t* buffer, char* mask, int length) {
    buffer_t* ptr = buffer;
    int n = 0;

    while(ptr != NULL) {
        for(int i = 0; i < ptr->length; ++i) {
            ptr->data[i] ^= mask[n];
            n = (n + 1) % length;
        }

        ptr = buffer->next;
    }
}

void buffer_truncate(buffer_t* buffer) {
    if(buffer->next != NULL)
        buffer_free(buffer->next);

    buffer->next = NULL;
    buffer->length = 0;
}

void _buffer_truncate_to(buffer_t* origin, buffer_t* buffer, int to) {
    to -= buffer->length;

    if(to <= 0) {
        int left;
        to += buffer->length;
        char* data = buffer_read(buffer, &left);
        buffer_free(origin == buffer ? buffer->next : buffer);

        origin->length = 0;
        origin->next = NULL;
        buffer_write(origin, data + to, left - to);
    } else {
        buffer_t* next = buffer->next;
        if(origin != buffer)
            free(buffer);

        if(next == NULL) {
            origin->next = NULL;
            origin->length = 0;
        } else
            _buffer_truncate_to(origin, next, to);
    }
}

void buffer_truncate_to(buffer_t* buffer, int to) {
    _buffer_truncate_to(buffer, buffer, to);
}

void buffer_free(buffer_t* buffer) {
    if(buffer == NULL)
        return;
    if(buffer->next != NULL)
        buffer_free(buffer->next);
    free(buffer);
}
