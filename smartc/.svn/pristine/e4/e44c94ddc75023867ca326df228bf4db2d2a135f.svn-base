
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

#include "sock_epoll.h"
#include "sock_io.h"
#include "log.h"
#include "server.h"

struct fd_t *fd_table;
#define MAX_EVENTS 1024
/* epoll structs */
static int kdpfd;
static struct epoll_event events[MAX_EVENTS];
static unsigned *epoll_state;   /* keep track of the epoll state */


void sock_epoll_init(void)
{
	kdpfd = epoll_create(MAX_FD);
	if(kdpfd == -1) {
		error("sock_epoll init %s\n", strerror(errno));
		abort();
	}
	epoll_state = calloc(MAX_FD, sizeof(*epoll_state));
}

void sock_set_event(int fd, int mark) 
{
	int epoll_ctl_type = 0;
	struct epoll_event ev;
	
	assert(fd >= 0);

	ev.data.fd = fd;
	ev.events = 0;

	if (mark & SOCK_EVENT_RD)
		ev.events |= EPOLLIN;

	if (mark & SOCK_EVENT_WR)
		ev.events |= EPOLLOUT;

	if (ev.events)
		ev.events |= EPOLLHUP | EPOLLERR;

	if (ev.events != epoll_state[fd]) {
		/* If the struct is already in epoll MOD or DEL, else ADD */
		if (!ev.events) {
			epoll_ctl_type = EPOLL_CTL_DEL;
		} 
		else if (epoll_state[fd]) {
			epoll_ctl_type = EPOLL_CTL_MOD;
		}
	   	else {
			epoll_ctl_type = EPOLL_CTL_ADD;
		}

		/* Update the state */
		epoll_state[fd] = ev.events;
		
		if (epoll_ctl(kdpfd, epoll_ctl_type, fd, &ev) < 0) {
				log_debug(__FILE__, __LINE__, "sock_set_event: failed on fd=%d: %s\n", \
									fd, strerror(errno));
		}
	}
	
	log_debug(__FILE__, __LINE__, "sock_set_event(fd %d, need read %d, need write %d)\n", \
					fd, mark & SOCK_EVENT_RD, mark & SOCK_EVENT_WR);
}

void do_sock_epoll(int msec)
{
	int i;
	int num;
	int fd;
	int cur_biggest = biggest_fd;
	struct epoll_event *cevents;


	current_time_get();
	epoll_time = current_time.tv_sec;
	/* check timeout */
	for(i = 0; i <= cur_biggest; i++) {
		struct fd_t *f = &fd_table[i];
		if((f->type & FD_SOCKET) && f->flags.open) {
			if(current_time.tv_sec > f->timeout) {
				PF *handler = f->ops.timeout_handler;
				void *data = f->ops.timeout_data;

				if(handler)
					handler(f->fd, data);
				
				f->ops.timeout_handler = f->ops.timeout_data = NULL;
			}
		}
	}

	assert(msec >= 0);
	num = epoll_wait(kdpfd, events, MAX_EVENTS, msec);
	if (num < 0) {
		warning("do_sock_epoll %s\n", strerror(errno));
		return;
	}
	else if (num == 0) {
		return;
	}

	for (i = 0, cevents = events; i < num; i++, cevents++) {
		fd = cevents->data.fd;
		sock_call_handler(fd, cevents->events & ~EPOLLOUT, cevents->events & ~EPOLLIN);
	}
}

void sock_call_handler(int fd, int need_read, int need_write) 
{
	assert(fd_table);
	assert(fd >= 0);
	
	debug("sock_call_handler fd %d read %d write %d\n", fd, need_read, need_write);
	struct fd_t *f = &fd_table[fd];
	if(need_read && f->ops.read_handler) {
		void *rdata = f->ops.read_data;
		PF *handle = f->ops.read_handler;

		f->ops.read_handler = NULL;
		f->ops.read_data = NULL;

		handle(fd, rdata);
	}
	else if(need_write && f->ops.write_handler) {
		void *wdata = f->ops.write_data;
		PF *handle = f->ops.write_handler;

		f->ops.write_handler = NULL;
		f->ops.write_data = NULL;
		
		handle(fd, wdata);
	}
}
