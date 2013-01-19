
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
#include <string.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>  

#include "server.h"
#include "sock_io.h"
#include "sock_epoll.h"
#include "http.h"
#include "log.h"

static void fd_close(int fd);
static void sock_set_reuseaddr(int fd);
static int sock_listen_open(void);
static void sock_handle_write(int fd, void *data);
static void sock_handle_sendfile(int fd, void *data);
static void sock_set_cork(int fd);

void sock_timeout_set(int fd, void *data, PF *callback, time_t time)
{
	if(time <= 0) 
		return;
	
	struct fd_t *f = &fd_table[fd];
	time_t now;
	if(!f->flags.open) 
		return;
	
	current_time_get();
	now = current_time.tv_sec;
	f->timeout = now + time;
	f->ops.timeout_data = data;
	f->ops.timeout_handler = callback;
	
	debug("sock_timeout_set fd %d, data %p, callback %p, time %ld\n", fd, data, callback, time);
}

void sock_event_run(int fd, void *data)
{
	struct fd_t *f = &fd_table[fd];
	int fin = 0;
	struct event *e = f->wqueue;

	debug("sock_event_run fd %d\n", fd);
	if(e == NULL) {
		sock_set_event(fd, SOCK_EVENT_NO);
		return;
	}
	fin = event_exec_one(e);

	if( -1 == fin) {
		if(f->ops.write_handler == NULL) {
			int type = 1;
			sock_register_event(fd, NULL, NULL, sock_event_run, &type);
		}
	}
}

void sock_event_append(enum event_type type, const int fd, PF *callback, void *data)
{
	struct fd_t *f = &fd_table[fd];
	if(type == EVENT_WRITE) {
		if(!f->wqueue) 
			f->wqueue = event_create(fd, callback, data);
		else 
			event_add(f->wqueue, fd, callback, data);
	}	
}

void sock_file_send(const int in, const int out, const int offset, const int size, WPF *callback, void *data)
{
	assert(data);
	struct http_state *http = data;
	int size_need;
	struct fstate_write *fstate = calloc(1, sizeof(*fstate));
	if(fstate == NULL) {
		error("sock_file_send(%s)\n", strerror(errno));
		abort();
	}
	/* machine state */
	fstate->in_fd = in;
	fstate->out_fd = out;
	fstate->offset = offset;
	fstate->size = size;
	fstate->callback = callback;
	fstate->data = data;
	
	sock_handle_sendfile(fstate->out_fd, fstate);
}

void sock_write(int fd, char *buf, int size, WPF *callback, void *data, int use_event)
{
	int size_write;
	struct mstate_write *wstate = calloc(1, sizeof(*wstate));
	if(wstate == NULL) {
		error("sock_write %s\n", strerror(errno));
		abort();
	}

	wstate->callback = callback;
	wstate->data = data;
	wstate->fd = fd;
	wstate->buf = calloc(1, size); /* remind to free */
	memcpy(wstate->buf, buf, size);
	wstate->offset = 0;
	wstate->size = size;
	
	if(use_event) {
		sock_event_append(EVENT_WRITE, fd, sock_handle_write, wstate);
		sock_set_event(fd, SOCK_EVENT_WR);
		if(fd_table[fd].ops.write_handler != NULL) {
			debug("warning! fd %d write handler is not null\n", fd);
			return;
		}
		sock_register_event(fd, NULL, NULL, sock_event_run, NULL);
		return;
	}

	sock_handle_write(fd, wstate);
}

int sock_set_noblocking(int fd) 
{
	int flag;
	int dummy = 0;
	
	if(-1 == (flag = fcntl(fd, F_GETFL, dummy))) {
		log_debug(__FILE__, __LINE__, "%s. fd %d\n", strerror(errno), fd);
		return -1;
	}

	if(-1 == (flag = fcntl(fd, F_SETFL, dummy | O_NONBLOCK | O_NDELAY))) {
		log_debug(__FILE__, __LINE__, "%s. fd %d\n", strerror(errno), fd);
		return -1;
	}

	return 0;
}

void sock_init() 
{
	assert(config);
	
	if(fd_table)
		return;
	if(MAX_FD < 20240) 
		warning("sock_init MAX_FD %d, too small\n", MAX_FD);
	fd_table = calloc(MAX_FD, sizeof(*fd_table));
	int listen_fd = -1;
	
	assert(fd_table);

	sock_epoll_init();
	
	listen_fd = sock_listen_open();

	sock_set_event(listen_fd, SOCK_EVENT_RD);
	
	sock_register_event(listen_fd, http_request_accept, NULL, NULL, NULL);
	
	log_debug(__FILE__, __LINE__, "Listen port %d, fd %d\n", ntohs(config->server.port), listen_fd);
}

void sock_close(int fd) 
{
	assert(fd_table);
	
	if(!fd_table[fd].flags.open)
		return;

	debug("sock_close fd %d\n", fd);	
	sock_set_event(fd, SOCK_EVENT_NO); /* do not need to listen again */
	fd_close(fd);
	close(fd);
}

int sock_open(void) 
{
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if(sd == -1) {
		log_debug(__FILE__, __LINE__, "sock_open(%s)\n", strerror(errno));
		return -1;
	}
	
	fd_open(sd, FD_SOCKET);
	return sd;
}

