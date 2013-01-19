
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
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "server.h"
#include "hashtable.h"
#include "md5.h"
#include "cache_file.h"
#include "log.h"
#include "http.h"
#include "file_aio.h"
#include "sock_io.h"
#include "cache_dir.h"
#include "cache_index.h"


DEFINE_HASHTABLE_INSERT(insert_some, cache_key, struct cache_file);
DEFINE_HASHTABLE_SEARCH(search_some, cache_key, struct cache_file);
DEFINE_HASHTABLE_REMOVE(remove_some, cache_key, struct cache_file);
DEFINE_HASHTABLE_ITERATOR_SEARCH(search_itr_some, cache_key);

static struct hashtable *cache_table; 

static int cache_key_compare(void *k1, void *k2);
static unsigned int cache_key_hash(void *k);
static struct disk_file *disk_file_create(const char *file, const int size);
static struct block *block_create(const int size);
static void block_clean(struct block *b);
static void block_reset(struct block *b);
static struct mem *mem_create(const int size);
static void disk_file_clean(struct disk_file *disk);
static void mem_swapout_done(int error, void *data, int size);
static int list_count(struct list_head *h);


void cache_file_reset_from_reply(struct cache_file *cache, struct reply_t *reply)
{
	if(!cache) 
		return;
	if(!reply)
		return;

	cache->body_size = reply->body_size;
	cache->header_size = reply->header_size;
	cache->statu = CACHE_PENDING;
	memset(&cache->time, -1, sizeof(struct cache_time));
	disk_file_clean(cache->disk);
	mem_clean(cache->mem);
	cache->mem = mem_create(0);
	mem_cache_clean(cache->mem_cache);
	cache->mem_cache = mem_cache_create(0, 0);
	shm_cache_clean(cache->shm_cache);
	cache->shm_cache = shm_cache_create(-1, 0, 0);
	cache->reply.statu = reply->statu;
	cache->reply.version = reply->version;
	cache->reply.headers = header_ref(reply->headers);
	list_del_init(&cache->list);
	/* truncate cache file in disk */
	truncate(cache->file->buf, 0);
}

char *cache_statu(int statu)
{
	if(statu == CACHE_NONE)
		return "CACHE_NONE";
	else if(statu == CACHE_DISK)
		return "CACHE_DISK";
	else if(statu == CACHE_MEM)
		return "CACHE_MEM";
	else if(statu == CACHE_SHM)
		return "CACHE_SHM";
	else if(statu == CACHE_PENDING) 
		return "CACHE_PENDING";
	else 
		return "CACHE_ERROR";
}

void cache_file_free(struct cache_file *cache)
{
	if(!cache)
		return;

	unlink(cache->file->buf);
	string_clean(cache->key);
	string_clean(cache->file);
	disk_file_clean(cache->disk);
	mem_clean(cache->mem);
	mem_cache_clean(cache->mem_cache);
	shm_cache_clean(cache->shm_cache);
	header_free(cache->reply.headers);
	list_del(&cache->list);

	safe_free(cache);
}

void cache_file_delete_and_free(struct cache_file *cache)
{
	if(!cache) 
		return;

	cache_file_delete(cache);
	cache_file_free(cache);
}

void mem_cache_clean(struct mem_cache *mc)
{
	if(!mc)
		return;
	
	string_clean(mc->mbuf);
	safe_free(mc);
}

void shm_cache_clean(struct shm_cache *sc)
{
	if(!sc) 
		return ;
	
	list_del(&sc->list);
	if(sc->fd != -1 ) 
		close(sc->fd);

	safe_free(sc);
}

void shm_file_clean(struct shm_file *shm) 
{
	if(!shm) 
		return;
	
	if(shm->disk_fd != -1) 
		close(shm->disk_fd);

	mmap_clean(shm->map);
	
	safe_free(shm);
}

