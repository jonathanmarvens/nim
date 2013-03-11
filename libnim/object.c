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

#include "nim/object.h"
#include "nim/gc.h"
#include "nim/task.h"

NimCmpResult
nim_object_cmp (NimRef *a, NimRef *b)
{
    NimRef *a_class, *b_class;

    /* reference equality is nice & easy */
    if (a == b) {
        return NIM_CMP_EQ;
    }

    a_class = NIM_ANY_CLASS(a);
    b_class = NIM_ANY_CLASS(b);

    if (NIM_CLASS(a_class)->cmp != NULL) {
        return NIM_CLASS(a_class)->cmp (a, b);
    }
    else if (NIM_CLASS(b_class)->cmp != NULL) {
        return NIM_CLASS(b_class)->cmp (a, b);
    }
    else {
        return NIM_CMP_NOT_IMPL;
    }
}

NimRef *
nim_object_str (NimRef *self)
{
    NimRef *klass = NIM_ANY_CLASS(self);

    if (NIM_CLASS(klass)->str != NULL) {
        return NIM_CLASS(klass)->str (self);
    }

    NIM_BUG ("cannot convert type to string. probably a bug.");
    return NULL;
}

NimRef *
nim_object_add (NimRef *left, NimRef *right)
{
    NimRef *klass = NIM_ANY_CLASS(left);

    while (klass != NULL) {
        if (NIM_CLASS(klass)->add != NULL) {
            return NIM_CLASS(klass)->add (left, right);
        }
        klass = NIM_CLASS(klass)->super;
    }

    NIM_BUG ("`%s` type does not support the '+' operator",
            NIM_STR_DATA(NIM_CLASS(NIM_ANY_CLASS(left))->name));
    return NULL;
}

NimRef *
nim_object_sub (NimRef *left, NimRef *right)
{
    NimRef *klass = NIM_ANY_CLASS(left);

    while (klass != NULL) {
        if (NIM_CLASS(klass)->sub != NULL) {
            return NIM_CLASS(klass)->sub (left, right);
        }
        klass = NIM_CLASS(klass)->super;
    }

    NIM_BUG ("type does not support the '-' operator");
    return NULL;
}

NimRef *
nim_object_mul (NimRef *left, NimRef *right)
{
    NimRef *klass = NIM_ANY_CLASS(left);

    while (klass != NULL) {
        if (NIM_CLASS(klass)->mul != NULL) {
            return NIM_CLASS(klass)->mul (left, right);
        }
        klass = NIM_CLASS(klass)->super;
    }

    NIM_BUG ("type does not support the '*' operator");
    return NULL;
}

NimRef *
nim_object_div (NimRef *left, NimRef *right)
{
    NimRef *klass = NIM_ANY_CLASS(left);

    while (klass != NULL) {
        if (NIM_CLASS(klass)->mul != NULL) {
            return NIM_CLASS(klass)->div (left, right);
        }
        klass = NIM_CLASS(klass)->super;
    }

    NIM_BUG ("type does not support the '/' operator");
    return NULL;
}

NimRef *
nim_object_call (NimRef *target, NimRef *args)
{
    NimRef *klass = NIM_ANY_CLASS(target);

    while (klass != NULL) {
        if (NIM_CLASS(klass)->call != NULL) {
            return NIM_CLASS(klass)->call (target, args);
        }
        klass = NIM_CLASS(klass)->super;
    }
    return NULL;
}

NimRef *
nim_object_call_method (NimRef *target, const char *name, NimRef *args)
{
    NimRef *method = nim_object_getattr_str (target, name);
    if (method == NULL) {
        return NULL;
    }
    return nim_object_call (method, args);
}

NimRef *
nim_object_getattr (NimRef *self, NimRef *name)
{
    NimRef *klass = NIM_ANY_CLASS(self);

    while (klass != NULL) {
        if (NIM_CLASS(klass)->getattr != NULL) {
            return NIM_CLASS(klass)->getattr (self, name);
        }
        klass = NIM_CLASS(klass)->super;
    }
    return NULL;
}

NimRef *
nim_object_getitem (NimRef *self, NimRef *key)
{
    NimRef *klass = NIM_ANY_CLASS(self);

    while (klass != NULL) {
        if (NIM_CLASS(klass)->getitem != NULL) {
            return NIM_CLASS(klass)->getitem (self, key);
        }
        klass = NIM_CLASS(klass)->super;
    }
    return NULL;
}

NimRef *
nim_object_getattr_str (NimRef *self, const char *name)
{
    return nim_object_getattr (self, nim_str_new (name, strlen (name)));
}

nim_bool_t
nim_object_instance_of (NimRef *object, NimRef *klass)
{
    NimRef *c = NIM_ANY_CLASS(object);
    while (c != NULL) {
        if (c == klass) {
            return NIM_TRUE;
        }
        c = NIM_CLASS(c)->super;
    }
    return NIM_FALSE;
}