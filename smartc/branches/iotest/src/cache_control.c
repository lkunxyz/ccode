
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
 *		tangtang 9/2/2008
 *
 * Contributor(s):
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "server.h"
#include "log.h"
#include "cache_control.h"
#define CONTROL_COUNT 6

static void cache_control_request_parse(struct cache_control *cc, struct header_t *head);
static void cache_control_reply_parse(struct cache_control *cc, struct header_t *head);
static void cache_control_parse(struct cache_control *cc, char *value, int size);



#define LM_FACTOR 0.2
#define DEFAULT_CACHE_MIN_LIFETIME 60
#define DEFAULT_CACHE_MAX_LIFETIME (7*24*60*60)
#define MAX_STALE "max-stale"
#define MIN_STALE "min-stale"
#define MIN_FRESH "min-fresh"
#define MAX_AGE "max-age"
#define NO_CACHE "no-cache"
#define NO_STORE "no-store"
#define ONLY_IF_CACHED "only-if-cached"
#define PUBLIC "public"
#define PRIVATE "private"
#define NO_TRANSFORM "no-transform"
#define MUST_REVALIDATE "must-revalidate"
#define PROXY_REVALIDATE "proxy-revalidate"
#define S_MAXAGE "s-maxage"


/* computer cache age */
time_t cache_age(struct cache_file *cache)
{
	if(!cache) 
		return -1;

	int apparent_age;
	int corrected_apparent_age;
	int response_delay_estimate;
	int age_when_document_arrived_at_our_cache;
	int how_long_copy_has_been_in_our_cache ;
	int age;

	if(cache->time.date < 0) {
		debug("cache_age response Date %d < 0, invalid\n", cache->time.date);
		return 0;
	}

	apparent_age = (cache->time.response_got - cache->time.date) > 0 ? \
				   (cache->time.response_got - cache->time.date) : 0;
	
	corrected_apparent_age = cache->time.header_age > apparent_age ? \
							 cache->time.header_age : apparent_age;
	response_delay_estimate  = cache->time.response_got - cache->time.request_issued;

	age_when_document_arrived_at_our_cache = corrected_apparent_age + response_delay_estimate;

	current_time_get();
	how_long_copy_has_been_in_our_cache = current_time.tv_sec - cache->time.response_got;

	age = age_when_document_arrived_at_our_cache + how_long_copy_has_been_in_our_cache;

	debug("cache_age %ld\n", age);
	return age;
}

/* after cache time and cache-control parsed */
time_t cache_freshness_lifetime_server(struct cache_file *cache)
{
	if(!cache) 
		return -1;

	int heuristic = 0;
	int server_freshness_limit;
	int time_since_last_modify;

	if (cache->time.max_age > 0)
	{
		server_freshness_limit = cache->time.max_age;
		debug("cache_freshness_lifetime_server server_freshness_limit %ld, max_age set\n", server_freshness_limit);
	}
	else if (cache->time.expires > 0)
	{
		server_freshness_limit = cache->time.expires - cache->time.date;
		debug("cache_freshness_lifetime_server server_freshness_limit %ld, expires set\n", server_freshness_limit);
	}
	else if (cache->time.last_modified > 0)
	{
		time_since_last_modify = (cache->time.date - cache->time.last_modified) > 0 ? \
									 (cache->time.date - cache->time.last_modified) : 0;
		server_freshness_limit = time_since_last_modify * LM_FACTOR;
		heuristic = 1;
		debug("cache_freshness_lifetime_server server_freshness_limit %ld, last_modified set\n", server_freshness_limit);
	}
	else
	{
		server_freshness_limit = DEFAULT_CACHE_MIN_LIFETIME;
		heuristic = 1;
		debug("cache_freshness_lifetime_server server_freshness_limit %ld, all not set\n", server_freshness_limit);
	}

	if (heuristic)
	{
			if (server_freshness_limit > DEFAULT_CACHE_MAX_LIFETIME)
				server_freshness_limit = DEFAULT_CACHE_MAX_LIFETIME;
			if (server_freshness_limit < DEFAULT_CACHE_MIN_LIFETIME)
				server_freshness_limit = DEFAULT_CACHE_MIN_LIFETIME;
	}

	debug("cache_freshness_lifetime_server life %ld\n", server_freshness_limit);
	return(server_freshness_limit);
}

/* can not cache */
int cache_able(struct cache_file *cache, struct request_t *req, struct reply_t *reply)
{
	assert(req);
	if(!cache) 
		return CACHE_NONE;
	if(!reply)
		return CACHE_NONE;

	int no_store = 0;

	if(req->cc.no_store)  {
		debug("cache_able '%s', client request declare no-store\n", req->uri);
		return CACHE_NONE;
	}
	else if(reply->cc.no_store) {
		debug("cache_able '%s', server reply declare no-store\n", req->uri);
		return CACHE_NONE;
	}

	switch(reply->statu) {
		case 200:
		case 203:
		case 300:
		case 301:
		case 410:
			return CACHE_PENDING;
			break;
		default:
			return CACHE_NONE;
			break;
	}
}

