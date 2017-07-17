#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include "lib/sf.h"
#include "lib/buf.h"
#include "client.h"
#include "server.h"
#include "spam_filter.h"
#include "protocol.h"

const int DEFAULT_BACKLOG = 128;

void on_new_connection(uv_stream_t* serv_sock, int status);

void server_signal_close(uv_signal_t* signal_handle, int signum)
{
	server_free((server_t*) signal_handle->data);
}

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

	/* signals */
	
    uv_signal_init(uv_default_loop(), &serv->sigterm);
	serv->sigterm.data = serv;
    uv_signal_start(&serv->sigterm, server_signal_close, SIGTERM);
    
    uv_signal_init(uv_default_loop(), &serv->sigint);
	serv->sigint.data = serv;
    uv_signal_start(&serv->sigint, server_signal_close, SIGINT);

    uv_unref((uv_handle_t *) &serv->sigterm);
    uv_unref((uv_handle_t *) &serv->sigint);
    
    printf("Listening on port %d\n", port);
    return ret;
}

void server_free(server_t* serv)
{
    spam_filter_deinit(&serv->sf);
    uv_close((uv_handle_t*) &serv->sock, NULL);
	uv_close((uv_handle_t*) &serv->sigint, NULL);
	uv_close((uv_handle_t*) &serv->sigterm, NULL);
}

void on_close(uv_handle_t* sock)
{
	client_free((client_t*) sock->data);
}

void on_write(uv_write_t* req, int status)
{
	client_t* client = (client_t*) req->handle->data;

	if (status < 0)
		fprintf(stderr, "Write error: %s\n", uv_strerror(status));
	
	free(req);
	uv_close((uv_handle_t*) &client->sock, on_close);
}

void on_read(uv_stream_t* sock, ssize_t nread, const uv_buf_t* buf)
{
	client_t* client = (client_t*) sock->data;

	if (nread < 0)
	{
		fprintf(stderr, "Read error: %s\n", uv_strerror(nread));
		uv_close((uv_handle_t*) &client->sock, on_close);
		return;
	}
	else if (nread == 0)
		return;
	
	int ret;
	if ((ret = buf_append(&client->buf, buf->base, nread)) < 0)
	{
		fprintf(stderr, "Buffer error: %s\n", uv_strerror(ret));
		free(buf->base);
		uv_close((uv_handle_t*) &client->sock, on_close);
		return;
	}

	free(buf->base);
		
	char* msg = NULL;
	ret = sf_protocol_read_request(&client->buf, &msg);
	if (ret < 0)
	{
		uv_close((uv_handle_t*) &client->sock, on_close);
		return;
	}
	else if (ret > 0)
		return;

	uv_read_stop(sock);

	if (!msg)
	{
		fprintf(stderr, "Msg error\n");
		uv_close((uv_handle_t*) &client->sock, on_close);
		return;
	}
	
	printf("Client: '%s'\n", msg);

	free(client->buf.base);

	/* Response */
	
	msg_type_t msg_type;
	ret = spam_filter_check_msg(&client->server->sf, msg, &msg_type);
	free(msg);

    client->buf = uv_buf_init(NULL, 0);
	sf_protocol_write_response(&client->buf, ret, msg_type);

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

    if (client_init(client, (server_t*) serv_sock->data) < 0)
		return;
    
    if ((status = uv_accept(serv_sock, (uv_stream_t*) &client->sock)) == 0)
        uv_read_start((uv_stream_t*) &client->sock, alloc_buffer, on_read);
    else
    {
        fprintf(stderr, "Failed to accept connection %s\n", uv_strerror(status));
		uv_close((uv_handle_t*) &client->sock, on_close);
    }
}