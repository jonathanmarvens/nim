#include "chimp/object.h"
#include "chimp/gc.h"
#include "chimp/task.h"

#define CHIMP_OBJECT_INIT(ref, c) \
    do { \
        CHIMP_ANY(ref)->type = CHIMP_VALUE_TYPE_OBJECT; \
        CHIMP_ANY(ref)->klass = ((c) == NULL ? chimp_object_class : (c)); \
    } while (0)

ChimpCmpResult
chimp_object_cmp (ChimpRef *a, ChimpRef *b)
{
    ChimpRef *a_class, *b_class;

    /* reference equality is nice & easy */
    if (a == b) {
        return CHIMP_CMP_EQ;
    }

    a_class = CHIMP_ANY_CLASS(a);
    b_class = CHIMP_ANY_CLASS(b);

    if (CHIMP_CLASS(a_class)->cmp != NULL) {
        return CHIMP_CLASS(a_class)->cmp (a, b);
    }
    else if (CHIMP_CLASS(b_class)->cmp != NULL) {
        return CHIMP_CLASS(b_class)->cmp (a, b);
    }
    else {
        return CHIMP_CMP_NOT_IMPL;
    }
}

ChimpRef *
chimp_object_str (ChimpRef *self)
{
    ChimpRef *klass = CHIMP_ANY_CLASS(self);

    while (klass != NULL) {
        if (CHIMP_CLASS(klass)->str != NULL) {
            return CHIMP_CLASS(klass)->str (self);
        }
        klass = CHIMP_CLASS(klass)->super;
    }

    chimp_bug (__FILE__, __LINE__, "cannot convert type to string. probably a bug.");
    return NULL;
}

ChimpRef *
chimp_object_add (ChimpRef *left, ChimpRef *right)
{
    ChimpRef *klass = CHIMP_ANY_CLASS(left);

    while (klass != NULL) {
        if (CHIMP_CLASS(klass)->add != NULL) {
            return CHIMP_CLASS(klass)->add (left, right);
        }
        klass = CHIMP_CLASS(klass)->super;
    }

    chimp_bug (__FILE__, __LINE__, "`%s` type does not support the '+' operator", CHIMP_STR_DATA(CHIMP_CLASS(CHIMP_ANY_CLASS(left))->name));
    return NULL;
}

ChimpRef *
chimp_object_sub (ChimpRef *left, ChimpRef *right)
{
    ChimpRef *klass = CHIMP_ANY_CLASS(left);

    while (klass != NULL) {
        if (CHIMP_CLASS(klass)->sub != NULL) {
            return CHIMP_CLASS(klass)->sub (left, right);
        }
        klass = CHIMP_CLASS(klass)->super;
    }

    chimp_bug (__FILE__, __LINE__, "type does not support the '-' operator");
    return NULL;
}

ChimpRef *
chimp_object_mul (ChimpRef *left, ChimpRef *right)
{
    ChimpRef *klass = CHIMP_ANY_CLASS(left);

    while (klass != NULL) {
        if (CHIMP_CLASS(klass)->mul != NULL) {
            return CHIMP_CLASS(klass)->mul (left, right);
        }
        klass = CHIMP_CLASS(klass)->super;
    }

    chimp_bug (__FILE__, __LINE__, "type does not support the '*' operator");
    return NULL;
}

ChimpRef *
chimp_object_div (ChimpRef *left, ChimpRef *right)
{
    ChimpRef *klass = CHIMP_ANY_CLASS(left);

    while (klass != NULL) {
        if (CHIMP_CLASS(klass)->mul != NULL) {
            return CHIMP_CLASS(klass)->div (left, right);
        }
        klass = CHIMP_CLASS(klass)->super;
    }

    chimp_bug (__FILE__, __LINE__, "type does not support the '/' operator");
    return NULL;
}

ChimpRef *
chimp_object_call (ChimpRef *target, ChimpRef *args)
{
    ChimpRef *klass = CHIMP_ANY_CLASS(target);

    while (klass != NULL) {
        if (CHIMP_CLASS(klass)->call != NULL) {
            return CHIMP_CLASS(klass)->call (target, args);
        }
        klass = CHIMP_CLASS(klass)->super;
    }
    return NULL;
}

ChimpRef *
chimp_object_getattr (ChimpRef *self, ChimpRef *name)
{
    ChimpRef *klass = CHIMP_ANY_CLASS(self);

    while (klass != NULL) {
        if (CHIMP_CLASS(klass)->getattr != NULL) {
            return CHIMP_CLASS(klass)->getattr (self, name);
        }
        klass = CHIMP_CLASS(klass)->super;
    }
    return NULL;
}

ChimpRef *
chimp_object_getitem (ChimpRef *self, ChimpRef *key)
{
    ChimpRef *klass = CHIMP_ANY_CLASS(self);

    while (klass != NULL) {
        if (CHIMP_CLASS(klass)->getitem != NULL) {
            return CHIMP_CLASS(klass)->getitem (self, key);
        }
        klass = CHIMP_CLASS(klass)->super;
    }
    return NULL;
}

ChimpRef *
chimp_object_getattr_str (ChimpRef *self, const char *name)
{
    return chimp_object_getattr (self, chimp_str_new (name, strlen (name)));
}

chimp_bool_t
chimp_object_instance_of (ChimpRef *object, ChimpRef *klass)
{
    ChimpRef *c = CHIMP_ANY_CLASS(object);
    while (c != NULL) {
        if (c == klass) {
            return CHIMP_TRUE;
        }
        c = CHIMP_CLASS(c)->super;
    }
    return CHIMP_FALSE;
}

