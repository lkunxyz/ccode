
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <assert.h>
#include <strings.h>
#include <sys/mman.h>

#include "server.h"
#include "sock_io.h"
#include "http.h"
#include "log.h"
#include "cache_file.h"
#include "md5.h"
#include "file_aio.h"
#include "cache_dir.h"
#include "cache_index.h"

static const char *HttpRequestLineFormat = "%s %s HTTP/%d.%d\r\n";
static const char *HttpReplyLineFormat = "HTTP/%d.%d %d %s\r\n";

static struct request_t *request_parse(const char *buf, const size_t size);
static void http_original_connect(int fd, void *data);
static void http_original_connect_finish(int fd, void *data);
static void http_request_send(int fd, void *data);
static struct reply_t * http_reply_create(void);
static void reply_free(struct reply_t *reply);
static void http_request_send_finish(int error, void *data, int size) ;
static void http_request_hit(struct http_state *http);
static void http_reply_header_send(int fd, void *data);
static void http_reply_header_send_done(int error, void *data, int size);
static void http_reply_body_read(int fd, void *data);
static void http_reply_body_read_done(int error, void *data, int size);
static void http_reply_body_send(int fd, void *data);
static void http_reply_body_send_done(int error, void *data, int size);
static struct string *reply_header_build(struct http_state *http, \
				struct header_t *head, enum http_status statu, struct http_version version);
static int http_server_fd_create(struct http_state *http);
static void http_revalidate_reply_handle(int fd, void *data);

void cache_time_set(struct http_state *http, struct cache_file *cache)
{
	if(!http || !cache) 
		return ;
	if(!http->reply) 
		return;
	
	struct header_entry_t *entry;
	struct header_t *reply_header = http->reply->headers;
	if(!reply_header)
		return;
	
	current_time_get();
	cache->time.response_got = current_time.tv_sec;
	cache->time.max_age = http->reply->cc.max_age;

	if((entry = header_entry_get(reply_header, HDR_DATE))) 
		cache->time.date = parse_rfc1123(entry->value, strlen(entry->value));
	else 
		cache->time.date = -1;

	if((entry = header_entry_get(reply_header, HDR_EXPIRES))) 
		cache->time.expires = parse_rfc1123(entry->value, strlen(entry->value));
	else 
		cache->time.expires = -1;
	
	if((entry = header_entry_get(reply_header, HDR_LAST_MODIFIED))) 
		cache->time.last_modified = parse_rfc1123(entry->value, strlen(entry->value));
	else 
		cache->time.last_modified = -1;
	
	if((entry = header_entry_get(reply_header, HDR_AGE))) 
		cache->time.header_age = atol(entry->value);
	else 
		cache->time.header_age = -1;
}

void http_connect_timeout_handler(int fd, void *data)
{
	warning("http_connect_timeout_handler fd %d, not connected.\n", fd);
	struct http_state *http = data;
	if(!data)
		return;
	
	http->ref_count = 0;
	
	http_state_free(http);
}

void http_server_life_timeout_handler(int fd, void *data)
{
	warning("http_server_life_timeout_handler fd %d, no data in.\n", fd);
	struct http_state *http = data;
	if(!data)
		return;
	
	http->ref_count = 0;
	
	http_state_free(http);
}

void http_request_timeout_handler(int fd, void *data)
{
	warning("http_request_timeout_handler fd %d, no data in.\n", fd);
	struct http_state *http = data;
	if(!data)
		return;
	
	http->ref_count = 0;
	
	http_state_free(http);
}

void http_life_timeout_handler(int fd, void *data)
{
	warning("http_life_timeout_handler fd %d, no data in.\n", fd);
	struct http_state *http = data;
	if(!data)
		return;
	
	http->ref_count = 0;
	
	http_state_free(http);
}

void http_reply_timeout_handler(int fd, void *data)
{
	warning("http_reply_timeout_handler fd %d, no data in.\n", fd);
	struct http_state *http = data;
	if(!data)
		return;
	
	http->ref_count = 0;
	
	http_state_free(http);
}

char * obj_state_string(int stat)
{
	if(stat == TCP_NONE)
		return "TCP_NONE";
	else if(stat == TCP_MISS)
		return "TCP_MISS";
	else if(stat == TCP_HIT)
		return "TCP_HIT";
	else if(stat == TCP_SHM_HIT) 
		return "TCP_SHM_HIT";
	else if(stat == TCP_MEM_HIT) 
		return "TCP_MEM_HIT";
	else if(stat == TCP_REFRESH_HIT) 
		return "TCP_REFRESH_HIT";
	else if(stat == TCP_REFRESH_MISS) 
		return "TCP_REFRESH_MISS";
	else if(stat == TCP_REFRESH_ERROR) 
		return "TCP_REFRESH_MISS";
	else 
		return "TCP_ERROR";
}

char *http_request_string_build(const struct http_state *http) 
{
	if(!http)
		return NULL;
	
	static char retv[MAX_HEADER];
	char line[MAX_LINE];
	int i;
	struct request_t *request = http->request;
	struct header_entry_t *e;
	struct header_t *h = request->headers;

	snprintf(retv, MAX_HEADER, HttpRequestLineFormat, \
				request->method.name, request->url_path, request->version.major, request->version.minor);
	
	for(i = 0; i < h->entries.count; i++) {
		e = h->entries.items[i];
		assert(e);
		strcat(retv, e->name);
		strcat(retv, ": ");
		strcat(retv, e->value);
		strcat(retv, "\r\n");
	}
	/* handle revalidate */
	if(http->stat == TCP_REVALIDATE) {
		/* support ETag later */
		struct cache_file *cache = http->cache;
		assert(cache);
		if(cache->time.last_modified > 0) {
			snprintf(line, MAX_LINE - 1, "If-Modified-Since: %s\r\n", mkrfc1123(cache->time.last_modified));
		}
		else {
			warning("http_request_string_build last modfied %ld < 0, use Date time.\n", cache->time.last_modified);
			snprintf(line, MAX_LINE - 1, "If-Modified-Since: %s\r\n", mkrfc1123(cache->time.date));
		}
		strcat(retv, line);
	}
	/* add smartc mark */
	strcat(retv, "Via: Smartc cache server\r\n");
	strcat(retv, "\r\n");	
	
	return retv;
}

