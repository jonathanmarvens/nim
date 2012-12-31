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

#ifndef _CHIMP_ERROR_H_INCLUDED_
#define _CHIMP_ERROR_H_INCLUDED_

#include <chimp/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpError {
    ChimpAny base;
    ChimpRef *message;
    ChimpRef *backtrace;
    ChimpRef *cause;
} ChimpError;

chimp_bool_t
chimp_error_class_bootstrap (void);

ChimpRef *
chimp_error_new (ChimpRef *message);

ChimpRef *
chimp_error_new_with_format (const char *fmt, ...);

#define CHIMP_ERROR(ref)  CHIMP_CHECK_CAST(ChimpError, (ref), chimp_error_class)
#define CHIMP_ERROR_MESSAGE(ref) CHIMP_ERROR(ref)->message
#define CHIMP_ERROR_BACKTRACE(ref) CHIMP_ERROR(ref)->backtrace
#define CHIMP_ERROR_CAUSE(ref) CHIMP_ERROR(ref)->cause

CHIMP_EXTERN_CLASS(error);

#ifdef __cplusplus
};
#endif

#endif

