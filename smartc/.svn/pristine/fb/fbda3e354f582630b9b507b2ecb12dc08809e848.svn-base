
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
#ifndef SOCK_IO_H
#define SOCK_IO_H
#include "server.h"
#include "fd_event.h"
#include <sys/select.h>
#define MAX_FD FD_SETSIZE

enum event_type {
	EVENT_NONE = -1,
	EVENT_READ = 0,
	EVENT_WRITE = 1
};

struct fd_t 
{
	int fd;
	int type;  /* 0: network fd; 1: file fd; */
	time_t timeout;
	struct {
		int open;
	}flags;
	struct {
		void *read_data;
		void *write_data;
		void *timeout_data;
		void (*timeout_handler)(int fd, void *data);
		void (*read_handler)(int fd, void *data);
		void (*write_handler)(int fd, void *data);
	}ops;
	struct event *wqueue;
};

struct mstate_write {
	int fd;
	char *buf;
	int offset;
	int size;
	WPF *callback;
	void *data;
};

struct fstate_write {
	int in_fd;
	int out_fd;
	int offset;
	int size;
	int size_sent;
	WPF *callback;
	void *data;
};


extern int sock_set_noblocking(int fd);
extern void sock_set_event(int fd, int mark);
extern void fd_open(int fd, int type);
extern void sock_close(int fd);
extern void sock_init(void);
extern void sock_register_event(int fd, PF *read_handler, void *read_data, PF *write_handler, void *write_data);
extern int sock_open(void);
extern void sock_write(const int fd, char *buf, int size, WPF *callback, void *data, int use_event);
extern void sock_file_send(const int in, const int out, const int offset, const int size, WPF *callback, void *data);
extern void sock_event_append(enum event_type type, const int fd, PF *callback, void *data);
extern void sock_event_run(int fd, void *data);
extern void sock_timeout_set(int fd, void *data, PF *callback, time_t time);
#endif
