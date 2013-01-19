#ifndef ARRAY_H
#define ARRAY_H

typedef struct {
    int capacity;
    int count;
    void **items;
}array;

extern array *array_create(void);
extern void array_init(array * s);
extern void array_clean(array * s);
extern void array_destroy(array * s);
extern void array_append(array * s, void *obj);
extern void array_insert(array * s, void *obj, int position);
extern void array_preAppend(array * s, int app_count);
#endif
