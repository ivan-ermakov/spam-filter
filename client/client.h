#ifndef CLIENT_H
#define CLIENT_H

#include <uv.h>
#include "lib/sf.h"

typedef struct client_s
{
    uv_tcp_t sock;
    uv_connect_t connect;
    uv_buf_t buf;
    char* msg;
} client_t;

int client_init(client_t* c);
void client_free(client_t* c);
int client_check_msg(client_t* c, char* ip, int port, char* msg);

#endif