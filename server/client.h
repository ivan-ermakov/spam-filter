#ifndef CLIENT_H
#define CLIENT_H

#include <uv.h>

#include "lib/sf.h"
#include "lib/buf.h"

typedef struct server_s server_t;

typedef struct client_s
{
	uv_tcp_t sock;
	uv_buf_t buf;
	server_t* server;
} client_t;

void client_init(client_t* c, server_t* s);
void client_free(client_t* c);

#endif