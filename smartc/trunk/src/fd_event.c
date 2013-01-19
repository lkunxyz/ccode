
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
#include <assert.h>

#include "fd_event.h"
#include "log.h"

static void event_del(struct event **h);

int event_count(struct event *h) 
{
	if(!h)
		return 0;

	struct list_head *l;
	int count = 0;
	list_for_each(l, &h->list) 
		count++;

	return count;
}

int event_add(struct event *head, const int fd, PF *handler , void *data)
{
	if(!head)
		return -1;

	assert(handler);

	struct event *ev = calloc(1, sizeof(*ev));
	if(!ev)
		return -1;

	ev->fd = fd;
	ev->handler = handler;
	ev->data = (void *)data;
	ev->free = event_del;
	INIT_LIST_HEAD(&ev->list);
	
	list_add_tail(&ev->list, &head->list);
	debug("event_add %p, count %d\n", ev, event_count(head));
	return 0;
}

int event_exec_all(struct event *h)
{
	if(!h)
		return 0;
	
	int count = 0;
	struct list_head *p, *n;
	struct event *e;
	list_for_each_safe(p, n, &h->list) {
		e = list_entry(p, struct event, list);
		assert(e);
		list_del_init(p);

		PF *handler = e->handler;
		void *data = e->data;
		handler(e->fd, data);
		count++;
	}

	if(list_empty(&h->list))
		return count;	

	return -1;
}

int event_exec_one(struct event *h)
{
	if(!h) 
		return -1;

	struct event *e = h;
	assert(e);
	debug("event_exec_one %p, pending %d, done %d\n", h, e->flags.pending, e->flags.done);
	if(e->flags.pending == 1) {
		debug("sorry! %p is executing now!\n", e);
		return -1;
	}

	e->flags.pending = 1;
	PF *handler = e->handler;
	handler(e->fd, e->data);
	return 0;	
}

struct event *event_create(const int fd, PF *handler, void *data)
{
	struct event *h = calloc(1, sizeof(*h));
	if(!h)
		return NULL;

	h->fd = fd;
	h->handler = handler;
	h->data = (void *)data;
	h->free = event_del;
	INIT_LIST_HEAD(&h->list);

	debug("event_create fd %d, handler %p, data %p\n", fd, handler, data);
	return h;
}

static void event_del(struct event **h)
{
	if(!*h)
		return;	
	
	struct event *e = *h;
	struct event *next;
	struct list_head *p = e->list.next;
	next = list_entry(p, struct event, list);
	list_del(&e->list);
	if(next == *h)
		*h = NULL;
	else 
		*h = next;
	safe_free(e);	

	debug("event_del new head %p, count %d\n", *h, event_count(*h));
}
