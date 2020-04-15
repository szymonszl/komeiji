#include "buffer.h"

buffer_t* buffer_create() {
    buffer_t* buffer = malloc(sizeof(buffer));
    buffer->length = 0;
    buffer->next = NULL;

    return buffer;
}

void buffer_write(buffer_t* buffer, char* data, int length) {
    if(buffer->next != NULL)
        buffer_write(buffer->next, data, length);

    int written = MIN(length, KMJ_BUFFER_NODE_SIZE - buffer->length);
    memcpy(buffer->data + buffer->length, data, written);
    buffer->length += written;
    length -= written;

    if(length > 0)
        buffer_write(buffer->next = buffer_create(), data + written, length);
}

void _buffer_read(buffer_t* buffer, char* data) {
    memcpy(data, buffer->data, buffer->length);
    if(buffer->next != NULL)
        _buffer_read(buffer->next, data + buffer->length);
}

char* buffer_read(buffer_t* buffer, int* length) {
    int _length = buffer_length(buffer);
    if(length != NULL)
        *length = _length;

    char* data = malloc(sizeof(char) * _length);
    _buffer_read(buffer, data);

    return data;
}

int buffer_length(buffer_t* buffer) {
    return buffer->length +
        (buffer->next == NULL) ? 0 : buffer_length(buffer->next);
}

buffer_t* buffer_free(buffer_t* buffer) {
    if(buffer->next != NULL)
        buffer_free(buffer->next);
    free(buffer);
}