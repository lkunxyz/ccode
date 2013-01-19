
#ifndef CONF_H
#define CONF_H
#include <stdio.h>
#include <arpa/inet.h>
#include "array.h"

struct config_t {
	struct {
		FILE *debug_log;
		FILE *error_log;
		FILE *warning_log;
		FILE *accesslog;
	}log;

	struct {
		unsigned short port;
		struct in_addr inet4_addr;
		int debug_mode;
		long long max_size_in_shm;
		long long max_size_in_mem;
	}server;

	struct {
		time_t client_life;
		time_t server_life;
		time_t request;
		time_t reply;
		time_t connect;
	}timeout;
	
	struct array_t *cache_dir;
};

extern void parse_conf(void);
#endif
