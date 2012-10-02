#include "chimp/stackframe.h"
#include "chimp/class.h"
#include "chimp/object.h"
#include "chimp/str.h"

#define CHIMP_STACK_FRAME_INIT(ref) \
    do { \
        CHIMP_ANY(ref)->type = CHIMP_VALUE_TYPE_STACK_FRAME; \
        CHIMP_ANY(ref)->klass = chimp_stack_frame_class; \
    } while (0)

ChimpRef *chimp_stack_frame_class = NULL;

chimp_bool_t
chimp_stack_frame_class_bootstrap (ChimpGC *gc)
{
    chimp_stack_frame_class =
        chimp_class_new (gc, CHIMP_STR_NEW(gc, "stackframe"), chimp_object_class);
    if (chimp_stack_frame_class == NULL) {
        return CHIMP_FALSE;
    }
    chimp_gc_make_root (gc, chimp_stack_frame_class);
    return CHIMP_TRUE;
}

ChimpRef *
chimp_stack_frame_new (ChimpGC *gc)
{
    ChimpRef *ref = chimp_gc_new_object (gc);
    if (ref == NULL) {
        return NULL;
    }
    CHIMP_STACK_FRAME_INIT(ref);
    return ref;
}

