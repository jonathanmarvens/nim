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

#ifndef _CHIMP_FRAME_H_INCLUDED_
#define _CHIMP_FRAME_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpFrame {
    ChimpAny   base;
    ChimpRef  *method;
    ChimpRef  *locals;
} ChimpFrame;

chimp_bool_t
chimp_frame_class_bootstrap (void);

ChimpRef *
chimp_frame_new (ChimpRef *method);

#define CHIMP_FRAME(ref) \
    CHIMP_CHECK_CAST(ChimpFrame, (ref), chimp_frame_class)

/* XXX it's ugly as hell to distinguish between closures & normal methods here */
#define CHIMP_FRAME_CODE(ref) \
    (CHIMP_METHOD_TYPE(CHIMP_FRAME(ref)->method) == CHIMP_METHOD_TYPE_BYTECODE ? \
        CHIMP_METHOD(CHIMP_FRAME(ref)->method)->bytecode.code : \
        CHIMP_METHOD(CHIMP_FRAME(ref)->method)->closure.code)

CHIMP_EXTERN_CLASS(frame);

#ifdef __cplusplus
};
#endif

#endif

