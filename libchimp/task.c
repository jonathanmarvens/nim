#include <pthread.h>

#include <stddef.h>
#include <inttypes.h>

#include "chimp/gc.h"
#include "chimp/task.h"
#include "chimp/object.h"
#include "chimp/array.h"
#include "chimp/frame.h"
#include "chimp/vm.h"

static pthread_once_t current_task_key_once = PTHREAD_ONCE_INIT;

enum {
    CHIMP_TASK_FLAG_MAIN     = 0x01,
    CHIMP_TASK_FLAG_READY    = 0x02,
    CHIMP_TASK_FLAG_DETACHED = 0x04,
    CHIMP_TASK_FLAG_DONE     = 0x80,
};

#define CHIMP_TASK_IS_MAIN(task) \
    ((((task)->flags) & CHIMP_TASK_FLAG_MAIN) == CHIMP_TASK_FLAG_MAIN)

#define CHIMP_TASK_IS_READY(task) \
    ((((task)->flags) & CHIMP_TASK_FLAG_READY) == CHIMP_TASK_FLAG_READY)

#define CHIMP_TASK_IS_DETACHED(task) \
    ((((task)->flags) & CHIMP_TASK_FLAG_DETACHED) == CHIMP_TASK_FLAG_DETACHED)

#define CHIMP_TASK_IS_DONE(task) \
    ((((task)->flags) & CHIMP_TASK_FLAG_DONE) == CHIMP_TASK_FLAG_DONE)

#define CHIMP_TASK_IS_JOINABLE(task) \
    (!(CHIMP_TASK_IS_DETACHED(task) || \
        CHIMP_TASK_IS_MAIN(task) || \
        CHIMP_TASK_IS_DONE(task)))

#define CHIMP_TASK_LOCK(task) \
    pthread_mutex_lock (&(task)->lock)

#define CHIMP_TASK_UNLOCK(task) \
    pthread_mutex_unlock (&(task)->lock)

static pthread_key_t current_task_key;

struct _ChimpTaskInternal {
    ChimpGC           *gc;
    ChimpVM           *vm;
    ChimpRef          *self;    /* ChimpTask */
    ChimpRef          *method;  /* ChimpMethod -- NULL for main */
    ChimpRef          *modules; /* ChimpHash */
    ChimpTaskInternal *parent;
    ChimpTaskInternal *children;
    ChimpTaskInternal *next;
    pthread_cond_t     flags_cond;
    int                flags;
    pthread_t          thread;
    pthread_mutex_t    lock;
};

static void
chimp_task_cleanup (ChimpTaskInternal *task);

static void
chimp_task_init_per_thread_key (void)
{
    pthread_key_create (&current_task_key, NULL);
}

static void
chimp_task_init_per_thread_key_once (ChimpTaskInternal *task)
{
    pthread_once (&current_task_key_once, chimp_task_init_per_thread_key);
    pthread_setspecific (current_task_key, task);
}

static void *
chimp_task_thread_func (void *arg)
{
    /* NOTE: task->lock will be held by chimp_task_new at this point.    */
    /*       this will prevent other threads fiddling before we're ready */

    ChimpTaskInternal *task = (ChimpTaskInternal *) arg;
    /* printf ("[%p] started\n", task); */
    task->gc = chimp_gc_new ((void *)&task);
    if (task->gc == NULL) {
        CHIMP_FREE (task);
        return NULL;
    }

    chimp_task_init_per_thread_key_once (task);

    task->vm = chimp_vm_new ();
    if (task->vm == NULL) {
        chimp_gc_delete (task->gc);
        CHIMP_FREE (task);
        return NULL;
    }

    task->flags |= CHIMP_TASK_FLAG_READY;
    CHIMP_TASK_UNLOCK(task);
    pthread_cond_broadcast (&task->flags_cond);

    if (task->method != NULL) {
        /* TODO pass pipe in as an argument here */
        ChimpRef *args = chimp_array_new_var (NULL);
        if (chimp_vm_invoke (task->vm, task->method, args) == NULL) {
            chimp_gc_delete (task->gc);
            chimp_vm_delete (task->vm);
            CHIMP_FREE (task);
            return NULL;
        }
    }

    /* if the current thread has been detached, we're now responsible
     * for cleaning ourselves up at this point (normally it's left to
     * a join on a task).
     */
    CHIMP_TASK_LOCK(task);
    if (CHIMP_TASK_IS_DETACHED(task)) {
        chimp_task_cleanup (task);
    }
    else {
        CHIMP_TASK_UNLOCK(task);
    }

    return NULL;
}

ChimpTaskInternal *
chimp_task_new (ChimpRef *callable)
{
    ChimpTaskInternal *task = CHIMP_MALLOC(ChimpTaskInternal, sizeof(*task));
    if (task == NULL) {
        return NULL;
    }
    memset (task, 0, sizeof (*task));
    task->parent = chimp_task_current ();

    CHIMP_TASK_LOCK(task->parent);
    task->next = task->parent->children;
    task->parent->children = task;
    CHIMP_TASK_UNLOCK(task->parent);

    /* XXX can we guarantee callable won't be collected? think so ... */
    task->method = callable;
    task->flags = 0;
    if (pthread_mutex_init (&task->lock, NULL) != 0) {
        CHIMP_FREE (task);
        return NULL;
    }
    if (pthread_cond_init (&task->flags_cond, NULL) != 0) {
        pthread_mutex_destroy (&task->lock);
        CHIMP_FREE (task);
        return NULL;
    }
    CHIMP_TASK_LOCK(task);
    if (pthread_create (&task->thread, NULL, chimp_task_thread_func, task) != 0) {
        pthread_mutex_destroy (&task->lock);
        CHIMP_FREE (task);
        return NULL;
    }
    return task;
}

