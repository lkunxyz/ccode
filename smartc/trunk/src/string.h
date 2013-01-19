
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
#ifndef STRING_H
#define STRING_H

struct string {
	unsigned int size; /* buffer size */
	unsigned int offset; /* current length */
	char *buf;
};

extern struct string *string_init(const char *str);
extern struct string *string_limit_init(const int size);
extern void string_clean(struct string *s);
extern void string_append(struct string *s, const char *str, const int size);
extern void string_append_r(struct string *s, struct string *m);
extern void string_reset(struct string *s);

#endif
