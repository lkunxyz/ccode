/*
 * Array is an array of (void*) items with unlimited capacity
 *
 * Array grows when arrayAppend() is called and no space is left
 * Currently, array does not have an interface for deleting an item because
 *     we do not need such an interface yet.
 */

#include "common.h"
#include "array.h"

static void array_grow(array * a, int min_capacity);

array *array_create(void)
{
    array *a = malloc(sizeof(array));
    array_init(a);
    return a;
}

void array_init(array * a)
{
    assert(a != NULL);
    memset(a, 0, sizeof(array));
}

void array_clean(array * a)
{
    assert(a != NULL);
    /* could also warn if some objects are left */
    free(a->items);
    a->items = NULL;
}

void array_destroy(array * a)
{
    assert(a != NULL);
    array_clean(a);
    free(a);
}

void array_append(array * a, void *obj)
{
    assert(a != NULL);
    if (a->count >= a->capacity)
		array_grow(a, a->count + 1);
    a->items[a->count++] = obj;
}

void array_insert(array *a, void *obj, int position)
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
void array_preappend(array * a, int app_count)
{
    assert(a != NULL);
    if (a->count + app_count > a->capacity)
	array_grow(a, a->count + app_count);
}

/* grows internal buffer to satisfy required minimal capacity */
static void array_grow(array * a, int min_capacity)
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
