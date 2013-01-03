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

#ifndef _CHIMP_CLASS_H_INCLUDED_
#define _CHIMP_CLASS_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/any.h>
#include <chimp/method.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _ChimpCmpResult {
    CHIMP_CMP_ERROR    = -3,
    CHIMP_CMP_NOT_IMPL = -2,
    CHIMP_CMP_LT       = -1,
    CHIMP_CMP_EQ       = 0,
    CHIMP_CMP_GT       = 1
} ChimpCmpResult;

typedef struct _ChimpClass {
    ChimpAny  base;
    ChimpRef *name;
    ChimpRef *super;
    ChimpCmpResult (*cmp)(ChimpRef *, ChimpRef *);
    ChimpRef *(*init)(ChimpRef *, ChimpRef *);
    void (*dtor)(ChimpRef *);
    ChimpRef *(*str)(ChimpRef *);
    void (*mark)(struct _ChimpGC *gc, ChimpRef *);
    ChimpRef *(*add)(ChimpRef *, ChimpRef *);
    ChimpRef *(*sub)(ChimpRef *, ChimpRef *);
    ChimpRef *(*mul)(ChimpRef *, ChimpRef *);
    ChimpRef *(*div)(ChimpRef *, ChimpRef *);
    ChimpRef *(*call)(ChimpRef *, ChimpRef *);
    ChimpRef *(*getattr)(ChimpRef *, ChimpRef *);
    ChimpRef *(*getitem)(ChimpRef *, ChimpRef *);
    struct _ChimpLWHash *methods;
} ChimpClass;

ChimpRef *
chimp_class_new (ChimpRef *name, ChimpRef *super, size_t size);

ChimpRef *
chimp_class_new_instance (ChimpRef *klass, ...);

chimp_bool_t
chimp_class_add_method (ChimpRef *klass, ChimpRef *name, ChimpRef *method);

chimp_bool_t
chimp_class_add_native_method (ChimpRef *klass, const char *name, ChimpNativeMethodFunc func);

chimp_bool_t
_chimp_bootstrap_L3 (void);

#define CHIMP_CLASS(ref)  CHIMP_CHECK_CAST(ChimpClass, (ref), chimp_class_class)

#define CHIMP_CLASS_NAME(ref) (CHIMP_CLASS(ref)->name)
#define CHIMP_CLASS_SUPER(ref) (CHIMP_CLASS(ref)->super)

#define CHIMP_SUPER(ref) CHIMP_CLASS(CHIMP_CLASS_SUPER(CHIMP_ANY_CLASS(ref)))

CHIMP_EXTERN_CLASS(class);

#ifdef __cplusplus
};
#endif

#endif

