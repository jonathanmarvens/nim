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

#ifndef _CHIMP_GC_H_INCLUDED_
#define _CHIMP_GC_H_INCLUDED_

#include <chimp/core.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _ChimpLWHash;

typedef struct _ChimpGC ChimpGC;
typedef struct _ChimpRef ChimpRef;

ChimpGC *
chimp_gc_new (void *stack_start);

void
chimp_gc_delete (ChimpGC *gc);

ChimpRef *
chimp_gc_new_object (ChimpGC *gc);

chimp_bool_t
chimp_gc_make_root (ChimpGC *gc, ChimpRef *ref);

void *
chimp_gc_ref_check_cast (ChimpRef *ref, ChimpRef *klass);

chimp_bool_t
chimp_gc_collect (ChimpGC *gc);

void
chimp_gc_mark_ref (ChimpGC *gc, ChimpRef *ref);

void
chimp_gc_mark_lwhash (ChimpGC *gc, struct _ChimpLWHash *lwhash);

uint64_t
chimp_gc_collection_count (ChimpGC *gc);

uint64_t
chimp_gc_num_live (ChimpGC *gc);

uint64_t
chimp_gc_num_free (ChimpGC *gc);

#define CHIMP_GC_MAKE_STACK_ROOT(p) \
    (*((ChimpRef **)alloca(sizeof(ChimpRef *)))) = (p)

#ifdef __cplusplus
};
#endif

#endif

