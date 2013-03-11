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

#ifndef _NIM_ARRAY_H_INCLUDED_
#define _NIM_ARRAY_H_INCLUDED_

#include <nim/gc.h>
#include <nim/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NimArray {
    NimAny   base;
    NimRef **items;
    size_t     size;
    size_t     capacity;
} NimArray;

nim_bool_t
nim_array_class_bootstrap (void);

NimRef *
nim_array_new (void);

NimRef *
nim_array_new_with_capacity (size_t capacity);

NimRef *
nim_array_new_var (NimRef *a, ...);

nim_bool_t
nim_array_insert (NimRef *self, int32_t pos, NimRef *value);

nim_bool_t
nim_array_unshift (NimRef *self, NimRef *value);

NimRef *
nim_array_shift (NimRef *self);

nim_bool_t
nim_array_push (NimRef *self, NimRef *value);

NimRef *
nim_array_pop (NimRef *self);

NimRef *
nim_array_get (NimRef *self, int32_t pos);

int32_t
nim_array_find (NimRef *self, NimRef *value);

NimRef *
nim_array_first (NimRef *self);

NimRef *
nim_array_last (NimRef *self);

#define NIM_ARRAY(ref)  NIM_CHECK_CAST(NimArray, (ref), nim_array_class)

#define NIM_ARRAY_ITEMS(ref) (NIM_ARRAY(ref)->items)
#define NIM_ARRAY_SIZE(ref) (NIM_ARRAY(ref)->size)

#define NIM_ARRAY_FIRST(ref) nim_array_get(ref, 0)
#define NIM_ARRAY_LAST(ref)  nim_array_get(ref, -1)
#define NIM_ARRAY_ITEM(ref, i) nim_array_get((ref), (i))

NIM_EXTERN_CLASS(array);

#ifdef __cplusplus
};
#endif

#endif

