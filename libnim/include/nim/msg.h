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

#ifndef _NIM_MSG_H_INCLUDED_
#define _NIM_MSG_H_INCLUDED_

#include <pthread.h>

#include <nim/any.h>
#include <nim/gc.h>
#include <nim/task.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NimMsgCell {
    enum {
        NIM_MSG_CELL_NIL,
        NIM_MSG_CELL_INT,
        NIM_MSG_CELL_STR,
        NIM_MSG_CELL_ARRAY,
        NIM_MSG_CELL_MODULE,
        NIM_MSG_CELL_METHOD,
        NIM_MSG_CELL_TASK
    } type;
    union {
        int64_t  int_;
        struct {
            char    *data;
            size_t   size;
        } str;
        struct {
            struct _NimMsgCell *items;
            size_t                size;
        } array;
        NimRef          *method;
        NimRef          *module;
        NimTaskInternal *task;
    };
} NimMsgCell;

typedef struct _NimMsgInternal {
    size_t                    size;
    NimMsgCell             *cell;
    struct _NimMsgInternal *next;
} NimMsgInternal;

NimMsgInternal *
nim_msg_pack (NimRef *array);

NimRef *
nim_msg_unpack (NimMsgInternal *internal);

#ifdef __cplusplus
};
#endif

#endif