int ignore_error(int error_no) 
{
	switch (error_no) {
		case EINPROGRESS:
		case EAGAIN:
		case EINTR:
		case ERESTART:	
			return 1;
		default:
			return 0;
	}
		/* NOTREACHED */
}

void http_request_accept(int fd, void *data)
{
	sock_register_event(fd, http_request_accept, NULL, NULL, NULL);

	struct http_state *conn = calloc(1, sizeof(*conn));
	if(!conn) {
		error("http_request_accept %s\n", strerror(errno));
		return;
	}

	int socklen = sizeof(struct sockaddr);
	int client_fd = accept(fd, (struct sockaddr*)&conn->client_addr, (socklen_t *)&socklen);
	
	if(client_fd == -1) {
		error("http_request_accept %s! Exit.\n", strerror(errno));
		safe_free(conn);
		return;
	}

	fd_open(client_fd, FD_SOCKET);

	conn->client_fd = client_fd;
	current_time_get();
	conn->connect_time = current_time.tv_sec;

	if(NULL == inet_ntop(AF_INET, &(conn->client_addr.sin_addr), conn->client_ip, sizeof(conn->client_ip))) {
		warning("http_request_accept(%s)\n", strerror(errno));
	}
	
	if(!conn->cin)
		conn->cin = string_limit_init(BUF_SIZE);
	assert(conn->cin);

	sock_timeout_set(client_fd, conn, http_request_timeout_handler, config->timeout.request);
	sock_set_event(client_fd, SOCK_EVENT_RD);
	sock_register_event(client_fd, http_request_read, http_state_ref(conn), NULL, NULL);

	log_debug(__FILE__, __LINE__, "http_request_accept: accept fd %d, %s\n", client_fd, conn->client_ip);
}

void http_request_read(int fd, void *data)
{
	assert(data);
	
	struct http_state *conn = data;
	struct request_t *request;
	int size_need = conn->cin->size - conn->cin->offset;
	int offset = conn->cin->offset;
	int len;
	
	http_state_unref(conn);

	len = read(fd, conn->cin->buf + offset, size_need);

	if(len == -1) { 
		warning("%s. http_request_read(fd %d, data %p)\n", \
							strerror(errno), fd, data);
		if(!ignore_error(errno)) {
			sock_close(fd);
			http_state_free(conn);
		}
		else {
			sock_register_event(fd, http_request_read, http_state_ref(conn), NULL, NULL);
		}
		return;
	}
	else if(len == 0) {
		/* use timeout */
		if(conn->cin->offset == 0) {
			warning("http_request_read fd %d not data in.\n", fd);
			sock_close(fd);
			http_state_free(conn);
		}
		else 
			sock_register_event(fd, http_request_read, http_state_ref(conn), NULL, NULL);
		return;
	}
	else {
		conn->cin->offset += size_need;
		conn->cin->buf[conn->cin->offset] = '\0';

		log_debug(__FILE__, __LINE__, "conn->cin.buf %d\n------\n%s\n------\n", conn->cin->offset, conn->cin->buf);
		
		/* parse buf to request_t */
		request = request_parse(conn->cin->buf, conn->cin->offset);
		if(NULL == request) {
			warning("http_request_read fd %d failed to parse request\n", fd);
			sock_close(fd);
			http_state_free(conn);
			return;
		}
		
		sock_timeout_set(fd, http_life_timeout_handler, (void *)conn, config->timeout.client_life);
		conn->request = request;
		/* handle request */
		http_request_handle(fd, http_state_ref(conn));
	}
}

void http_request_handle(int fd, void *data) 
{
	if(!data)
		return;

	struct http_state *conn = data;
	struct request_t *req = conn->request;
	struct cache_file *cache;
	int server_fd = -1;
	
	http_state_unref(conn);

	if(req->method.id != METHOD_GET) {
		/* now only support GET */
		log_debug(__FILE__, __LINE__, "http_request_handle(don't support no GET method)\n");
		sock_close(fd);
		http_state_free(conn);
		return;
	}

	assert(req->uri);

	cache_control_set(&req->cc, req->headers);
	cache = cache_file_search(cache_key_public(req->uri, strlen(req->uri)));
	if(cache) {
		debug("http_request_handle fd %d '%s' HIT\n", fd, req->uri);

		conn->cache = cache_ref(cache);
		/* HIT */
		if(CACHE_FRESH == freshness_check(cache, req)) {
			conn->stat = TCP_HIT;
			conn->reply = http_reply_create();
			conn->reply->header_size = cache->header_size;
			conn->reply->body_size = cache->body_size;
			conn->reply->statu = cache->reply.statu;
			conn->reply->headers = cache->reply.headers;
			conn->reply->version = cache->reply.version;
		
			http_request_hit(http_state_ref(conn));
		}	
		else {
			/* need to revalidate */
			conn->stat = TCP_REVALIDATE;
			server_fd = http_server_fd_create(conn);
			debug("http_request_handle %s %s revalidate from server\n", req->uri, conn->server_ip);
			
			sock_timeout_set(server_fd, http_connect_timeout_handler, (void *)conn, config->timeout.connect);
			http_original_connect(server_fd, http_state_ref(conn));
		}
	}
	else {
		conn->stat = TCP_MISS;
		
		server_fd = http_server_fd_create(conn);

		log_debug(__FILE__, __LINE__, "http_request_handle %s %s not in cache\n", req->uri, conn->server_ip);
		
		cache = cache_file_create(req->uri);

		conn->cache = cache_ref(cache);

		sock_timeout_set(server_fd, http_connect_timeout_handler, (void *)conn, config->timeout.connect);

		http_original_connect(server_fd, http_state_ref(conn));
	}
}

