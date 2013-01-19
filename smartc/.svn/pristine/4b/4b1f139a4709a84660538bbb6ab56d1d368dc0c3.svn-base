
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
/*
 * Array is an array of (void*) items with unlimited capacity
 *
 * Array grows when arrayAppend() is called and no space is left
 * Currently, array does not have an interface for deleting an item because
 *     we do not need such an interface yet.
 */
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "array.h"

static void array_grow(struct array_t * a, int min_capacity);

struct array_t *array_create(void)
{
    struct array_t *a = malloc(sizeof(struct array_t));
    array_init(a);
    return a;
}

void array_init(struct array_t * a)
{
    assert(a != NULL);
    memset(a, 0, sizeof(struct array_t));
}

void array_clean(struct array_t * a)
{
    assert(a != NULL);
    /* could also warn if some objects are left */
    free(a->items);
    a->items = NULL;
}

void array_destroy(struct array_t * a)
{
    assert(a != NULL);
    array_clean(a);
    free(a);
}

void array_append(struct array_t * a, void *obj)
{
    assert(a != NULL);
    if (a->count >= a->capacity)
		array_grow(a, a->count + 1);
    a->items[a->count++] = obj;
}

void array_insert(struct array_t *a, void *obj, int position)
{
    assert(a != NULL);
    if (a->count >= a->capacity)
	array_grow(a, a->count + 1);
    if (position > a->count)
	position = a->count;
    if (position < a->count)
	memmove(&a->items[position + 1], &a->items[position], (a->count - position) * sizeof(void *));
    a->items[position] = obj;
    a->count++;
}

/* if you are going to append a known and large number of items, call this first */
void array_preappend(struct array_t * a, int app_count)
{
    assert(a != NULL);
    if (a->count + app_count > a->capacity)
	array_grow(a, a->count + app_count);
}

/* grows internal buffer to satisfy required minimal capacity */
static void array_grow(struct array_t * a, int min_capacity)
{
    const int min_delta = 16;
    int delta;
    assert(a->capacity < min_capacity);
    delta = min_capacity;
    /* make delta a multiple of min_delta */
    delta += min_delta - 1;
    delta /= min_delta;
    delta *= min_delta;
    /* actual grow */
    assert(delta > 0);
    a->capacity += delta;
    a->items = a->items ?
		realloc(a->items, a->capacity * sizeof(void *)) :
		malloc(a->capacity * sizeof(void *));
    /* reset, just in case */
    memset(a->items + a->count, 0, (a->capacity - a->count) * sizeof(void *));
}
