
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>

#include "server.h"
#include "log.h"
#include "cache_dir.h"
#include "lua_api.h"

static char line[MAX_LINE];
static int line_no;

static void http_port_parse(unsigned short *port);
static void inet4_addr_parse(struct in_addr *addrs);
static void cache_dir_parse(struct array_t *dirs);
static void size_parse(long long *sz);
static void time_parse(time_t *time);

static void time_parse(time_t *time)
{
	if(!time) 
		return;
	
	int len;
	char *token = (char *) strtok(NULL, SPACE_CHAR);
	if(!token) {
		fprintf(stderr, "time_parse failed {%s : %d}\n", \
					line, line_no);
		return;
	}
	
	*time = atol(token);
	len = strlen(token);
	if(tolower(token[len - 1]) == 'm')
		*time *= 60;
	else if(tolower(token[len - 1]) == 'h')
		*time *= 360;
}

static void size_parse(long long *sz)
{
	long long fsz = 0LL;
	int len = 0;
	char *token = (char *)strtok(NULL, SPACE_CHAR);
	if(!token) {
		error("size_parse {%s : %d}\n", line, line_no);
		return;
	}

	fsz = atoi(token);
	len = strlen(token);
	
	if(token[len-1] == 'K' || token[len-1] == 'k') 
		*sz = fsz << 10;
	if(token[len-1] == 'M' || token[len-1] == 'm') 
		*sz = fsz << 20;
	if(token[len-1] == 'G' || token[len-1] == 'g') 
		*sz = fsz << 30;
}

static void cache_dir_parse(struct array_t *dirs)
{
	struct cache_dir *dentry = calloc(1, sizeof(*dentry));
	if(!dentry) {
		error("cache_dir_parse config->dirs %p failed to calloc\n", *dirs);
		exit(1) ;
	}

	int len = 0;
	char *token = (char *)strtok(NULL, SPACE_CHAR);
	if(!token) {
		debug("cache_dir_parse {%s : %d} failed\n", line, line_no); 
		safe_free(dentry);
		return ;
	}
	dentry->path = strdup(token);

	token = (char *)strtok(NULL, SPACE_CHAR);
	if(!token) {
		debug("cache_dir_parse {%s : %d} failed\n", line, line_no); 
		safe_free(dentry);
		return;
	}
	dentry->max_size = atoi(token);
	len = strlen(token);
	
	if(token[len-1] == 'K' || token[len-1] == 'k') 
		dentry->max_size <<= 10;
	if(token[len-1] == 'M' || token[len-1] == 'm') 
		dentry->max_size <<= 20;
	if(token[len-1] == 'G' || token[len-1] == 'g') 
		dentry->max_size <<= 30;

	array_append(dirs, dentry);
	fprintf(stderr, "cache_dir_parse %s %lld bytes add.\n", dentry->path, dentry->max_size);
}

static void http_port_parse(unsigned short *port)
{
	char *token;
	int p ;

	token = (char *)strtok(NULL, SPACE_CHAR);
	if(!token) {
		warning("http_port_parse %p failed to parse http_port\n", port);
		return ;
	}

	p = atoi(token);
	if(p > 63553 || p < 0) {
		warning("http_port_parse %d ill suited\n", p);
		return ;
	}
	
	*port = htons(p);
}

static void inet4_addr_parse(struct in_addr *addrs)
{
	char *token ;
	
	token = (char *)strtok(NULL, SPACE_CHAR);
	if(!token) {
		warning("inet4_addr_parse %p can not get inet4 addr\n", addrs);
		return ;
	}
	
	inet_pton(AF_INET, token, addrs);
}

void parse_conf(void)
{
#if USE_LUA
	parse_lua_conf(conf_path);
#else 
	FILE *conf = NULL;
	char *tmp;

	assert(conf_path);
	line_no = 0;
	conf = fopen(conf_path, "r");
	if(!conf) {
		fprintf(stderr, "parse_conf can not find conf file {%s}\n", strerror(errno));
		exit(1);
	}
	
	while((tmp = fgets(line, MAX_LINE - 1, conf))) {
		line_no ++;
		if(line[0] == '#')
			continue;
		if(line[0] == ';')
			continue;
		if(line[0] == ' ')
			continue;
		if(line[0] == '\t')
			continue;
		char *token = (char *)strtok(line, " \t\r\n");
		if(!token) {
			log_debug(__FILE__, __LINE__, "parse_conf can not get token\n");
			continue;
		}

		if(!strcasecmp(token, "http_port"))
			http_port_parse(&config->port);
		else if(!strcasecmp(token, "inet4_addr"))
			inet4_addr_parse(&config->bind_addr);
		else if(!strcasecmp(token, "cache_dir")) 
			cache_dir_parse(config->dirs);
		else if(!strcasecmp(token, "max_size_in_shm")) 
			size_parse(&config->max_size_in_shm);
		else if(!strcasecmp(token, "max_size_in_mem"))
			size_parse(&config->max_size_in_mem);
		else if(!strcasecmp(token, "client_life")) 
			time_parse(&config->client_life);
		else if(!strcasecmp(token, "request_timeout")) 
			time_parse(&config->request_timeout);
		else if(!strcasecmp(token, "reply_timeout")) 
			time_parse(&config->reply_timeout);
		else if(!strcasecmp(token, "connect_timeout"))
			time_parse(&config->connect_timeout);
		else if(!strcasecmp(token, "server_life"))
			time_parse(&config->server_life);
		else {
			fprintf(stderr, "parse_conf failed {%d : %s}\n", line_no, line);
		}
	}
#endif
}
