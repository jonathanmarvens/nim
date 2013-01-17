/*****************************************************************************
 *                                                                           *
 * Copyright 2012 Thomas Lee                                                 *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License");           *
 * you may not use this file except in compliance with the License.          *
 * You may obtain a copy of the License at                                   *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 * Unless required by applicable law or agreed to in writing, software       *
 * distributed under the License is distributed on an "AS IS" BASIS,         *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 * See the License for the specific language governing permissions and       *
 * limitations under the License.                                            *
 *                                                                           *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "chimp/int.h"
#include "chimp/str.h"
#include "chimp/class.h"
#include "chimp/ast.h"

ChimpRef *chimp_int_class = NULL;

static ChimpRef *
_chimp_int_init (ChimpRef *self, ChimpRef *args)
{
    /* TODO convert str arg to int */
    return self;
}

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
        CHIMP_BUG ("chimp_int_str output truncated: %" PRId64,
                    CHIMP_INT(self)->value);
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

    if (CHIMP_ANY_CLASS(left) != chimp_int_class) {
        return CHIMP_CMP_LT;
    }

    if (CHIMP_ANY_CLASS(left) != CHIMP_ANY_CLASS(right)) {
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
        chimp_class_new (CHIMP_STR_NEW ("int"), NULL, sizeof(ChimpInt));
    if (chimp_int_class == NULL) {
        return CHIMP_FALSE;
    }
    chimp_gc_make_root (NULL, chimp_int_class);
    CHIMP_CLASS(chimp_int_class)->init = _chimp_int_init;
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
    ChimpRef *ref = chimp_class_new_instance (chimp_int_class, NULL);
    CHIMP_INT(ref)->value = value;
    return ref;
}