void sock_register_event(int fd, PF *read_handler, void *read_data, PF *write_handler, void *write_data)
{
	assert(fd_table);

	log_debug(__FILE__, __LINE__, "sock_register_event (fd %d, read handler %p, "
			"read data %p, write handler %p, write data %p)\n", fd, read_handler, read_data,
					write_handler, write_data);
	
	fd_table[fd].ops.read_handler = read_handler;
	fd_table[fd].ops.read_data = read_data;
	
	fd_table[fd].ops.write_handler = write_handler;
	fd_table[fd].ops.write_data = write_data;
}

		
void fd_open(int fd, int type)
{
	assert(fd_table);

	struct fd_t *f = &fd_table[fd];
	f->fd = fd;
	f->flags.open = 1;
	f->type = type;
	f->wqueue = NULL;
	if(fd > biggest_fd)
		biggest_fd = fd;
	
	sock_set_noblocking(fd);
	sock_set_cork(fd);
}

static void fd_close(int fd)
{
	assert(fd_table);

	struct fd_t *f = &fd_table[fd];
	f->fd = -1;
	f->flags.open = 0;
	f->ops.timeout_data = f->ops.timeout_handler = NULL;
	f->ops.read_handler = f->ops.write_handler = NULL;
	f->ops.read_data = f->ops.write_data = NULL;

	if(fd >= biggest_fd) {
		biggest_fd --;
		assert(biggest_fd > 0);
	}
}


static void sock_set_reuseaddr(int fd)
{
		int on = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) < 0)
				log_debug(__FILE__, __LINE__, "%s. FD %d\n", strerror(errno), fd);
}

static int sock_listen_open(void)
{
	struct sockaddr_in srv;
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if(sd == -1) {
		error("sock_listen_open %s\n", strerror(errno));
		abort();
	}
	
	srv.sin_family = AF_INET;
	srv.sin_port = config->server.port;
	srv.sin_addr = config->server.inet4_addr;

	sock_set_reuseaddr(sd);
	
	if(-1 == bind(sd, (struct sockaddr *) &srv, sizeof(srv))) {
		error("sock_listen_open %s\n", strerror(errno));
		abort();
	}

	listen(sd, 1<<10);

	fd_open(sd, FD_SOCKET);

	return sd;
}

static void sock_handle_write(int fd, void *data)
{
	if(!data) 
		return;
	
	int error = 0;
	struct mstate_write *wstate = data;
	assert(fd == wstate->fd);
	
	int size_need = wstate->size - wstate->offset;
	int size_write = write(fd, wstate->buf + wstate->offset, size_need);
	debug("sock_handle_write fd %d, offset %d, size %d\n", fd, wstate->offset, wstate->size);
	if(size_write == -1) {
		warning("sock_handle_write(%s)\n", strerror(errno));
		if(ignore_error(errno)) {
			sock_set_event(fd, SOCK_EVENT_WR);
			sock_register_event(fd, NULL, NULL, sock_handle_write, wstate);
		}
		else {
			error = 1;
			WPF *handle = wstate->callback;
			void *data = wstate->data;
			int res = wstate->offset;
			
			safe_free(wstate->buf);
			safe_free(wstate);
		
			handle(error, data, res);
		}
		return;
	}
	
	wstate->offset += size_write;

	if(wstate->offset < wstate->size)  {
		sock_set_event(fd, SOCK_EVENT_WR);
		sock_register_event(fd, NULL, NULL, sock_handle_write, wstate);
	}
	else {
		WPF *handle = wstate->callback;
		void *data = wstate->data;
		int res = wstate->offset;
		safe_free(wstate->buf);
		safe_free(wstate);
		
		handle(0, data, res);
	}
}

static void sock_handle_sendfile(int fd, void *data)
{
	int error = 0;
	struct fstate_write *fs = data;

	assert(fs->out_fd == fd);
	
	int size_need = fs->size - fs->size_sent;
	int size_write = sendfile(fs->out_fd, fs->in_fd, (off_t *)&fs->offset, size_need);
	int res = 0;

	if(size_write == -1) {
		warning("sock_handle_sendfile fd %d %s\n", fd, strerror(errno));

		if(ignore_error(errno))  {
			sock_set_event(fd, SOCK_EVENT_WR);
			sock_register_event(fd, NULL, NULL, sock_handle_sendfile, fs);
		}
		else {
			s_error("sock_handle_sendfile fd %d %s\n", fd, strerror(errno));
			error = 1;
			WPF *handle = fs->callback;
			void *data = fs->data;
			res = fs->size_sent;
			safe_free(fs);

			handle(error, data, res);
		}
		return;
	}

	fs->size_sent += size_write;

	if(fs->size_sent < fs->size) {
		sock_set_event(fd, SOCK_EVENT_WR);
		sock_register_event(fd, NULL, NULL, sock_handle_sendfile, fs);
	}
	else {
		WPF *handle = fs->callback;
		res = fs->size_sent;
		void *data = fs->data;
		safe_free(fs);

		handle(error, data, res);
	}
}

static void sock_set_cork(int fd)
{
	int on = 1;
	
	if(-1 == setsockopt(fd, IPPROTO_TCP, TCP_CORK, (char *)&on, sizeof(on))) {
		log_debug(__FILE__, __LINE__, "warning! sock_set_cork(%s)\n", strerror(errno));
	}
}
