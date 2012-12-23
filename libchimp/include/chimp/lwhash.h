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

#ifndef _CHIMP_LWHASH_H_INCLUDED_
#define _CHIMP_LWHASH_H_INCLUDED_

#include <chimp/gc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpLWHash ChimpLWHash;

ChimpLWHash *
chimp_lwhash_new (void);

void
chimp_lwhash_delete (ChimpLWHash *self);

chimp_bool_t
chimp_lwhash_put (ChimpLWHash *self, ChimpRef *key, ChimpRef *value);

chimp_bool_t
chimp_lwhash_get (ChimpLWHash *self, ChimpRef *key, ChimpRef **value);

void
chimp_lwhash_foreach (ChimpLWHash *self, void (*fn)(ChimpLWHash *, ChimpRef *, ChimpRef *, void *), void *);

#ifdef __cplusplus
};
#endif

#endif

