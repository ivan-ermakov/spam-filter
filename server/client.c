#include <stdlib.h>
#include "client.h"

void client_init(client_t* c, server_t* s)
{
    c->buf.base = NULL;
    c->buf.len = 0;
    uv_tcp_init(uv_default_loop(), &c->sock);
    c->sock.data = c;
    c->server = s;
}

void client_free(client_t* c)
{
    uv_close((uv_handle_t*) &c->sock, NULL);
    free(c->buf.base);
	free(c);
}