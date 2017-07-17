#ifndef SERVER_H
#define SERVER_H

#include "spam_filter.h"

typedef struct server_s
{
    uv_tcp_t sock;
    spam_filter_t sf;
    uv_signal_t sigterm;
    uv_signal_t sigint;
} server_t;

int server_init(server_t* serv, int port);
void server_free(server_t* serv);

#endif