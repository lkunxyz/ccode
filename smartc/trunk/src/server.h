
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
#ifndef SERVER_H
#define SERVER_H
#include "config.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <glib.h>

#include "array.h"
#include "conf.h"

#define G_THREADS_ENABLED
#define safe_free(x) if(x){free(x);x=NULL;}

#define SOCK_EVENT_WR 0x01
#define SOCK_EVENT_RD 0x10
#define SOCK_EVENT_NO 0x00

#define FD_SOCKET 0x01
#define FD_FILE 0x10
#define FD_NONE 0x00

#define SPACE_CHAR " \t\r\n"
#define YANOW_HEADER "X-Cache: Smart cache system\r\n"
#define MAX_MEM 8192
#define CHUNK_SIZE (1024 * 512)
#define CACHE_HASH_LENGTH 12582910
#define MAX_URL 8192
#define MAX_HEADER 8192
#define MAX_LINE 2048
#define MAX_FILE 4096
#define BUF_SIZE 8192
#define MAX_FILE_NAME 512
#define LIFE_LIMIT 60
#define MAX_SIZE_IN_SHM (128 * 1024 * 1024)
#define CHUNK_NUM_IN_SHM 3
#define MAX_SIZE_IN_MEM (1024*1024*128)
#define PER_OBJECT_SIZE_IN_MEM (1024*512)
#define MAX_CACHE_DIR 10

typedef void PF(int, void *);
typedef void WPF(int, void *, int);
typedef void SGHANDLE(int);

struct handler_obj {
	void *data;
	WPF *handle;
};

struct server {
	struct {
		long long offset;
		long long size;
	}shm;
	struct {
		long long offset;
		long long size;
	}mem;
	struct array_t *dirs;
	time_t up;
};

/* global vars */
extern struct config_t *config;
extern struct timeval current_time;
extern unsigned int dir_count;
extern struct fd_t *fd_table;
extern struct server srv;
extern GAsyncQueue *io_queue;
extern int cache_dir_count;
extern char conf_path[MAX_LINE];
extern int biggest_fd;
extern time_t epoll_time;
/* funcs */
extern char *smart_time(void);
extern void current_time_get(void);
#endif
