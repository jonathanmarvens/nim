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

#ifndef _NIM_CLASS_H_INCLUDED_
#define _NIM_CLASS_H_INCLUDED_

#include <nim/gc.h>
#include <nim/any.h>
#include <nim/method.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _NimCmpResult {
    NIM_CMP_ERROR    = -3,
    NIM_CMP_NOT_IMPL = -2,
    NIM_CMP_LT       = -1,
    NIM_CMP_EQ       = 0,
    NIM_CMP_GT       = 1
} NimCmpResult;

typedef struct _NimClass {
    NimAny  base;
    NimRef *name;
    NimRef *super;
    NimCmpResult (*cmp)(NimRef *, NimRef *);
    NimRef *(*init)(NimRef *, NimRef *);
    void (*dtor)(NimRef *);
    NimRef *(*str)(NimRef *);
    void (*mark)(struct _NimGC *gc, NimRef *);
    NimRef *(*add)(NimRef *, NimRef *);
    NimRef *(*sub)(NimRef *, NimRef *);
    NimRef *(*mul)(NimRef *, NimRef *);
    NimRef *(*div)(NimRef *, NimRef *);
    NimRef *(*call)(NimRef *, NimRef *);
    NimRef *(*getattr)(NimRef *, NimRef *);
    NimRef *(*getitem)(NimRef *, NimRef *);
    NimRef *(*nonzero)(NimRef *);
    struct _NimLWHash *methods;
} NimClass;

NimRef *
nim_class_new (NimRef *name, NimRef *super, size_t size);

NimRef *
nim_class_new_instance (NimRef *klass, ...);

nim_bool_t
nim_class_add_method (NimRef *klass, NimRef *name, NimRef *method);

nim_bool_t
nim_class_add_native_method (NimRef *klass, const char *name, NimNativeMethodFunc func);

nim_bool_t
_nim_bootstrap_L3 (void);

#define NIM_CLASS(ref)  NIM_CHECK_CAST(NimClass, (ref), nim_class_class)

#define NIM_CLASS_NAME(ref) (NIM_CLASS(ref)->name)
#define NIM_CLASS_SUPER(ref) (NIM_CLASS(ref)->super)

#define NIM_SUPER(ref) NIM_CLASS(NIM_CLASS_SUPER(NIM_ANY_CLASS(ref)))

NIM_EXTERN_CLASS(class);

#ifdef __cplusplus
};
#endif

#endif

