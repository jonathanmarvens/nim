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

#include "nim/gc.h"
#include "nim/object.h"
#include "nim/class.h"
#include "nim/lwhash.h"
#include "nim/str.h"
#include "nim/method.h"

static NimRef *
nim_class_call (NimRef *self, NimRef *args)
{
    NimRef *ctor;
    NimRef *ref;
    
    /* XXX having two code paths here is probably wrong/begging for trouble */
    if (self == nim_class_class) {
        ref = NIM_ANY_CLASS(NIM_ARRAY_ITEM(args, 0));
    }
    else {
        ref = nim_gc_new_object (NULL);
        if (ref == NULL) {
            return NULL;
        }
        NIM_ANY(ref)->klass = self;
        if (NIM_CLASS(self)->init != NULL) {
            ref = NIM_CLASS(self)->init (ref, args);
            if (ref == NULL) {
                return NULL;
            }
        }
        else {
            ctor = nim_object_getattr_str (ref, "init");
            if (ctor != NULL && ctor != nim_nil) {
                NimRef *newref;
                newref = nim_object_call (ctor, args);
                if (newref == NULL) {
                    return NULL;
                }
            }
        }
    }
    return ref;
}

static const char empty[] = "";

static NimRef *
nim_str_init (NimRef *self, NimRef *args)
{
    /* TODO don't copy string data on every call ... */
    if (NIM_ARRAY_SIZE(args) == 0) {
        NIM_STR(self)->data = strdup (empty);
        NIM_STR(self)->size = 0;
    }
    else if (NIM_ARRAY_SIZE(args) == 1) {
        NimRef *temp = NIM_ARRAY_FIRST(args);
        /* optimization for str("some string") */
        if (NIM_ANY_CLASS(temp) == nim_str_class) {
            return temp;
        }
        temp = nim_object_str (NIM_ARRAY_FIRST(args));
        NIM_STR(self)->data = strdup (NIM_STR_DATA(temp));
        NIM_STR(self)->size = strlen (NIM_STR_DATA(self));
    }
    else {
        size_t i;
        size_t len;
        char *p;
        NimRef *str_values = nim_array_new ();
        if (str_values == NULL) {
            return NULL;
        }

        /* 1. convert all constructor args to strings */
        for (i = 0; i < NIM_ARRAY_SIZE(args); i++) {
            NimRef *str = NIM_ARRAY_ITEM(args, i);
            if (NIM_ANY_CLASS(str) != nim_str_class) {
                str = nim_object_str (NIM_ARRAY_ITEM(args, i));
            }
            if (str == NULL) {
                return NULL;
            }
            if (!nim_array_push (str_values, str)) {
                return NULL;
            }
        }

        /* 2. compute total length of the arg strings */
        len = 0;
        for (i = 0; i < NIM_ARRAY_SIZE(str_values); i++) {
            NimRef *str = NIM_ARRAY_ITEM(str_values, i);
            if (str == NULL) {
                return NULL;
            }
            len += NIM_STR_SIZE(str);
        }

        /* 3. allocate buffer & copy arg strings into it */
        NIM_STR(self)->data = NIM_MALLOC (char, len + 1);
        if (NIM_STR(self)->data == NULL) {
            return NULL;
        }
        NIM_STR(self)->size = len;
        p = NIM_STR(self)->data;
        for (i = 0; i < NIM_ARRAY_SIZE(str_values); i++) {
            NimRef *str = NIM_ARRAY_ITEM(str_values, i);
            if (str == NULL) {
                return NULL;
            }
            memcpy (p, NIM_STR_DATA(str), NIM_STR_SIZE(str));
            p += NIM_STR_SIZE(str);
        }
        *p = '\0';
    }
    return self;
}

static void
nim_str_dtor (NimRef *self)
{
    NIM_FREE(NIM_STR(self)->data);
}

