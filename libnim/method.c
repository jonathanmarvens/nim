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

#include <inttypes.h>

#include "nim/method.h"
#include "nim/class.h"
#include "nim/array.h"
#include "nim/str.h"
#include "nim/int.h"
#include "nim/task.h"
#include "nim/vm.h"

NimRef *nim_method_class = NULL;

static NimRef *
nim_method_call (NimRef *self, NimRef *args)
{
    if (NIM_METHOD_TYPE(self) == NIM_METHOD_TYPE_NATIVE) {
        return NIM_NATIVE_METHOD(self)->func (NIM_METHOD(self)->self, args);
    }
    else {
        return nim_vm_invoke (NULL, self, args);
    }
}

static void
_nim_method_mark (NimGC *gc, NimRef *self)
{
    NIM_SUPER (self)->mark (gc, self);

    nim_gc_mark_ref (gc, NIM_METHOD(self)->self);
    if (NIM_METHOD_TYPE(self) == NIM_METHOD_TYPE_BYTECODE) {
        nim_gc_mark_ref (gc, NIM_METHOD(self)->bytecode.code);
    }
    else if (NIM_METHOD_TYPE(self) == NIM_METHOD_TYPE_CLOSURE) {
        nim_gc_mark_ref (gc, NIM_METHOD(self)->closure.code);
        nim_gc_mark_ref (gc, NIM_METHOD(self)->closure.bindings);
    }
    nim_gc_mark_ref (gc, NIM_METHOD(self)->module);
}

nim_bool_t
nim_method_class_bootstrap (void)
{
    nim_method_class =
        nim_class_new (NIM_STR_NEW("method"), NULL, sizeof(NimMethod));
    if (nim_method_class == NULL) {
        return NIM_FALSE;
    }
    nim_gc_make_root (NULL, nim_method_class);
    NIM_CLASS(nim_method_class)->call = nim_method_call;
    NIM_CLASS(nim_method_class)->mark = _nim_method_mark;
    return NIM_TRUE;
}

NimRef *
nim_method_new_native (NimRef *module, NimNativeMethodFunc func)
{
    NimRef *ref = nim_class_new_instance (nim_method_class, NULL);
    NIM_METHOD(ref)->type = NIM_METHOD_TYPE_NATIVE;
    NIM_METHOD(ref)->module = module;
    NIM_NATIVE_METHOD(ref)->func = func;
    return ref;
}

NimRef *
nim_method_new_bytecode (NimRef *module, NimRef *code)
{
    NimRef *ref = nim_class_new_instance (nim_method_class, NULL);
    NIM_ANY(ref)->klass = nim_method_class;
    NIM_METHOD(ref)->type = NIM_METHOD_TYPE_BYTECODE;
    NIM_METHOD(ref)->module = module;
    NIM_BYTECODE_METHOD(ref)->code = code;
    return ref;
}

NimRef *
nim_method_new_closure (NimRef *method, NimRef *bindings)
{
    NimRef *ref;
    if (NIM_METHOD(method)->type != NIM_METHOD_TYPE_BYTECODE) {
        NIM_BUG("closures must be created from bytecode methods");
        return NULL;
    }
    ref = nim_class_new_instance (nim_method_class, NULL);
    NIM_METHOD(ref)->type = NIM_METHOD_TYPE_CLOSURE;
    NIM_METHOD(ref)->module = NIM_METHOD(method)->module;
    NIM_CLOSURE_METHOD(ref)->code = NIM_BYTECODE_METHOD(method)->code;
    NIM_CLOSURE_METHOD(ref)->bindings = bindings;
    /* TODO check for incomplete bindings hash? */
    return ref;
}

NimRef *
nim_method_new_bound (NimRef *unbound, NimRef *self)
{
    /* TODO ensure unbound is actually ... er ... unbound */
    NimRef *ref = nim_gc_new_object (NULL);
    if (ref == NULL) {
        return NULL;
    }
    NIM_ANY(ref)->klass = nim_method_class;
    NIM_METHOD(ref)->type = NIM_METHOD(unbound)->type;
    NIM_METHOD(ref)->self = self;
    NIM_METHOD(ref)->module = NIM_METHOD(unbound)->module;
    if (NIM_METHOD(ref)->type == NIM_METHOD_TYPE_NATIVE) {
        NIM_NATIVE_METHOD(ref)->func = NIM_NATIVE_METHOD(unbound)->func;
    }
    else {
        NIM_BYTECODE_METHOD(ref)->code =
            NIM_BYTECODE_METHOD(unbound)->code;
    }
    return ref;
}

nim_bool_t
nim_method_no_args (NimRef *args)
{
    if (NIM_ARRAY_SIZE(args) > 0) {
        NIM_BUG ("method takes no arguments");
        return NIM_FALSE;
    }
    return NIM_TRUE;
}

nim_bool_t
nim_method_parse_args (NimRef *args, const char *fmt, ...)
{
    size_t i;
    va_list argp;
    const size_t len = strlen (fmt);
    size_t n = 0;
    nim_bool_t optional = NIM_FALSE;
    const size_t nmax = NIM_ARRAY_SIZE(args);

    va_start (argp, fmt);
    for (i = 0; i < len; i++) {
        if (n >= nmax) {
            /* XXX the second part of this test is a huge hack */
            if (!optional && fmt[i] != '|') {
                NIM_BUG ("not enough arguments");
                goto error;
            }
            else {
                return NIM_TRUE;
            }
        }
        switch (fmt[i]) {
            case 'o':
                {
                    NimRef **arg = va_arg (argp, NimRef **);
                    *arg = NIM_ARRAY_ITEM(args, n++);
                    break;
                }
            case 's':
                {
                    char **arg = va_arg (argp, char **);
                    /* TODO ensure array item is a str object */
                    *arg = NIM_STR_DATA(NIM_ARRAY_ITEM(args, n++));
                    break;
                }
            case 'i':
                {
                    int32_t *arg = va_arg (argp, int32_t *);
                    /* TODO ensure array item is an int object */
                    *arg = (int32_t) NIM_INT(NIM_ARRAY_ITEM(args, n++))->value;
                    break;
                }
            case 'I':
                {
                    int64_t *arg = va_arg (argp, int64_t *);
                    *arg = (int64_t) NIM_INT(NIM_ARRAY_ITEM(args, n++))->value;
                    break;
                }
            case '|':
                {
                    optional = NIM_TRUE;
                    break;
                }
            default:
                NIM_BUG ("unknown format character: %c", fmt[i]);
                goto error;
        };
    }
    va_end (argp);

    if (n < nmax && !optional) {
        NIM_BUG ("too many arguments");
        return NIM_FALSE;
    }

    return NIM_TRUE;

error:
    va_end (argp);
    return NIM_FALSE;
}

