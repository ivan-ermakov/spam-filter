#ifndef LIB_SF_H
#define LIB_SF_H

#include <uv.h>

typedef enum
{
	MSG_TYPE_HAM = 0,
	MSG_TYPE_SPAM
} msg_type_t;

enum
{
	PROTOCOL_VER = 0,
	DEFAULT_PORT = 6000
}

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);

#endif
