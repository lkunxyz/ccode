
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
 *		tangtang 19/02/2008
 *
 * Contributor(s):
 *
 */
#include <stdio.h>
#include <assert.h>

#include "server.h"
#include "lua_api.h"
#include "cache_dir.h"

#define MIN_EXTRA 20

static void lua_conf_server_parse(lua_State *l);
static void lua_conf_timeout_parse(lua_State *l);
static void lua_conf_cache_dir_parse(lua_State *l);
static void lua_size_parse(long long *sz, const char *value);
static void lua_time_parse(time_t *time, const char *value);
static void cache_dir_add(struct array_t *dir, lua_State *l);
static void lua_stack_view(lua_State *l);

void parse_lua_conf(const char *lua_conf)
{
	if(!lua_conf) {
		fprintf(stderr, "%s::%d lua conf is null\n",__FILE__, __LINE__);
		exit(1);
	}
	
	lua_State *l = (lua_State *)luaL_newstate();
	if(!l) {
		fprintf(stderr, "%s::%d parse_lua_conf failed ", __FILE__, __LINE__);
		lua_error(l);
	}
	
	luaopen_base(l);
	luaopen_table(l);
	luaopen_string(l);

	//lua_stack_view(l);
	/* check stack */
	lua_checkstack(l, MIN_EXTRA);
	/* load lua conf */
	if(luaL_dofile(l, lua_conf) == 1) {
		fprintf(stderr, "%s::%d parse_lua_conf failed to execute luaL_dofile %s\n", \
						__FILE__, __LINE__, lua_conf);
		lua_error(l);
		abort();
	}
	else 
		fprintf(stderr, "%s::%d parse_lua_conf success to execute luaL_dofile %s\n", \
						__FILE__, __LINE__, lua_conf);
	
   	lua_conf_server_parse(l);
	lua_conf_timeout_parse(l);
	lua_conf_cache_dir_parse(l);
	
	lua_close(l);
}

static void lua_conf_server_parse(lua_State *l)
{
	assert(l);

	lua_getglobal(l, "server"); /* remaid to pop */
	if(lua_isnil(l, -1)) {
		fprintf(stderr, "%s::%d parse server failed, return nil\n", __FILE__, __LINE__);
		lua_pop(l, 1);
		return;
	}

	if(!lua_istable(l, -1)) {
		fprintf(stderr, "%s::%d parse server failed ", __FILE__, __LINE__);
		lua_error(l);
		return;
	}
	/* parse server.port */
	lua_getfield(l, -1, "port");
	if(!lua_isnumber(l, -1)) {
		fprintf(stderr, "%s::%d parse server.port failed", __FILE__, __LINE__);
		lua_error(l);
	}
	else {
		int port = (int)lua_tointeger(l, -1);
		config->server.port = htons(port);
		fprintf(stderr, "%s::%d server.port =  %d\n", __FILE__, __LINE__, config->server.port);
	}
	lua_pop(l, 1);

	/* parse server.inet4_addr */
	lua_getfield(l, -1, "inet4_addr");
	if(!lua_isstring(l, -1)) {
		fprintf(stderr, "%s::%d parse server.inet4_addr failed", __FILE__, __LINE__);
		lua_error(l);
	}
	else {
		char *addr = (char *)lua_tostring(l, -1);
		inet_pton(AF_INET, addr, &config->server.inet4_addr);
		fprintf(stderr, "%s::%d server.inet4_addr =  %s\n", __FILE__, __LINE__, addr);
	}
	lua_pop(l, 1);

	/* parse server.max_size_in_shm */
	lua_getfield(l, -1, "max_size_in_shm");
	if(lua_isstring(l, -1)) {
		char *value = (char *)lua_tostring(l, -1);
		lua_size_parse(&config->server.max_size_in_shm, value);
		fprintf(stderr, "%s::%d server.max_size_in_shm = %lld\n",\
					   	__FILE__, __LINE__, config->server.max_size_in_shm);
	}
	else if(lua_isnumber(l, -1)) {
		config->server.max_size_in_shm = lua_tointeger(l, -1);
		fprintf(stderr, "%s::%d server.max_size_in_shm =  %lld\n",\
					   	__FILE__, __LINE__, config->server.max_size_in_shm);
	}
	else {
		fprintf(stderr, "%s::%d parse server.max_size_in_shm failed", __FILE__, __LINE__);
		lua_error(l);
	}
	lua_pop(l, 1);

	/* parse server.max_size_in_mem */
	lua_getfield(l, -1, "max_size_in_mem");
	if(lua_isstring(l, -1)) {
		char *value = (char *)lua_tostring(l, -1);
		lua_size_parse(&config->server.max_size_in_mem, value);
		fprintf(stderr, "%s::%d server.max_size_in_mem =  %lld\n",\
					   	__FILE__, __LINE__, config->server.max_size_in_mem);
	}
	else if(lua_isnumber(l, -1)) {
		config->server.max_size_in_mem = lua_tointeger(l, -1);
		fprintf(stderr, "%s::%d server.max_size_in_shm =  %lld\n",\
					   	__FILE__, __LINE__, config->server.max_size_in_mem);
	}
	else {
		fprintf(stderr, "%s::%d parse server.max_size_in_mem failed", __FILE__, __LINE__);
		lua_error(l);
	}
	lua_pop(l, 1);

	/* pop server table */
	lua_pop(l, 1);
}


