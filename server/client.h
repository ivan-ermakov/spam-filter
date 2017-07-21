#ifndef CLIENT_H
#define CLIENT_H

#include <uv.h>

#include "lib/sf.h"
#include "lib/buf.h"

typedef struct server_s server_t;
typedef struct client_s client_t;

client_t* client_init(server_t* s);
void client_free(client_t* c);
uv_buf_t* client_get_buf(client_t* c);
void client_set_buf(client_t* c, uv_buf_t buf);
uv_tcp_t* client_get_sock(client_t* c);
server_t* client_get_serv(client_t* c);

#endif