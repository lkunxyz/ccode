
/*
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is HongWei.Peng code.
 *
 * The Initial Developer of the Original Code is
 * HongWei.Peng.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 * 
 * Author:
 *		tangtang 1/6/2007
 *
 * Contributor(s):
 *
 */
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <getopt.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <signal.h>
#include <glib.h>

#include "server.h"
#include "log.h"
#include "sock_io.h"
#include "sock_epoll.h"
#include "cache_file.h"
#include "file_aio.h"
#include "accesslog.h"
#include "cache_dir.h"
#include "conf.h"

static void parse_options(int argc, char **argv);
static void server_init(void);
static void config_init(void);
static void usage(void);
static void server_down(int signum);
static void signal_register(int sig, SGHANDLE *func, int flags);

struct timeval current_time;
struct config_t *config;
struct fd_t *fd_table;
struct server srv;
int cache_dir_count;
GAsyncQueue *io_queue;
char conf_path[MAX_LINE];
int biggest_fd;
time_t epoll_time;

void current_time_get(void)
{
	if(-1 == gettimeofday(&current_time, NULL)) {
		error("do_sock_epoll %s\n", strerror(errno));
	}
}

char *smart_time(void) 
{
	static char timestr[128];
	struct tm *tm = gmtime(&(current_time.tv_sec));
	if(tm == NULL)
		return NULL;
	
	strftime(timestr, 127, "%Y/%m/%d %H:%M:%S", tm);
	return timestr;
}

int main(int argc, char **argv) 
{
	/* init data */
	config_init();

	parse_conf();

	parse_options(argc, argv);
	
	server_init();
	/* main loop */
	for(; ;) {
		int msec = 1000;
		current_time_get();
		if(current_time.tv_sec - epoll_time > 1000) 
			msec = 0;		
		
		do_sock_epoll(msec);
		file_aio_loop();
	}
	
	/* not reached forever */
	return 0;
}

static void parse_options(int argc, char **argv)
{
	assert(config);
	int c;
	
	while(-1 != (c = getopt(argc, argv, "p:H:hDf:"))) {
		switch(c) {
			case 'p': 
				if(!optarg) 
					usage();
				int p = atoi(optarg);
				if(p <= 0 || p > 65535) {
					log_debug(__FILE__, __LINE__, "listen port can not less than 0 and larger than 65535\n");
					abort();
				}	
				break;
			case 'H':
				if(!optarg)
					usage();
				inet_pton(AF_INET, optarg, &config->server.inet4_addr);
				break;
			case 'h':
				usage();
				break;
			case 'D':
				config->server.debug_mode = 1;
				break;
			default:
				break;
		}
	}
}

static void usage(void) 
{
	fprintf(stderr, "Usage: yanow [options]\n" \
					"Options: \n" \
					"       -p port                    Listen port. Default 80.\n" \
					"       -H host                    Bind host. Default INADDR_ANY.\n"\
					"       -D                         Debug mode.\n" \
					"       -d dir                     Cache dir. Default '/var/cache'\n"\
					"       -h                         Help information.\n");
	exit(1);
}

static void server_init(void) 
{

	log_init();
	
	accesslog_init();

	log_debug(__FILE__, __LINE__, "Starting http server Yanow...\n");

	sock_init();

	cache_file_table_init();

	cache_dir_init();
	
	cache_index_init();

	file_aio_init(32);
	/* signals */
	signal_register(SIGINT, server_down, SA_NODEFER | SA_RESETHAND | SA_RESTART);
	signal_register(SIGTERM, server_down, SA_NODEFER | SA_RESETHAND | SA_RESTART);
	signal_register(SIGPIPE, SIG_IGN, SA_RESTART);
	
	srv.dirs = config->cache_dir;
	srv.shm.size = config->server.max_size_in_shm;
	srv.shm.offset = 0;
	srv.mem.size = config->server.max_size_in_mem;
	srv.mem.offset = 0;
	current_time_get();
	srv.up = current_time.tv_sec;
	epoll_time = srv.up;
}

static void config_init(void)
{
	if(config)
		return ;

	config = calloc(1, sizeof(*config));
	
	if(config == NULL) {
		fprintf(stderr, "%s %d: %s\n", __FILE__, __LINE__, strerror(errno));
		abort();
	}
	
	char *a = "000.000.000.000";

	strcpy(conf_path, DEFAULT_CONF_FILE);

	config->server.port = htons(80);
	inet_pton(AF_INET, a, &config->server.inet4_addr);
	config->server.debug_mode = 0;
	if(!config->cache_dir) 
		config->cache_dir = array_create();

	config->server.max_size_in_shm = MAX_SIZE_IN_SHM;
	config->server.max_size_in_mem = MAX_SIZE_IN_MEM;
	config->timeout.request = 60;
	config->timeout.reply= 60;
	config->timeout.connect = 60;
	config->timeout.client_life = 86400;
	config->timeout.server_life = 86400;
}

static void signal_register(int sig, SGHANDLE *func, int flags)
{
#if HAVE_SIGACTION
	struct sigaction sa;
	sa.sa_handler = func;
	sa.sa_flags = flags;
	sigemptyset(&sa.sa_mask);
	if (sigaction(sig, &sa, NULL) < 0)
		debug("signal_register sig=%d func=%p: %s\n", sig, func, strerror(errno));
#else 
	signal(sig, func);
#endif
}

static void server_down(int signum)
{
	warning("server_down signum %d\n", signum);
	if(config->log.accesslog) 
		fflush(config->log.accesslog);
	cache_dir_close();
	
	exit(1);
}
