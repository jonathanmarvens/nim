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

#ifndef _NIM_VAR_H_INCLUDED_
#define _NIM_VAR_H_INCLUDED_

#include <nim/gc.h>
#include <nim/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NimVar {
    NimAny  base;
    NimRef *value;
} NimVar;

nim_bool_t
nim_var_class_bootstrap (void);

NimRef *
nim_var_new (void);

#define NIM_VAR(ref)  NIM_CHECK_CAST(NimVar, (ref), nim_var_class)
#define NIM_VAR_VALUE(ref) NIM_VAR(ref)->value

NIM_EXTERN_CLASS(var);

#ifdef __cplusplus
};
#endif

#endif

