#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uv.h>

#include "lib/sf.h"
#include "lib/buf.h"

#include "client.h"
#include "server.h"
#include "spam_filter.h"

const int DEFAULT_BACKLOG = 128;

void on_new_connection(uv_stream_t* serv_sock, int status);

int server_init(server_t* serv, int port)
{	
	if (spam_filter_init(&serv->sf, "patterns.txt") == SF_EFAIL)
	{
		printf("No patterns to match\n");
		return SF_EFAIL;
	}

    uv_tcp_init(uv_default_loop(), &serv->sock);
	serv->sock.data = serv;

    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", port, &addr);
    uv_tcp_bind(&serv->sock, (const struct sockaddr*) &addr, 0);
    int ret = uv_listen((uv_stream_t*) &serv->sock, DEFAULT_BACKLOG, on_new_connection);
    if (ret)
    {
        fprintf(stderr, "Listen error %s\n", uv_strerror(ret));
        return ret;
    }
    
    printf("Listening on port %d\n", port);
    return ret;
}

void server_free(server_t* serv)
{
    spam_filter_deinit(&serv->sf);
    uv_close((uv_handle_t*) &serv->sock, NULL);
}

void on_write(uv_write_t* req, int status)
{
	client_t* client = (client_t*) req->data;

	if (status < 0)
		fprintf(stderr, "Write error: %s\n", uv_strerror(status));
	
	client_free(client);
	free(req);
}

void on_read(uv_stream_t* sock, ssize_t nread, const uv_buf_t* buf)
{
	client_t* client = (client_t*) sock->data;

	if (nread < 0)
	{
		fprintf(stderr, "Read error: %s\n", uv_strerror(nread));
		client_free(client);
		return;
	}
	else if (nread == 0)
		return;
	
	buf_append(&client->buf, buf->base, nread);
	free(buf->base);
	
	int protocol_ver = *((unsigned char*) client->buf.base);
	
	if (protocol_ver != PROTOCOL_VER)
	{
		fprintf(stderr, "Protocol version mismatch: %d\n", protocol_ver);
		client_free(client);
		return;
	}
	
	if (client->buf.len < 5)
		return;
	
	int msg_len = *((int*) (client->buf.base + 1));
	
	if (client->buf.len < msg_len + 5)
		return;
		
	char* msg = malloc(msg_len + 1);
	memcpy(msg, client->buf.base + 5, msg_len);
	msg[msg_len] = '\0';
	
	printf("Client%d [%d]: '%s'\n", protocol_ver, msg_len, msg);

	free(client->buf.base);

	/* Response */
	
	msg_type_t msg_type;
	int ret = spam_filter_check_msg(&client->server->sf, msg, &msg_type); /* TODO */
	free(msg);

    client->buf = uv_buf_init(malloc(2), 2);
    *client->buf.base = ret;
    *(client->buf.base + 1) = msg_type;

	uv_write_t* write_req = (uv_write_t*) malloc(sizeof(uv_write_t));
	write_req->data = client;
    uv_write(write_req, (uv_stream_t*) sock, &client->buf, 1, on_write);
}

void on_new_connection(uv_stream_t* serv_sock, int status)
{
    if (status < 0)
    {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        return;
    }

    client_t* client = (client_t*) malloc(sizeof(client_t));
	if (!client)
		return;

    client_init(client, (server_t*) serv_sock->data);
    
    if ((status = uv_accept(serv_sock, (uv_stream_t*) &client->sock)) == 0)
        uv_read_start((uv_stream_t*) &client->sock, alloc_buffer, on_read);
    else
    {
        client_free(client);
        fprintf(stderr, "Failed to accept connection %s\n", uv_strerror(status));
    }
}