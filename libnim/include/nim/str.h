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

#ifndef _NIM_STR_H_INCLUDED_
#define _NIM_STR_H_INCLUDED_

#include <stdarg.h>

#include <nim/gc.h>
#include <nim/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NimStr {
    NimAny  base;
    char     *data;
    size_t    size;
} NimStr;

int
nim_str_class_init_2 (void);

NimRef *
nim_str_new (const char *data, size_t size);

NimRef *
nim_str_new_take (char *data, size_t size);

NimRef *
nim_str_new_format (const char *fmt, ...);

NimRef *
nim_str_new_formatv (const char *fmt, va_list args);

NimRef *
nim_str_new_concat (const char *a, ...);

/* XXX these mutate string state ... revisit me */

nim_bool_t
nim_str_append (NimRef *str, NimRef *append_me);

nim_bool_t
nim_str_append_str (NimRef *str, const char *s);

nim_bool_t
nim_str_append_strn (NimRef *str, const char *s, size_t n);

nim_bool_t
nim_str_append_char (NimRef *str, char c);

const char *
nim_str_data (NimRef *str);

#define NIM_STR(ref)    NIM_CHECK_CAST(NimStr, (ref), nim_str_class)

#define NIM_STR_DATA(ref) (NIM_STR(ref)->data)
#define NIM_STR_SIZE(ref) (NIM_STR(ref)->size)

#define NIM_STR_NEW(data) nim_str_new ((data), sizeof(data)-1)

NIM_EXTERN_CLASS(str);

#ifdef __cplusplus
};
#endif

#endif

