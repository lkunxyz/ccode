
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
#ifndef CACHE_FILE_H
#define CACHE_FILE_H
#include "http_header.h"
#include "string.h"
#include "list.h"

typedef char cache_key;

enum cache_status {
	CACHE_NONE = -1,  /* can not store in the cache */
	CACHE_PENDING, /* downloading now */
	CACHE_SHM, 
	CACHE_DISK,
	CACHE_MEM,
	CACHE_FRESH,
	CACHE_STALE
};

struct cache_control {
	int max_age;
	int s_maxage;
	int public; /* The document is cacheable by any cache. */
	int private; /* The document is not cacheable by a shared cache. */
	int no_store;
	int no_cache;  /* can cache, but must revalidate */
	int only_if_cached; /* The client wants a copy only if it is in the cache */
	int min_stale;
	int max_stale; 
	int min_fresh;	
	int no_transform; /* Do not convert the entity-body. Useful for applications that require that the message received is exactly what was sent by the server. */
	int proxy_revalidate; /* Client must revalidate data except for private client caches. Public caches must verify the status of stale documents. Like must-revalidate, excluding private caches. */
	int must_revalidate; /* The cache must verify the status of stale documents, i.e., the cache cannot blindly use a document that has expired */
};

struct cache_time {
	time_t freshness_lifetime; 
	time_t age; /* two time above are very important */
	time_t response_got;
	time_t request_issued;
	time_t expires;
	time_t date;
	time_t last_modified;
	time_t max_age;
	time_t header_age;
};

struct cache_file {
	int body_size;
	int header_size;
	int dir_no;
	int ref_count;
	int statu;
	struct cache_time time;
	struct string *key;	
	struct string *file;
	struct disk_file *disk;
	struct mem *mem;
	struct mem_cache *mem_cache; /* cache in user process memory */
	struct shm_cache *shm_cache; /* cache in /dev/shm */
	void *other; /* use for transfer */

	struct {
	    struct http_version version;
		struct header_t *headers;
		enum http_status statu;
	}reply;
	
	struct list_head list; /* a list of cache objects which can be freed, last time less than now 60*/
};

/* structure of files in /dev/shm */
struct shm_file {
	int low;  /* declare position of file */
	int high; /* high - low = offset */
	int disk_fd;
	int body_size;
	struct mmap *map;
};

struct mmap {
	int cached;
	int shm_fd; /* in /dev/shm */
	int size;
	int offset;
	void *start;
};

struct disk_file {
	int fd;
	int offset;
	int size;
	int flags_open;  
};

struct mem {
	int low;
	int high;
	int capacity;
	int submit;
	struct string *context;
};

struct mem_cache {
	int ref;
	int low;
	int high;
	struct string *mbuf;
};

struct shm_cache {
	int ref;
	int low;
	int high;
	int fd; 
	struct list_head list;
};

extern void cache_file_table_init(void);
extern void cache_file_table_clean(void);
extern int cache_file_insert(struct cache_file *cache);
extern struct cache_file *cache_file_search(const char *md5);
extern struct cache_file *cache_ref(struct cache_file *cache);
extern void cache_unref(struct cache_file *cache);
extern struct cache_file *cache_file_create(const char *url);
extern struct cache_file *cache_file_delete(struct cache_file *cache);
extern void cache_file_destroy(struct cache_file *cache);
extern int mem_swapout(struct cache_file *cache, void *data);
extern int mem_string_append(struct cache_file *cache, const struct string *s);
extern void mem_clean(struct mem *m);
extern struct mmap *mmap_create(int size, int offset);
extern void mmap_clean(struct mmap *map);
extern struct shm_file *shm_file_create(char *file, int body_size) ;
extern void cache_file_del(struct cache_file *cache);
extern void shm_file_clean(struct shm_file *shm);
extern struct shm_cache * shm_cache_append(struct shm_cache *list, struct shm_file *shm);
extern void mmap_reset(struct mmap *map, int size);
extern int mem_cache_append(struct mem_cache *head, char *buf, int size) ;
extern struct shm_cache *shm_cache_create(int fd, int low, int high);
extern struct mem_cache *mem_cache_create(int low, int high);
extern void mem_cache_clean(struct mem_cache *mc);
extern void shm_cache_clean(struct shm_cache *sc);
extern void cache_file_delete_and_free(struct cache_file *cache);	
extern void cache_file_free(struct cache_file *cache);
extern char *cache_statu(int statu);
#endif
