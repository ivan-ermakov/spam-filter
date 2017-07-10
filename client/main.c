#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <uv.h>
#include "lib/sf.h"

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
		b->len = nread + b->pos;
		b->base = (char*) realloc(b->base, b->len);
		if (!b->base)
		{
			uv_handle_t* handle = (uv_handle_t*) server;
			buf_t* buf = (buf_t*) handle->data;
			free(buf->base);
			free(buf);
			uv_close(handle, NULL);
			return;
		}		
	}	
	
	memcpy(b->base + b->pos, buf->base, nread);
	b->pos += nread;
	
	if (b->pos < 2)
		return;
	
	int error = *((unsigned char*)b->base);
	msg_type_t msg_type = (msg_type_t) *(b->base + 1);
	
	printf("Server <%d>: %s\n", error, msg_type == MSG_TYPE_HAM ? "HAM" : "SPAM");
	uv_close((uv_handle_t*) server, NULL);
}

void on_write_end(uv_write_t *req, int status)
{
	if (status < 0)
	{
		fprintf(stderr, "Write error: %s\n", uv_strerror(status));
		uv_close((uv_handle_t*) req->handle, NULL);
		return;
	}
	
	buf_t* buf = (buf_t*) malloc(sizeof(buf_t));
    buf->len = 0;
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

static void print_usage(char* argv[])
{
   fprintf(stderr, "Spam filter version: %u.%u\n", VERSION_MAJOR, VERSION_MINOR);
   fprintf(stderr, "Usage: %s [ip [port]] <msg>\n", argv[0]);
}

int main(int argc, char* argv[])
{
	char* ip = 	"127.0.0.1";
	int port = DEFAULT_PORT;
	
	switch (argc)
	{
		case 4:
			msg = argv[3];
			sscanf(argv[2], "%d", &port);
			
			if (port <= 0 || port > USHRT_MAX)
			{
				print_usage(argv);
				return 2;
			}
			
			ip = argv[1];
			break;
		case 3:
			msg = argv[2];
			ip = argv[1];
			break;
		case 2:
			msg = argv[1];
			break;
		default:
			print_usage(argv);
			return 1;
	}
	
    uv_tcp_t socket;
	uv_tcp_init(uv_default_loop(), &socket);

	uv_connect_t connect;
	
	// Resolve adress
	
	struct addrinfo hints;
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;

    uv_getaddrinfo_t resolver;
    fprintf(stderr, "%s is... ", ip);
    int ret = uv_getaddrinfo(uv_default_loop(), &resolver, NULL, ip, NULL /* port */, &hints);
	
	char addr[17] = {};
	
    if (ret == 0)
    {
		uv_ip4_name((struct sockaddr_in*) resolver.addrinfo->ai_addr, addr, sizeof(addr) - 1);
		fprintf(stderr, "%s\n", addr);
		ip = addr;
	}
    
	struct sockaddr_in dest;
	ret = uv_ip4_addr(ip, port, &dest);
	if (ret < 0)
    {
        fprintf(stderr, "Ip adress error: %s\n", uv_strerror(ret));
        print_usage(argv);
        return ret;
    }
    
    // Connect
	
	printf("Connecting to %s:%d...\n", ip, port);
	uv_tcp_connect(&connect, &socket, (const struct sockaddr*) &dest, on_connect);

    ret = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());
    return ret;
}
