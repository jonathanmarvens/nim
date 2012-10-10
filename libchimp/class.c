#include "chimp/gc.h"
#include "chimp/object.h"
#include "chimp/class.h"
#include "chimp/lwhash.h"
#include "chimp/str.h"
#include "chimp/method.h"

#define CHIMP_CLASS_INIT(ref) \
    CHIMP_ANY(ref)->type = CHIMP_VALUE_TYPE_CLASS; \
    CHIMP_ANY(ref)->klass = chimp_class_class;

static ChimpRef *
chimp_class_call (ChimpRef *self, ChimpRef *args)
{
    ChimpRef *ref = chimp_gc_new_object (NULL);
    if (ref == NULL) {
        return NULL;
    }
    CHIMP_ANY(ref)->klass = self;
    CHIMP_ANY(ref)->type = CHIMP_CLASS(self)->inst_type;
    return ref;
}

ChimpRef *
chimp_class_new (ChimpGC *gc, ChimpRef *name, ChimpRef *super)
{
    ChimpRef *ref = chimp_gc_new_object (gc);
    if (ref == NULL) {
        return NULL;
    }
    CHIMP_CLASS_INIT(ref);
    CHIMP_CLASS(ref)->name = name;
    CHIMP_CLASS(ref)->super = super;
    CHIMP_CLASS(ref)->methods = chimp_lwhash_new ();
    CHIMP_CLASS(ref)->call = chimp_class_call;
    CHIMP_CLASS(ref)->inst_type = CHIMP_VALUE_TYPE_OBJECT;
    return ref;
}

chimp_bool_t
chimp_class_add_method (ChimpGC *gc, ChimpRef *self, ChimpRef *name, ChimpRef *method)
{
    return chimp_lwhash_put (CHIMP_CLASS(self)->methods, name, method);
}

chimp_bool_t
chimp_class_add_native_method (ChimpGC *gc, ChimpRef *self, const char *name, ChimpNativeMethodFunc func)
{
    ChimpRef *method_ref;
    ChimpRef *name_ref = chimp_str_new (gc, name, strlen (name));
    if (name_ref == NULL) {
        return CHIMP_FALSE;
    }
    method_ref = chimp_method_new_native (gc, func);
    if (method_ref == NULL) {
        return CHIMP_FALSE;
    }
    return chimp_class_add_method (gc, self, name_ref, method_ref);
}

chimp_bool_t
_chimp_bootstrap_L3 (void)
{
    CHIMP_CLASS(chimp_class_class)->methods = chimp_lwhash_new ();
    CHIMP_CLASS(chimp_class_class)->call = chimp_class_call;
    CHIMP_CLASS(chimp_class_class)->inst_type = CHIMP_VALUE_TYPE_CLASS;

    CHIMP_CLASS(chimp_object_class)->methods = chimp_lwhash_new ();
    CHIMP_CLASS(chimp_object_class)->call = chimp_class_call;
    CHIMP_CLASS(chimp_object_class)->inst_type = CHIMP_VALUE_TYPE_OBJECT;

    CHIMP_CLASS(chimp_str_class)->methods = chimp_lwhash_new ();
    CHIMP_CLASS(chimp_str_class)->call = chimp_class_call;
    CHIMP_CLASS(chimp_str_class)->inst_type = CHIMP_VALUE_TYPE_STR;

    return CHIMP_TRUE;
}

