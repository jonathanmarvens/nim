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

#ifndef _CHIMP_ANY_H_INCLUDED_
#define _CHIMP_ANY_H_INCLUDED_

#include <chimp/gc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpAny {
    ChimpRef       *klass;
} ChimpAny;

#define CHIMP_CHECK_CAST(struc, ref, type) \
  ((struc *) chimp_gc_ref_check_cast ((ref), (type)))

#define CHIMP_ANY(ref)    CHIMP_CHECK_CAST(ChimpAny, (ref), NULL)

#define CHIMP_ANY_CLASS(ref) (CHIMP_ANY(ref)->klass)
#define CHIMP_ANY_TYPE(ref) (CHIMP_ANY(ref)->type)

#define CHIMP_EXTERN_CLASS(name) extern struct _ChimpRef *chimp_ ## name ## _class

#ifdef __cplusplus
};
#endif

#endif

