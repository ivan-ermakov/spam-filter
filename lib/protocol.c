#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "lib/buf.h"
#include "protocol.h"

enum { SF_PROTOCOL_HEADER_SIZE = 5 };

/* Client */

int sf_protocol_write_request(uv_buf_t* buf, char* msg)
{
	uint8_t protocol_ver = PROTOCOL_VER;
    buf_append(buf, (char*) &protocol_ver, sizeof(protocol_ver));
	uint32_t slen = strlen(msg);
	buf_append(buf, (char*) &slen, sizeof(slen));
	buf_append(buf, msg, slen);
    return SF_OK;
}

int sf_protocol_read_response(uv_buf_t* buf, int* error, msg_type_t* msg_type)
{
    if (buf->len < 2)
		return SF_PROTOCOL_INCOMPLETE_MSG;

    *error = *((uint8_t*) buf->base);
	*msg_type = *(buf->base + 1);
    
    return SF_PROTOCOL_COMPLETE_MSG;
}

/* Server */

int sf_protocol_read_request(uv_buf_t* buf, char** msg)
{
    if (buf->len < 1)
		return SF_PROTOCOL_INCOMPLETE_MSG;

    int protocol_ver = *((uint8_t*) buf->base);
	
	if (protocol_ver != PROTOCOL_VER)
	{
		fprintf(stderr, "Protocol version mismatch: %d\n", protocol_ver);
		return SF_PROTOCOL_VERSION_MISMATCH;
	}
	
	if (buf->len < SF_PROTOCOL_HEADER_SIZE)
		return SF_PROTOCOL_INCOMPLETE_MSG;
	
	uint32_t msg_len = *((uint32_t*) (buf->base + 1));
	fprintf(stderr, "Msg len: %d\n", msg_len);
	
	if (buf->len < msg_len + SF_PROTOCOL_HEADER_SIZE)
		return SF_PROTOCOL_INCOMPLETE_MSG;
    
    *msg = calloc(msg_len + 1, 1);
    if (!*msg)
        return SF_FAIL;

    memcpy(*msg, buf->base + SF_PROTOCOL_HEADER_SIZE, msg_len);
	(*msg)[msg_len] = '\0';

    return SF_PROTOCOL_COMPLETE_MSG;
}

int sf_protocol_write_response(uv_buf_t* buf, unsigned char error, msg_type_t msg_type)
{
    buf_append(buf, (char*) &error, sizeof(error));
	buf_append(buf, (char*) &msg_type, sizeof(msg_type));

    return SF_OK;
}