void http_state_free(struct http_state *conn)
{
	if(!conn)
		return;

	if(conn->ref_count > 0) {
		warning("http_state_free %p linked\n", conn);
		return;
	}

	debug("http_state_free %p\n", conn);

	accesslog(conn);

	sock_close(conn->client_fd);
	sock_close(conn->server_fd);

	request_free(conn->request);
	reply_free(conn->reply);
	
	string_clean(conn->cin);

	shm_file_clean(conn->shm);
	
	cache_unref(conn->cache);

	safe_free(conn);
}

void request_free(struct request_t *request) 
{
	if(!request) 
		return;

	debug("request_free %p\n", request);
	safe_free(request->uri);

	safe_free(request->url_path);
	
	safe_free(request->host);

	header_free(request->headers);

	method_free(&request->method);

	safe_free(request);
}


static struct request_t *request_parse(const char *buf, const size_t size)
{
	assert(buf);
	assert(size >= 0);

	struct request_t *req;
	char *start = (char *)buf;
	char *end;
	char *meth = NULL;
	char *urlpath = NULL;
	int minor;
	int major;
	size_t request_line_size = 0;
	char line[MAX_URL];
		
	/* parse request line */
	if((end = (char *)memchr((char *)buf, '\n', size)) == NULL) {
		log_debug(__FILE__, __LINE__, "Incomplete request, waiting for end of request line.\n------\n%s\n------\n", \
					 buf);
		return NULL;
	}
	req = calloc(1, sizeof(*req));
	assert(req);
	
	request_line_size = end - start;

	end = (char *)memchr(start, ' ', request_line_size);
	
	assert(end);

	/* parse method */
	meth = (char *) strndup(start, end - start);
	req->method.name = meth;
	req->method.id = http_method_id_get(meth);
	if(req->method.id == METHOD_NONE) {
		log_debug(__FILE__, __LINE__, "request_parse(method '%s' unknown)\n", meth);
		safe_free(meth);
		safe_free(req);
		return NULL;
	}

	log_debug(__FILE__, __LINE__, "request_parse: method %s, %d\n", \
					req->method.name, req->method.id);
	
	/* parse url path */
	while(*end == ' ' || *end == '\t')
		end++;
	start = end;

	end = strchr(start, ' ');
	
	assert(end);

	urlpath = (char *)strndup(start, end - start);

	log_debug(__FILE__, __LINE__, "request_parse: urlpath %s\n", \
					urlpath);

	req->url_path = urlpath;

	/* parse http version */

	start = end + 1;
	
	if(strncasecmp(start, "http/", 5) != 0) {
		log_debug(__FILE__, __LINE__, "request_parse: can not find HTTP/\n");
		safe_free(meth);
		safe_free(req->url_path);
		safe_free(req);
		return NULL;
	}

	/* parse http version */
	major = atoi(start + 5);
	end = strchr(start + 5, '.');
	minor = atoi(end + 1);
	req->version.major = major;
	req->version.minor = minor;

	log_debug(__FILE__, __LINE__, "request_parse: http version %d.%d\n", major, minor);
		
	/* parse http header */
	req->headers = http_header_parse(buf + request_line_size + 1, size - request_line_size - 1);

	/* get host */
	struct header_entry_t *e = header_entry_get(req->headers, HDR_HOST);
	req->host = strdup(e->value);
	
	snprintf(line, MAX_URL, "http://%s%s", req->host, req->url_path);

	req->uri = strdup(line);

	return req;
}

static void http_original_connect(int fd, void *data)
{
	assert(data);

	struct http_state *conn = data;
	
	assert(fd == conn->server_fd);
	http_state_unref(conn);

	if(-1 == connect(conn->server_fd, (struct sockaddr *)&conn->server_addr, sizeof(struct sockaddr))) {
		if(EINPROGRESS == errno) {
			/* register event to listen server_fd */
			debug("http_original_connect(%s %d INPROGRESS)\n", \
						conn->server_ip, conn->server_fd);
			
			sock_set_event(fd, SOCK_EVENT_WR);
			sock_register_event(fd, NULL, NULL, http_original_connect_finish, http_state_ref(conn));
		}
		else {
			warning("http_original_connect fd %d, {%s cann't connect server}\n", fd);
			sock_close(conn->client_fd);
			sock_close(conn->server_fd);
			http_state_free(conn);
		}
		return;
	}

	/* connected sucessfully */
	log_debug(__FILE__, __LINE__, "http_original_connect(original %s)\n", conn->server_ip);
	http_original_connect_finish(fd, http_state_ref(conn));
}

