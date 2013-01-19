
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
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "server.h"
#include "cache_index.h"
#include "log.h"
#include "cache_dir.h"
#include "cache_file.h"
#include "http.h"


static long long cache_num;

static int cache_header_preload(struct cache_file *cache);
static void cache_index_read(int fd);

void cache_index_log(struct cache_index *ci, int index_fd)
{
	assert(ci);
	if(index_fd == -1) {
		warning("cache_index_log index fd %d\n",  index_fd);
		return;
	}

	int size = sizeof(struct cache_index);
	int res = -1;

	res = write(index_fd, ci, size);
	if(-1 == res) {
		error("cache_index_log %s\n", strerror(errno));
		return ;
	}
	
	debug("cache_index_log success! Add %s to cache index\n", ci->key);
}

void cache_index_init(void)
{
	assert(config->cache_dir);
	
	if(config->cache_dir->count == 0) {
		s_error("cache_index_init dir count %d\n", config->cache_dir->count);
		abort();
	}

	int i;
	char path[MAX_FILE];
	for(i = 0; i < config->cache_dir->count; i++) {
		debug("cache_index_init begin scaning dir %d\n", i);
		struct cache_dir *dentry = config->cache_dir->items[i];
		cache_index_read(dentry->index_fd);
		
		snprintf(path, MAX_FILE - 1, "%s/cache.index", dentry->path);
		unlink(path);
	}

	debug("cache_index_init Finish scan cache %d.\n", cache_num);
}

static void cache_index_read(int fd) 
{
	assert(fd != -1);
	
	int size = sizeof(struct cache_index);
	int res; 
	struct cache_index ci;
	char buf[MAX_FILE];
	char *temp;
	struct cache_dir *dentry;

	for(; ;) {
		struct stat sbuf;
		char content[size * 1024];
		int used = 0;

		res = read(fd, content, size * 1024);
		if(-1 == res) {
			error("cache_index_read %s\n", strerror(errno));
			break;
		}
		else if(0 == res) 
			break;
		while(used < res) {
			if(memcpy(&ci, content + used, size)) 
				used += size;
			else {
				fprintf(stderr, "cache_index_read memcpy failed\n");
				abort();
			}
			if(ci.operate == CACHE_INDEX_DEL)
				continue;

			struct cache_file *cache = calloc(1, sizeof(*cache));
			assert(cache);
			cache->header_size = ci.header_size;
			cache->body_size = ci.body_size;
			cache->key = string_init(ci.key);
			cache->dir_no = ci.dirno;
			cache->time = ci.time;
			cache->statu = CACHE_DISK;
			/* get file path */
			dentry = config->cache_dir->items[cache->dir_no];
			snprintf(buf, MAX_FILE - 1, "%s/%s", dentry->path, cache->key->buf);
			cache->file = string_init(buf);
		
			if(-1 == stat(cache->file->buf, &sbuf)) {
				debug("cache_index_read %s not exist\n", cache->file->buf);
				safe_free(cache);
				continue;
			}
			cache_index_log(&ci, dentry->new_fd);
			/*preload header */
			if(-1 == cache_header_preload(cache)) {
				safe_free(cache);
				continue;
			}
			INIT_LIST_HEAD(&cache->list);
			cache->shm_cache = shm_cache_create(-1, 0, 0);
			cache->mem_cache = mem_cache_create(0, 0);

			if(!cache_file_insert(cache))
				cache_num ++;
		}
	}
	close(fd);
}

struct cache_index *index_build_from_cache(struct cache_file *cache)
{
	if(!cache) 
		return NULL;
	
	static struct cache_index ci;

	strncpy(ci.key, cache->key->buf, 32);
	ci.dirno = cache->dir_no;
	ci.header_size = cache->header_size;
	ci.body_size = cache->body_size;
	ci.time = cache->time;

	return &ci;
}

static int cache_header_preload(struct cache_file *cache)
{
	if(!cache) 
		return;

	int size_need = cache->header_size;
	struct reply_t rep;
	char header_buf[MAX_HEADER];
	int res;
	int fd;

	fd = open(cache->file->buf, O_RDONLY);
	if(-1 == fd) {
		debug("cache_header_preload {%s %s}\n", cache->file->buf, strerror(errno));
		return -1;
	}

	res = read(fd, header_buf, size_need);
	if(-1 == res) {
		debug("cache_header_preload {%s %s}\n", cache->file->buf, strerror(errno));
		close(fd);
		return -1;
	}
	else if(res != size_need) {
		log_debug(__FILE__, __LINE__, "cache_header_preload {%s size not gain fully}\n", cache->file->buf);
		close(fd);
		return -1;
	}

	if(0 ==  reply_parse(&rep, header_buf, res)) {
		cache->reply.headers = header_ref(rep.headers);
		cache->reply.statu = rep.statu;
		cache->reply.version = rep.version;
	}
	else {
		close(fd);
		warning("cache_header_preload failed! load %s.\n", cache->file->buf);
		return -1;
	}
	
	close(fd);
	debug("cache_header_preload success! load %s.\n", cache->file->buf);
	return 0;
}
