#include "chimp/array.h"
#include "chimp/object.h"
#include "chimp/core.h"
#include "chimp/class.h"
#include "chimp/str.h"

#define CHIMP_ARRAY_INIT(ref) \
    CHIMP_ANY(ref)->type = CHIMP_VALUE_TYPE_ARRAY; \
    CHIMP_ANY(ref)->klass = chimp_array_class;

ChimpRef *chimp_array_class = NULL;

static ChimpRef *
_chimp_array_push (ChimpRef *self, ChimpRef *args)
{
    /* TODO error if args len == 0 */
    if (!chimp_array_push (self, chimp_array_get (args, 0))) {
        /* XXX error? exception? abort? */
        return NULL;
    }
    return chimp_nil;
}

static ChimpRef *
_chimp_array_pop (ChimpRef *self, ChimpRef *args)
{
    return chimp_array_pop (self);
}

chimp_bool_t
chimp_array_class_bootstrap (ChimpGC *gc)
{
    chimp_array_class =
        chimp_class_new (gc, CHIMP_STR_NEW(gc, "array"), chimp_object_class);
    if (chimp_array_class == NULL) {
        return CHIMP_FALSE;
    }
    chimp_gc_make_root (gc, chimp_array_class);
    chimp_class_add_native_method (gc, chimp_array_class, "push", _chimp_array_push);
    chimp_class_add_native_method (gc, chimp_array_class, "pop", _chimp_array_pop);
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

ChimpRef *
chimp_array_get (ChimpRef *self, int32_t pos)
{
    /* XXX index bounds checks */
    ChimpArray *arr = CHIMP_ARRAY(self);
    if (pos >= 0) {
        return arr->items[pos];
    }
    else {
        return arr->items[arr->size - (size_t)(-pos)];
    }
}