static void http_original_connect_finish(int fd, void *data)
{
	assert(data);

	struct http_state *conn = data;
	int err;
	int errlen = sizeof(err);
	assert(fd == conn->server_fd);

	http_state_unref(conn);

	if(0 != getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen)) {
		warning("Warning! http_original_connect_finish(%s)\n", \
					strerror(errno));
		sock_close(fd);
		sock_close(conn->client_fd);
		http_state_free(conn);
		return;
	}
	
	if(err != 0) {
		warning("Warning! http_original_connect_finish {%s}\n", \
					strerror(errno));
		sock_close(fd);
		sock_close(conn->client_fd);
		http_state_free(conn);
		return;
	}
	
	log_debug(__FILE__, __LINE__, "http_original_connect_finish %s connected successfully\n", conn->server_ip);
	http_request_send(fd, http_state_ref(conn));
}

static void http_request_send(int fd, void *data)
{
	assert(data);
	
	struct http_state *conn = data;
	char *request_string = NULL;

	assert(fd == conn->server_fd);
	http_state_unref(conn);

	request_string = http_request_string_build(conn);

	if(request_string == NULL) {
		error("http_request_send fd %d request string is NULL\n", fd);
		http_state_free(conn);
		return;
	}
	
	log_debug(__FILE__, __LINE__, "http_request_send \n------\n%s\n------\n", request_string);

	sock_write(fd, request_string, strlen(request_string) , http_request_send_finish, http_state_ref(conn), 0);	
}

static void http_request_send_finish(int error, void *data, int size) 
{
	assert(data);
	
	struct http_state *http = data;
	struct cache_file *cache = http->cache;

	http_state_unref(http);

	if(error) {
		http_state_free(http);
		return;
	}
	
	log_debug(__FILE__, __LINE__, "success! http_request_send_finish %d bytes send\n", size);

	/* record request issued time, used to computer network delay later */
	current_time_get();
	cache->time.request_issued = current_time.tv_sec;

	sock_timeout_set(http->server_fd, http_reply_timeout_handler, (void *)http, config->timeout.reply);
	http_reply_read(http->server_fd, http_state_ref(http));
}

void http_reply_read(int fd, void *data)
{
	if(!data)
		return;

	struct http_state *http = data;
	struct reply_t *reply = http->reply;
	struct cache_file *cache = http->cache;
	int  size_reply;
	
	assert(fd == http->server_fd);
	
	http_state_unref(http);

	if(http->reply == NULL)
		http->reply = http_reply_create();
	else if(http->reply->parsed)
		string_reset(http->reply->in);

	reply = http->reply;
	
	size_reply = read(fd, reply->in->buf + reply->in->offset, reply->in->size - reply->in->offset);

	if(-1 == size_reply) {
		if(!ignore_error(errno)) {
			log_debug(__FILE__, __LINE__, "warning! http_reply_read(%s)\n", strerror(errno));
			if(reply->no_content_length) {
				if(cache->mem->context->offset > 0)
					mem_swapout(cache, http);
			
				assert(cache->body_size == reply->body_in);

				cache_file_insert(cache);
				cache_index_add(cache, CACHE_INDEX_ADD);
				sock_close(fd);
				http_state_free(http);
				return;
			}
			sock_close(fd);
		}
		else {
			/* register again */
			sock_set_event(fd, SOCK_EVENT_RD);
			sock_register_event(fd, http_reply_read, http_state_ref(http), NULL, NULL);
		}
		return;
	}
	else if(size_reply == 0) {
		/* need to finish */
		debug("waring! http_reply_read %d zero in\n", fd);
		if(reply->no_content_length) {
			sock_register_event(fd, http_reply_read, http_state_ref(http), NULL, NULL);
			return;
		}
		if(reply->body_in >= reply->body_size)
			sock_close(fd);
		else
			sock_register_event(fd, http_reply_read, http_state_ref(http), NULL, NULL);
			
		return;
	}
	else {
		log_debug(__FILE__, __LINE__, "http_reply_read %d\n", size_reply);	
		reply->in->offset +=  size_reply;
		
		if(!reply->parsed) {
			int error = reply_parse(reply, reply->in->buf, reply->in->offset);
			if(error == -1) {
				http_state_free(http);
				return;
			}
			if(!reply->parsed) {
				sock_register_event(fd, http_reply_read, http_state_ref(http), NULL, NULL);
				return;
			}
			if(http->stat == TCP_REVALIDATE) {
				http_revalidate_reply_handle(fd, http_state_ref(http));
				return;
			}
			/* set cache data structure */
			cache->header_size = reply->header_size;
			cache->body_size = reply->body_size;
			cache->reply.headers = header_ref(reply->headers);
			cache->reply.statu = reply->statu;
			cache->reply.version = reply->version;	
			
			reply->body_in = reply->in->offset - reply->header_size;
			sock_timeout_set(fd, http_server_life_timeout_handler, (void *)http, config->timeout.server_life);
			cache_control_set(&reply->cc, reply->headers);
			/* set cache time */
			cache_time_set(http, cache);
			cache->statu = cache_able(cache, http->request, http->reply);
			if(cache->statu == CACHE_NONE) {
				debug("http_reply_read '%s' can not cache. Delete from cache table.\n", http->request->uri);
				cache_file_delete(cache);
			}
			else {
				cache->time.freshness_lifetime = cache_freshness_lifetime_server(cache);
				if(!reply->no_content_length)
					cache_file_insert(cache);
			}
		}
		else {
			reply->body_in += size_reply;
			if(cache->body_size < 0) 
				cache->body_size = 0;
			cache->body_size += size_reply;
		}
		
		mem_string_append(cache, reply->in);
		
		if(cache->statu != CACHE_NONE)
			mem_cache_append(cache->mem_cache, reply->in->buf, reply->in->offset);
		
		if(cache->statu != CACHE_NONE && (reply->body_in >= reply->body_size) && \
						(reply->no_content_length == 0)) {
			debug("success! http_reply_read %d bytes download fully!\n", reply->body_in);
			/* add record to cache.index */
			cache_index_add(cache, CACHE_INDEX_ADD);
			sock_close(http->server_fd); /* if keep-alive, not to close */
		}

		if(!reply->header_send) {
			http_reply_header_send(http->client_fd, http_state_ref(http));
			reply->header_send = 1;
		}
		else 
			http_reply_body_send(http->client_fd, http_state_ref(http));
	}
}

