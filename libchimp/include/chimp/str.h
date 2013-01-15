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

#ifndef _CHIMP_STR_H_INCLUDED_
#define _CHIMP_STR_H_INCLUDED_

#include <stdarg.h>

#include <chimp/gc.h>
#include <chimp/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpStr {
    ChimpAny  base;
    char     *data;
    size_t    size;
} ChimpStr;

ChimpRef *
chimp_str_new (const char *data, size_t size);

ChimpRef *
chimp_str_new_take (char *data, size_t size);

ChimpRef *
chimp_str_new_format (const char *fmt, ...);

ChimpRef *
chimp_str_new_formatv (const char *fmt, va_list args);

ChimpRef *
chimp_str_new_concat (const char *a, ...);

/* XXX these mutate string state ... revisit me */

chimp_bool_t
chimp_str_append (ChimpRef *str, ChimpRef *append_me);

chimp_bool_t
chimp_str_append_str (ChimpRef *str, const char *s);

chimp_bool_t
chimp_str_append_strn (ChimpRef *str, const char *s, size_t n);

chimp_bool_t
chimp_str_append_char (ChimpRef *str, char c);

#define CHIMP_STR(ref)    CHIMP_CHECK_CAST(ChimpStr, (ref), chimp_str_class)

#define CHIMP_STR_DATA(ref) (CHIMP_STR(ref)->data)
#define CHIMP_STR_SIZE(ref) (CHIMP_STR(ref)->size)

#define CHIMP_STR_NEW(data) chimp_str_new ((data), sizeof(data)-1)

CHIMP_EXTERN_CLASS(str);

#ifdef __cplusplus
};
#endif

#endif

