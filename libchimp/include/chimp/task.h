#ifndef _CHIMP_TASK_H_INCLUDED_
#define _CHIMP_TASK_H_INCLUDED_

#include <chimp/vm.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpTask ChimpTask;

ChimpTask *
chimp_task_new (ChimpRef *callable);

ChimpTask *
chimp_task_new_main (void *stack_start);

chimp_bool_t
chimp_task_send (ChimpTask *task, ChimpRef *msg);

ChimpRef *
chimp_task_recv (ChimpTask *task);

void
chimp_task_delete (ChimpTask *task);

void
chimp_task_wait (ChimpTask *task);

/*
ChimpRef *
chimp_task_push_frame (ChimpTask *task);

ChimpRef *
chimp_task_pop_frame (ChimpTask *task);

ChimpRef *
chimp_task_get_frame (ChimpTask *task);
*/

ChimpTask *
chimp_task_current (void);

ChimpGC *
chimp_task_get_gc (ChimpTask *task);

ChimpVM *
chimp_task_get_vm (ChimpTask *task);

chimp_bool_t
chimp_task_add_module (ChimpTask *task, ChimpRef *module);

ChimpRef *
chimp_task_find_module (ChimpTask *task, ChimpRef *name);

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

#ifdef __cplusplus
};
#endif

#endif