struct http_state *http_state_ref(struct http_state *conn)
{
	assert(conn);
	
	conn->ref_count++;

	debug("http_state_ref %d\n", conn->ref_count);

	return conn;
}

void http_state_unref(struct http_state *conn)
{
	assert(conn);
	
	conn->ref_count--;

	if(conn->ref_count == 0)
		log_debug(__FILE__, __LINE__, "Tip! http_state_unref(%p ref_count 0)\n", conn);
}

static struct reply_t * http_reply_create(void)
{
	struct reply_t *reply = calloc(1, sizeof(*reply));
	if(reply == NULL) {
		log_debug(__FILE__, __LINE__, "fatal! http_reply_create(%s)\n", strerror(errno));
		return NULL;
	}
	reply->in = string_limit_init(BUF_SIZE);

	reply->headers = NULL;

	return reply;
}

/* -1: error; 0: success */
int reply_parse(struct reply_t *reply, const char *buf, const size_t size)
{
	assert(reply);
	assert(buf);
	assert(size > 0);
	
	char sline[MAX_LINE];
	char *start, *end;
	char *token;
	int s_len;
	int header_len;
	
	if(reply->parsed == 1) 
		return 0;
	
	start = (char *)buf;
	if(NULL == (end = strchr(start, '\n'))) {
		log_debug(__FILE__, __LINE__, "warning! reply_parse(uncompleted header)\n");
		return -1;
	}

	if(0 != strncasecmp(start, "HTTP/", 5)) {
		log_debug(__FILE__, __LINE__, "warning! reply_parse(unsupport reply)\n");
		return -1;
	}

	s_len = end - start;
	assert(s_len > 0);
	
	strncpy(sline, start, s_len + 1);  /* include '\n' */
	/* get http version */
	token = (char *)strtok(sline, " \t\r\n");
	if(token == NULL) {
		log_debug(__FILE__, __LINE__, "warning! reply_parse(uncompleted header)\n");
		return -1;
	}
	
	reply->version.major = atoi(start + 5);
	
	if(NULL == (start = strchr(sline, '.'))) {
		log_debug(__FILE__, __LINE__, "warning! reply_parse(unknown http version)\n");
		return -1;
	}
	reply->version.minor = atoi(start + 1);
	/* get http status */
	if(NULL == (token = (char *)strtok(NULL, " \t\r\n"))) {
		log_debug(__FILE__, __LINE__, "warning! reply_parse(not find statu code)\n");
		return -1;
	}
	reply->statu = atoi(token);
	debug("reply_parse statu %d\n", reply->statu);

	/* get reason */
	/*
	if(NULL == (token = (char *)strtok(NULL, " \t\r\n"))) {
		log_debug(__FILE__, __LINE__, "warning! reply_parse(not find reason)\n");
		return -1;
	}
	reply->reason = strdup(token);
	*/

	/* begin to parse header */
	start = end + 1;
	header_len = headers_end(start, size - s_len - 1);
	reply->headers = http_header_parse(start, header_len + 1);

	reply->header_size = headers_end(buf, size); /* include statu line */
	if(reply->headers) {
		reply->parsed = 1;
		struct header_entry_t *e = header_entry_get(reply->headers, HDR_CONTENT_LENGTH);
		if(e) {
			reply->body_size = atoi(e->value);
			log_debug(__FILE__, __LINE__, "reply_parse(header %d, body %d)\n", \
							reply->header_size, reply->body_size);
		}
		else {
			log_debug(__FILE__, __LINE__, "warning! reply_parse(not contain Content-Length)\n");
			reply->body_size = -1;
			reply->no_content_length = 1;
		}
		return 0;
	}
	else 
		return -1;
}

static void reply_free(struct reply_t *reply)
{
	if(!reply) 
		return;

	debug("reply_free %p\n", reply);
	string_clean(reply->in);
	header_free(reply->headers);

	safe_free(reply);
}

static void http_request_hit(struct http_state *http)
{
	if(!http) 
		return;
	
	struct cache_file *cache = http->cache;

	http_state_unref(http);

	if(http->shm == NULL) {
		http->shm = shm_file_create(cache->file->buf, cache->body_size);
	}
	
	http_reply_header_send(http->client_fd, http_state_ref(http));
}

static void http_reply_header_send(int fd, void *data)
{	
	assert(data);
	struct http_state *http = data;
	struct cache_file *cache = http->cache;
	struct reply_t *reply = http->reply;
	
	assert(cache);
	assert(reply);
	http_state_unref(http);

	if(cache->reply.headers == NULL) {
		debug("http_reply_header_send fd %d header not parsed\n", fd);
		sock_set_event(fd, SOCK_EVENT_WR);
		sock_register_event(fd, NULL, NULL, http_reply_header_send, http_state_ref(http));
		return;
	}
	if(!http->shm && (http->stat == TCP_HIT || http->stat == TCP_REFRESH_HIT)) {
		http->shm = shm_file_create(cache->file->buf, cache->body_size);

		if(!http->shm) {
			sock_set_event(fd, SOCK_EVENT_WR);
			sock_register_event(fd, NULL, NULL, http_reply_header_send, http_state_ref(http));
			return;
		}
	}

	struct string *hstr = reply_header_build(http, cache->reply.headers, cache->reply.statu, cache->reply.version);
	
	assert(hstr);
	
	sock_write(http->client_fd, hstr->buf, hstr->offset, http_reply_header_send_done, http_state_ref(http), 0);	
	
	string_clean(hstr);
}