void shm_file_read(int fd, void *data) 
{	
	assert(data);
	assert(fd > 0);

	struct http_state *http = data;
	struct shm_file *shm = http->shm;
	int size_read;

	assert(fd == shm->disk_fd);
		
	size_read = read(fd, shm->map->start + shm->map->offset, shm->map->size - shm->map->offset);
	if(size_read == -1) {
		s_error("shm_file_read %s\n", strerror(errno));
		sock_close(http->client_fd);
		http_state_free(http);
		return;
	}
	
}

struct shm_file *shm_file_create(char *file, int body_size) 
{
	struct shm_file *shm;
	
	if(body_size == 0) {
		return NULL;
	}
	
	shm = calloc(1, sizeof(*shm));
	if(NULL == shm) {
		error("shm_file_create %s\n", strerror(errno));
		return NULL;
	}
	/* open disk file */
	shm->disk_fd = open(file, O_RDONLY | O_NOATIME);
	if(shm->disk_fd == -1) {
		debug("shm_file_create {%s %s}\n", file, strerror(errno));
		safe_free(shm);
		return NULL;
	}

	shm->body_size = body_size;
	shm->low = 0;
	shm->high = 0;
	shm->map = mmap_create(body_size, 0);
	if(shm->map == NULL) 
		debug("shm_file_create mmap failed body %d offset 0\n", body_size);
	
	return shm;
}

struct mmap *mmap_create(int size, int offset)
{
	int size_need;
	char temp_file[32];
	int fd;
	struct mmap *map = calloc(1, sizeof(*map));
	if(NULL == map) {
		error("mmap_create %s\n", strerror(errno));
		return NULL;
	}
		
	strcpy(temp_file, "/dev/shm/c-XXXXXX");
	if(-1 == (fd = mkstemp(temp_file))) {
		error("shm_file_create(%s)\n", strerror(errno));
		safe_free(map);
		return NULL;
	}
	if(-1 == unlink(temp_file)) 
		warning("shm_file_create %s\n", strerror(errno));
	
	size_need = (size > CHUNK_SIZE) ? CHUNK_SIZE : size;
	
	ftruncate(fd, size_need);	
	
	map->start = mmap(NULL, size_need, PROT_WRITE | PROT_READ, MAP_SHARED, fd, offset);
	if(MAP_FAILED == map->start) {
		error("mmap_create %s, fd %d, bytes %d, offset %lld\n", \
						strerror(errno), fd, size_need, offset);
		safe_free(map);
		close(fd);
		return NULL;
	}
	if( -1 == madvise(map->start, size_need, MADV_NORMAL)) {
		error("mmap_create %s\n", strerror(errno));
	}

	map->shm_fd = fd;
	map->size = size_need; 
	map->offset = 0;

	debug("success! mmap_create %d bytes, fd %d\n", size_need, fd);
	
	return map;
}

