#include "chimp/frame.h"
#include "chimp/class.h"
#include "chimp/object.h"
#include "chimp/str.h"

#define CHIMP_FRAME_INIT(ref) \
    do { \
        CHIMP_ANY(ref)->type = CHIMP_VALUE_TYPE_FRAME; \
        CHIMP_ANY(ref)->klass = chimp_frame_class; \
    } while (0)

ChimpRef *chimp_frame_class = NULL;

chimp_bool_t
chimp_frame_class_bootstrap (void)
{
    chimp_frame_class =
        chimp_class_new (NULL, CHIMP_STR_NEW("frame"), chimp_object_class);
    if (chimp_frame_class == NULL) {
        return CHIMP_FALSE;
    }
    chimp_gc_make_root (NULL, chimp_frame_class);
    return CHIMP_TRUE;
}

ChimpRef *
chimp_frame_new (ChimpRef *method)
{
    ChimpRef *locals;
    ChimpRef *ref = chimp_gc_new_object (NULL);
    if (ref == NULL) {
        return NULL;
    }
    CHIMP_FRAME_INIT(ref);
    locals = chimp_hash_new (NULL);
    if (locals == NULL) {
        return NULL;
    }
    CHIMP_FRAME(ref)->method = method;
    CHIMP_FRAME(ref)->locals = locals;
    return ref;
}

