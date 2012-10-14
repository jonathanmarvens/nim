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

extern ChimpRef *
chimp_vm_eval_invoke (ChimpVM *vm, ChimpRef *method, ChimpRef *args);

static ChimpRef *
chimp_method_call (ChimpRef *self, ChimpRef *args)
{
    if (CHIMP_METHOD_TYPE(self) == CHIMP_METHOD_TYPE_NATIVE) {
        return CHIMP_NATIVE_METHOD(self)->func (CHIMP_METHOD(self)->self, args);
    }
    else {
        return chimp_vm_eval_invoke (NULL, self, args);
    }
}

chimp_bool_t
chimp_method_class_bootstrap (ChimpGC *gc)
{
    chimp_method_class =
        chimp_class_new (gc, CHIMP_STR_NEW(gc, "method"), NULL);
    if (chimp_method_class == NULL) {
        return CHIMP_FALSE;
    }
    chimp_gc_make_root (gc, chimp_method_class);
    CHIMP_CLASS(chimp_method_class)->call = chimp_method_call;
    CHIMP_CLASS(chimp_method_class)->inst_type = CHIMP_VALUE_TYPE_METHOD;
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
    CHIMP_METHOD(ref)->type = CHIMP_METHOD_TYPE_NATIVE;
    CHIMP_NATIVE_METHOD(ref)->func = func;
    return ref;
}

ChimpRef *
chimp_method_new_bytecode (ChimpGC *gc, ChimpRef *code)
{
    ChimpRef *ref = chimp_gc_new_object (gc);
    if (ref == NULL) {
        return NULL;
    }
    CHIMP_METHOD_INIT(ref);
    CHIMP_METHOD(ref)->type = CHIMP_METHOD_TYPE_BYTECODE;
    CHIMP_BYTECODE_METHOD(ref)->code = code;
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
    if (CHIMP_METHOD(ref)->type == CHIMP_METHOD_TYPE_NATIVE) {
        CHIMP_NATIVE_METHOD(ref)->func = CHIMP_NATIVE_METHOD(unbound)->func;
    }
    else {
        CHIMP_BYTECODE_METHOD(ref)->code =
            CHIMP_BYTECODE_METHOD(unbound)->code;
    }
    return ref;
}

