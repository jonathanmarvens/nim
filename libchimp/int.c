#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "chimp/int.h"
#include "chimp/str.h"
#include "chimp/class.h"
#include "chimp/ast.h"

#define CHIMP_INT_INIT(ref) \
    CHIMP_ANY(ref)->type = CHIMP_VALUE_TYPE_INT; \
    CHIMP_ANY(ref)->klass = chimp_int_class;

ChimpRef *chimp_int_class = NULL;

static ChimpRef *
chimp_int_str (ChimpRef *self)
{
    char buf[64];
    int len;

    len = snprintf (buf, sizeof(buf), "%" PRId64, CHIMP_INT(self)->value);

    if (len < 0) {
        return NULL;
    }
    else if (len > sizeof(buf)) {
        chimp_bug (__FILE__, __LINE__, "chimp_int_str output truncated: %" PRId64, CHIMP_INT(self)->value);
        return NULL;
    }

    return chimp_str_new (buf, len);
}

static ChimpRef *
chimp_int_add (ChimpRef *left, ChimpRef *right)
{
    int64_t left_value, right_value;
    
    left_value = CHIMP_INT(left)->value;
    if (CHIMP_ANY_CLASS(right) != chimp_int_class) {
        return NULL;
    }
    right_value = CHIMP_INT(right)->value;
    return chimp_int_new (left_value + right_value);
}

static ChimpRef *
chimp_int_sub (ChimpRef *left, ChimpRef *right)
{
    int64_t left_value, right_value;
    
    left_value = CHIMP_INT(left)->value;
    if (CHIMP_ANY_CLASS(right) != chimp_int_class) {
        return NULL;
    }
    right_value = CHIMP_INT(right)->value;
    return chimp_int_new (left_value - right_value);
}

static ChimpRef *
chimp_int_mul (ChimpRef *left, ChimpRef *right)
{
    int64_t left_value, right_value;
    
    left_value = CHIMP_INT(left)->value;
    if (CHIMP_ANY_CLASS(right) != chimp_int_class) {
        return NULL;
    }
    right_value = CHIMP_INT(right)->value;
    return chimp_int_new (left_value * right_value);
}

static ChimpRef *
chimp_int_div (ChimpRef *left, ChimpRef *right)
{
    int64_t left_value, right_value;
    
    left_value = CHIMP_INT(left)->value;
    if (CHIMP_ANY_CLASS(right) != chimp_int_class) {
        return NULL;
    }
    right_value = CHIMP_INT(right)->value;
    return chimp_int_new (left_value / right_value);
}

static ChimpCmpResult
chimp_int_cmp (ChimpRef *left, ChimpRef *right)
{
    ChimpInt *a;
    ChimpInt *b;
    ChimpValueType at, bt;

    at = CHIMP_ANY_TYPE(left);
    bt = CHIMP_ANY_TYPE(right);

    if (at != CHIMP_VALUE_TYPE_INT) {
        return CHIMP_CMP_ERROR;
    }

    if (at != bt) {
        return CHIMP_CMP_NOT_IMPL;
    }

    a = CHIMP_INT(left);
    b = CHIMP_INT(right);

    if (a->value > b->value) {
        return CHIMP_CMP_GT;
    }
    else if (a->value < b->value) {
        return CHIMP_CMP_LT;
    }
    else {
        return CHIMP_CMP_EQ;
    }
}

chimp_bool_t
chimp_int_class_bootstrap (void)
{
    chimp_int_class =
        chimp_class_new (CHIMP_STR_NEW ("int"), NULL);
    if (chimp_int_class == NULL) {
        return CHIMP_FALSE;
    }
    chimp_gc_make_root (NULL, chimp_int_class);
    CHIMP_CLASS(chimp_int_class)->str = chimp_int_str;
    CHIMP_CLASS(chimp_int_class)->add = chimp_int_add;
    CHIMP_CLASS(chimp_int_class)->sub = chimp_int_sub;
    CHIMP_CLASS(chimp_int_class)->mul = chimp_int_mul;
    CHIMP_CLASS(chimp_int_class)->div = chimp_int_div;
    CHIMP_CLASS(chimp_int_class)->cmp = chimp_int_cmp;
    return CHIMP_TRUE;
}

ChimpRef *
chimp_int_new (int64_t value)
{
    ChimpRef *ref = chimp_gc_new_object (NULL);
    if (ref == NULL) {
        return NULL;
    }
    CHIMP_INT_INIT(ref);
    CHIMP_INT(ref)->value = value;
    return ref;
}

