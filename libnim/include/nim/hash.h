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

#ifndef _NIM_HASH_H_INCLUDED_
#define _NIM_HASH_H_INCLUDED_

#include <nim/gc.h>
#include <nim/any.h>
#include <nim/lwhash.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NimHash {
    NimAny   base;
    NimRef **keys;
    NimRef **values;
    size_t     size;
} NimHash;

nim_bool_t
nim_hash_class_bootstrap (void);

NimRef *
nim_hash_new (void);

nim_bool_t
nim_hash_put (NimRef *self, NimRef *key, NimRef *value);

nim_bool_t
nim_hash_put_str (NimRef *self, const char *key, NimRef *value);

int
nim_hash_get (NimRef *self, NimRef *key, NimRef **value);

NimRef *
nim_hash_keys (NimRef *self);

NimRef *
nim_hash_values (NimRef *self);

#define NIM_HASH(ref) \
    NIM_CHECK_CAST(NimHash, (ref), nim_hash_class)

#define NIM_HASH_PUT(ref, key, value) \
    nim_hash_put ((ref), NIM_STR_NEW(NULL, (key)), (value))

#define NIM_HASH_GET(ref, key) \
    nim_hash_get ((ref), NIM_STR_NEW(NULL, (key)))

#define NIM_HASH_SIZE(ref) NIM_HASH(ref)->size

NIM_EXTERN_CLASS(hash);

#ifdef __cplusplus
};
#endif

#endif


