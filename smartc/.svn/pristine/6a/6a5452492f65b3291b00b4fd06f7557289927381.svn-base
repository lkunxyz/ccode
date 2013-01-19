
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
#ifndef CACHE_INDEX_H
#define CACHE_INDEX_H
#include "cache_file.h"

enum cache_op {
	CACHE_INDEX_ADD,
	CACHE_INDEX_DEL,
	CACHE_INDEX_EDIT
};

struct cache_index {
	cache_key key[32];
	int dirno;
	int header_size;
	int body_size;
	struct cache_time time;
	int operate; /* 0: ADD 1: EDIT 2: DELETE */
};

extern void cache_index_log(struct cache_index *ci, int index_fd);
extern void cache_index_init(void);
extern struct cache_index *index_build_from_cache(struct cache_file *cache);
#endif
