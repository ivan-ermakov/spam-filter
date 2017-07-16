#include <string.h>
#include "lib/buf.h"
#include "protocol.h"

int sf_protocol_write_request(uv_buf_t* buf, char* msg)
{
    buf_append(buf, (char*) &PROTOCOL_VER, sizeof(PROTOCOL_VER));
	int slen = strlen(msg);
	buf_append(buf, (char*) &slen, sizeof(slen));
	buf_append(buf, msg, slen);
    return 0;
}

int sf_protocol_read_response(uv_buf_t* buf, int* error, msg_type_t* msg_type)
{
    if (buf->len < 2)
		return 1;

    *error = *((unsigned char*) buf->base);
	*msg_type = *(buf->base + 1);
    
    return 0;
}