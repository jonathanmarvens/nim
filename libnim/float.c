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

#include "nim/float.h"
#include "nim/str.h"
#include "nim/class.h"
#include "nim/object.h"
#include "nim/ast.h"

NimRef *nim_float_class = NULL;

static NimRef *
_nim_float_init (NimRef *self, NimRef *args)
{
    /* TODO convert str arg to int */
    return self;
}

static NimRef *
nim_float_str (NimRef *self)
{
    char buf[64];
    int len;

    len = snprintf (buf, sizeof(buf), "%f" , NIM_FLOAT(self)->value);

    if (len < 0) {
        return NULL;
    }
    else if (len > sizeof(buf)) {
        NIM_BUG ("nim_float_str output truncated: %" PRId64,
                    NIM_FLOAT(self)->value);
        return NULL;
    }

    return nim_str_new (buf, len);
}

NimRef *
nim_float_nonzero (NimRef *self)
{
    return NIM_BOOL_REF(NIM_FLOAT(self)->value > 0.0);
}

static NimCmpResult
nim_float_cmp (NimRef *left, NimRef *right)
{
    NimFloat *a;
    NimFloat *b;

    if (NIM_ANY_CLASS(left) != nim_float_class) {
        return NIM_CMP_LT;
    }

    if (NIM_ANY_CLASS(left) != NIM_ANY_CLASS(right)) {
        return NIM_CMP_NOT_IMPL;
    }

    a = NIM_FLOAT(left);
    b = NIM_FLOAT(right);

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


nim_bool_t
nim_float_class_bootstrap (void)
{
    nim_float_class = 
        nim_class_new (NIM_STR_NEW ("float"), NULL, sizeof(NimFloat));
    if (nim_float_class == NULL) {
        return NIM_FALSE;
    }
    nim_gc_make_root (NULL, nim_float_class);
    NIM_CLASS(nim_float_class)->init = _nim_float_init;
    NIM_CLASS(nim_float_class)->str = nim_float_str;
    NIM_CLASS(nim_float_class)->add = nim_num_add;
    NIM_CLASS(nim_float_class)->sub = nim_num_sub;
    NIM_CLASS(nim_float_class)->mul = nim_num_mul;
    NIM_CLASS(nim_float_class)->div = nim_num_div;
    NIM_CLASS(nim_float_class)->cmp = nim_float_cmp;
    NIM_CLASS(nim_float_class)->nonzero = nim_float_nonzero;
    return NIM_TRUE;
}

NimRef* 
nim_float_new (double value)
{
    NimRef *ref = nim_class_new_instance (nim_float_class, NULL);
    NIM_FLOAT(ref)->value = value;
    return ref;
}