static void http_reply_header_send_done(int error, void *data, int size)
{
	assert(data);

	struct http_state *http = data;
	struct cache_file *cache = http->cache;
	struct mem *m = cache->mem;

	http_state_unref(http);

	if(error) {
		s_error("http_reply_header_send_done %s\n", strerror(errno));
		http_state_free(http);
		return;
	}
	
	http->reply->header_out = size;
	debug("http_reply_header_send_done client %s, header out %d\n", http->client_ip, size);
	
	if(http->stat == TCP_HIT || TCP_REFRESH_HIT == http->stat) {
		http->shm->low = http->shm->high = cache->header_size; /* is useful? */
		
		assert(http->shm->body_size > 0);
		if(shm_cache_find(http, http->cache->shm_cache))  {
			http->stat = TCP_SHM_HIT;
			http_reply_body_send(http->client_fd, http_state_ref(http));
		}
		else if(mem_cache_find(http, http->cache->mem_cache)) {
			http->stat = TCP_MEM_HIT;
			http_reply_body_send(http->client_fd, http_state_ref(http));
		}
		else {
			debug("http_reply_header_send_done fd %d not find shm or mem cache\n", http->client_fd);
			http_reply_body_read(http->client_fd, http_state_ref(http));
		}
	}
	else if(http->stat == TCP_MISS || TCP_REFRESH_MISS == http->stat || TCP_REFRESH_ERROR == http->stat) {
		m->submit = cache->header_size;
		http_reply_body_send(http->client_fd, http_state_ref(http));
	}
}

static struct string *reply_header_build(struct http_state *http, struct header_t *head, enum http_status statu, struct http_version version)
{
	int i;
	struct header_entry_t *e;
	struct header_t *h = head;
	int header_sz = 0;
	struct string *retv = string_limit_init(BUF_SIZE);
	char sline[MAX_LINE];
	int res;

	res = snprintf(sline, MAX_LINE - 1, HttpReplyLineFormat, \
			version.major, version.minor, statu, http_status_string(statu));
	
	string_append(retv, sline, res);
	for(i = 0; i < h->entries.count; i++) {
		e = h->entries.items[i];
		assert(e);
		if(e->name) 
			string_append(retv, e->name, strlen(e->name));
		string_append(retv, ": ", 2);
		if(e->value)
			string_append(retv, e->value, strlen(e->value));
		string_append(retv, "\r\n", 2);
	}
	/* add mark */
	snprintf(sline, MAX_LINE, "Via: %s with %s by smartc\r\n", obj_state_string(http->stat), \
					http->cache ? cache_statu(http->cache->statu) : "-");
	string_append(retv, sline, strlen(sline));
	string_append(retv, "\r\n", 2);	
	return retv;
}

static void http_reply_body_read(int fd, void *data)
{
	assert(data);
	
	struct http_state *http = data;
	struct cache_file *cache = http->cache;
	struct shm_file *shm = http->shm;
	struct reply_t *reply = http->reply;
	struct mmap *map;
	int size_need = 0;
	int offset = 0;

	assert(reply);
	assert(fd == http->client_fd);
	assert(shm);

	http_state_unref(http);

	map = shm->map;
	if(!map) {
		shm->map = mmap_create(shm->body_size, 0);
		if(shm->map == NULL) {
			http_state_free(http);
			return ;
		}
		map = shm->map;
	}

	if(map->start == NULL) {
		int map_size = (cache->body_size - reply->body_out) > CHUNK_SIZE ? CHUNK_SIZE : \
					   (cache->body_size - reply->body_out);
		mmap_reset(map, map_size);
	}	

	size_need = map->size - map->offset;
	offset =  shm->low + map->offset; /* ignore header string */
	
	debug("http_reply_body_read disk %d, start %p, %d bytes, offset %d, body out %d\n", \
					shm->disk_fd, map->start, size_need, offset, reply->body_out);

	file_aio_read(shm->disk_fd, map->start + map->offset, size_need, offset, \
					http_reply_body_read_done, http_state_ref(http));
}

static void http_reply_body_read_done(int error, void *data, int size)
{
	assert(data);
	
	struct http_state *http = data;
	struct shm_file *shm = http->shm;
	struct mmap *map = shm->map;
	struct cache_file *cache = http->cache;

	http_state_unref(http);

	if(error) {
		s_error("http_reply_body_read_done %s\n", strerror(errno));
		sock_close(http->client_fd);
		http_state_free(http);
		return;
	}
	assert(map);
	assert(size >= 0);

	map->offset += size;
	shm->high += size;

	debug("http_reply_body_read_done map %d ===> %d\n", \
					map->offset, map->size);
	debug("success! http_reply_body_read_done %d-%d, size %d\n", \
					shm->low, shm->high, size);

	if(shm->map->offset < shm->map->size) {
		/* continue to read */
		http_reply_body_read(http->client_fd, http_state_ref(http));
	}
	else {
		struct shm_cache *temp = NULL;
		if(!shm_cache_find(http, cache->shm_cache)) {
			if((temp = shm_cache_append(cache->shm_cache, shm)) != NULL) {
				debug("http_reply_body_read_done %d-%d cached\n" , temp->low, temp->high);
				shm->map->cached = 1;
			}
			else 
				shm->map->cached = 0;
		}
		
		shm->low += map->offset;
		assert(shm->low == shm->high);
		
		if(map->start) {
			munmap(map->start, map->size);
			map->start = NULL;
		}
		/* begin to send body */
		if(http->stat != TCP_HIT && http->stat != TCP_REFRESH_HIT)
			http->stat = TCP_HIT;
		http_reply_body_send(http->client_fd, http_state_ref(http));
	}
}

