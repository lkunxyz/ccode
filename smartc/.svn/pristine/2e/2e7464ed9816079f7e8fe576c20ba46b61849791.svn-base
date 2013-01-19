
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
#ifndef FD_EVENT_H
#define FD_EVENT_H
#include "server.h"
#include "list.h"

struct event {
	struct list_head list;
	int fd;
	struct {
	int pending;
	int done;
	}flags;
	PF *handler;
	void *data;
	void (*free)(struct event **);
};

extern int event_count(struct event *h) ;
extern struct event *event_create(const int fd, PF *handler, void *data);
extern int event_add(struct event *ev, const int fd, PF *handler , void *data);
extern int event_exec_all(struct event *head);
extern int event_exec_one(struct event *head);

#endif
