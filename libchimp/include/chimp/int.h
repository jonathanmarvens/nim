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

#ifndef _CHIMP_INT_H_INCLUDED_
#define _CHIMP_INT_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpInt {
    ChimpAny base;
    int64_t  value;
} ChimpInt;

chimp_bool_t
chimp_int_class_bootstrap (void);

ChimpRef *
chimp_int_new (int64_t value);

#define CHIMP_INT(ref) CHIMP_CHECK_CAST(ChimpInt, (ref), chimp_int_class)

#define CHIMP_INT_VALUE(ref) CHIMP_INT(ref)->value

CHIMP_EXTERN_CLASS(int);

#ifdef __cplusplus
};
#endif

#endif