static void http_reply_body_send(int fd, void *data)
{
	assert(data);
	struct http_state *http = data;
	struct reply_t *reply = http->reply;
	struct cache_file *cache = http->cache;
	struct shm_file *shm = http->shm;
	struct mem *m = cache->mem;
	struct shm_cache *sch = http->sc;
	struct mem_cache *mc = http->mc;
	int size_need = 0;
	int offset = 0;

	assert(reply);
	assert(fd == http->client_fd);
	http_state_unref(http);

	if(http->stat == TCP_HIT || TCP_REFRESH_HIT == http->stat) { 
		size_need = shm->high - http->reply->body_out - cache->header_size;
		offset = shm->map->offset - size_need;
		debug("http_reply_body_send %s fd %d, offset %d, size %d\n", obj_state_string(http->stat), fd, offset, size_need);
		
		sock_file_send(shm->map->shm_fd, http->client_fd, offset, \
						size_need, http_reply_body_send_done, http_state_ref(http));
	}
	else if(http->stat == TCP_SHM_HIT) {
		assert(sch);
		assert(sch->high > sch->low);
		size_need = sch->high - reply->body_out - reply->header_size;
		offset = reply->body_out + reply->header_size - sch->low;
		
		debug("http_reply_body_send TCP_SHM_HIT fd %d, offset %d, size %d\n", fd, offset, size_need);

		sock_file_send(sch->fd, fd, offset, size_need, http_reply_body_send_done, http_state_ref(http));
	}
	else if(http->stat == TCP_MEM_HIT) {
		assert(mc);
		int body_in_mem = mc->high - cache->header_size;
		size_need = body_in_mem - reply->body_out;
		assert(size_need <= mc->mbuf->offset);
		assert(mc->high - mc->low == mc->mbuf->offset);
		offset = reply->header_size + reply->body_out;

		debug("http_reply_body_send string offset %d, size %d\n", mc->mbuf->offset, mc->mbuf->size);
		debug("http_reply_body_send TCP_MEM_HIT fd %d, offset %d, size %d\n", fd, offset, size_need);
		sock_write(fd, mc->mbuf->buf + offset, size_need, http_reply_body_send_done, http_state_ref(http), 0);
	}
	else if(http->stat == TCP_MISS || TCP_REFRESH_MISS == http->stat || TCP_REFRESH_ERROR == http->stat) {
		if(!m) 
			return;
		struct string *s = m->context;
		assert((m->high - m->low) == s->offset);
		debug("http_reply_body_send fd %d , offset %d, size %d\n", fd, s->offset, s->size);

		size_need = s->offset - m->submit;
		debug("http_reply_body_send fd %d , %d bytes to send, submited %d bytes\n", fd, size_need, m->submit);
		/* append event to write queue */	
		sock_write(fd, s->buf + m->submit, size_need, http_reply_body_send_done, http_state_ref(http), 1);

		m->submit += size_need;
	
		if(m->context->offset >= m->capacity) {
			mem_swapout(cache, http);
			return;
		}

		if((m->high >= (cache->header_size + cache->body_size)) && \
						(http->reply->no_content_length == 0)) {
			mem_swapout(cache, http);
			return;
		}
		/* reregister http_reply_read for server fd */
		sock_register_event(http->server_fd, http_reply_read, http_state_ref(http), NULL, NULL);
	}
}

static void http_reply_body_send_done(int error, void *data, int size)
{
	assert(data);

	struct http_state *http = data;
	struct cache_file *cache = http->cache;
	int fd = http->client_fd;
	struct fd_t *f = &fd_table[fd];
	struct event *e = f->wqueue;
	struct reply_t *reply = http->reply;
	
	assert(reply);
	http_state_unref(http);
		
	if(e) {
		void (*efree)(struct event **) = e->free;
		efree(&f->wqueue);
	}
	
	if(error) {
		s_error("http_reply_body_send_done fd %d %s\n", fd, strerror(errno));
		sock_close(http->client_fd);
		sock_close(http->server_fd);
		http_state_free(http);
		return;
	}

	reply->body_out += size;

	debug("http_reply_body_send_done %d bytes, offset %d, size %d\n", \
					size, reply->body_out, reply->body_size);

	if(reply->body_out >= reply->body_size && !reply->no_content_length) {
		debug("success! http_reply_body_send_done %d bytes all send!\n", reply->body_out);
		sock_close(http->client_fd);
		http_state_free(http);
		return;
	}

	if(http->stat == TCP_HIT || TCP_REFRESH_HIT == http->stat) {
		http_reply_body_read(http->client_fd, http_state_ref(http));
	}
	else if(http->stat == TCP_SHM_HIT || http->stat == TCP_MEM_HIT) {
		if(shm_cache_find(http, http->cache->shm_cache)) 
			http_reply_body_send(http->client_fd, http_state_ref(http));
		else if(mem_cache_find(http, http->cache->mem_cache)) 
			http_reply_body_send(http->client_fd, http_state_ref(http));
		else 
			http_reply_body_read(http->client_fd, http_state_ref(http));
	}
	else if(TCP_MISS == http->stat || TCP_REFRESH_MISS == http->stat || TCP_REFRESH_ERROR == http->stat) { 
		if(f->ops.write_handler != NULL) {
			debug("warning! http_reply_send_finish fd %d write handle is not finished!\n", fd);
			return;
		}
		if(f->wqueue == NULL) {
			log_debug(__FILE__, __LINE__, "http_reply_body_send_done events null\n");
		}
		/* run event again */
		sock_register_event(http->client_fd, NULL, NULL, sock_event_run, NULL);
	}
}


