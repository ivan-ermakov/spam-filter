#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <uv.h>
#include "lib/sf.h"
#include "spam_filter.h"

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
		b->len = nread + b->pos;
		b->base = (char*) realloc(b->base, b->len);
		if (!b->base)
		{
			uv_handle_t* handle = (uv_handle_t*) client;
			buf_t* buf = (buf_t*) handle->data;
			free(buf->base);
			free(buf);
			uv_close(handle, NULL);
			return;
		}
	}	
	
	memcpy(b->base + b->pos, buf->base, nread);
	b->pos += nread;
	
	int protocol_ver = *((unsigned char*)b->base);
	
	if (protocol_ver != PROTOCOL_VER)
	{
		fprintf(stderr, "Protocol version mismatch: %d\n", protocol_ver);
		uv_close((uv_handle_t*) client, NULL);
		return;
	}
	
	if (b->pos < 5)
		return;
	
	int msg_len = *((int*) (b->base + 1));
	
	if (b->pos < msg_len + 5)
		return;
		
	char* msg = malloc(msg_len + 1);
	memcpy(msg, b->base + 5, msg_len);
	msg[msg_len] = '\0';
	
	printf("Client%d [%d]: '%s'\n", protocol_ver, msg_len, msg);
	
	uv_write_t* write_req = (uv_write_t*) malloc(sizeof(uv_write_t));
	
	msg_type_t msg_type;
	int ret = check_msg_type(msg, &msg_type);
    uv_buf_t bf = uv_buf_init(malloc(2), 2);
    *bf.base = ret;
    *(bf.base + 1) = msg_type;

    write_req->data = client;
    uv_write(write_req, (uv_stream_t*) client, &bf, 1, on_write);
}

void on_new_connection(uv_stream_t* server, int status)
{
    if (status < 0)
    {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        return;
    }

    uv_tcp_t* client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(uv_default_loop(), client);
    
    buf_t* buf = (buf_t*) malloc(sizeof(buf_t));
    buf->len = 0;
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
        fprintf(stderr, "Failed to accept connection %s\n", uv_strerror(status));
    }
}

static void print_usage(char* argv[])
{
   fprintf(stderr, "Spam filter version: %u.%u\n", VERSION_MAJOR, VERSION_MINOR);
   fprintf(stderr, "Usage: %s [port]\n", argv[0]);
}

int main(int argc, char* argv[])
{
	int port = DEFAULT_PORT;
	
	if (argc == 2)
	{
		sscanf(argv[1], "%d", &port);
			
		if (port <= 0 || port > USHRT_MAX)
		{
			print_usage(argv);
			return 2;
		}
	}
	else if (argc != 1)
	{
		print_usage(argv);
		return 1;
	}
	
	if (load_patterns() == 0)
	{
		printf("No patterns to match\n");
		return 3;
	}
	
    uv_tcp_t server;
    uv_tcp_init(uv_default_loop(), &server);
	
	struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", port, &addr);

    uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
    int ret = uv_listen((uv_stream_t*) &server, DEFAULT_BACKLOG, on_new_connection);
    if (ret)
    {
        fprintf(stderr, "Listen error %s\n", uv_strerror(ret));
        return ret;
    }
    
    printf("Listening on port %d\n", port);
    
    ret = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());
    
    free_patterns();
    
    return ret;
}
