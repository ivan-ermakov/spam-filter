#ifndef LIB_BUF_H
#define LIB_BUF_H

#include <uv.h>

typedef struct buf_s
{
	char* base;
	int len;
	int pos;
} buf_t;

int buf_append(uv_buf_t* b, char* data, size_t size);

#endif