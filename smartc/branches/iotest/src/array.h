
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
#ifndef ARRAY_H
#define ARRAY_H

struct array_t{
    int capacity;
    int count;
    void **items;
};

extern struct array_t *array_create(void);
extern void array_init(struct array_t * s);
extern void array_clean(struct array_t * s);
extern void array_destroy(struct array_t * s);
extern void array_append(struct array_t * s, void *obj);
extern void array_insert(struct array_t * s, void *obj, int position);
extern void array_preAppend(struct array_t * s, int app_count);
#endif
