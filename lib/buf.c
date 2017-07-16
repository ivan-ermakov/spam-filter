#include <stdlib.h>
#include <string.h>
#include "buf.h"

void buf_init(buf_t* b, char* data, size_t size)
{
    b->base = data;
    b->len = size;
    b->pos = size;
}

void buf_free(buf_t* b)
{
    free(b->base);
}

int buf_append(uv_buf_t* b, char* data, size_t size)
{
    b->base = realloc(b->base, b->len + size);

    if (!b->base)
        return -1;

    memcpy(b->base + b->len, data, size);
    b->len += size;
    return 0;
}