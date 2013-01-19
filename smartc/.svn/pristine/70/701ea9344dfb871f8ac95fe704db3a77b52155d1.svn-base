
#ifndef STRING_H
#define STRING_H

typedef struct {
	unsigned int size; /* buffer size */
	unsigned int offset; /* current length */
	char *buf;
}string;

extern string *string_init(const char *str);
extern string *string_limit_init(const int size);
extern void string_clean(string *s);
extern void string_append(string *s, const char *str, const int size);
extern void string_append_r(string *s, string *m);
extern void string_reset(string *s);

#endif
