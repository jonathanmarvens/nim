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

#ifndef _CHIMP_HASH_H_INCLUDED_
#define _CHIMP_HASH_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/any.h>
#include <chimp/lwhash.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpHash {
    ChimpAny   base;
    ChimpRef **keys;
    ChimpRef **values;
    size_t     size;
} ChimpHash;

chimp_bool_t
chimp_hash_class_bootstrap (void);

ChimpRef *
chimp_hash_new (void);

chimp_bool_t
chimp_hash_put (ChimpRef *self, ChimpRef *key, ChimpRef *value);

chimp_bool_t
chimp_hash_put_str (ChimpRef *self, const char *key, ChimpRef *value);

int
chimp_hash_get (ChimpRef *self, ChimpRef *key, ChimpRef **value);

ChimpRef *
chimp_hash_keys (ChimpRef *self);

ChimpRef *
chimp_hash_values (ChimpRef *self);

#define CHIMP_HASH(ref) \
    CHIMP_CHECK_CAST(ChimpHash, (ref), chimp_hash_class)

#define CHIMP_HASH_PUT(ref, key, value) \
    chimp_hash_put ((ref), CHIMP_STR_NEW(NULL, (key)), (value))

#define CHIMP_HASH_GET(ref, key) \
    chimp_hash_get ((ref), CHIMP_STR_NEW(NULL, (key)))

#define CHIMP_HASH_SIZE(ref) CHIMP_HASH(ref)->size

CHIMP_EXTERN_CLASS(hash);

#ifdef __cplusplus
};
#endif

#endif


