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

#ifndef _NIM_TASK_H_INCLUDED_
#define _NIM_TASK_H_INCLUDED_

#include <nim/vm.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NimTaskInternal NimTaskInternal;

typedef struct _NimTask {
    NimAny           base;
    NimTaskInternal *priv;
    nim_bool_t       local;
} NimTask;

nim_bool_t
nim_task_class_bootstrap (void);

NimRef *
nim_task_new (NimRef *callable);

NimRef *
nim_task_new_from_internal (NimTaskInternal *task);

NimTaskInternal *
nim_task_new_main (void *stack_start);

nim_bool_t
nim_task_main_ready (void);

void
nim_task_main_delete ();

void
nim_task_join (NimTaskInternal *task);

void
nim_task_ref (NimTaskInternal *task);

void
nim_task_unref (NimTaskInternal *task);

nim_bool_t
nim_task_send (NimRef *self, NimRef *value);

NimRef *
nim_task_recv (NimRef *self);

void
nim_task_mark (NimGC *gc, NimTaskInternal *task);

NimTaskInternal *
nim_task_current (void);

NimTaskInternal *
nim_task_main (void);

NimRef *
nim_task_get_self (NimTaskInternal *task);

NimGC *
nim_task_get_gc (NimTaskInternal *task);

NimVM *
nim_task_get_vm (NimTaskInternal *task);

nim_bool_t
nim_task_add_module (NimTaskInternal *task, NimRef *module);

NimRef *
nim_task_find_module (NimTaskInternal *task, NimRef *name);

#define NIM_CURRENT_TASK nim_task_current ()
#define NIM_CURRENT_STACK_FRAME \
    nim_task_top_stack_frame (nim_task_current())
#define NIM_CURRENT_GC nim_task_get_gc (nim_task_current())
#define NIM_CURRENT_VM nim_task_get_vm (nim_task_current())
/* #define NIM_CURRENT_FRAME nim_task_get_frame (nim_task_current()) */

#define NIM_MAIN_GC nim_task_get_gc (nim_task_main())

#define NIM_PUSH_STACK_FRAME() \
    nim_task_push_stack_frame (nim_task_current ())

#define NIM_POP_STACK_FRAME() \
    nim_task_pop_stack_frame (nim_task_current ())

#define NIM_TASK(ref)  NIM_CHECK_CAST(NimTask, (ref), nim_task_class)

NIM_EXTERN_CLASS(task);

#ifdef __cplusplus
};
#endif

#endif

