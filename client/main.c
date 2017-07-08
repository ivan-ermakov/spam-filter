#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include "sf.h"

char* msg = NULL;

void on_read(uv_stream_t* server, ssize_t nread, const uv_buf_t* buf)
{
	if (nread < 0)
	{
		fprintf(stderr, "Read error: %s\n", uv_strerror(nread));
		uv_close((uv_handle_t*) server, NULL);
		return;
	}
	else if (nread == 0)
		return;
		
	buf_t* b = (buf_t*) server->data;
	if (b->len - b->pos < nread)
	{
		char* m = (char*) malloc(nread + b->pos);
		if (!m)
		{
			uv_handle_t* handle = (uv_handle_t*) server;
			buf_t* buf = (buf_t*) handle->data;
			free(buf->base);
			free(buf);
			uv_close(handle, NULL);
			return;
		}
		
		memcpy(m, b->base, b->len);
		free(b->base);
		b->base = m;
		b->len = nread + b->pos;
	}	
	
	memcpy(b->base + b->pos, buf->base, nread);
	b->pos += nread;
	
	if (b->pos < 1) // redundant
		return;
	
	int protocol_ver = *b->base; // uchar*
	
	fprintf(stderr, "Protocol version: %d\n", protocol_ver);
	
	if (protocol_ver != PROTOCOL_VER)
	{
		fprintf(stderr, "Protocol version mismatch: %d\n", protocol_ver);
		// close good
		uv_close((uv_handle_t*) server, NULL);
		return;
	}
	
	if (b->pos < 2)
		return;
		
	msg_type_t msg_type = (msg_type_t) *(b->base + 1);
	
	printf("Server: %s\n", msg_type == HAM ? "HAM" : "SPAM");
	uv_close((uv_handle_t*) server, NULL);
}

void on_write_end(uv_write_t *req, int status)
{
	if (status < 0)
	{
		fprintf(stderr, "Write error: %s\n", uv_strerror(status));
		// error!
		return;
	}
	
	buf_t* buf = (buf_t*) malloc(sizeof(buf_t));
    buf->len = 2;
    buf->base = (char*) malloc(buf->len);
    buf->pos = 0;
    
    req->handle->data = buf;

	uv_read_start((uv_stream_t*) req->handle, alloc_buffer, on_read);
}

void on_connect(uv_connect_t* connect, int status)
{
    if (status < 0)
    {
        fprintf(stderr, "Connection error: %s\n", uv_strerror(status));
        // error!
        return;
    }
	
	int slen = strlen(msg);
	int len = 5 + slen;
	char* bf = (char*) malloc(len);
	
	if (!bf)
	{
		uv_close((uv_handle_t*) connect->handle, NULL);
		return;
	}
	
	*bf = PROTOCOL_VER;
	*((int*) (bf + 1)) = slen;
	strcpy(bf + 5, msg);
	
	uv_buf_t buf = uv_buf_init(bf, len);
	
	uv_stream_t* tcp = connect->handle;
	uv_write_t write_req;
	uv_write(&write_req, tcp, &buf, 1, on_write_end);
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s msg\n", argv[0]);
		return 1;
	}
	
	msg = argv[1];
	
    uv_tcp_t* socket = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
	uv_tcp_init(uv_default_loop(), socket);

	uv_connect_t* connect = (uv_connect_t*) malloc(sizeof(uv_connect_t));

	struct sockaddr_in dest;
	int ret = uv_ip4_addr("127.0.0.1", DEFAULT_PORT, &dest);
	if (ret < 0)
    {
        fprintf(stderr, "Ip adress error: %s\n", uv_strerror(ret));
        return ret;
    }

	uv_tcp_connect(connect, socket, (const struct sockaddr*) &dest, on_connect);

    ret = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());
    return ret;
}
