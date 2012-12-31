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

#include "chimp/frame.h"
#include "chimp/class.h"
#include "chimp/object.h"
#include "chimp/str.h"

#define CHIMP_FRAME_INIT(ref) \
    do { \
        CHIMP_ANY(ref)->type = CHIMP_VALUE_TYPE_FRAME; \
        CHIMP_ANY(ref)->klass = chimp_frame_class; \
    } while (0)

ChimpRef *chimp_frame_class = NULL;

static void
_chimp_frame_mark (ChimpGC *gc, ChimpRef *self)
{
    chimp_gc_mark_ref (gc, CHIMP_FRAME(self)->method);
    chimp_gc_mark_ref (gc, CHIMP_FRAME(self)->locals);
}

chimp_bool_t
chimp_frame_class_bootstrap (void)
{
    chimp_frame_class =
        chimp_class_new (CHIMP_STR_NEW("frame"), chimp_object_class);
    if (chimp_frame_class == NULL) {
        return CHIMP_FALSE;
    }
    CHIMP_CLASS(chimp_frame_class)->mark = _chimp_frame_mark;
    chimp_gc_make_root (NULL, chimp_frame_class);
    return CHIMP_TRUE;
}

ChimpRef *
chimp_frame_new (ChimpRef *method)
{
    ChimpRef *locals;
    ChimpRef *ref = chimp_gc_new_object (NULL);
    if (ref == NULL) {
        return NULL;
    }
    CHIMP_FRAME_INIT(ref);
    locals = chimp_hash_new ();
    if (locals == NULL) {
        return NULL;
    }
    CHIMP_FRAME(ref)->method = method;
    CHIMP_FRAME(ref)->locals = locals;
    if (CHIMP_METHOD_TYPE(method) == CHIMP_METHOD_TYPE_BYTECODE ||
            CHIMP_METHOD_TYPE(method) == CHIMP_METHOD_TYPE_CLOSURE) {
        ChimpRef *code;
        size_t i;

        if (CHIMP_METHOD_TYPE(method) == CHIMP_METHOD_TYPE_BYTECODE) {
            code = CHIMP_METHOD(method)->bytecode.code;
        }
        else {
            code = CHIMP_METHOD(method)->closure.code;
        }

        /* allocate ChimpVar entries in `locals` for each non-free var */
        for (i = 0; i < CHIMP_ARRAY_SIZE(CHIMP_CODE(code)->vars); i++) {
            ChimpRef *varname =
                CHIMP_ARRAY_ITEM(CHIMP_CODE(code)->vars, i);
            ChimpRef *var = chimp_var_new ();
            if (!chimp_hash_put (CHIMP_FRAME(ref)->locals, varname, var)) {
                return CHIMP_FALSE;
            }
        }

        if (CHIMP_METHOD_TYPE(method) == CHIMP_METHOD_TYPE_CLOSURE) {
            ChimpRef *bindings = CHIMP_CLOSURE_METHOD(method)->bindings;
            for (i = 0; i < CHIMP_HASH_SIZE(bindings); i++) {
                ChimpRef *varname = CHIMP_HASH(bindings)->keys[i];
                ChimpRef *value = CHIMP_HASH(bindings)->values[i];
                if (!chimp_hash_put (CHIMP_FRAME(ref)->locals, varname, value)) {
                    return CHIMP_FALSE;
                }
            }
        }
    }
    return ref;
}

