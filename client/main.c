#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <getopt.h>
#include "client.h"

static void print_usage(char* argv[])
{
   fprintf(stderr, "Spam filter version: %u.%u\n", VERSION_MAJOR, VERSION_MINOR);
   fprintf(stderr, "Usage: %s\n", argv[0]);
   fprintf(stderr, "    [-h|--help		show help and exit]\n");
   fprintf(stderr, "    [-i|--ip   		server ip]\n");
   fprintf(stderr, "    [-p|--port		server port]\n");
   fprintf(stderr, "    <message>\n");
}

int main(int argc, char* argv[])
{
	char ip[17] = 	"127.0.0.1";
	int port = DEFAULT_PORT;
	char* msg = NULL;

	int opt = 0;
	int long_index = 0;
	int show_help = 0;
	struct option options[5];

	options[0].name = "help";
	options[0].has_arg = no_argument;
	options[0].val = 1;
	options[0].flag = &show_help;

	options[1].name = "ip";
	options[1].has_arg = required_argument;
	options[1].val = 0;
	options[1].flag = NULL;

	options[2].name = "port";
	options[2].has_arg = required_argument;
	options[2].val = 0;
	options[2].flag = NULL;

	options[3].name = "msg";
	options[3].has_arg = required_argument;
	options[3].val = 0;
	options[3].flag = NULL;

	options[4].name = NULL;

	optind = 0;
	while ((opt = getopt_long(argc, argv, "hi:p:m:", options, &long_index)) != -1)
	{
		switch (opt)
		{
			case 0:
				if (strcmp(options[long_index].name, "ip") == 0)
					strcpy(ip, optarg);
				else if (strcmp(options[long_index].name, "port") == 0)
					port = strtol(optarg, NULL, 10);
				else if (strcmp(options[long_index].name, "msg") == 0)
					msg = optarg;
				break;

			case 'h':
				show_help = 1;
				break;

			case 'i':
				strcpy(ip, optarg);
				break;

			case 'p':
				port = strtol(optarg, NULL, 10);
				break;

			case 'm':
				msg = optarg;
				break;

			default:
				print_usage(argv);
				return 1;
		}
	}

	if (show_help || !port || port > USHRT_MAX || optind >= argc)
	{
		print_usage(argv);
		return 0;
	}

	msg = argv[optind];
	
    client_t* client = client_init();

	if (!client)
		return 2;

	client_check_msg(client, ip, port, msg);

    int ret = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());

    return ret;
}
