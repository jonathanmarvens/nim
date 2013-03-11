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

#ifndef _NIM_FRAME_H_INCLUDED_
#define _NIM_FRAME_H_INCLUDED_

#include <nim/gc.h>
#include <nim/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NimFrame {
    NimAny   base;
    NimRef  *method;
    NimRef  *locals;
} NimFrame;

nim_bool_t
nim_frame_class_bootstrap (void);

NimRef *
nim_frame_new (NimRef *method);

#define NIM_FRAME(ref) \
    NIM_CHECK_CAST(NimFrame, (ref), nim_frame_class)

/* XXX it's ugly as hell to distinguish between closures & normal methods here */
#define NIM_FRAME_CODE(ref) \
    (NIM_METHOD_TYPE(NIM_FRAME(ref)->method) == NIM_METHOD_TYPE_BYTECODE ? \
        NIM_METHOD(NIM_FRAME(ref)->method)->bytecode.code : \
        NIM_METHOD(NIM_FRAME(ref)->method)->closure.code)

NIM_EXTERN_CLASS(frame);

#ifdef __cplusplus
};
#endif

#endif

