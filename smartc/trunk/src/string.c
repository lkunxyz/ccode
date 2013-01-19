
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "server.h"
#include "string.h"

static void string_grow(struct string *s, const size_t len);

void string_append_r(struct string *s, struct string *m)
{
	assert(s);
	assert(m);
	
	if(s->offset + m->offset > s->size)
		string_grow(s, m->offset);

	memcpy(s->buf + s->offset, m->buf, m->offset);
	s->offset += m->offset;
}

struct string *string_init(const char *str)
{
	if(!str)
		return NULL;
	int size = strlen(str);
	struct string *s = string_limit_init(size);
	if(!s)
		return NULL;

	memcpy(s->buf, str, size);
	s->offset += size;	
	return s;
}

struct string *string_limit_init(const int size)
{
	if(size <= 0) 
		return ;

	struct string *str = calloc(1, sizeof(*str));
	if(!str)
		return NULL;

	str->buf = calloc(1, size);
	assert(str->buf);

	str->size = size;
	str->offset = 0;

	return str;
}

void string_clean(struct string *s)
{
	if(!s)
		return ;
	safe_free(s->buf);
	s->offset = s->size = 0;
	safe_free(s);
}

void string_append(struct string *s, const char *str, const int size)
{
	assert(s);
	assert(str);

	if(s->offset + size > s->size)
		string_grow(s, size);

	memcpy(s->buf + s->offset, str, size);
	s->offset += size;
}

void string_reset(struct string *s)
{
	if(!s)
		return;
	assert(s);
	
	s->offset = 0;
	memset(s->buf, 0, s->size);
}


static void string_grow(struct string *s, const size_t len)
{
	assert(s);
	assert(len > 0);
	s->size += len;

	s->buf = realloc(s->buf, s->size);
	memset(s->buf + (s->size - len), 0, len);
}
