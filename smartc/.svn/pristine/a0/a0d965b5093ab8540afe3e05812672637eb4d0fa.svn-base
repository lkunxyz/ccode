#include "common.h"
#include "string.h"

static void string_grow(string *s, const size_t len);

void string_append_r(string *s, string *m)
{
	assert(s);
	assert(m);
	
	if(s->offset + m->offset > s->size)
		string_grow(s, m->offset);

	memcpy(s->buf + s->offset, m->buf, m->offset);
	s->offset += m->offset;
}

string *string_init(const char *str)
{
	if(!str)
		return NULL;
	int size = strlen(str);
	string *s = string_limit_init(size);
	if(!s)
		return NULL;

	memcpy(s->buf, str, size);
	s->offset += size;	
	return s;
}

string *string_limit_init(const int size)
{
	if(size <= 0) 
		return ;

	string *str = calloc(1, sizeof(*str));
	if(!str)
		return NULL;

	str->buf = calloc(1, size);
	assert(str->buf);

	str->size = size;
	str->offset = 0;

	return str;
}

void string_clean(string *s)
{
	if(!s)
		return ;
	safe_free(s->buf);
	s->offset = s->size = 0;
	safe_free(s);
}

void string_append(string *s, const char *str, const int size)
{
	assert(s);
	assert(str);

	if(s->offset + size > s->size)
		string_grow(s, size);

	memcpy(s->buf + s->offset, str, size);
	s->offset += size;
}

void string_reset(string *s)
{
	if(!s)
		return;
	assert(s);
	
	s->offset = 0;
	memset(s->buf, 0, s->size);
}


static void string_grow(string *s, const size_t len)
{
	assert(s);
	assert(len > 0);
	s->size += len;

	s->buf = realloc(s->buf, s->size);
	memset(s->buf + (s->size - len), 0, len);
}
