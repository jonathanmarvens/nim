#include <pthread.h>

#include "chimp/gc.h"
#include "chimp/task.h"
#include "chimp/object.h"
#include "chimp/array.h"
#include "chimp/stackframe.h"
#include "chimp/vm.h"

static __thread ChimpTask *current_task = NULL;

struct _ChimpTask {
    ChimpGC  *gc;
    ChimpVM  *vm;
    chimp_bool_t is_main;
    pthread_t thread;
    ChimpRef *impl;
    ChimpRef *stack;
    chimp_bool_t done;
};

static void *
chimp_task_thread_func (void *arg)
{
    ChimpTask *task = (ChimpTask *) arg;
    task->gc = chimp_gc_new ((void *)&task);
    if (task->gc == NULL) {
        CHIMP_FREE (task);
        return NULL;
    }
    current_task = task;
    task->vm = chimp_vm_new ();
    if (task->vm == NULL) {
        chimp_gc_delete (task->gc);
        CHIMP_FREE (task);
        return NULL;
    }
    if (task->impl != NULL) {
        chimp_object_call (task->impl, chimp_array_new (NULL));
    }
    return NULL;
}

ChimpTask *
chimp_task_new (ChimpRef *callable)
{
    ChimpTask *task = CHIMP_MALLOC(ChimpTask, sizeof(*task));
    if (task == NULL) {
        return NULL;
    }
    memset (task, 0, sizeof (*task));
    /* XXX nothing ensures that 'callable' won't get collected by the active GC */
    /*     do we want to make it a root of the current GC? something else? */
    task->impl = callable;
    task->is_main = CHIMP_FALSE;
    /* TODO error checking */
    pthread_create (&task->thread, NULL, chimp_task_thread_func, task);
    return task;
}

ChimpTask *
chimp_task_new_main (void *stack_start)
{
    ChimpTask *task = CHIMP_MALLOC(ChimpTask, sizeof(*task));
    if (task == NULL) {
        return NULL;
    }
    memset (task, 0, sizeof (*task));
    task->impl = NULL;
    task->is_main = CHIMP_TRUE;
    task->gc = chimp_gc_new (stack_start);
    if (task->gc == NULL) {
        CHIMP_FREE (task);
        return NULL;
    }
    current_task = task;
    task->vm = chimp_vm_new ();
    if (task->vm == NULL) {
        chimp_gc_delete (task->gc);
        CHIMP_FREE (task);
        return NULL;
    }
    return task;
}

void
chimp_task_delete (ChimpTask *task)
{
    if (task != NULL) {
        if (!task->done) {
            chimp_task_wait (task);
        }
        chimp_vm_delete (task->vm);
        chimp_gc_delete (task->gc);
        CHIMP_FREE (task);
    }
}

void
chimp_task_wait (ChimpTask *task)
{
    if (!task->is_main && !task->done) {
        pthread_join (task->thread, NULL);
    }
    task->done = CHIMP_TRUE;
}

ChimpRef *
chimp_task_push_stack_frame (ChimpTask *task)
{
    ChimpRef *frame;
    if (task->stack == NULL) {
        task->stack = chimp_array_new (task->gc);
        if (task->stack == NULL) {
            return NULL;
        }
        if (!chimp_gc_make_root (task->gc, task->stack)) {
            return NULL;
        }
    }
    frame = chimp_stack_frame_new (task->gc);
    if (!chimp_array_push (task->stack, frame)) {
        return NULL;
    }
    return frame;
}

ChimpRef *
chimp_task_pop_stack_frame (ChimpTask *task)
{
    if (task->stack != NULL) {
        return chimp_array_pop (task->stack);
    }
    else {
        return chimp_nil;
    }
}

ChimpRef *
chimp_task_top_stack_frame (ChimpTask *task)
{
    if (task->stack != NULL) {
        return CHIMP_ARRAY_LAST (task->stack);
    }
    else {
        return chimp_nil;
    }
}

ChimpTask *
chimp_task_current (void)
{
    CHIMP_ASSERT(current_task != NULL);

    return current_task;
}

ChimpGC *
chimp_task_get_gc (ChimpTask *task)
{
    CHIMP_ASSERT(task != NULL);

    return task->gc;
}

ChimpVM *
chimp_task_get_vm (ChimpTask *task)
{
    CHIMP_ASSERT(task != NULL);

    return task->vm;
}
