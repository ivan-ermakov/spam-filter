#ifndef CLIENT_H
#define CLIENT_H

#include <uv.h>
#include "lib/sf.h"

typedef struct client_s client_t;

client_t* client_init();
void client_free(client_t* c);
int client_check_msg(client_t* c, char* ip, int port, char* msg);

#endif