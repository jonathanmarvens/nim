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

#ifndef _NIM_FLOAT_H_INCLUDED_
#define _NIM_FLOAT_H_INCLUDED_

#include <nim/gc.h>
#include <nim/any.h>

#ifdef __cplusplus
extern "C" { 
#endif

typedef struct _NimFloat { 
    NimAny base;
    double value;
} NimFloat;

nim_bool_t
nim_float_class_bootstrap (void);

NimRef *
nim_float_new (double value);

#define NIM_FLOAT(ref) NIM_CHECK_CAST(NimFloat, (ref), nim_float_class)

#define NIM_FLOAT_VALUE(ref) NIM_FLOAT(ref)->value

NIM_EXTERN_CLASS(float);

#ifdef __cplusplus
};
#endif

#endif
