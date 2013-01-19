#include "common.h"
#include "header.h"

static header_entry * header_entry_create(const char *str, size_t size);
static void clean_space(char **str, int *size);
static void header_append_entry(header *h, header_entry *e);
static int header_id_get(const char *name, size_t name_size);
static void header_entry_clean(header_entry *e);

static const char *HttpStatusLineFormat = "HTTP/%d.%d %3d %s\r\n";

static const header_field Headers[] =
{
    {"Accept", HDR_ACCEPT},
    {"Accept-Charset", HDR_ACCEPT_CHARSET},
    {"Accept-Encoding", HDR_ACCEPT_ENCODING},
    {"Accept-Language", HDR_ACCEPT_LANGUAGE},
    {"Accept-Ranges", HDR_ACCEPT_RANGES},
    {"Age", HDR_AGE},
    {"Allow", HDR_ALLOW},
    {"Authorization", HDR_AUTHORIZATION},	/* for now */
    {"Cache-Control", HDR_CACHE_CONTROL},
    {"Connection", HDR_CONNECTION},
    {"Content-Base", HDR_CONTENT_BASE},
    {"Content-Disposition", HDR_CONTENT_DISPOSITION},
    {"Content-Encoding", HDR_CONTENT_ENCODING},
    {"Content-Language", HDR_CONTENT_LANGUAGE},
    {"Content-Length", HDR_CONTENT_LENGTH},
    {"Content-Location", HDR_CONTENT_LOCATION},
    {"Content-MD5", HDR_CONTENT_MD5},	/* for now */
    {"Content-Range", HDR_CONTENT_RANGE},
    {"Content-Type", HDR_CONTENT_TYPE},
    {"Cookie", HDR_COOKIE},
    {"Date", HDR_DATE},
    {"ETag", HDR_ETAG},
    {"Expires", HDR_EXPIRES},
    {"From", HDR_FROM},
    {"Host", HDR_HOST},
    {"If-Match", HDR_IF_MATCH},	/* for now */
    {"If-Modified-Since", HDR_IF_MODIFIED_SINCE},
    {"If-None-Match", HDR_IF_NONE_MATCH},	/* for now */
    {"If-Range", HDR_IF_RANGE},
    {"Last-Modified", HDR_LAST_MODIFIED},
    {"Link", HDR_LINK},
    {"Location", HDR_LOCATION},
    {"Max-Forwards", HDR_MAX_FORWARDS},
    {"Mime-Version", HDR_MIME_VERSION},	/* for now */
    {"Pragma", HDR_PRAGMA},
    {"Proxy-Authenticate", HDR_PROXY_AUTHENTICATE},
    {"Proxy-Authentication-Info", HDR_PROXY_AUTHENTICATION_INFO},
    {"Proxy-Authorization", HDR_PROXY_AUTHORIZATION},
    {"Proxy-Connection", HDR_PROXY_CONNECTION},
    {"Proxy-support", HDR_PROXY_SUPPORT},
    {"Public", HDR_PUBLIC},
    {"Range", HDR_RANGE},
    {"Referer", HDR_REFERER},
    {"Request-Range", HDR_REQUEST_RANGE},	/* usually matches HDR_RANGE */
    {"Retry-After", HDR_RETRY_AFTER},	/* for now (ftDate_1123 or ftInt!) */
    {"Server", HDR_SERVER},
    {"Set-Cookie", HDR_SET_COOKIE},
    {"Transfer-Encoding", HDR_TRANSFER_ENCODING},
    {"Te", HDR_TE},
    {"Trailer", HDR_TRAILER},
    {"Upgrade", HDR_UPGRADE},	/* for now */
    {"User-Agent", HDR_USER_AGENT},
    {"Vary", HDR_VARY},	/* for now */
    {"Via", HDR_VIA},	/* for now */
    {"Expect", HDR_EXPECT},
    {"Warning", HDR_WARNING},	/* for now */
    {"WWW-Authenticate", HDR_WWW_AUTHENTICATE},
    {"Authentication-Info", HDR_AUTHENTICATION_INFO},
    {"I-Cache", HDR_X_CACHE},
    {"I-Cache-Lookup", HDR_X_CACHE_LOOKUP},
    {"I-Forwarded-For", HDR_X_FORWARDED_FOR},
    {"I-Request-URI", HDR_X_REQUEST_URI},
    {"I-Squid-Error", HDR_X_SQUID_ERROR},
    {"Negotiate", HDR_NEGOTIATE},
    {"I-Error-URL", HDR_X_ERROR_URL},
    {"X-Error-Status", HDR_X_ERROR_STATUS},
    {"Front-End-Https", HDR_FRONT_END_HTTPS},
    {"Keep-Alive", HDR_KEEP_ALIVE},
    {"Other:", HDR_OTHER},
};

