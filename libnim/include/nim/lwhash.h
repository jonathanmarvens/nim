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

#ifndef _NIM_LWHASH_H_INCLUDED_
#define _NIM_LWHASH_H_INCLUDED_

#include <nim/gc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NimLWHash NimLWHash;

NimLWHash *
nim_lwhash_new (void);

void
nim_lwhash_delete (NimLWHash *self);

nim_bool_t
nim_lwhash_put (NimLWHash *self, NimRef *key, NimRef *value);

nim_bool_t
nim_lwhash_get (NimLWHash *self, NimRef *key, NimRef **value);

void
nim_lwhash_foreach (NimLWHash *self, void (*fn)(NimLWHash *, NimRef *, NimRef *, void *), void *);

#ifdef __cplusplus
};
#endif

#endif

