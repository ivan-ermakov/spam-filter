#include <stdlib.h>
#include <string.h>
#include "lib/buf.h"
#include "protocol.h"

int sf_protocol_read_request(uv_buf_t* buf, char** msg)
{
    if (buf->len < 1)
		return 1;

    int protocol_ver = *((unsigned char*) buf->base);
	
	if (protocol_ver != PROTOCOL_VER)
	{
		fprintf(stderr, "Protocol version mismatch: %d\n", protocol_ver);
		return -1;
	}
	
	if (buf->len < 5)
		return 1;
	
	int msg_len = *((int*) (buf->base + 1));
	fprintf(stderr, "Msg len: %d\n", msg_len);
	
	if (buf->len < msg_len + 5)
		return 1;
    
    *msg = calloc(msg_len + 1, 1);
    if (!*msg)
        return -2;

    memcpy(*msg, buf->base + 5, msg_len);
	(*msg)[msg_len] = '\0';

    return 0;
}

int sf_protocol_write_response(uv_buf_t* buf, unsigned char error, msg_type_t msg_type)
{
    buf_append(buf, (char*) &error, sizeof(error));
	buf_append(buf, (char*) &msg_type, sizeof(msg_type));

    return 0;
}