void mmap_reset(struct mmap *map, int size)
{
	if(!map)
		return;

	int size_need;
	char temp_file[32];
	int fd;
		
	/* reset value */
	if(map->start) {
		munmap(map->start, map->size);
		map->start = NULL;
	}
	if(map->shm_fd != -1 && map->cached == 0) {
		debug("mmap_reset truncate %d\n", map->shm_fd);
		ftruncate(map->shm_fd, 0);
		close(map->shm_fd);
		map->shm_fd = -1;
	}

	strcpy(temp_file, "/dev/shm/c-XXXXXX");
	if(-1 == (fd = mkstemp(temp_file))) {
		error("shm_file_create(%s)\n", strerror(errno));
		return ;
	}
	if(-1 == unlink(temp_file)) 
		warning("shm_file_create %s\n", strerror(errno));
	
	size_need = (size > CHUNK_SIZE) ? CHUNK_SIZE : size;
	
	ftruncate(fd, size_need);	
	
	map->start = mmap(NULL, size_need, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if(MAP_FAILED == map->start) {
		error("mmap_create %s, fd %d, bytes %d, offset 0\n", \
						strerror(errno), fd, size_need);
		close(fd);
		return;
	}
	if( -1 == madvise(map->start, size_need, MADV_NORMAL)) {
		error("mmap_create %s\n", strerror(errno));
	}

	map->shm_fd = fd;
	map->size = size_need; 
	map->offset = 0;
}

void mmap_clean(struct mmap *map)
{
	if(!map) 
		return;

	if(map->start)
		munmap(map->start, map->size);
	if(map->shm_fd != -1 && !map->cached) {
		ftruncate(map->shm_fd, 0);
		close(map->shm_fd);
	}

	safe_free(map);
}

void mem_clean(struct mem *m)
{
	if(!m)
		return;

	string_clean(m->context);
	safe_free(m);
}

int mem_string_append(struct cache_file *cache, const struct string *s)
{
	assert(cache);
	assert(s);

	struct mem *m = cache->mem;

	if(!m) {
		int size = (cache->header_size + cache->body_size) > CHUNK_SIZE ? CHUNK_SIZE : \
				   (cache->header_size + cache->body_size);
		
		if(cache->body_size == -1) 
			size = CHUNK_SIZE;

		cache->mem = mem_create(size);
		if(!cache->mem)
			return -1;
		m = cache->mem;
	}

	string_append(m->context, s->buf, s->offset);

	m->high += s->offset;	
	return s->offset;
}

int mem_swapout(struct cache_file *cache, void *data)
{
	if(!cache || !data) 
		return 0;
	if(cache->statu == CACHE_NONE)
		return 0;

	struct http_state *http = data;
	struct disk_file *disk = cache->disk;
	struct mem *m = cache->mem;
	struct string *s;

	if(!disk && m->low == 0) {
		int size = cache->header_size + cache->body_size;
		
		if(cache->body_size == -1) 
			size = CHUNK_SIZE;

		cache->disk = disk_file_create(cache->file->buf, size);
		disk = cache->disk;	
	}

	assert(m);
	s = m->context;
	assert(s);
	cache->other = http;

	file_aio_write(disk->fd, s, m->low, mem_swapout_done, cache);
}

int cache_file_insert(struct cache_file *cache)
{
	assert(cache);
	assert(cache->file);
	
	cache_key *key = (char *)strndup(cache->key->buf, cache->file->offset);
	if(key == NULL) {
		warning("cache_file_insert %s\n", strerror(errno));
		return -1;
	}
	
	insert_some(cache_table, key, cache);
	debug("cache_file_insert %s\n", cache->key->buf);
	
	return 0;
}

struct cache_file *cache_file_delete(struct cache_file *cache)
{
	if(!cache) 
		return NULL;

	struct cache_dir *d;
	struct cache_index *ci;
	cache_key *key = cache->key->buf;
	if(key == NULL) {
		warning("cache_file_delete %p, not have key\n", cache);
		return NULL;
	}
	
	remove_some(cache_table, key);

	ci = index_build_from_cache(cache);
	ci->operate = CACHE_INDEX_DEL;
	d = config->cache_dir->items[cache->dir_no];
	
	cache_index_log(ci, d->new_fd);
	return cache;
}

void cache_file_destroy(struct cache_file *cache)
{
	if(!cache) 
		return;

	string_clean(cache->key);
	string_clean(cache->file);
	header_free(cache->reply.headers);
	disk_file_clean(cache->disk);
	mem_clean(cache->mem);
	mem_cache_clean(cache->mem_cache);
	shm_cache_clean(cache->shm_cache);

	list_del(&cache->list);
}

struct cache_file *cache_file_search(const char *key)
{
	assert(key);
	assert(cache_table);
	
	return search_some(cache_table, (cache_key *)key);
}

void cache_file_table_init(void)
{
	if(cache_table == NULL) 
		cache_table = create_hashtable(CACHE_HASH_LENGTH, cache_key_hash, cache_key_compare);

	log_debug(__FILE__, __LINE__, "cache_file_table_init(size %d)\n", cache_table->tablelength);
}

void cache_file_table_clean(void)
{
	if(cache_table)
		return ;
	hashtable_destroy(cache_table, 0);
}

struct cache_file *cache_file_create(const char *url) 
{
	assert(url);
	
	char buf[MAX_FILE];
	struct cache_dir *dir;
	struct cache_file *cache = calloc(1, sizeof(*cache));
	if(NULL == cache) {
		error("cache_file_create(%s)\n", strerror(errno));
		return NULL;
	}

	/* init struct cache_file */
	cache->key = string_init(cache_key_public(url, strlen(url)));
	cache->dir_no = cache_dir_select();
	dir = config->cache_dir->items[cache->dir_no];
	
	/* init cache file full path */
	cache->file = string_init(dir->path);
	if(cache->file->buf[cache->file->offset - 1] != '/')
		string_append(cache->file, "/", 1);
	string_append_r(cache->file, cache->key);
	string_append(cache->file, "\0", 1);

	cache->disk = NULL;
	cache->statu = CACHE_NONE;
	cache->shm_cache = shm_cache_create(-1, 0, 0);
	cache->mem_cache = mem_cache_create(0, 0);
	INIT_LIST_HEAD(&cache->list);

	log_debug(__FILE__, __LINE__, "cache_file_create %s\n", cache->file->buf);
	return cache;
}

struct cache_file *cache_ref(struct cache_file *cache)
{
	assert(cache);
	
	cache->ref_count++;

	return cache;
}

void cache_unref(struct cache_file *cache)
{
	if(!cache)
		return ;	
	cache->ref_count--;

	if(cache->statu == CACHE_NONE && cache->ref_count == 0) {
		debug("cache_unref delete and free %s\n", cache->key->buf);
		cache_file_delete_and_free(cache);	
	}
}

static int cache_key_compare(void *k1, void *k2) 
{
	return (0 == strcmp(k1, k2));
}
					
static unsigned int cache_key_hash(void *k)
{
	return hash_djb2(k);
}


static struct disk_file *disk_file_create(const char *file, const int size)
{	
	assert(size >= 0);
	assert(file);

	struct disk_file *disk;
	int fd;
	if(-1 == (fd = open(file, O_RDWR | O_CREAT | O_TRUNC, 0666))) {
		log_debug(__FILE__, __LINE__, "warning! disk_file_create(%s)\n", strerror(errno));
		return NULL;
	}
	
	disk = calloc(1, sizeof(*disk));
	disk->fd = fd;
	disk->size = size;
	disk->flags_open = 1;
	disk->offset = 0;
	return disk;
}

static struct mem *mem_create(const int size)
{
	if(size <= 0) 
		return NULL;

	struct mem *m = calloc(1, sizeof(*m));
	if(!m) {
		error("mem_create %s\n", strerror(errno));
		return NULL;
	}

	m->low = m->high = 0;
	m->capacity = size;
	m->context = string_limit_init(size);
	debug("mem_create %d bytes\n", size);
	return m;
}

static void disk_file_clean(struct disk_file *disk)
{
	if(!disk) 
		return;
	
	if(disk->flags_open) {
		disk->flags_open = 0;
		close(disk->fd);
	}
	safe_free(disk);
}

static void mem_swapout_done(int error, void *data, int size)
{
	assert(data);

	struct cache_file *cache = data;
	struct mem *m = cache->mem;
	struct disk_file *disk = cache->disk;
	struct cache_dir *dentry = config->cache_dir->items[cache->dir_no];
	struct http_state *http = cache->other;
	
	if(error) {
		s_error("mem_swapout_done %s\n", strerror(errno));
		return;
	}

	if(!m) {
		warning("mem_swapout cache->mem %p is null\n", cache->mem);
		return;
	}

	m->submit = 0;
	m->low += size;
	dentry->offset += size;
	
	assert(disk);
	disk->offset += size;
	
	string_reset(m->context);
	
	debug("mem_swapout_done %d bytes, disk %d\n", size, disk->fd);
	
	/* 
	if(disk->size <= disk->offset) {
		debug("success! mem_swapout_done %d bytes, disk %d fully\n", disk->offset, disk->fd);
		disk_file_clean(disk);
		cache->disk = NULL;
		cache->statu = CACHE_DISK;
		mem_clean(m);
		cache->mem = NULL;
	}
	*/
	
	/* register read event again */

	sock_register_event(http->server_fd, http_reply_read, http_state_ref(http), NULL, NULL);
}

struct shm_cache *shm_cache_create(int fd, int low, int high)
{
	struct shm_cache *sch = calloc(1, sizeof(*sch));
	if(!sch) {
		error("shm_cache_create calloc failed %p\n", sch);
		return NULL;
	}
	
	sch->fd = fd;
	sch->low = low;
	sch->high = high;
	INIT_LIST_HEAD(&sch->list);

	return sch;
}


struct shm_cache * shm_cache_append(struct shm_cache *list, struct shm_file *shm)
{
	struct shm_cache *sc = NULL;
	struct mmap *map = shm->map;
	if(!map)
		return NULL;
	
	int size = map->offset;
	if(srv.shm.offset + size > srv.shm.size) 
		return NULL;
	if(list_count(&list->list) > CHUNK_NUM_IN_SHM)
		return NULL;

	sc = shm_cache_create(map->shm_fd, shm->low, shm->high);
	if(sc == NULL)
		return NULL;
	map->cached = 1;
	
	list_add_tail(&sc->list, &list->list);

	srv.shm.offset += size;
	debug("shm_cache_append [%d - %d], count %d\n", sc->low, sc->high, list_count(&list->list));
	return sc;
}

struct shm_cache *shm_cache_find(struct http_state *http, struct shm_cache *head)
{
	if(!head)
		return NULL;
	if(!http)
		return NULL;
	if(!http->shm)
		return NULL;
	
	struct cache_file *cache = http->cache;
	struct list_head *p;

	assert(cache);
	debug("shm_cache_find head %p\n", head);

	list_for_each(p, &head->list) {
		struct shm_cache *s = list_entry(p, struct shm_cache, list);
		debug("shm_cache_find offset %d, [%d ==> %d]\n", cache->header_size + http->reply->body_out,\
					   	s->low, s->high);
		if(cache->header_size + http->reply->body_out >= s->low && \
						cache->header_size + http->reply->body_out < s->high) {
			http->sc = s;
			return s;
		}
	}

	debug("shm_cache_find offset %d not find, count %d\n", cache->header_size + http->reply->body_out, \
					list_count(&head->list));
	return NULL;
}

int list_count(struct list_head *h) 
{
	if(!h)
		return 0;

	struct list_head *l;
	int count = 0;

	list_for_each(l, h) 
		count++;

	return count;
}

struct mem_cache *mem_cache_find(struct http_state *http, struct mem_cache *head)
{
	if(!head)
		return NULL;
	
	if(!http) 
		return NULL;

	struct cache_file *cache = http->cache;
	assert(cache);
	if((cache->header_size + http->reply->body_out) >= head->low && \
				  (cache->header_size + http->reply->body_out) < head->high) {
		debug("mem_cache_find [%d-%d], out %d\n", head->low, head->high, \
						cache->header_size + http->reply->body_out);
		http->mc = head;
		head->ref++;
		return head;
	}
	
	debug("mem_cache_find offset %d not find mem cache\n", \
					cache->header_size + http->reply->body_out);
	return NULL;
}

int mem_cache_append(struct mem_cache *head, char *buf, int size) 
{
	int size_need = 0;

	if(!head)
		return 0;
	if(!buf)
		return 0;
	if(size <= 0) 
		return 0;
	if(srv.mem.offset + size > srv.mem.size) 
		return 0;
	if(head->high > PER_OBJECT_SIZE_IN_MEM) 
		return 0;
	
	head->high += size;

	string_append(head->mbuf, buf, size);

	debug("mem_cache_append success! append %d bytes [%d-%d]\n", size, head->low, head->high);
	return size;
}

struct mem_cache *mem_cache_create(int low, int high)
{
	struct mem_cache *mc = calloc(1, sizeof(*mc));
	if(!mc) 
		return NULL;

	mc->low = low;
	mc->high = high;

	mc->mbuf = string_limit_init(4096);

	return mc;
}
