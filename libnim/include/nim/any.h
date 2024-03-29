/*****************************************************************************
 *                                                                           *
 * Copyright 2013 Thomas Lee                                                 *
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

#ifndef _NIM_ANY_H_INCLUDED_
#define _NIM_ANY_H_INCLUDED_

#include <nim/gc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NimAny {
    NimRef       *klass;
} NimAny;

#define NIM_CHECK_CAST(struc, ref, type) \
  ((struc *) nim_gc_ref_check_cast ((ref), (type)))

#define NIM_ANY(ref)    NIM_CHECK_CAST(NimAny, (ref), NULL)

#define NIM_ANY_CLASS(ref) (NIM_ANY(ref)->klass)
#define NIM_ANY_TYPE(ref) (NIM_ANY(ref)->type)

#define NIM_EXTERN_CLASS(name) extern struct _NimRef *nim_ ## name ## _class

#ifdef __cplusplus
};
#endif

#endif

