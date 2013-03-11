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

#include "nim/frame.h"
#include "nim/class.h"
#include "nim/object.h"
#include "nim/str.h"

NimRef *nim_frame_class = NULL;

static NimRef *
_nim_frame_init (NimRef *self, NimRef *args)
{
    NimRef *locals;
    NimRef *method;

    if (!nim_method_parse_args (args, "o", &method)) {
        return NULL;
    }

    locals = nim_hash_new ();
    if (locals == NULL) {
        return NULL;
    }
    NIM_FRAME(self)->method = method;
    NIM_FRAME(self)->locals = locals;
    if (NIM_METHOD_TYPE(method) == NIM_METHOD_TYPE_BYTECODE ||
            NIM_METHOD_TYPE(method) == NIM_METHOD_TYPE_CLOSURE) {
        NimRef *code;
        size_t i;

        if (NIM_METHOD_TYPE(method) == NIM_METHOD_TYPE_BYTECODE) {
            code = NIM_METHOD(method)->bytecode.code;
        }
        else {
            code = NIM_METHOD(method)->closure.code;
        }

        /* allocate NimVar entries in `locals` for each non-free var */
        for (i = 0; i < NIM_ARRAY_SIZE(NIM_CODE(code)->vars); i++) {
            NimRef *varname =
                NIM_ARRAY_ITEM(NIM_CODE(code)->vars, i);
            NimRef *var = nim_var_new ();
            if (!nim_hash_put (NIM_FRAME(self)->locals, varname, var)) {
                return NIM_FALSE;
            }
        }

        if (NIM_METHOD_TYPE(method) == NIM_METHOD_TYPE_CLOSURE) {
            NimRef *bindings = NIM_CLOSURE_METHOD(method)->bindings;
            for (i = 0; i < NIM_HASH_SIZE(bindings); i++) {
                NimRef *varname = NIM_HASH(bindings)->keys[i];
                NimRef *value = NIM_HASH(bindings)->values[i];
                if (!nim_hash_put (NIM_FRAME(self)->locals, varname, value)) {
                    return NIM_FALSE;
                }
            }
        }
    }
    return self;
}

static void
_nim_frame_mark (NimGC *gc, NimRef *self)
{
    NIM_SUPER (self)->mark (gc, self);

    nim_gc_mark_ref (gc, NIM_FRAME(self)->method);
    nim_gc_mark_ref (gc, NIM_FRAME(self)->locals);
}

nim_bool_t
nim_frame_class_bootstrap (void)
{
    nim_frame_class = nim_class_new (
        NIM_STR_NEW("frame"), nim_object_class, sizeof(NimFrame));
    if (nim_frame_class == NULL) {
        return NIM_FALSE;
    }
    NIM_CLASS(nim_frame_class)->init = _nim_frame_init;
    NIM_CLASS(nim_frame_class)->mark = _nim_frame_mark;
    nim_gc_make_root (NULL, nim_frame_class);
    return NIM_TRUE;
}

NimRef *
nim_frame_new (NimRef *method)
{
    return nim_class_new_instance (nim_frame_class, method, NULL);
}

