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

#ifndef _NIM_MODULE_H_INCLUDED_
#define _NIM_MODULE_H_INCLUDED_

#include <nim/any.h>
#include <nim/method.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NimModule {
    NimAny base;
    NimRef *name;
    NimRef *locals;
} NimModule;

nim_bool_t
nim_module_class_bootstrap (void);

NimRef *
nim_module_load (NimRef *name, NimRef *path);

NimRef *
nim_module_new (NimRef *name, NimRef *locals);

NimRef *
nim_module_new_str (const char *name, NimRef *locals);

nim_bool_t
nim_module_add_method_str (
    NimRef *self,
    const char *name,
    NimNativeMethodFunc impl
);

nim_bool_t
nim_module_add_local (
    NimRef *self,
    NimRef *name,
    NimRef *value
);

nim_bool_t
nim_module_add_local_str (
    NimRef *self,
    const char *name,
    NimRef *value
);

nim_bool_t
nim_module_add_builtin (NimRef *module);

#define NIM_MODULE(ref) NIM_CHECK_CAST(NimModule, (ref), nim_module_class)
#define NIM_MODULE_NAME(ref) (NIM_MODULE(ref)->name)
#define NIM_MODULE_LOCALS(ref) (NIM_MODULE(ref)->locals)

NIM_EXTERN_CLASS(module);

#ifdef __cplusplus
};
#endif

#endif

