#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <getopt.h>
#include <uv.h>
#include "lib/sf.h"
#include "server.h"

static void print_usage(char* argv[])
{
   fprintf(stderr, "Spam filter version: %u.%u\n", VERSION_MAJOR, VERSION_MINOR);
   fprintf(stderr, "Usage: %s\n", argv[0]);
   fprintf(stderr, "    [-h|--help		show help and exit]\n");
   fprintf(stderr, "    [-p|--port		server port]\n");
}

int main(int argc, char* argv[])
{
	int port = DEFAULT_PORT;
	
	int opt = 0;
	int long_index = 0;
	int show_help = 0;
	struct option options[3];

	options[0].name = "help";
	options[0].has_arg = no_argument;
	options[0].val = 1;
	options[0].flag = &show_help;

	options[1].name = "port";
	options[1].has_arg = required_argument;
	options[1].val = 0;
	options[1].flag = NULL;

	options[2].name = NULL;

	optind = 0;
	while ((opt = getopt_long(argc, argv, "hp:", options, &long_index)) != -1)
	{
		switch (opt)
		{
			case 0:
				if (strcmp(options[long_index].name, "port") == 0)
					port = strtol(optarg, NULL, 10);
				break;

			case 'h':
				show_help = 1;
				break;

			case 'p':
				port = strtol(optarg, NULL, 10);
				break;

			default:
				print_usage(argv);
				return 1;
		}
	}

	if (show_help || !port || port > USHRT_MAX)
	{
		print_usage(argv);
		return 0;
	}
	
    server_t server;
	int ret = server_init(&server, port);

	if (ret)
        return ret;
    
    ret = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());
    
    return ret;
}