static int http_server_fd_create(struct http_state *http)
{
	assert(http);

	struct request_t *req = http->request;
	struct sockaddr_in srv; 
	struct hostent *srv_host;
	int server_fd = sock_open();

	if(server_fd == -1) {
		warning("http_server_fd_create fd %d, failed to open new socket\n", server_fd);
		sock_close(http->client_fd);
		http_state_free(http);
		return -1;
	}

	srv_host = gethostbyname(req->host);
	if(srv_host == NULL) {
		warning("http_server_fd_create(%s)\n", strerror(errno));
		sock_close(http->client_fd);
		sock_close(server_fd);
		http_state_free(http);
		return -1;
	}

	srv.sin_family = AF_INET;
	srv.sin_port = htons(80);

	if(NULL == inet_ntop(AF_INET, srv_host->h_addr, http->server_ip, 16)) {
		warning("http_server_fd_create(inet_ntop failed %s)\n", strerror(errno));
		sock_close(http->client_fd);
		sock_close(server_fd);
		http_state_free(http);
		return -1;
	}
		
	if(inet_pton(AF_INET, http->server_ip, &srv.sin_addr) < 0) {
		debug("http_server_fd_create %s\n", strerror(errno));
		sock_close(http->client_fd);
		sock_close(server_fd);
		http_state_free(http);
		return -1;
	}

	http->server_fd = server_fd;
	http->server_addr = srv;
	
	debug("http_server_fd_create success create %d!\n", server_fd);
	return server_fd;
}

static void http_revalidate_reply_handle(int fd, void *data)
{
	if(!data) 
		return;

	struct http_state *http = data;
	struct cache_file *cache = http->cache;
	struct reply_t *reply = http->reply;

	assert(cache);
	assert(reply);
	http_state_unref(http);

	if(reply->statu == HTTP_NOT_MODIFIED) {
		debug("http_revalidate_reply_handle fd %d HTTP_NOT_MODIFIED\n", reply->statu);
		cache_control_set(&reply->cc, reply->headers);
		cache_time_set(http, cache);
		http->stat = TCP_REFRESH_HIT;
		reply->header_size = cache->header_size;
		reply->body_size = cache->body_size;
		reply->statu = cache->reply.statu;
		reply->headers = cache->reply.headers;
		reply->version = cache->reply.version;
		http_request_hit(http_state_ref(http));
		return;
	}
	else if(reply->statu == HTTP_OK) {
		debug("http_revalidate_reply_handle %d HTTP_OK, document has modified\n", reply->statu);
		cache_file_reset_from_reply(cache, reply);
		cache_file_delete(cache);
			
		http->stat = TCP_REFRESH_MISS;
		reply->body_in = reply->in->offset - reply->header_size;
		sock_timeout_set(fd, http_server_life_timeout_handler, (void *)http, config->timeout.server_life);
		cache_control_set(&reply->cc, reply->headers);
		/* set cache time */
		cache_time_set(http, cache);
		cache->statu = cache_able(cache, http->request, http->reply);
		if(cache->statu == CACHE_NONE) {
			debug("http_reply_read '%s' can not cache. Delete from cache table.\n", http->request->uri);
		}
		else {
			cache->time.freshness_lifetime = cache_freshness_lifetime_server(cache);
		}
		
		mem_string_append(cache, reply->in);

		if(cache->statu != CACHE_NONE)
				mem_cache_append(cache->mem_cache, reply->in->buf, reply->in->offset);

		if(cache->statu != CACHE_NONE && reply->body_in >= reply->body_size) {
				debug("success! http_reply_read %d bytes download fully!\n", reply->body_in);
				/* add record to cache.index */
				struct cache_index *ci = index_build_from_cache(cache);
				ci->operate = CACHE_INDEX_ADD;
				struct cache_dir *d = config->cache_dir->items[cache->dir_no];
				if(cache->statu != CACHE_NONE)
						cache_index_log(ci, d->new_fd);
				sock_close(http->server_fd); /* if keep-alive, not to close */
		}

		if(!reply->header_send) {
				http_reply_header_send(http->client_fd, http_state_ref(http));
				reply->header_send = 1;
		}
		else
				http_reply_body_send(http->client_fd, http_state_ref(http));
		
		return;
	}
	/* delete the document */
	debug("http_revalidate_reply_handle error %d reply\n", reply->statu);
	http->stat = TCP_REFRESH_ERROR;
	cache_file_reset_from_reply(cache, reply);
	
	cache->statu = CACHE_NONE;	
	reply->body_in = reply->in->offset - reply->header_size;
	sock_timeout_set(fd, http_server_life_timeout_handler, (void *)http, config->timeout.server_life);
	cache_file_delete(cache);
	
	sock_register_event(fd, http_reply_read, http_state_ref(http), NULL, NULL);
	return;
}

