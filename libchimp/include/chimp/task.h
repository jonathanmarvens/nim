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

#ifndef _CHIMP_TASK_H_INCLUDED_
#define _CHIMP_TASK_H_INCLUDED_

#include <chimp/vm.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpTaskInternal ChimpTaskInternal;

typedef struct _ChimpTask {
    ChimpAny           base;
    ChimpTaskInternal *priv;
    chimp_bool_t       local;
} ChimpTask;

chimp_bool_t
chimp_task_class_bootstrap (void);

ChimpRef *
chimp_task_new (ChimpRef *callable);

ChimpRef *
chimp_task_new_from_internal (ChimpTaskInternal *task);

ChimpTaskInternal *
chimp_task_new_main (void *stack_start);

chimp_bool_t
chimp_task_main_ready (void);

void
chimp_task_main_delete ();

void
chimp_task_ref (ChimpTaskInternal *task);

void
chimp_task_unref (ChimpTaskInternal *task);

chimp_bool_t
chimp_task_send (ChimpRef *self, ChimpRef *value);

ChimpRef *
chimp_task_recv (ChimpRef *self);

void
chimp_task_mark (ChimpGC *gc, ChimpTaskInternal *task);

ChimpTaskInternal *
chimp_task_current (void);

ChimpRef *
chimp_task_get_self (ChimpTaskInternal *task);

ChimpGC *
chimp_task_get_gc (ChimpTaskInternal *task);

ChimpVM *
chimp_task_get_vm (ChimpTaskInternal *task);

chimp_bool_t
chimp_task_add_module (ChimpTaskInternal *task, ChimpRef *module);

ChimpRef *
chimp_task_find_module (ChimpTaskInternal *task, ChimpRef *name);

#define CHIMP_CURRENT_TASK chimp_task_current ()
#define CHIMP_CURRENT_STACK_FRAME \
    chimp_task_top_stack_frame (chimp_task_current())
#define CHIMP_CURRENT_GC chimp_task_get_gc (chimp_task_current())
#define CHIMP_CURRENT_VM chimp_task_get_vm (chimp_task_current())
/* #define CHIMP_CURRENT_FRAME chimp_task_get_frame (chimp_task_current()) */

#define CHIMP_PUSH_STACK_FRAME() \
    chimp_task_push_stack_frame (chimp_task_current ())

#define CHIMP_POP_STACK_FRAME() \
    chimp_task_pop_stack_frame (chimp_task_current ())

#define CHIMP_TASK(ref)  CHIMP_CHECK_CAST(ChimpTask, (ref), chimp_task_class)

CHIMP_EXTERN_CLASS(task);

#ifdef __cplusplus
};
#endif

#endif

