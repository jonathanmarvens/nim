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

#ifndef _NIM_ERROR_H_INCLUDED_
#define _NIM_ERROR_H_INCLUDED_

#include <nim/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NimError {
    NimAny base;
    NimRef *message;
    NimRef *backtrace;
    NimRef *cause;
} NimError;

nim_bool_t
nim_error_class_bootstrap (void);

NimRef *
nim_error_new (NimRef *message);

NimRef *
nim_error_new_with_format (const char *fmt, ...);

#define NIM_ERROR(ref)  NIM_CHECK_CAST(NimError, (ref), nim_error_class)
#define NIM_ERROR_MESSAGE(ref) NIM_ERROR(ref)->message
#define NIM_ERROR_BACKTRACE(ref) NIM_ERROR(ref)->backtrace
#define NIM_ERROR_CAUSE(ref) NIM_ERROR(ref)->cause

NIM_EXTERN_CLASS(error);

#ifdef __cplusplus
};
#endif

#endif

