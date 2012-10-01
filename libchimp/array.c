#include "chimp/array.h"
#include "chimp/object.h"
#include "chimp/core.h"
#include "chimp/class.h"
#include "chimp/str.h"

#define CHIMP_ARRAY_INIT(ref) \
    CHIMP_ANY(ref)->type = CHIMP_VALUE_TYPE_ARRAY; \
    CHIMP_ANY(ref)->klass = chimp_array_class;

ChimpRef *chimp_array_class = NULL;

chimp_bool_t
chimp_array_class_bootstrap (ChimpGC *gc)
{
    chimp_array_class =
        chimp_class_new (gc, CHIMP_STR_NEW(gc, "array"), chimp_object_class);
    if (chimp_array_class == NULL) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

ChimpRef *
chimp_array_new (ChimpGC *gc)
{
    ChimpRef *ref = chimp_gc_new_object (gc);
    if (ref == NULL) {
        return NULL;
    }
    CHIMP_ARRAY_INIT(ref);
    CHIMP_ARRAY(ref)->items = NULL;
    CHIMP_ARRAY(ref)->size = 0;
    return ref;
}

chimp_bool_t
chimp_array_push (ChimpRef *self, ChimpRef *value)
{
    ChimpRef **items;
    ChimpArray *arr = CHIMP_ARRAY(self);
    items = CHIMP_REALLOC(ChimpRef *, arr->items, sizeof(*arr->items) * (arr->size + 1));
    if (items == NULL) {
        return CHIMP_FALSE;
    }
    arr->items = items;
    items[arr->size++] = value;
    return CHIMP_TRUE;
}

ChimpRef *
chimp_array_pop (ChimpRef *self)
{
    ChimpArray *arr = CHIMP_ARRAY(self);
    return arr->items[--arr->size];
}