static void lua_size_parse(long long *sz, const char *value)
{
	assert(value);
	assert(sz);

	char *token = (char *)value;
	int len = strlen(value);

	*sz = atoll(value);

	if(isdigit(value[len - 1]))
		return ;

	if(token[len-1] == 'K' || token[len-1] == 'k') 
		*sz = *sz << 10;
	if(token[len-1] == 'M' || token[len-1] == 'm') 
		*sz = *sz << 20;
	if(token[len-1] == 'G' || token[len-1] == 'g') 
		*sz = *sz << 30;
}

static void lua_time_parse(time_t *time, const char *value)
{
	assert(value);
	assert(time);
	
	char *token = (char *)value;
	int len = strlen(value);

	*time = atol(value);
	
	if(isdigit(value[len -1]))
		return ;
	
	if(tolower(token[len - 1]) == 's')
		(void *)0;
	else if(tolower(token[len - 1]) == 'm')
		*time *= 60;
	else if(tolower(token[len - 1]) == 'h')
		*time *= 360;
	else if(tolower(token[len - 1]) == 'd') 
		*time *= 86400;
	else{
		fprintf(stderr, "%s::%d lua_time_parse unkown time %s\n", value);
	}
}

static void lua_conf_timeout_parse(lua_State *l)
{
	assert(l);

	lua_getglobal(l, "timeout"); /* remaid to pop */
	if(lua_isnil(l, -1)) {
		fprintf(stderr, "%s::%d parse timeout failed, return nil\n", __FILE__, __LINE__);
		lua_pop(l, 1);
		return;
	}

	if(!lua_istable(l, -1)) {
		fprintf(stderr, "%s::%d parse timeout failed", __FILE__, __LINE__);
		lua_error(l);
	}

	/* parse timeout.request */
	lua_getfield(l, -1, "request");
	if(lua_isnumber(l, -1)) {
		config->timeout.request = lua_tointeger(l, -1);
		fprintf(stderr, "%s::%d timeout.request = %ld\n", __FILE__, __LINE__,  config->timeout.request);
	}
	else if(lua_isstring(l, -1)) {
		char *value = (char *)lua_tostring(l, -1);
		lua_time_parse(&config->timeout.request, value);
		fprintf(stderr, "%s::%d timeout.request = %d\n", __FILE__, __LINE__, config->timeout.request);
	}
	else {
		fprintf(stderr, "%s::%d timeout.request failed\n", __FILE__, __LINE__);
	}
	lua_pop(l, 1);


	/* parse timeout.reply */
	lua_getfield(l, -1, "reply");
	if(lua_isnumber(l, -1)) {
		config->timeout.reply = lua_tointeger(l, -1);
		fprintf(stderr, "%s::%d timeout.reply = %d\n", __FILE__, __LINE__, config->timeout.reply);
	}
	else if(lua_isstring(l, -1)) {
		char *value = (char *)lua_tostring(l, -1);
		lua_time_parse(&config->timeout.reply, value);
		fprintf(stderr, "%s::%d timeout.reply = %d\n", __FILE__, __LINE__, config->timeout.reply);
	}
	else {
		fprintf(stderr, "%s::%d timeout.reply failed\n", __FILE__, __LINE__);
	}
	lua_pop(l, 1);

	/* parse timeout.connect */
	lua_getfield(l, -1, "connect");
	if(lua_isnumber(l, -1)) {
		config->timeout.connect = lua_tointeger(l, -1);
		fprintf(stderr, "%s::%d timeout.connect = %d\n", __FILE__, __LINE__, config->timeout.connect);
	}
	else if(lua_isstring(l, -1)) {
		char *value = (char *)lua_tostring(l, -1);
		lua_time_parse(&config->timeout.connect, value);
		fprintf(stderr, "%s::%d timeout.connect = %d\n", __FILE__, __LINE__,  config->timeout.connect);
	}
	else {
		fprintf(stderr, "%s::%d timeout.connect failed\n", __FILE__, __LINE__);
	}
	lua_pop(l, 1);


	/* parse timeout.client_life */
	lua_getfield(l, -1, "client_life");
	if(lua_isnumber(l, -1)) {
		config->timeout.client_life = lua_tointeger(l, -1);
		fprintf(stderr, "%s::%d timeout.client_life = %d\n", __FILE__, __LINE__, config->timeout.client_life);
	}
	else if(lua_isstring(l, -1)) {
		char *value = (char *)lua_tostring(l, -1);
		lua_time_parse(&config->timeout.client_life, value);
		fprintf(stderr, "%s::%d timeout.client_life = %d\n", __FILE__, __LINE__, config->timeout.client_life);
	}
	else {
		fprintf(stderr, "%s::%d timeout.client_life failed\n", __FILE__, __LINE__);
	}
	lua_pop(l, 1);


	/* parse timeout.server_life */
	lua_getfield(l, -1, "server_life");
	if(lua_isnumber(l, -1)) {
		config->timeout.server_life = lua_tointeger(l, -1);
		fprintf(stderr, "%s::%d timeout.server_life = %d\n", __FILE__, __LINE__, config->timeout.server_life);
	}
	else if(lua_isstring(l, -1)) {
		char *value = (char *)lua_tostring(l, -1);
		lua_time_parse(&config->timeout.server_life, value);
		fprintf(stderr, "%s::%d timeout.server_life = %d\n", __FILE__, __LINE__, config->timeout.server_life);
	}
	else {
		fprintf(stderr, "%s::%d timeout.server_life failed\n", __FILE__, __LINE__);
	}
	lua_pop(l, 1);
	
	/* pop timeout */
	lua_pop(l, 1);
}

