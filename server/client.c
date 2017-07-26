#include <stdlib.h>
#include "client.h"

struct client_s
{
	uv_tcp_t sock;
	uv_buf_t buf;
	server_t* server;
};

client_t* client_init(server_t* s)
{
    client_t* c = (client_t*) malloc(sizeof(client_t));
    if (!c)
        goto fail;

    c->buf = uv_buf_init(NULL, 0);
    c->sock.data = c;
    c->server = s;

    if (uv_tcp_init(uv_default_loop(), &c->sock))
        goto free_client;

    goto done;

free_client:
    free(c);
fail:
    return NULL;
done:
    return c;
}

void client_free(client_t* c)
{
    free(c->buf.base);
	free(c);
}

static void client_on_close(uv_handle_t* sock)
{
	client_free((client_t*) sock->data);
}


void client_close(client_t* c)
{
    uv_close((uv_handle_t*) &c->sock, client_on_close);
}

uv_buf_t* client_get_buf(client_t* c)
{
    return &c->buf;
}

void client_set_buf(client_t* c, uv_buf_t buf)
{
    c->buf = buf;
}

uv_tcp_t* client_get_sock(client_t* c)
{
    return &c->sock;
}

server_t* client_get_serv(client_t* c)
{
    return c->server;
}