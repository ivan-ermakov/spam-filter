#include <stdlib.h>
#include "sf.h"

const unsigned char PROTOCOL_VER = 0;
const short DEFAULT_PORT = 6000;

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}
