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

#ifndef _NIM_METHOD_H_INCLUDED_
#define _NIM_METHOD_H_INCLUDED_

#include <nim/gc.h>
#include <nim/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _NimMethodType {
    NIM_METHOD_TYPE_NATIVE,
    NIM_METHOD_TYPE_BYTECODE,
    NIM_METHOD_TYPE_CLOSURE
} NimMethodType;

typedef NimRef *(*NimNativeMethodFunc)(NimRef *, NimRef *);

typedef struct _NimMethod {
    NimAny         base;
    NimRef        *self; /* NULL for unbound/function */
    NimMethodType  type;
    NimRef        *module;
    union {
        struct {
            NimNativeMethodFunc func;
        } native;
        struct {
            NimRef *code;
        } bytecode;
        struct {
            NimRef *code;
            NimRef *bindings;
        } closure;
    };
} NimMethod;

nim_bool_t
nim_method_class_bootstrap (void);

NimRef *
nim_method_new_native (NimRef *module, NimNativeMethodFunc func);

NimRef *
nim_method_new_bytecode (NimRef *module, NimRef *code);

NimRef *
nim_method_new_closure (NimRef *method, NimRef *bindings);

NimRef *
nim_method_new_bound (NimRef *unbound, NimRef *self);

nim_bool_t
nim_method_no_args (NimRef *args);

nim_bool_t
nim_method_parse_args (NimRef *args, const char *fmt, ...);

#define NIM_METHOD(ref) NIM_CHECK_CAST(NimMethod, (ref), nim_method_class)
#define NIM_METHOD_SELF(ref) (NIM_METHOD(ref)->self)
#define NIM_METHOD_TYPE(ref) (NIM_METHOD(ref)->type)

#define NIM_NATIVE_METHOD(ref) (&(NIM_METHOD(ref)->native))
#define NIM_BYTECODE_METHOD(ref) (&(NIM_METHOD(ref)->bytecode))
#define NIM_CLOSURE_METHOD(ref) (&(NIM_METHOD(ref)->closure))

NIM_EXTERN_CLASS(method);

#ifdef __cplusplus
};
#endif

#endif