static const struct method_t Methods[] =
{
	{METHOD_GET, "GET"},
	{METHOD_POST, "POST"},
	{METHOD_PUT, "PUT"},
	{METHOD_HEAD, "HEAD"},
	{METHOD_CONNECT, "CONNECT"},
	{METHOD_TRACE, "TRACE"},
	{METHOD_PURGE, "PURGE"},
	{METHOD_OPTIONS, "OPTIONS"},
	{METHOD_DELETE, "DELETE"},
	{METHOD_PROPFIND, "PROPFIND"},
	{METHOD_PROPPATCH, "PROPPATCH"},
	{METHOD_MKCOL, "MKCOL"},
	{METHOD_COPY, "COPY"},
	{METHOD_MOVE, "MOVE"},
	{METHOD_LOCK, "LOCK"},
	{METHOD_UNLOCK, "UNLOCK"},
	{METHOD_BMOVE, "BMOVE"},
	{METHOD_BDELETE, "BDELETE"},
	{METHOD_BPROPFIND, "BPROPFIND"},
	{METHOD_BPROPPATCH, "BPROPPATCH"},
	{METHOD_BCOPY, "BCOPY"},
	{METHOD_SEARCH, "SEARCH"},
	{METHOD_SUBSCRIBE, "SUBSCRIBE"},
	{METHOD_UNSUBSCRIBE, "UNSUBSCRIBE"},
	{METHOD_POLL, "POLL"},
	{METHOD_REPORT, "REPORT"},
	{METHOD_MKACTIVITY, "MKACTIVITY"},
	{METHOD_CHECKOUT, "CHECKOUT"},
	{METHOD_MERGE, "MERGE"},
	{METHOD_ENUM_END, "ERROR"}
};

void http_header_entry_append(header *h, char *name, char *value)
{
	assert(name);
	assert(value);
	assert(h);

	header_entry *e = calloc(1, sizeof(*e));
	
	e->id = header_id_get(name, strlen(name));
	e->name = strdup(name);
	e->value = strdup(value);
	
	header_append_entry(h, e);
}

char *http_reply_build(header *h, int major, int minor, enum http_status statu)
{
	assert(h);
	
	header_entry *e;
	char *retv = calloc(1, MAX_HEADER); /* remaind to free */
	int i;
    
	/* append status */
	snprintf(retv, MAX_HEADER, HttpStatusLineFormat, major, minor, statu, http_status_string(statu));

	for(i = 0; i < h->entries.count; i++) {
		e = h->entries.items[i];
		assert(e);
		strcat(retv, e->name);
		strcat(retv, ": ");
		strcat(retv, e->value);
		strcat(retv, "\r\n");
	}
	strcat(retv, "\r\n");

	return retv;
}


header_entry *header_entry_get(const header *h, int id)
{
	assert(h);
	
	int i;
	for(i = 0; i < h->entries.count; i++) {
		header_entry *e = h->entries.items[i];
		if(id == e->id)
			return e;
	}

	return NULL;
}

void method_free(struct method_t *m)
{
	assert(m);
	
	safe_free(m->name);
}

int http_method_id_get(const char *m)
{
	assert(m);
	int i;

	for(i = 0; i < METHOD_ENUM_END; i++) {
		if(!strcasecmp(m, Methods[i].name))
			return Methods[i].id;
	}

	return METHOD_NONE;
}

void header_free(header *header) 
{
	if(!header) 
		return;
	if(header->link > 0) {
		debug("warning! header_free %p linked\n", header);
		return;
	}
	header_entry *e;
	int i;
	for(i = 0; i < header->entries.count; i++) {
		e = header->entries.items[i];
		header_entry_clean(e);
	}

	array_clean(&header->entries);

	safe_free(header);
}

