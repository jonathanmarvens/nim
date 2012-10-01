#include "chimp/method.h"
#include "chimp/class.h"
#include "chimp/str.h"
#include "chimp/task.h"

#define CHIMP_METHOD_INIT(ref) \
    do { \
        CHIMP_ANY(ref)->type = CHIMP_VALUE_TYPE_METHOD; \
        CHIMP_ANY(ref)->klass = chimp_method_class; \
    } while (0)

ChimpRef *chimp_method_class = NULL;

static ChimpRef *
chimp_method_call (ChimpRef *self, ChimpRef *args)
{
    /* TODO bytecode methods */
    return CHIMP_NATIVE_METHOD(self)->func (CHIMP_METHOD(self)->self, args);
}

chimp_bool_t
chimp_method_class_bootstrap (ChimpGC *gc)
{
    chimp_method_class =
        chimp_class_new (gc, CHIMP_STR_NEW(gc, "method"), chimp_object_class);
    if (chimp_method_class == NULL) {
        return CHIMP_FALSE;
    }
    CHIMP_CLASS(chimp_method_class)->call = chimp_method_call;
    return CHIMP_TRUE;
}

ChimpRef *
chimp_method_new_native (ChimpGC *gc, ChimpNativeMethodFunc func)
{
    ChimpRef *ref = chimp_gc_new_object (gc);
    if (ref == NULL) {
        return NULL;
    }
    CHIMP_METHOD_INIT(ref);
    CHIMP_METHOD(ref)->type = CHIMP_METHOD_NATIVE;
    CHIMP_NATIVE_METHOD(ref)->func = func;
    return ref;
}

ChimpRef *
chimp_method_new_bound (ChimpRef *unbound, ChimpRef *self)
{
    /* TODO ensure unbound is actually ... er ... unbound */
    ChimpRef *ref = chimp_gc_new_object (CHIMP_CURRENT_GC);
    if (ref == NULL) {
        return NULL;
    }
    CHIMP_METHOD_INIT(ref);
    CHIMP_METHOD(ref)->type = CHIMP_METHOD(unbound)->type;
    CHIMP_METHOD(ref)->self = self;
    if (CHIMP_METHOD(ref)->type == CHIMP_METHOD_NATIVE) {
        CHIMP_NATIVE_METHOD(ref)->func = CHIMP_NATIVE_METHOD(unbound)->func;
        return ref;
    }
    else {
        chimp_bug (__FILE__, __LINE__, "code to bind bytecode methods not written yet");
        return NULL;
    }
}

