#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <uv.h>
#include "lib/sf.h"

enum
{
	SF_PROTOCOL_VERSION_MISMATCH = -1,
	SF_FAIL = -2,
	SF_OK = 0,
	SF_PROTOCOL_COMPLETE_MSG = 0,
	SF_PROTOCOL_INCOMPLETE_MSG = 1
};

/* Client */

int sf_protocol_write_request(uv_buf_t* buf, char* msg);
int sf_protocol_read_response(uv_buf_t* buf, int* error, msg_type_t* msg_type);

/* Server */

int sf_protocol_read_request(uv_buf_t* buf, char** msg);
int sf_protocol_write_response(uv_buf_t* buf, unsigned char error, msg_type_t msg_type);

#endif