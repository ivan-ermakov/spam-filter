#include <stdlib.h>
#include "sf.h"

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}
