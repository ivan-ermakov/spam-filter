#include <stdio.h>
#include <limits.h>

#include <uv.h>

#include "lib/sf.h"

#include "server.h"

static void print_usage(char* argv[])
{
   fprintf(stderr, "Spam filter version: %u.%u\n", VERSION_MAJOR, VERSION_MINOR);
   fprintf(stderr, "Usage: %s [port]\n", argv[0]);
}

int main(int argc, char* argv[])
{
	int port = DEFAULT_PORT;
	
	if (argc == 2)
	{
		sscanf(argv[1], "%d", &port);
			
		if (port <= 0 || port > USHRT_MAX)
		{
			print_usage(argv);
			return 2;
		}
	}
	else if (argc != 1)
	{
		print_usage(argv);
		return 1;
	}
	
    server_t server;
	int ret = server_init(&server, port);

	if (ret)
        return ret;
    
    ret = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());
    
    server_free(&server);
    
    return ret;
}
