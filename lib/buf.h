#ifndef LIB_BUF_H
#define LIB_BUF_H

#include <uv.h>

int buf_append(uv_buf_t* b, char* data, size_t size);

#endif