#include <pthread.h>

#include "chimp/gc.h"
#include "chimp/task.h"
#include "chimp/object.h"
#include "chimp/array.h"

static __thread ChimpTask *current_task = NULL;

struct _ChimpTask {
    ChimpGC  *gc;
    chimp_bool_t is_main;
    pthread_t thread;
    ChimpRef *impl;
};

static void *
chimp_task_thread_func (void *arg)
{
    ChimpTask *task = (ChimpTask *) arg;
    current_task = task;
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
    /* XXX nothing ensures that 'callable' won't get collected by the active GC */
    task->impl = callable;
    task->is_main = CHIMP_FALSE;
    task->gc = chimp_gc_new ();
    if (task->gc == NULL) {
        CHIMP_FREE (task);
        return NULL;
    }
    pthread_create (&task->thread, NULL, chimp_task_thread_func, task);
    return task;
}

ChimpTask *
chimp_task_new_main (void)
{
    ChimpTask *task = CHIMP_MALLOC(ChimpTask, sizeof(*task));
    if (task == NULL) {
        return NULL;
    }
    task->impl = NULL;
    task->is_main = CHIMP_TRUE;
    task->gc = chimp_gc_new ();
    if (task->gc == NULL) {
        CHIMP_FREE (task);
        return NULL;
    }
    current_task = task;
    return task;
}

void
chimp_task_delete (ChimpTask *task)
{
    if (task != NULL) {
        if (!task->is_main) {
            pthread_join (task->thread, NULL);
        }
        chimp_gc_delete (task->gc);
        CHIMP_FREE (task);
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

    return current_task->gc;
}

