
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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "server.h"
#include "cache_dir.h"
#include "log.h"

void cache_dir_init(void)
{
	if(!config->cache_dir) 
		return;

	int fd;
	int flags = O_RDONLY;
	char path[MAX_FILE];
	struct stat sbuf;
	char *p;
	struct cache_dir *dentry;
	int i;
	
	for(i = 0; i < config->cache_dir->count; i++) {
		dentry = config->cache_dir->items[i];
		p = dentry->path;
		
		snprintf(path, MAX_FILE - 1, "%s/cache.index", p);
		if(-1 == stat(path, &sbuf)) {
			snprintf(path, MAX_FILE - 1, "%s/cache.index.new", p);
			if( -1 == stat(path, &sbuf)) {
				flags = O_RDONLY | O_CREAT;
				snprintf(path, MAX_FILE - 1, "%s/cache.index", p);
			}
		}

		fd = open(path, flags, 0666);
		if(fd == -1) {
			s_error("cache_dir_init %s\n", strerror(errno));
			abort();
		}
		dentry->index_fd = fd;

		/* open cache.index.new for writing later */
		snprintf(path, MAX_FILE - 1, "%s/cache.index.new", p);
		fd = open(path, flags | O_WRONLY | O_CREAT, 0666);
		if(-1 == fd) {
			s_error("cache_dir_init {%s %s}\n", path, strerror(errno));
			abort();
		}
		dentry->new_fd = fd;
	}
}

int cache_dir_select(void) 
{
	long long tail_sz = 0;
	long long max = 0;
	int max_dir = 0;
	int i = 0;
	struct cache_dir *dentry ;

	for(i = 0; i < config->cache_dir->count; i++) {
		dentry = config->cache_dir->items[i];
		tail_sz = dentry->max_size - dentry->offset;
		if(tail_sz > max) {
			max = tail_sz;
			max_dir = i;
		}
	}
	debug("cache_dir_select dir %d\n", max_dir);
	return max_dir;
}

int cache_dir_close(void) 
{
	struct cache_dir *dentry;
	char path[MAX_FILE];
	char new_path[MAX_FILE];
	int i;

	for(i = 0; i < config->cache_dir->count; i++) {
		dentry = config->cache_dir->items[i];
		
		if(dentry->new_fd != -1) {
			close(dentry->index_fd);
			snprintf(path, MAX_FILE - 1, "%s/cache.index", dentry->path);
			snprintf(new_path, MAX_FILE - 1, "%s/cache.index.new", dentry->path);
			rename(new_path, path);
		}
	}
}
