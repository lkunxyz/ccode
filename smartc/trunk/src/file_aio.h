
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
#ifndef FILE_AIO_H
#define FILE_AIO_H
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "config.h"

#if USE_POSIX_AIO
#include <aio.h>
#include "list.h"
#endif

#include "server.h"
#include "string.h"

#if USE_POSIX_AIO
struct job {
	WPF *handler;
	void *data;
	struct aiocb *iocb;
	struct list_head list;
};
#endif

extern void file_aio_loop(void);
extern int file_aio_init(int max);
extern int file_aio_write(int fd, struct string *s, long long offset, WPF *handler, void *data);
extern int file_aio_read(int fd, void *start, int size, long long offset, WPF *handler, void *data);
#endif
