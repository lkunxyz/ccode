
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
#include <openssl/md5.h>

#include "server.h"
#include "md5.h"

static unsigned char digest[16];
static char ascii_digest[32];

char *md5_string_get(const char *url, const int size)
{
	int i;

	MD5_CTX c;
	MD5_Init(&c);
	MD5_Update(&c, url, size);
	MD5_Final(digest, &c);
	
	digest[16] = '\0';
	
	for(i = 0; i < 16; i++) 
		snprintf(ascii_digest + 2*i, 32, "%02x", digest[i]);

	ascii_digest[32] = '\0';

	return ascii_digest;
}

char *cache_key_private(const char *uri, int size)
{
	if(!uri)
		return NULL;

	char buf[MAX_URL];
	int res; 

	res = snprintf(buf, MAX_URL - 1, "private:%s", uri);

	return md5_string_get(buf, res);
}

char *cache_key_public(const char *uri, int size)
{
	if(!uri)
		return NULL;

	char buf[MAX_URL];
	int res; 

	res = snprintf(buf, MAX_URL - 1, "public:%s", uri);

	return md5_string_get(buf, res);
}