int freshness_check(struct cache_file *cache, struct request_t *request)
{
	if(!cache) {
		debug("freshness_check cache %p is null, return TCP_MISS\n", cache);
		return CACHE_STALE;
	}
	if(!request) {
		debug("freshness_check request %p is null, return TCP_MISS\n", cache);
		return CACHE_STALE;
	}
	if(request->cc.no_cache > 0) 
		return CACHE_STALE;

	int age = cache_age(cache);
	int freshness_lifetime = cache->time.freshness_lifetime;
	int interval;

	
	if(age < freshness_lifetime) {
		interval = freshness_lifetime - age;
		if(request->cc.min_stale > interval) {
			debug("freshness_check freshness %d < min_stale %d, return CACHE_STALE\n", \
							interval, request->cc.min_stale);
			return CACHE_STALE;
		}
		else {
			debug("freshness_check freshness %d, return CACHE_FRESH\n", interval);
			return CACHE_FRESH;
		}
	}
	else {
		interval = age - freshness_lifetime;
		if(request->cc.max_stale > interval) {
			debug("freshness_check staleness %d < max_stale %d, return CACHE_FRESH\n", \
							interval, request->cc.max_stale);
			return CACHE_FRESH;
		}
		else {
			debug("freshness_check staleness %d, return CACHE_STALE\n", interval);
			return CACHE_STALE;
		}
		debug("freshness_check staleness %d\n", interval);
		return CACHE_FRESH;
	}
}

void cache_control_set(struct cache_control *cc, struct header_t *header)
{
	if(!cc) {
		warning("cache_control_set cc %p is null\n", cc);
		return ;
	}
	if(!header) {
		debug("cache_control_set header %p is nil\n", header);
		return;
	}
	
	char *token ;
	char *cvalue = NULL;
	struct header_entry_t *entry = header_entry_get(header, HDR_CACHE_CONTROL);
	if(!entry) {
		debug("cache_control_set Cache-Control not found %p\n", entry);
		if(NULL == (entry = header_entry_get(header, HDR_PRAGMA))) {
			debug("cache_control_set Pragma not found %p\n", entry);
			return ;
		}
	}

	debug("cache_control_set %s\n", entry->value);
	cvalue = strdup(entry->value); /* remaind to free */
	token = (char *) strtok(cvalue,  ";, ");
	if(!token) {
		cache_control_parse(cc, cvalue, strlen(cvalue));
	}
	else {
		while(token) {
			cache_control_parse(cc, token, strlen(token));
			token = (char *) strtok(NULL, ";, ");
		}
	}

	safe_free(cvalue);
}

static void cache_control_parse(struct cache_control *cc, char *value, int size)
{
	assert(cc);
	assert(value);
	assert(size > 0);

	const char *token = value;
	
	if(!strncasecmp(token, MAX_STALE, strlen(MAX_STALE))) {
		char *mid = strchr(token, '=');
		if(!mid) {
			debug("cache_control_request_parse %s unknown\n", token);
			return;
		}

		while(isspace(*(++mid)));
		cc->max_stale = atoi(mid);
	}
	else if(!strncasecmp(token, MIN_STALE, strlen(MIN_STALE))) {
		char *mid = strchr(token, '=');
		if(!mid) {
			debug("cache_control_request_parse %s unknown\n", token);
			return;
		}

		while(isspace(*(++mid)));
		cc->min_stale = atoi(mid);
	}
	else if(!strncasecmp(token, MIN_FRESH, strlen(MIN_FRESH))) {
		char *mid = strchr(token, '=');
		if(!mid) {
			debug("cache_control_request_parse %s unknown\n", token);
			return ;
		}

		while(isspace(*(++mid)));
		cc->min_fresh = atoi(mid);
	}
	else if(!strcasecmp(token, PUBLIC)) {
		cc->public = 1;
	}
	else if(!strcasecmp(token, PRIVATE)) {
		cc->private = 1;
	}
	else if(!strncasecmp(token, MAX_AGE, strlen(MAX_AGE))) {
		char *mid = strchr(token, '=');
		if(!mid) {
			debug("cache_control_reply_parse %s unknown\n", token);
			return;
		}

		while(isspace(*(++mid)));
		cc->max_age = atoi(mid);
	}
	else if(!strncasecmp(token, S_MAXAGE, strlen(MAX_AGE))) {
		char *mid = strchr(token, '=');
		if(!mid) {
			debug("cache_control_reply_parse %s unknown\n", token);
			return;
		}

		while(isspace(*(++mid)));
		cc->s_maxage = atoi(mid);
	}
	else if(!strcasecmp(token, NO_CACHE))
		cc->no_cache = 1;
	else if(!strcasecmp(token, NO_STORE)) 
		cc->no_store = 1;
	else if(!strcasecmp(token, NO_TRANSFORM)) 
		cc->no_transform = 1;
	else if(!strcasecmp(token, MUST_REVALIDATE))
		cc->must_revalidate = 1;
	else if(!strcasecmp(token, PROXY_REVALIDATE)) 
		cc->proxy_revalidate = 1;
	else {
			debug("cache_control_reply_parse cache control unknown value %s\n", token);
	}
}
