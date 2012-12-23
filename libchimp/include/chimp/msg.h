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

#ifndef _CHIMP_MSG_H_INCLUDED_
#define _CHIMP_MSG_H_INCLUDED_

#include <pthread.h>

#include <chimp/any.h>
#include <chimp/gc.h>
#include <chimp/task.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpMsgCell {
    enum {
        CHIMP_MSG_CELL_INT,
        CHIMP_MSG_CELL_STR,
        CHIMP_MSG_CELL_ARRAY,
        CHIMP_MSG_CELL_TASK
    } type;
    union {
        int64_t  int_;
        struct {
            char    *data;
            size_t   size;
        } str;
        struct {
            struct _ChimpMsgCell *items;
            size_t                size;
        } array;
        ChimpTaskInternal    *task;
    };
} ChimpMsgCell;

typedef struct _ChimpMsgInternal {
    size_t                    size;
    ChimpMsgCell             *cell;
    struct _ChimpMsgInternal *next;
} ChimpMsgInternal;

ChimpMsgInternal *
chimp_msg_pack (ChimpRef *array);

ChimpRef *
chimp_msg_unpack (ChimpMsgInternal *internal);

#ifdef __cplusplus
};
#endif

#endif

