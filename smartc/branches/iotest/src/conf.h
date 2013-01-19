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
