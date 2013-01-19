
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
 * Portions created by the Initial Developer are Copyright (C) 2007-2008
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
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "server.h"
#include "accesslog.h"
#include "log.h"

/* format "%h %l %u %t \"%r\" %>s %b" */
static const char *hyphen = "-";
static const char *format = "%s %s %s [%s] \"%s\" %d %d %s %s/%s";
static const char *accesslog_path = DEFAULT_ACCESS_LOG;

static char *request_line(struct method_t *method, char *urlpath, struct http_version ver);

void accesslog_init()
{
	if(config->log.accesslog)
		return;
	
	config->log.accesslog = fopen(accesslog_path, "a");
	if(config->log.accesslog == NULL) 
		warning("accesslog_init %s, can't log access\n", strerror(errno));
}

void accesslog(struct http_state *http)
{
	if(!http) 
		return ;
	
	static char outline[MAX_LINE];
	char *host = NULL;
	char *ident = NULL;
	char *user = NULL;
	time_t request_time;
	int code = 0;
	int  size = 0;
	int stat = 0;
	char *server = NULL;
	char *reqline;
	
	/* get values */
	host = http->client_ip;
	ident = NULL;
	user =  NULL;
	request_time = http->connect_time;
	if(http->cache) 
		code = http->cache->reply.statu;
	else
		code = 0;
	if(http->reply)
		size = http->reply->body_out;
	else 
		size = 0;
	stat = http->stat;
	server = http->server_ip;
	if(http->request) 
		reqline = request_line(&http->request->method, http->request->url_path, http->request->version);
	else 
		reqline = NULL;

	snprintf(outline, MAX_LINE - 1, format, \
					host, \
					ident == NULL ? hyphen : ident, \
					user == NULL ? hyphen : user, \
					mkhttpdlogtime(&request_time), \
					reqline == NULL ? "-" : reqline, \
					code, \
					size, \
					obj_state_string(stat), \
					stat == TCP_MISS ? "DIRECT" : hyphen, \
					stat == TCP_MISS ? server : hyphen);

	fprintf(config->log.accesslog, "%s\n", outline);		
}

static char *request_line(struct method_t *method, char *urlpath, struct http_version ver)
{
	static char reqline[2048];
	snprintf(reqline, 2047, "%s %s HTTP/%d.%d", \
			method->name, urlpath, ver.major, ver.minor);

	return reqline;
}
