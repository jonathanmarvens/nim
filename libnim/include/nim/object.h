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

#ifndef _NIM_OBJECT_H_INCLUDED_
#define _NIM_OBJECT_H_INCLUDED_

#include <nim/core.h>
#include <nim/gc.h>
#include <nim/class.h>
#include <nim/str.h>
#include <nim/int.h>
#include <nim/array.h>
#include <nim/frame.h>
#include <nim/code.h>
#include <nim/method.h>
#include <nim/hash.h>
#include <nim/ast.h>
#include <nim/module.h>
#include <nim/symtable.h>
#include <nim/msg.h>
#include <nim/task.h>
#include <nim/var.h>
#include <nim/error.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NimObject {
    NimAny     base;
} NimObject;

NimCmpResult
nim_object_cmp (NimRef *a, NimRef *b);

NimRef *
nim_object_str (NimRef *self);

NimRef *
nim_object_add (NimRef *left, NimRef *right);

NimRef *
nim_object_sub (NimRef *left, NimRef *right);

NimRef *
nim_object_mul (NimRef *left, NimRef *right);

NimRef *
nim_object_div (NimRef *left, NimRef *right);

NimRef *
nim_object_call (NimRef *target, NimRef *args);

NimRef *
nim_object_call_method (NimRef *target, const char *name, NimRef *args);

NimRef *
nim_object_getattr (NimRef *self, NimRef *name);

NimRef *
nim_object_getitem (NimRef *self, NimRef *key);

NimRef *
nim_object_getattr_str (NimRef *self, const char *name);

nim_bool_t
nim_object_instance_of (NimRef *object, NimRef *klass);

#define NIM_OBJECT(ref) NIM_CHECK_CAST(NimObject, (ref), nim_object_class)

NIM_EXTERN_CLASS(object);
NIM_EXTERN_CLASS(bool);
NIM_EXTERN_CLASS(nil);

extern struct _NimRef *nim_nil;
extern struct _NimRef *nim_true;
extern struct _NimRef *nim_false;
extern struct _NimRef *nim_builtins;

#define NIM_BOOL_REF(b) ((b) ? nim_true : nim_false)

#define NIM_OBJECT_STR(ref) NIM_STR_DATA(nim_object_str (NULL, (ref)))

#ifdef __cplusplus
};
#endif

#endif