static void lua_conf_cache_dir_parse(lua_State *l)
{
	assert(l);

	lua_getglobal(l, "cache_dir");
	if(lua_isnil(l, -1)) {
		fprintf(stderr, "%s::%d parse cache_dir failed, return nil\n", __FILE__, __LINE__);
		lua_pop(l, 1);
		return;
	}
	
	if(!lua_istable(l, -1)) {
		fprintf(stderr, "%s::%d parse cache_dir failed", __FILE__, __LINE__);
		lua_error(l);
	}
	lua_pushnil(l);  /* the first key */
	while (lua_next(l, -2) != 0) {
		if(lua_istable(l, -1)) 
			cache_dir_add(config->cache_dir, l);
		/* remove value */
		lua_pop(l, 1);
	}

	lua_pop(l, 1);
}

static void cache_dir_add(struct array_t *dir, lua_State *l)
{
	if(!dir) 
		return;

	if(!l) 
		return;

	struct cache_dir *dentry = calloc(1, sizeof(*dentry));
	assert(dentry);
	
	lua_getfield(l, -1, "path");
	if(lua_isstring(l, -1)) {
		dentry->path = strdup((char *)lua_tostring(l, -1));
		fprintf(stderr, "%s::%d cache_dir[].path = %s\n", \
						__FILE__, __LINE__, dentry->path);
	}
	else {
		fprintf(stderr, "%s::%d cache_dir[].path get failed\n", \
						__FILE__, __LINE__);
	}
	lua_pop(l, 1);
	
	/* parse max_size */	
	lua_getfield(l, -1, "max_size");
	if(lua_isstring(l, -1)) {
		char *value = (char *)lua_tostring(l, -1);
		lua_size_parse(&dentry->max_size, value);
		fprintf(stderr, "%s::%d cache_dir[].max_size = %lld\n", \
						__FILE__, __LINE__, dentry->max_size);
	}
	else if(lua_isnumber(l, -1)) {
		dentry->max_size = lua_tointeger(l, -1);
		fprintf(stderr, "%s::%d cache_dir[].max_size = %lld\n", \
						__FILE__, __LINE__, dentry->max_size);
	}
	else {
		fprintf(stderr, "%s::%d cache_dir[].max_size get failed\n", \
						__FILE__, __LINE__);
	}
	lua_pop(l, 1);

	array_append(dir, dentry);
}


static void lua_stack_view(lua_State *l)
{
	int i;
	int top;
	top = lua_gettop(l);
	
	fprintf(stderr, "stack count %d\n", top);

	for(i = 1; i <= top; i++) {
		fprintf(stderr, "[%d] %s\n", i, lua_typename(l, lua_type(l, -1)));
	}
	fprintf(stderr, "\n");
}
