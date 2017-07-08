#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <pcre2.h>
#include "sf.h"

const int DEFAULT_BACKLOG = 128;

void on_write(uv_write_t* req, int status)
{
	if (status < 0)
		fprintf(stderr, "Connection error: %s\n", uv_strerror(status));
	
	uv_handle_t* handle = (uv_handle_t*) req->data;
	buf_t* buf = (buf_t*) handle->data;
	free(buf->base);
	free(buf);
	uv_close(handle, NULL);
	free(req);
}

void on_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf)
{
	if (nread < 0)
	{
		fprintf(stderr, "Connection error: %s\n", uv_strerror(nread));
		uv_close((uv_handle_t*) client, NULL);
		return;
	}
	else if (nread == 0)
		return;
		
	buf_t* b = (buf_t*) client->data;
	if (b->len - b->pos < nread)
	{
		char* m = (char*) malloc(nread + b->pos);
		if (!m)
		{
			uv_handle_t* handle = (uv_handle_t*) client;
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
		uv_close((uv_handle_t*) client, NULL);
		return;
	}
	
	if (b->pos < 5)
		return;
	
	int msg_len = *((int*) (b->base + 1));
	fprintf(stderr, "Message length: %d\n", msg_len);
	
	if (b->pos < msg_len + 5)
		return;
		
	char* msg = malloc(msg_len + 1);
	memcpy(msg, b->base + 5, msg_len);
	msg[msg_len] = '\0';
	
	printf("Client %d: '%s'\n", msg_len, msg);
	
	uv_write_t* write_req = (uv_write_t*) malloc(sizeof(uv_write_t));

    uv_buf_t bf = uv_buf_init(malloc(2), 2);
    *bf.base = PROTOCOL_VER;
    *(bf.base + 1) = HAM;

    write_req->data = client;
    uv_write(write_req, (uv_stream_t*) client, &bf, 1, on_write);
}

void on_new_connection(uv_stream_t* server, int status)
{
    if (status < 0)
    {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        // error!
        return;
    }

    uv_tcp_t* client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(uv_default_loop(), client);
    
    buf_t* buf = (buf_t*) malloc(sizeof(buf_t));
    buf->len = 1024;
    buf->base = (char*) malloc(buf->len);
    buf->pos = 0;
    
    client->data = buf;
    
    if (uv_accept(server, (uv_stream_t*) client) == 0)
    {
        uv_read_start((uv_stream_t*) client, alloc_buffer, on_read);
    }
    else
    {
        uv_close((uv_handle_t*) client, NULL);
    }
}

int main()
{
    uv_tcp_t server;
    uv_tcp_init(uv_default_loop(), &server);
	
	struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", DEFAULT_PORT, &addr);

    uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
    int ret = uv_listen((uv_stream_t*) &server, DEFAULT_BACKLOG, on_new_connection);
    if (ret)
    {
        fprintf(stderr, "Listen error %s\n", uv_strerror(ret));
        return ret;
    }
    
    ret = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());
    return ret;
}
