#include <stdlib.h>
#include <string.h>
#include "lib/buf.h"
#include "client.h"
#include "lib/protocol.h"

struct client_s
{
    uv_tcp_t sock;
    uv_connect_t connect;
    uv_buf_t buf;
    char* msg;
};

static void on_connect(uv_connect_t* connect, int status);

client_t* client_init()
{
	client_t* c = (client_t*) malloc(sizeof(client_t));
	c->buf = uv_buf_init(NULL, 0);
	c->sock.data = c;
    
	if (uv_tcp_init(uv_default_loop(), &c->sock))
	{
		free(c);
		return NULL;
	}

	return c;
}

static void client_on_close(uv_handle_t* sock)
{
	free(sock->data);
}

void client_free(client_t* c)
{
	free(c->buf.base);
    uv_close((uv_handle_t*) &c->sock, client_on_close);
}

int client_check_msg(client_t* c, char* ip, int port, char* msg)
{	
	c->msg = msg;

	/* Resolve adress */
	
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
		uv_freeaddrinfo(resolver.addrinfo);
	}
    
	struct sockaddr_in dest;
	ret = uv_ip4_addr(ip, port, &dest);
	if (ret < 0)
    {
        fprintf(stderr, "Ip adress error: %s\n", uv_strerror(ret));
        return ret;
    }
    
    /* Connect */
	
	printf("Connecting to %s:%d...\n", ip, port);
	uv_tcp_connect(&c->connect, &c->sock, (const struct sockaddr*) &dest, on_connect);

    return 0;
}

static void on_read(uv_stream_t* server, ssize_t nread, const uv_buf_t* buf)
{
	client_t* client = (client_t*) server->data;

	if (nread < 0)
	{
		fprintf(stderr, "Read error: %s\n", uv_strerror(nread));
		free(buf->base);
		client_free(client);
		return;
	}
	else if (nread == 0)
		return;
		
	buf_append(&client->buf, buf->base, nread);
	free(buf->base);


	int error;
	msg_type_t msg_type;
	
	if (sf_protocol_read_response(&client->buf, &error, &msg_type) != 0)
		return;
	
	printf("Server <%d>: %s\n", error, msg_type == MSG_TYPE_HAM ? "HAM" : "SPAM");
	client_free(client);
}

static void on_write_end(uv_write_t* req, int status)
{
	client_t* client = (client_t*) req->data;
	free(req);

	if (status < 0)
	{
		fprintf(stderr, "Write error: %s\n", uv_strerror(status));
		client_free(client);
		return;
	}
	
	free(client->buf.base);
	client->buf = uv_buf_init(NULL, 0);

	uv_read_start((uv_stream_t*) &client->sock, alloc_buffer, on_read);
}

static void on_connect(uv_connect_t* connect, int status)
{
	client_t* client = (client_t*) connect->handle->data;

    if (status < 0)
    {
        fprintf(stderr, "Connection error: %s\n", uv_strerror(status));
		client_free(client);
        return;
    }	

	client->buf = uv_buf_init(NULL, 0);
	sf_protocol_write_request(&client->buf, client->msg);

	uv_write_t* write_req = (uv_write_t*) malloc(sizeof(uv_write_t));
	write_req->data = client;
	uv_write(write_req, (uv_stream_t*) &client->sock, &client->buf, 1, on_write_end);
}