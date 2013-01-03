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

#include "chimp/method.h"
#include "chimp/class.h"
#include "chimp/array.h"
#include "chimp/str.h"
#include "chimp/int.h"
#include "chimp/task.h"
#include "chimp/vm.h"

ChimpRef *chimp_method_class = NULL;

static ChimpRef *
chimp_method_call (ChimpRef *self, ChimpRef *args)
{
    if (CHIMP_METHOD_TYPE(self) == CHIMP_METHOD_TYPE_NATIVE) {
        return CHIMP_NATIVE_METHOD(self)->func (CHIMP_METHOD(self)->self, args);
    }
    else {
        return chimp_vm_invoke (NULL, self, args);
    }
}

static void
_chimp_method_mark (ChimpGC *gc, ChimpRef *self)
{
    CHIMP_SUPER (self)->mark (gc, self);

    chimp_gc_mark_ref (gc, CHIMP_METHOD(self)->self);
    if (CHIMP_METHOD_TYPE(self) == CHIMP_METHOD_TYPE_BYTECODE) {
        chimp_gc_mark_ref (gc, CHIMP_METHOD(self)->bytecode.code);
    }
    else if (CHIMP_METHOD_TYPE(self) == CHIMP_METHOD_TYPE_CLOSURE) {
        chimp_gc_mark_ref (gc, CHIMP_METHOD(self)->closure.code);
        chimp_gc_mark_ref (gc, CHIMP_METHOD(self)->closure.bindings);
    }
    chimp_gc_mark_ref (gc, CHIMP_METHOD(self)->module);
}

chimp_bool_t
chimp_method_class_bootstrap (void)
{
    chimp_method_class =
        chimp_class_new (CHIMP_STR_NEW("method"), NULL, sizeof(ChimpMethod));
    if (chimp_method_class == NULL) {
        return CHIMP_FALSE;
    }
    chimp_gc_make_root (NULL, chimp_method_class);
    CHIMP_CLASS(chimp_method_class)->call = chimp_method_call;
    CHIMP_CLASS(chimp_method_class)->mark = _chimp_method_mark;
    return CHIMP_TRUE;
}

ChimpRef *
chimp_method_new_native (ChimpRef *module, ChimpNativeMethodFunc func)
{
    ChimpRef *ref = chimp_class_new_instance (chimp_method_class, NULL);
    CHIMP_METHOD(ref)->type = CHIMP_METHOD_TYPE_NATIVE;
    CHIMP_METHOD(ref)->module = module;
    CHIMP_NATIVE_METHOD(ref)->func = func;
    return ref;
}

ChimpRef *
chimp_method_new_bytecode (ChimpRef *module, ChimpRef *code)
{
    ChimpRef *ref = chimp_class_new_instance (chimp_method_class, NULL);
    CHIMP_ANY(ref)->klass = chimp_method_class;
    CHIMP_METHOD(ref)->type = CHIMP_METHOD_TYPE_BYTECODE;
    CHIMP_METHOD(ref)->module = module;
    CHIMP_BYTECODE_METHOD(ref)->code = code;
    return ref;
}

ChimpRef *
chimp_method_new_closure (ChimpRef *method, ChimpRef *bindings)
{
    ChimpRef *ref;
    if (CHIMP_METHOD(method)->type != CHIMP_METHOD_TYPE_BYTECODE) {
        CHIMP_BUG("closures must be created from bytecode methods");
        return NULL;
    }
    ref = chimp_class_new_instance (chimp_method_class, NULL);
    CHIMP_METHOD(ref)->type = CHIMP_METHOD_TYPE_CLOSURE;
    CHIMP_METHOD(ref)->module = CHIMP_METHOD(method)->module;
    CHIMP_CLOSURE_METHOD(ref)->code = CHIMP_BYTECODE_METHOD(method)->code;
    CHIMP_CLOSURE_METHOD(ref)->bindings = bindings;
    /* TODO check for incomplete bindings hash? */
    return ref;
}

ChimpRef *
chimp_method_new_bound (ChimpRef *unbound, ChimpRef *self)
{
    /* TODO ensure unbound is actually ... er ... unbound */
    ChimpRef *ref = chimp_gc_new_object (NULL);
    if (ref == NULL) {
        return NULL;
    }
    CHIMP_ANY(ref)->klass = chimp_method_class;
    CHIMP_METHOD(ref)->type = CHIMP_METHOD(unbound)->type;
    CHIMP_METHOD(ref)->self = self;
    CHIMP_METHOD(ref)->module = CHIMP_METHOD(unbound)->module;
    if (CHIMP_METHOD(ref)->type == CHIMP_METHOD_TYPE_NATIVE) {
        CHIMP_NATIVE_METHOD(ref)->func = CHIMP_NATIVE_METHOD(unbound)->func;
    }
    else {
        CHIMP_BYTECODE_METHOD(ref)->code =
            CHIMP_BYTECODE_METHOD(unbound)->code;
    }
    return ref;
}

chimp_bool_t
chimp_method_no_args (ChimpRef *args)
{
    if (CHIMP_ARRAY_SIZE(args) > 0) {
        CHIMP_BUG ("method takes no arguments");
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

chimp_bool_t
chimp_method_parse_args (ChimpRef *args, const char *fmt, ...)
{
    size_t i;
    va_list argp;
    const size_t len = strlen (fmt);
    size_t n = 0;
    chimp_bool_t optional = CHIMP_FALSE;
    const size_t nmax = CHIMP_ARRAY_SIZE(args);

    va_start (argp, fmt);
    for (i = 0; i < len; i++) {
        if (n >= nmax) {
            /* XXX the second part of this test is a huge hack */
            if (!optional && fmt[0] != '|') {
                CHIMP_BUG ("not enough arguments");
                goto error;
            }
            else {
                return CHIMP_TRUE;
            }
        }
        switch (fmt[i]) {
            case 'o':
                {
                    ChimpRef **arg = va_arg (argp, ChimpRef **);
                    *arg = CHIMP_ARRAY_ITEM(args, n++);
                    break;
                }
            case 's':
                {
                    char **arg = va_arg (argp, char **);
                    /* TODO ensure array item is a str object */
                    *arg = CHIMP_STR_DATA(CHIMP_ARRAY_ITEM(args, n++));
                    break;
                }
            case 'i':
                {
                    int32_t *arg = va_arg (argp, int32_t *);
                    /* TODO ensure array item is an int object */
                    *arg = (int32_t) CHIMP_INT(CHIMP_ARRAY_ITEM(args, n++))->value;
                    break;
                }
            case 'I':
                {
                    int64_t *arg = va_arg (argp, int64_t *);
                    *arg = (int64_t) CHIMP_INT(CHIMP_ARRAY_ITEM(args, n++))->value;
                    break;
                }
            case '|':
                {
                    optional = CHIMP_TRUE;
                    break;
                }
            default:
                CHIMP_BUG ("unknown format character: %c", fmt[i]);
                goto error;
        };
    }
    va_end (argp);

    if (n < nmax && !optional) {
        CHIMP_BUG ("too many arguments");
        return CHIMP_FALSE;
    }

    return CHIMP_TRUE;

error:
    va_end (argp);
    return CHIMP_FALSE;
}

