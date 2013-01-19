
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <libaio.h>

#include "file_aio.h"
#include "server.h"
#include "log.h"
#include "http.h"

static io_context_t *aio_queue;

int file_aio_init(int max) 
{
	int events = 16;
	if(max > 0) 
		events = max;
	
	aio_queue = calloc(events, sizeof(*aio_queue));
	if(!aio_queue) {
		error("file_aio_init %s\n", strerror(errno));
		return -1;
	}
	
	if(0 != io_setup(events, aio_queue)) {
		error("file_aio_init %s\n", strerror(errno));
		return -1;
	}
	
	debug("file_aio_init success! %d events\n", events);
	return 0;
}

int file_aio_write(int fd, struct string *s, long long offset, WPF *handler, void *data)
{
	struct iocb *cb = calloc(1, sizeof(*cb)); /* remaind to free */
	if(!cb) {
		error("file_aio_write %s\n", strerror(errno));
		return -1;
	}
	struct handler_obj *handle = calloc(1, sizeof(*handle)); /* remaind to free */
	assert(handle);
	handle->handle = handler;
	handle->data = data;
	io_prep_pwrite(cb, fd, s->buf, s->offset, offset);
	cb->data = handle;
	
	return io_submit(*aio_queue, 1, &cb);
}

int file_aio_read(int fd, void *start, int size, long long offset, WPF *handler, void *data)
{
	struct iocb *cb = calloc(1, sizeof(*cb)); /* remaind to free */
	if(!cb) {
		error("file_aio_write %s\n", strerror(errno));
		return -1;
	}
	struct handler_obj *handle = calloc(1, sizeof(*handle)); /* remaind to free */
	assert(handle);
	handle->handle = handler;
	handle->data = data;

	io_prep_pread(cb, fd, start, size, offset);
	cb->data = handle;

	return io_submit(*aio_queue, 1, &cb);
}

void file_aio_loop(void) 
{
	struct io_event event[16];
	struct timespec io_ts;
	int res;
	
	io_ts.tv_sec = 0;
	io_ts.tv_nsec = 0;

	res = io_getevents(*aio_queue, 1, 16, event, &io_ts);
	if(res > 0) {
		int i;
		for(i = 0; i < res; i++) {
			struct handler_obj *obj = event[i].data;
			struct iocb *cb = event[i].obj;

			WPF *handle = obj->handle;
			void *data = obj->data;

			if((long)event[i].res < 0) {
				handle(1, data, 0);
			}
			else {
				handle(0, data, event[i].res);
			}
			/* begin to free some data */
			safe_free(obj);
			event[i].data = NULL;
			safe_free(cb);
			event[i].obj = NULL;
		}	
	}
	else if(res < 0) {
		error("file_aio_loop %s\n", strerror(errno));
	}
}
