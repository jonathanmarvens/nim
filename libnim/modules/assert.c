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

#include "nim/any.h"
#include "nim/object.h"
#include "nim/array.h"
#include "nim/str.h"
#include "nim/vm.h"

static NimRef *
_nim_assert_equal (NimRef *self, NimRef *args)
{
    NimCmpResult r;
    NimRef *left = NIM_ARRAY_ITEM(args, 0);
    NimRef *right = NIM_ARRAY_ITEM(args, 1);

    r = nim_object_cmp (left, right);

    if (r == NIM_CMP_ERROR) {
        return NULL;
    }
    if (r != NIM_CMP_EQ) {
        fprintf (stderr, "assertion failed: expected %s to not be equal to %s\n",
            NIM_STR_DATA(nim_object_str(left)),
            NIM_STR_DATA(nim_object_str(right)));
        exit(1);
    }
    else {
        return nim_nil;
    }
}

static NimRef *
_nim_assert_not_equal (NimRef *self, NimRef *args)
{
    NimCmpResult r;
    NimRef *left = NIM_ARRAY_ITEM(args, 0);
    NimRef *right = NIM_ARRAY_ITEM(args, 1);

    r = nim_object_cmp (left, right);

    if (r == NIM_CMP_ERROR) {
        return NULL;
    }
    if (r == NIM_CMP_EQ) {
        /* TODO complain */
        return NULL;
    }
    else {
        return nim_nil;
    }
}

NimRef *
_nim_assert_fail (NimRef *self, NimRef *args)
{
    NimRef *msg = NIM_ARRAY_ITEM(args, 0);
    fprintf (stderr, "assertion failed: %s\n",
        NIM_STR_DATA(nim_object_str(msg)));
    exit(1);
}

NimRef *
nim_init_assert_module (void)
{
    NimRef *mod;

    mod = nim_module_new_str ("assert", NULL);
    if (mod == NULL) {
        return NULL;
    }

    if (!nim_module_add_method_str (mod, "eq", _nim_assert_equal)) {
        return NULL;
    }

    if (!nim_module_add_method_str (mod, "neq", _nim_assert_not_equal)) {
        return NULL;
    }

    if (!nim_module_add_method_str (mod, "fail", _nim_assert_fail)) {
        return NULL;
    }

    return mod;
}

