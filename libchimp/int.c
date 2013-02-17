/*****************************************************************************
 *                                                                           *
 * Copyright 2013 Thomas Lee                                                 *
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

#include "chimp/array.h"
#include "chimp/int.h"
#include "chimp/str.h"
#include "chimp/class.h"
#include "chimp/ast.h"

ChimpRef *chimp_int_class = NULL;

static ChimpRef *
_chimp_int_init (ChimpRef *self, ChimpRef *args)
{
    if (CHIMP_ARRAY_SIZE(args) > 0) {
        const char *arg;
        if (!chimp_method_parse_args (args, "s", &arg)) {
            return NULL;
        }
        CHIMP_INT(self)->value = atoll (arg);
    }
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
    CHIMP_CLASS(chimp_int_class)->add = chimp_num_add;
    CHIMP_CLASS(chimp_int_class)->sub = chimp_num_sub;
    CHIMP_CLASS(chimp_int_class)->mul = chimp_num_mul;
    CHIMP_CLASS(chimp_int_class)->div = chimp_num_div;
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