header * header_ref(struct header_t *h) 
{
	++h->link;
	return h;
}

header * header_unref(struct header_t *h) 
{
	--h->link;
	return h;
}

size_t headers_end(const char *mime, size_t l)
{
	size_t e = 0;
	int state = 1;
	while (e < l && state < 3) {
		switch (state) {
			case 0:
				if ('\n' == mime[e])
					state = 1;
					break;
			case 1:
				if ('\r' == mime[e])
					state = 2;
				else if ('\n' == mime[e])
					state = 3;
				else
					state = 0;
				break;
			case 2:
				if ('\n' == mime[e])
					state = 3;
				else
					state = 0;
				break;
			default:
				break;
			}
			e++;
		}
		if (3 == state)
			return e;
		return 0;
}

header *http_header_parse(const char *buf, size_t size) 
{
	assert(buf);
	assert(size > 0);
	
	size_t header_size;
	const char *cur = buf;
	char *start;
	char *end;
	header *h;

	header_size = headers_end(buf, size);
	
	if(header_size == 0) {
		log_debug(__FILE__, __LINE__, "http_header_parse: error header size %d.", header_size);
		return NULL;
	}

	h = calloc(1, sizeof(*h));

	while((cur - buf) < header_size) {
		start = (char *)cur;
		end = (char *)memchr(start, '\n', header_size);
		if(NULL == end) {
			return NULL;
		}

		cur = end + 1;
		/* ignore <CR> */
		end--;
		while(*end == '\r')
			end--;

		log_debug(__FILE__, __LINE__, "http_header_parse: {%.*s}\n", end - start + 1, start);

		if(start >= end) {
			/* come to header end */ 
			log_debug(__FILE__, __LINE__, "scan completely.\n");
			break;
		}
		
		header_entry *entry = header_entry_create(start, end - start + 1);
		if(entry == NULL)
			continue;
		
		header_append_entry(h, entry);
	}

	log_debug(__FILE__, __LINE__, "Complete http_header_parse %d\n", header_size);

	return h;
}

static void header_append_entry(header *h, header_entry *e)
{
	assert(h);
	assert(e);
	
	array_append(&h->entries, e);
	
	h->len += strlen(e->name) + 2 + strlen(e->value) + 2;
}

static header_entry * header_entry_create(const char *str, size_t size)
{
	if(str == NULL) {
		log_debug(__FILE__, __LINE__, "header_entry_create(str == NULL)\n");
		return NULL;
	}
	
	assert(size > 0);
	
	header_entry *entry;
	const char *start;
	const char *end;
	const char *middle;

	start = str;
	end = str + size;
	if(NULL == (middle = (char *)memchr(start, ':', size))) {
		log_debug(__FILE__, __LINE__, "header_entry_create(can not find ':')\n");
		return NULL;
	}
	entry = calloc(1, sizeof(*entry));
	
	/* get header name */
	int name_size = middle - start + 1;
	clean_space((char **)&start, &name_size);
	entry->name = (char *)strndup((char *)start, (size_t)name_size);

	/* get header value */
	start = middle + 1;
	int value_size = end - start + 1;
	clean_space((char **)&start, &value_size);
	entry->value = (char *)strndup(start, value_size + 1);
	
	/* get header id */
	if((entry->id = header_id_get(entry->name, strlen(entry->name))) == HDR_UNKNOWN)
		log_debug(__FILE__, __LINE__, "Warning! unkown header '%s' \n", entry->name);

	log_debug(__FILE__, __LINE__, "header_entry_create %d |%s| : |%s| \n", \
					entry->id, entry->name, entry->value);
	return entry;
}

static void clean_space(char **s, int *size)
{
	assert(*size > 0);
	
	char *str = *s;
	size_t new_size = 0;
	int i = 0;
	int j = 0;
	while(*(str + i) == ' ' || str[i] ==  '\n' || str[i] == '\r' || \
					str[i] == '\t')
		i++;

	while(str[*size - j] == ' ' || str[*size - j] == '\n' || \
		    str[*size - j] == '\r' || str[*size -j] == '\t')
		++j;

	*s = str + i;
	
	new_size = *size - i - j;

	*size = new_size;
}

