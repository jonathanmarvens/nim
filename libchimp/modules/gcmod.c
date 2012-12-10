#include <stdio.h>
#include "chimp/any.h"
#include "chimp/object.h"
#include "chimp/array.h"
#include "chimp/str.h"

static ChimpRef *
_chimp_gc_get_collection_count (ChimpRef *self, ChimpRef *args)
{
    return chimp_int_new (chimp_gc_collection_count (NULL));
}

static ChimpRef *
_chimp_gc_get_live_count (ChimpRef *self, ChimpRef *args)
{
    return chimp_int_new (chimp_gc_num_live (NULL));
}

static ChimpRef *
_chimp_gc_get_free_count (ChimpRef *self, ChimpRef *args)
{
    return chimp_int_new (chimp_gc_num_free (NULL));
}

static ChimpRef *
_chimp_gc_collect (ChimpRef *self, ChimpRef *args)
{
    return chimp_gc_collect (NULL) ? chimp_true : chimp_false;
}

ChimpRef *
chimp_init_gc_module (void)
{
    ChimpRef *gc;

    gc = chimp_module_new_str ("gc", NULL);
    if (gc == NULL) {
        return NULL;
    }

    if (!chimp_module_add_method_str (
            gc, "get_collection_count", _chimp_gc_get_collection_count)) {
        return NULL;
    }

    if (!chimp_module_add_method_str (
            gc, "get_live_count", _chimp_gc_get_live_count)) {
        return NULL;
    }

    if (!chimp_module_add_method_str (
            gc, "get_free_count", _chimp_gc_get_free_count)) {
        return NULL;
    }

    if (!chimp_module_add_method_str (
            gc, "collect", _chimp_gc_collect)) {
        return NULL;
    }

    return gc;
}


