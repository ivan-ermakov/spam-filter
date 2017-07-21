#ifndef SERVER_H
#define SERVER_H

typedef struct server_s server_t;

enum { DEFAULT_BACKLOG = 128 };

server_t* server_init(int port);
void server_free(server_t* serv);

#endif