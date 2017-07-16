#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "client.h"

static void print_usage(char* argv[])
{
   fprintf(stderr, "Spam filter version: %u.%u\n", VERSION_MAJOR, VERSION_MINOR);
   fprintf(stderr, "Usage: %s [ip [port]] <msg>\n", argv[0]);
}

int main(int argc, char* argv[])
{
	char* ip = 	"127.0.0.1";
	int port = DEFAULT_PORT;
	char* msg = NULL;
	
	switch (argc)
	{
		case 4:
			msg = argv[3];
			sscanf(argv[2], "%d", &port);
			
			if (port <= 0 || port > USHRT_MAX)
			{
				print_usage(argv);
				return 2;
			}
			
			ip = argv[1];
			break;
		case 3:
			msg = argv[2];
			ip = argv[1];
			break;
		case 2:
			msg = argv[1];
			break;
		default:
			print_usage(argv);
			return 1;
	}
	
    client_t client;

	int ret = client_init(&client);
	if (ret)
		return ret;

	client_check_msg(&client, ip, port, msg);

    ret = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());

	//client_free(&client);

    return ret;
}
