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

#include "nim/array.h"
#include "nim/int.h"
#include "nim/str.h"
#include "nim/class.h"
#include "nim/object.h"
#include "nim/ast.h"

NimRef *nim_int_class = NULL;

static NimRef *
_nim_int_init (NimRef *self, NimRef *args)
{
    if (NIM_ARRAY_SIZE(args) > 0) {
        const char *arg;
        if (!nim_method_parse_args (args, "s", &arg)) {
            return NULL;
        }
        NIM_INT(self)->value = atoll (arg);
    }
    return self;
}

static NimRef *
nim_int_str (NimRef *self)
{
    char buf[64];
    int len;

    len = snprintf (buf, sizeof(buf), "%" PRId64, NIM_INT(self)->value);

    if (len < 0) {
        return NULL;
    }
    else if (len > sizeof(buf)) {
        NIM_BUG ("nim_int_str output truncated: %" PRId64,
                    NIM_INT(self)->value);
        return NULL;
    }

    return nim_str_new (buf, len);
}

static NimCmpResult
nim_int_cmp (NimRef *left, NimRef *right)
{
    NimInt *a;
    NimInt *b;

    if (NIM_ANY_CLASS(left) != nim_int_class) {
        return NIM_CMP_LT;
    }

    if (NIM_ANY_CLASS(left) != NIM_ANY_CLASS(right)) {
        return NIM_CMP_NOT_IMPL;
    }

    a = NIM_INT(left);
    b = NIM_INT(right);

    if (a->value > b->value) {
        return NIM_CMP_GT;
    }
    else if (a->value < b->value) {
        return NIM_CMP_LT;
    }
    else {
        return NIM_CMP_EQ;
    }
}

static NimRef *
_nim_int_nonzero (NimRef *self)
{
    return NIM_BOOL_REF(NIM_INT_VALUE(self) > 0);
}

nim_bool_t
nim_int_class_bootstrap (void)
{
    nim_int_class =
        nim_class_new (NIM_STR_NEW ("int"), NULL, sizeof(NimInt));
    if (nim_int_class == NULL) {
        return NIM_FALSE;
    }
    nim_gc_make_root (NULL, nim_int_class);
    NIM_CLASS(nim_int_class)->init = _nim_int_init;
    NIM_CLASS(nim_int_class)->str = nim_int_str;
    NIM_CLASS(nim_int_class)->add = nim_num_add;
    NIM_CLASS(nim_int_class)->sub = nim_num_sub;
    NIM_CLASS(nim_int_class)->mul = nim_num_mul;
    NIM_CLASS(nim_int_class)->div = nim_num_div;
    NIM_CLASS(nim_int_class)->cmp = nim_int_cmp;
    NIM_CLASS(nim_int_class)->nonzero = _nim_int_nonzero;
    return NIM_TRUE;
}

NimRef *
nim_int_new (int64_t value)
{
    NimRef *ref = nim_class_new_instance (nim_int_class, NULL);
    NIM_INT(ref)->value = value;
    return ref;
}