NimRef *
nim_class_new (NimRef *name, NimRef *super, size_t size)
{
    NimRef *ref;
    
    if (size > NIM_VALUE_SIZE) {
        NIM_BUG ("objects cannot be more than %zu bytes (%s is too big)",
            NIM_VALUE_SIZE, NIM_STR_DATA(name));
        return NULL;
    }

    ref = nim_gc_new_object (NULL);
    if (ref == NULL) {
        return NULL;
    }
    if (super == NULL || super == nim_nil) {
        super = nim_object_class;
    }
    NIM_ANY(ref)->klass = nim_class_class;
    NIM_CLASS(ref)->name = name;
    NIM_CLASS(ref)->super = super;
    NIM_CLASS(ref)->methods = nim_lwhash_new ();
    NIM_CLASS(ref)->call = nim_class_call;
    /* TODO wrap these in a struct & use memcpy */
    NIM_CLASS(ref)->init = NIM_CLASS(super)->init;
    NIM_CLASS(ref)->dtor = NIM_CLASS(super)->dtor;
    NIM_CLASS(ref)->str = NIM_CLASS(super)->str;
    NIM_CLASS(ref)->mark = NIM_CLASS(super)->mark;
    NIM_CLASS(ref)->add = NIM_CLASS(super)->add;
    NIM_CLASS(ref)->sub = NIM_CLASS(super)->sub;
    NIM_CLASS(ref)->mul = NIM_CLASS(super)->mul;
    NIM_CLASS(ref)->div = NIM_CLASS(super)->div;
    NIM_CLASS(ref)->getattr = NIM_CLASS(super)->getattr;
    NIM_CLASS(ref)->getitem = NIM_CLASS(super)->getitem;
    NIM_CLASS(ref)->nonzero = NIM_CLASS(super)->nonzero;
    return ref;
}

NimRef *
nim_class_new_instance (NimRef *klass, ...)
{
    va_list args;
    NimRef *arg;
    size_t n;
    NimRef *arr;
    
    va_start (args, klass);
    n = 0;
    while ((arg = va_arg(args, NimRef *)) != NULL) {
        n++;
    }
    va_end (args);

    arr = nim_array_new_with_capacity (n);
    if (arr == NULL) {
        return NULL;
    }

    va_start (args, klass);
    while ((arg = va_arg(args, NimRef *)) != NULL) {
        if (!nim_array_push (arr, arg)) {
            va_end (args);
            return NULL;
        }
    }
    va_end (args);

    return nim_class_call (klass, arr);
}

nim_bool_t
nim_class_add_method (NimRef *self, NimRef *name, NimRef *method)
{
    return nim_lwhash_put (NIM_CLASS(self)->methods, name, method);
}

nim_bool_t
nim_class_add_native_method (NimRef *self, const char *name, NimNativeMethodFunc func)
{
    NimRef *method_ref;
    NimRef *name_ref = nim_str_new (name, strlen (name));
    if (name_ref == NULL) {
        return NIM_FALSE;
    }
    /* XXX the module should strictly be the module in which this class is declared */
    method_ref = nim_method_new_native (NULL, func);
    if (method_ref == NULL) {
        return NIM_FALSE;
    }
    return nim_class_add_method (self, name_ref, method_ref);
}

static void
nim_class_dtor (NimRef *self)
{
    nim_lwhash_delete (NIM_CLASS(self)->methods);
}

static NimRef *
nim_class_str (NimRef *self)
{
    return nim_str_new_concat (
        "<class '", NIM_STR_DATA(NIM_CLASS(self)->name), "'>", NULL);
}

nim_bool_t
_nim_bootstrap_L3 (void)
{
    NIM_CLASS(nim_class_class)->methods = nim_lwhash_new ();
    NIM_CLASS(nim_class_class)->call = nim_class_call;
    NIM_CLASS(nim_class_class)->dtor = nim_class_dtor;
    NIM_CLASS(nim_class_class)->str = nim_class_str;

    NIM_CLASS(nim_object_class)->methods = nim_lwhash_new ();
    NIM_CLASS(nim_object_class)->call = nim_class_call;

    NIM_CLASS(nim_str_class)->methods = nim_lwhash_new ();
    NIM_CLASS(nim_str_class)->call = nim_class_call;
    NIM_CLASS(nim_str_class)->init = nim_str_init;
    NIM_CLASS(nim_str_class)->dtor = nim_str_dtor;

    return NIM_TRUE;
}

