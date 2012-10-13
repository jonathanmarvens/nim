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
    ChimpRef *ctor;
    ChimpRef *ref;
    
    /* XXX having two code paths here is probably wrong/begging for trouble */
    if (self == chimp_class_class) {
        ref = chimp_class_new (NULL, CHIMP_ARRAY_ITEM(args, 0), CHIMP_ARRAY_ITEM(args, 1));
    }
    else {
        ref = chimp_gc_new_object (NULL);
        if (ref == NULL) {
            return NULL;
        }
        CHIMP_ANY(ref)->klass = self;
        CHIMP_ANY(ref)->type = CHIMP_CLASS(self)->inst_type;
        ctor = chimp_object_getattr_str (ref, "init");
        if (ctor != NULL) {
            chimp_object_call (ctor, args);
        }
    }
    return ref;
}

static const char empty[] = "";

static ChimpRef *
chimp_str_init (ChimpRef *self, ChimpRef *args)
{
    /* TODO don't copy string data on every call ... */
    if (CHIMP_ARRAY_SIZE(args) == 0) {
        CHIMP_STR(self)->data = strdup (empty);
        CHIMP_STR(self)->size = 0;
    }
    else if (CHIMP_ARRAY_SIZE(args) == 1) {
        CHIMP_STR(self)->data = strdup (CHIMP_STR_DATA(chimp_object_str (NULL, CHIMP_ARRAY_FIRST(args))));
        CHIMP_STR(self)->size = strlen (CHIMP_STR_DATA(self));
    }
    else {
        size_t i;
        size_t len;
        char *p;
        ChimpRef *str_values = chimp_array_new (NULL);
        if (str_values == NULL) {
            return NULL;
        }

        /* 1. convert all constructor args to strings */
        for (i = 0; i < CHIMP_ARRAY_SIZE(args); i++) {
            ChimpRef *str = chimp_object_str (NULL, CHIMP_ARRAY_ITEM(args, i));
            if (str == NULL) {
                return NULL;
            }
            if (!chimp_array_push (str_values, str)) {
                return NULL;
            }
        }

        /* 2. compute total length of the arg strings */
        len = 0;
        for (i = 0; i < CHIMP_ARRAY_SIZE(str_values); i++) {
            ChimpRef *str = CHIMP_ARRAY_ITEM(str_values, i);
            if (str == NULL) {
                return NULL;
            }
            len += CHIMP_STR_SIZE(str);
        }

        /* 3. allocate buffer & copy arg strings into it */
        CHIMP_STR(self)->data = CHIMP_MALLOC (char, len + 1);
        if (CHIMP_STR(self)->data == NULL) {
            return NULL;
        }
        CHIMP_STR(self)->size = len;
        p = CHIMP_STR(self)->data;
        for (i = 0; i < CHIMP_ARRAY_SIZE(str_values); i++) {
            ChimpRef *str = CHIMP_ARRAY_ITEM(str_values, i);
            if (str == NULL) {
                return NULL;
            }
            memcpy (p, CHIMP_STR_DATA(str), CHIMP_STR_SIZE(str));
            p += CHIMP_STR_SIZE(str);
        }
        *p = '\0';
    }
    return chimp_nil;
}

ChimpRef *
chimp_class_new (ChimpGC *gc, ChimpRef *name, ChimpRef *super)
{
    ChimpRef *ref = chimp_gc_new_object (gc);
    if (ref == NULL) {
        return NULL;
    }
    if (super == NULL || super == chimp_nil) {
        super = chimp_object_class;
    }
    CHIMP_CLASS_INIT(ref);
    CHIMP_CLASS(ref)->name = name;
    CHIMP_CLASS(ref)->super = super;
    CHIMP_CLASS(ref)->methods = chimp_lwhash_new ();
    CHIMP_CLASS(ref)->call = chimp_class_call;
    CHIMP_CLASS(ref)->inst_type = CHIMP_VALUE_TYPE_OBJECT;
    return ref;
}

ChimpRef *
chimp_class_new_instance (ChimpRef *klass, ...)
{
    va_list args;
    ChimpRef *arg;
    ChimpRef *arr = chimp_array_new (NULL);
    if (arr == NULL) {
        return NULL;
    }

    va_start (args, klass);
    while ((arg = va_arg(args, ChimpRef *)) != NULL) {
        if (!chimp_array_push (arr, arg)) {
            va_end (args);
            return NULL;
        }
    }
    va_end (args);

    return chimp_class_call (klass, arr);
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
    chimp_class_add_native_method (NULL, chimp_str_class, "init", chimp_str_init);

    return CHIMP_TRUE;
}

