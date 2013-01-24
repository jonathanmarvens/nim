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

#ifndef _CHIMP_ARRAY_H_INCLUDED_
#define _CHIMP_ARRAY_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpArray {
    ChimpAny   base;
    ChimpRef **items;
    size_t     size;
    size_t     capacity;
} ChimpArray;

chimp_bool_t
chimp_array_class_bootstrap (void);

ChimpRef *
chimp_array_new (void);

ChimpRef *
chimp_array_new_with_capacity (size_t capacity);

ChimpRef *
chimp_array_new_var (ChimpRef *a, ...);

chimp_bool_t
chimp_array_insert (ChimpRef *self, int32_t pos, ChimpRef *value);

chimp_bool_t
chimp_array_unshift (ChimpRef *self, ChimpRef *value);

ChimpRef *
chimp_array_shift (ChimpRef *self);

chimp_bool_t
chimp_array_push (ChimpRef *self, ChimpRef *value);

ChimpRef *
chimp_array_pop (ChimpRef *self);

ChimpRef *
chimp_array_get (ChimpRef *self, int32_t pos);

int32_t
chimp_array_find (ChimpRef *self, ChimpRef *value);

ChimpRef *
chimp_array_first (ChimpRef *self);

ChimpRef *
chimp_array_last (ChimpRef *self);

#define CHIMP_ARRAY(ref)  CHIMP_CHECK_CAST(ChimpArray, (ref), chimp_array_class)

#define CHIMP_ARRAY_ITEMS(ref) (CHIMP_ARRAY(ref)->items)
#define CHIMP_ARRAY_SIZE(ref) (CHIMP_ARRAY(ref)->size)

#define CHIMP_ARRAY_FIRST(ref) chimp_array_get(ref, 0)
#define CHIMP_ARRAY_LAST(ref)  chimp_array_get(ref, -1)
#define CHIMP_ARRAY_ITEM(ref, i) chimp_array_get((ref), (i))

CHIMP_EXTERN_CLASS(array);

#ifdef __cplusplus
};
#endif

#endif

