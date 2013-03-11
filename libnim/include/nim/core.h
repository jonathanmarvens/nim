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

#ifndef _NIM_CORE_H_INCLUDED_
#define _NIM_CORE_H_INCLUDED_

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int nim_bool_t;

struct _NimRef;

typedef struct _NimRef NimRef;

#define NIM_TRUE ((nim_bool_t) 1)
#define NIM_FALSE ((nim_bool_t) 0)

#define SIGN(x) ((x > 0) - (x < 0))

nim_bool_t
nim_core_startup (const char *path, void *stack_start);

void
nim_core_shutdown (void);

void
nim_bug (const char *filename, int lineno, const char *format, ...);

nim_bool_t
nim_is_builtin (struct _NimRef *name);

extern struct _NimRef *nim_module_path;

NimRef *
nim_num_add (NimRef *left, NimRef *right);

NimRef *
nim_num_sub (NimRef *left, NimRef *right);

NimRef *
nim_num_mul (NimRef *left, NimRef *right);
                                               
NimRef *
nim_num_div (NimRef *left, NimRef *right);
                                               
#define NIM_BUG(fmt, ...) \
    nim_bug (__FILE__, __LINE__, (fmt), ## __VA_ARGS__)

#define NIM_MALLOC(type, size) ((type *) malloc (size))
#define NIM_REALLOC(type, ptr, size) ((type *) realloc ((ptr), (size)))
#define NIM_FREE(ptr) \
    do { \
        if ((ptr) != NULL) { free (ptr); } \
    } while (0)

#define NIM_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            nim_bug (__FILE__, __LINE__, "assertion failed: \"%s\"", #cond); \
        } \
    } while (0)

#ifdef __cplusplus
};
#endif

#endif

