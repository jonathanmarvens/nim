#include "chimp/method.h"

#define CHIMP_METHOD_INIT(ref) \
    do { \
        CHIMP_ANY(ref)->type = CHIMP_VALUE_TYPE_METHOD; \
        CHIMP_ANY(ref)->klass = chimp_method_class; \
    } while (0)

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