static int header_id_get(const char *name, size_t name_size)
{
	assert(name);
	
	int i;

	for(i = 0; i < HDR_ENUM_END; i++) {
		if(name_size != strlen(Headers[i].name))
			continue;
		if(!strcasecmp(name, Headers[i].name))
			return Headers[i].id;
	}
	
	return HDR_UNKNOWN;
};

static void header_entry_clean(header_entry *e)
{
	if(!e)
		return ;

	safe_free(e->name);
	safe_free(e->value);

}

const char *http_status_string(enum http_status status)
{
    /* why not to return matching string instead of using "p" ? @?@ */
    const char *p = NULL;
    switch (status) {
    case 0:
	p = "Init";		/* we init .status with code 0 */
	break;
    case HTTP_CONTINUE:
	p = "Continue";
	break;
    case HTTP_SWITCHING_PROTOCOLS:
	p = "Switching Protocols";
	break;
    case HTTP_OK:
	p = "OK";
	break;
    case HTTP_CREATED:
	p = "Created";
	break;
    case HTTP_ACCEPTED:
	p = "Accepted";
	break;
    case HTTP_NON_AUTHORITATIVE_INFORMATION:
	p = "Non-Authoritative Information";
	break;
    case HTTP_NO_CONTENT:
	p = "No Content";
	break;
    case HTTP_RESET_CONTENT:
	p = "Reset Content";
	break;
    case HTTP_PARTIAL_CONTENT:
	p = "Partial Content";
	break;
    case HTTP_MULTIPLE_CHOICES:
	p = "Multiple Choices";
	break;
    case HTTP_MOVED_PERMANENTLY:
	p = "Moved Permanently";
	break;
    case HTTP_MOVED_TEMPORARILY:
	p = "Moved Temporarily";
	break;
    case HTTP_SEE_OTHER:
	p = "See Other";
	break;
    case HTTP_NOT_MODIFIED:
	p = "Not Modified";
	break;
    case HTTP_USE_PROXY:
	p = "Use Proxy";
	break;
    case HTTP_TEMPORARY_REDIRECT:
	p = "Temporary Redirect";
	break;
    case HTTP_BAD_REQUEST:
	p = "Bad Request";
	break;
    case HTTP_UNAUTHORIZED:
	p = "Unauthorized";
	break;
    case HTTP_PAYMENT_REQUIRED:
	p = "Payment Required";
	break;
    case HTTP_FORBIDDEN:
	p = "Forbidden";
	break;
    case HTTP_NOT_FOUND:
	p = "Not Found";
	break;
    case HTTP_METHOD_NOT_ALLOWED:
	p = "Method Not Allowed";
	break;
    case HTTP_NOT_ACCEPTABLE:
	p = "Not Acceptable";
	break;
    case HTTP_PROXY_AUTHENTICATION_REQUIRED:
	p = "Proxy Authentication Required";
	break;
    case HTTP_REQUEST_TIMEOUT:
	p = "Request Time-out";
	break;
    case HTTP_CONFLICT:
	p = "Conflict";
	break;
    case HTTP_GONE:
	p = "Gone";
	break;
    case HTTP_LENGTH_REQUIRED:
	p = "Length Required";
	break;
    case HTTP_PRECONDITION_FAILED:
	p = "Precondition Failed";
	break;
    case HTTP_REQUEST_ENTITY_TOO_LARGE:
	p = "Request Entity Too Large";
	break;
    case HTTP_REQUEST_URI_TOO_LARGE:
	p = "Request-URI Too Large";
	break;
    case HTTP_UNSUPPORTED_MEDIA_TYPE:
	p = "Unsupported Media Type";
	break;
    case HTTP_INTERNAL_SERVER_ERROR:
	p = "Internal Server Error";
	break;
    case HTTP_NOT_IMPLEMENTED:
	p = "Not Implemented";
	break;
    case HTTP_BAD_GATEWAY:
	p = "Bad Gateway";
	break;
    case HTTP_SERVICE_UNAVAILABLE:
	p = "Service Unavailable";
	break;
    case HTTP_GATEWAY_TIMEOUT:
	p = "Gateway Time-out";
	break;
    case HTTP_HTTP_VERSION_NOT_SUPPORTED:
	p = "HTTP Version not supported";
	break;
    default:
	p = "Unknown";
	log_debug(__FILE__, __LINE__, "Unknown HTTP status code: %d\n", status);
	break;
    }
    return p;
}