ChimpTaskInternal *
chimp_task_new_main (void *stack_start)
{
    ChimpTaskInternal *task = CHIMP_MALLOC(ChimpTaskInternal, sizeof(*task));
    if (task == NULL) {
        return NULL;
    }
    memset (task, 0, sizeof (*task));
    task->flags = CHIMP_TASK_FLAG_MAIN;
    task->gc = chimp_gc_new (stack_start);
    if (task->gc == NULL) {
        CHIMP_FREE (task);
        return NULL;
    }

    chimp_task_init_per_thread_key_once (task);

    task->vm = chimp_vm_new ();
    if (task->vm == NULL) {
        chimp_gc_delete (task->gc);
        CHIMP_FREE (task);
        return NULL;
    }
    if (pthread_mutex_init (&task->lock, NULL) != 0) {
        chimp_gc_delete (task->gc);
        chimp_vm_delete (task->vm);
        CHIMP_FREE (task);
        return NULL;
    }
    if (pthread_cond_init (&task->flags_cond, NULL) != 0) {
        pthread_mutex_destroy (&task->lock);
        chimp_gc_delete (task->gc);
        chimp_vm_delete (task->vm);
        CHIMP_FREE (task);
        return NULL;
    }
    CHIMP_TASK_LOCK(task);
    return task;
}

chimp_bool_t
chimp_task_main_ready (void)
{
    /* NOTE: lock still held by chimp_task_main_new here */

    ChimpTaskInternal *task = CHIMP_CURRENT_TASK;
    task->flags |= CHIMP_TASK_FLAG_READY;
    CHIMP_TASK_UNLOCK(task);
    pthread_cond_broadcast (&task->flags_cond);
    return CHIMP_TRUE;
}

void
chimp_task_main_delete ()
{
    ChimpTaskInternal *task = CHIMP_CURRENT_TASK;
    CHIMP_TASK_LOCK(task);
    chimp_task_cleanup(task);
}

static void
chimp_task_cleanup (ChimpTaskInternal *task)
{
    ChimpTaskInternal *child;
    ChimpTaskInternal *prev;

    /* NOTE: we assume the task lock is held here */

    /* detach all child tasks since we can no longer join on them */
    child = task->children;
    while (child != NULL) {
        CHIMP_TASK_LOCK(child);
        child->flags |= CHIMP_TASK_FLAG_DETACHED;
        pthread_cond_broadcast (&child->flags_cond);
        child->parent = NULL;
        pthread_detach (child->thread);
        CHIMP_TASK_UNLOCK(child);
        child = child->next;
    }

    /* remove this task from its parent */
    if (task->parent != NULL) {
        CHIMP_TASK_LOCK(task->parent);
        child = task->parent->children;
        prev = NULL;
        while (child != NULL) {
            ChimpTaskInternal *next = child->next;
            if (child == task) {
                if (prev == NULL) {
                    task->parent->children = child->next;
                }
                else {
                    prev->next = child->next;
                }
                break;
            }
            prev = child;
            child = next;
        }
        CHIMP_TASK_UNLOCK(task->parent);
        task->parent = NULL;
    }

    chimp_vm_delete (task->vm);
    chimp_gc_delete (task->gc);
    CHIMP_TASK_UNLOCK (task);
    pthread_mutex_destroy (&task->lock);
    CHIMP_FREE (task);
}

static void
chimp_task_join (ChimpTaskInternal *task)
{
    CHIMP_TASK_LOCK(task);
    if (CHIMP_TASK_IS_JOINABLE(task)) {
        CHIMP_TASK_UNLOCK(task);

        pthread_join (task->thread, NULL);

        CHIMP_TASK_LOCK(task);

        task->flags |= CHIMP_TASK_FLAG_DONE;

        /* NOTE: lock is acquired inside the thread & then
         *       unlocked/freed by the next call
         */

        chimp_task_cleanup (task);
    }
    else {
        CHIMP_TASK_UNLOCK(task);
    }
}

void
chimp_task_mark (ChimpGC *gc, ChimpTaskInternal *task)
{
}

ChimpTaskInternal *
chimp_task_current (void)
{
    return (ChimpTaskInternal *) pthread_getspecific (current_task_key);
}

ChimpGC *
chimp_task_get_gc (ChimpTaskInternal *task)
{
    if (task == NULL) task = CHIMP_CURRENT_TASK;

    return task->gc;
}

ChimpVM *
chimp_task_get_vm (ChimpTaskInternal *task)
{
    if (task == NULL) task = CHIMP_CURRENT_TASK;

    return task->vm;
}

chimp_bool_t
chimp_task_add_module (ChimpTaskInternal *task, ChimpRef *module)
{
    if (task == NULL) {
        task = chimp_task_current ();
    }

    if (task->modules == NULL) {
        task->modules = chimp_hash_new ();
        if (task->modules == NULL) {
            return CHIMP_FALSE;
        }
        chimp_gc_make_root (NULL, task->modules);
    }

    return chimp_hash_put (task->modules, CHIMP_MODULE(module)->name, module);
}

ChimpRef *
chimp_task_find_module (ChimpTaskInternal *task, ChimpRef *name)
{
    /* XXX total waste of time. modules are not really per-task atm. */

    if (task == NULL) {
        task = chimp_task_current ();
    }

    while (task != NULL) {
        ChimpRef *value;
        
        if (task->modules != NULL) {
            value = chimp_hash_get (task->modules, name);
            if (value == NULL) {
                return NULL;
            }
            else if (value != chimp_nil) {
                return value;
            }
        }
        task = task->parent;
    }
    return NULL;
}

