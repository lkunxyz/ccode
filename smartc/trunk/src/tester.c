/* Copyright (C) 2002, 2004 Christopher Clark <firstname.lastname@cl.cam.ac.uk> */

#include "hashtable.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h> /* for memcmp */
#include <assert.h>
#include <openssl/md5.h>

struct url_list_t {
	char *url;
	struct url_list_t *next;
};

struct key
{
    char *str;
};

struct value
{
    char *id;
};

DEFINE_HASHTABLE_INSERT(insert_some, struct key, struct value);
DEFINE_HASHTABLE_SEARCH(search_some, struct key, struct value);
DEFINE_HASHTABLE_REMOVE(remove_some, struct key, struct value);
DEFINE_HASHTABLE_ITERATOR_SEARCH(search_itr_some, struct key);

static char *md5_string_get(const char *url)
{
	static unsigned char digest[16];
	static char ascii_digest[32];
	int i;

	MD5_CTX c;
	MD5_Init(&c);
	MD5_Update(&c, url, strlen(url));
	MD5_Final(digest, &c);
	
	digest[16] = '\0';
	
	for(i = 0; i < 16; i++) 
		snprintf(ascii_digest + 2*i, 32, "%02x", digest[i]);

	ascii_digest[32] = '\0';
	return ascii_digest;
}

static void url_list_insert(struct url_list_t **list, const char *url) 
{
	struct url_list_t *u = calloc(1, sizeof(*u));
	u->url = strdup(url);
	
	u->next = *list;
	*list = u;
}
/*****************************************************************************/
static unsigned int hashfromkey(void *ky)
{
	struct key *k = ky;
	return hash_djb2(k->str);
}

static int equalkeys(void *k1, void *k2)
{
	struct key *key1 = k1;
	struct key *key2 = k2;

    return (0 == strcasecmp(key1->str, key2->str));
}

int
main(int argc, char **argv)
{
    struct key *k;
	struct hashtable *h;
    struct value *v, *found;
    struct hashtable_itr *itr;
	char line[4096];
	struct url_list_t *list;
    int i;
	
	if(argc < 3) {
		fprintf(stderr, "usage: tester hash_table_length url_file\n");
		exit(1);
	}
	
	list = calloc(1, sizeof(*list));
    h = create_hashtable(atoi(argv[1]), hashfromkey, equalkeys);
    if (NULL == h) exit(-1); /*oom*/

	FILE *url_file = fopen(argv[2], "r");
	assert(url_file);
	
	while(fgets(line, 4096, url_file)) {
		if(isspace(*line))
			continue;
		k = calloc(1, sizeof(*k));
		k->str = strdup(md5_string_get(line));
		v = calloc(1, sizeof(*v));
		v->id = strdup(line);

		url_list_insert(&list, line);
		insert_some(h, k, v);
	}
        
    printf("After insertion, hashtable contains %u items.\n", hashtable_count(h));

	struct url_list_t *head = list;
	int found_num = 0;
	for(; head; head = head->next) {
		struct key k;
		
		if(head->url == NULL)
			break;
		k.str = md5_string_get(head->url);
		if(NULL == (found = search_some(h, &k))) {
			printf("BUG can not find %s\n", k.str);
		}
		else 
			printf("Found::: %s %s\n", k.str, found->id);
		found_num ++;
	}
	printf("Total find number %d, hash congestion %u, hash table length %d\n", found_num, h->same_hash, h->tablelength);
	hashtable_destroy(h, 0);
    return 0;
}

