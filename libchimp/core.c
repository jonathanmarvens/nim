#include "chimp/core.h"
#include "chimp/gc.h"
#include "chimp/object.h"

static ChimpGC *gc = NULL;

#define CHIMP_BOOTSTRAP_CLASS(gc, c, n, sup) \
    do { \
        CHIMP_ANY(c)->type = CHIMP_VALUE_TYPE_CLASS; \
        CHIMP_ANY(c)->klass = chimp_class_class; \
        CHIMP_CLASS(c)->super = (sup); \
        CHIMP_CLASS(c)->name = chimp_gc_new_object ((gc)); \
        CHIMP_ANY(CHIMP_CLASS(c)->name)->type = CHIMP_VALUE_TYPE_STR; \
        CHIMP_ANY(CHIMP_CLASS(c)->name)->klass = chimp_str_class; \
        CHIMP_STR(CHIMP_CLASS(c)->name)->data = strndup ((n), (sizeof(n)-1)); \
        if (CHIMP_STR(CHIMP_CLASS(c)->name)->data == NULL) { \
            chimp_gc_delete (gc); \
            gc = NULL; \
            return CHIMP_FALSE; \
        } \
        CHIMP_STR(CHIMP_CLASS(c)->name)->size = sizeof(n)-1; \
        chimp_gc_make_root ((gc), (c)); \
    } while (0)

ChimpRef *chimp_object_class = NULL;
ChimpRef *chimp_class_class = NULL;
ChimpRef *chimp_str_class = NULL;

chimp_bool_t
chimp_core_startup (void)
{
    gc = chimp_gc_new ();
    if (gc == NULL) {
        return CHIMP_FALSE;
    }

    chimp_object_class = chimp_gc_new_object (gc);
    chimp_class_class  = chimp_gc_new_object (gc);
    chimp_str_class    = chimp_gc_new_object (gc);

    CHIMP_BOOTSTRAP_CLASS(gc, chimp_object_class, "object", NULL);
    CHIMP_BOOTSTRAP_CLASS(gc, chimp_class_class, "class", chimp_object_class);
    CHIMP_BOOTSTRAP_CLASS(gc, chimp_str_class, "str", chimp_object_class);

    return CHIMP_TRUE;
}

void
chimp_core_shutdown (void)
{
    if (gc != NULL) {
        chimp_gc_delete (gc);
        gc = NULL;
        chimp_object_class = NULL;
        chimp_class_class = NULL;
        chimp_str_class = NULL;
    }
}

