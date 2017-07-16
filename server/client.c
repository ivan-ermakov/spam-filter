#include <stdlib.h>
#include "client.h"

void client_init(client_t* c, server_t* s)
{
    c->buf = uv_buf_init(NULL, 0);
    uv_tcp_init(uv_default_loop(), &c->sock);
    c->sock.data = c;
    c->server = s;
}

void client_free(client_t* c)
{
    free(c->buf.base);
	free(c);
}