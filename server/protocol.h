#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <uv.h>
#include "lib/sf.h"

int sf_protocol_read_request(uv_buf_t* buf, char** msg);
int sf_protocol_write_response(uv_buf_t* buf, unsigned char error, msg_type_t msg_type);

#endif