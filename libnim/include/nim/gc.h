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

#ifndef _NIM_GC_H_INCLUDED_
#define _NIM_GC_H_INCLUDED_

#include <nim/core.h>

#ifdef __cplusplus
extern "C" {
#endif

#if ((defined __x86_64) || \
        (defined __x86_64__) || \
        (defined __amd64__) || \
        (defined __amd64) || \
        (defined _M_X64) || \
        (defined _M_AMD64)) && \
    (defined __GNUC__)
#define NIM_ARCH_X86_64
#define NIM_ARCH "x86-64"
#elif ((defined i386) || \
        (defined __i386) || \
        (defined __i386__) || \
        (defined _M_IX86) || \
        (defined __X86__) || \
        (defined _X86_) || \
        (defined __INTEL__)) && \
    (defined __GNUC__)
#define NIM_ARCH_X86_32
#define NIM_ARCH "x86"
#else
#define NIM_ARCH_UNKNOWN
#define NIM_ARCH "unknown"
#endif

/* TODO sniff OS in build scripts */
#define NIM_OS "*nix"
#define NIM_VERSION "0.0.1"

struct _NimLWHash;

typedef struct _NimGC NimGC;

NimGC *
nim_gc_new (void *stack_start);

void
nim_gc_delete (NimGC *gc);

NimRef *
nim_gc_new_object (NimGC *gc);

nim_bool_t
nim_gc_make_root (NimGC *gc, NimRef *ref);

void *
nim_gc_ref_check_cast (NimRef *ref, NimRef *klass);

nim_bool_t
nim_gc_collect (NimGC *gc);

void
nim_gc_mark_ref (NimGC *gc, NimRef *ref);

void
nim_gc_mark_lwhash (NimGC *gc, struct _NimLWHash *lwhash);

uint64_t
nim_gc_collection_count (NimGC *gc);

uint64_t
nim_gc_num_live (NimGC *gc);

uint64_t
nim_gc_num_free (NimGC *gc);

#define NIM_VALUE_SIZE 256

#define NIM_GC_MAKE_STACK_ROOT(p) \
    (*((NimRef **)alloca(sizeof(NimRef *)))) = (p)

#ifdef __cplusplus
};
#endif

#endif

