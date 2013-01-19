
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
#ifndef HTTP_H
#define HTTP_H
#include <arpa/inet.h>
#include "http_header.h"
#include "cache_file.h"
#include "string.h"

enum obj_state {
	TCP_NONE = -1,
	TCP_MISS = 0,
	TCP_HIT = 1, 
	TCP_REVALIDATE,
	TCP_REFRESH_HIT, 
	TCP_REFRESH_MISS,
	TCP_REFRESH_ERROR,
	TCP_SHM_HIT, 
	TCP_MEM_HIT,
};

struct http_state {
	int ref_count;
	int client_fd;
	int server_fd;
	int cout;
	int sin;
	int stat;
	time_t connect_time;
	char client_ip[16];
	char server_ip[16];
	
	/* client bytes */
	struct string *cin;

	struct request_t *request;
	struct reply_t *reply;
	struct cache_file *cache; /* local cache file, not to free */
	struct shm_file *shm; /* used from HIT request */
	struct shm_cache *sc; /* cache in /dev/shm */
	struct mem_cache *mc; /* cache in memory */
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;
};


struct reply_t {
	int header_size;
	int body_size;
	int header_out;
	int body_out;
	int body_in;
	int parsed;
	int header_send;
	int statu;
	int no_content_length;  /* do not have Content-Length */
	struct cache_control cc;

	struct http_version version;
	struct header_t *headers;
	struct string *in;
};

struct request_t {
	char *uri;
	char *url_path;
	char *host;
	struct header_t *headers;
	struct http_version version;
	struct method_t method;
	struct cache_control cc;
};


extern void http_state_free(struct http_state *conn);
extern void request_free(struct request_t *r);
extern void http_request_accept(int fd, void *data);
extern void http_request_read(int fd, void *data);
extern int ignore_error(int error_no);
extern void http_request_handle(int fd, void *data);
extern void http_error_send(struct http_state *conn);
extern time_t parse_rfc1123(const char *str, int len);
extern const char *mkrfc1123(time_t t);
extern const char *mkhttpdlogtime(const time_t * t);
extern char *http_request_string_build(const struct http_state *http);
extern void http_reply_read(int fd, void *data);
extern struct http_state * http_state_ref(struct http_state *conn);
extern void http_state_unref(struct http_state *conn);
extern char * obj_state_string(int stat);
extern struct shm_cache *shm_cache_find(struct http_state *http, struct shm_cache *sc);
extern struct mem_cache *mem_cache_find(struct http_state *http, struct mem_cache *head);
extern int reply_parse(struct reply_t *reply, const char *buf, const size_t size);
extern void http_request_timeout_handler(int fd, void *data);
extern void http_life_timeout_handler(int fd, void *data);
extern void http_reply_timeout_handler(int fd, void *data);
extern void http_server_life_timeout_handler(int fd, void *data);
extern void http_connect_timeout_handler(int fd, void *data);
extern void cache_time_set(struct http_state *http, struct cache_file *cache);
extern void cache_file_reset_from_reply(struct cache_file *cache, struct reply_t *reply);
#endif
