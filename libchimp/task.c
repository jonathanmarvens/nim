#include <pthread.h>

#include "chimp/gc.h"
#include "chimp/task.h"
#include "chimp/object.h"
#include "chimp/array.h"
#include "chimp/frame.h"
#include "chimp/vm.h"

/* XXX consistency: when can I pass NULL for automatically getting the current
 *     task ?
 */

ChimpRef *chimp_task_class = NULL;

static pthread_key_t current_task_key;
static pthread_once_t current_task_key_once = PTHREAD_ONCE_INIT;

struct _ChimpTaskInternal {
    ChimpGC  *gc;
    ChimpVM  *vm;
    struct _ChimpTaskInternal *parent;
    chimp_bool_t is_main;
    pthread_t thread;
    pthread_mutex_t lock;
    pthread_cond_t  send_cond;
    pthread_cond_t  recv_cond;
    ChimpRef *impl;
    ChimpRef *modules;
    ChimpMsgInternal *inbox;
    chimp_bool_t done;
};

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
    ChimpTaskInternal *task = (ChimpTaskInternal *) arg;
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
    if (task->impl != NULL) {
        chimp_object_call (task->impl, chimp_array_new ());
    }
    return NULL;
}

static ChimpRef *
_chimp_task_send (ChimpRef *self, ChimpRef *args)
{
    ChimpRef *msg = CHIMP_ARRAY_ITEM(args, 0);
    return chimp_task_send (
            CHIMP_TASK(self)->impl, msg) ? chimp_true : chimp_false;
}

static ChimpRef *
_chimp_task_recv (ChimpRef *self, ChimpRef *args)
{
    return chimp_task_recv (CHIMP_TASK(self)->impl);
}

static ChimpRef *
_chimp_task_wait (ChimpRef *self, ChimpRef *args)
{
    chimp_task_wait (CHIMP_TASK(self)->impl);
    return chimp_nil;
}

chimp_bool_t
chimp_task_class_bootstrap (void)
{
    chimp_task_class =
        chimp_class_new (CHIMP_STR_NEW("task"), NULL);
    if (chimp_task_class == NULL) {
        return CHIMP_FALSE;
    }
    chimp_gc_make_root (NULL, chimp_task_class);
    CHIMP_CLASS(chimp_task_class)->inst_type = CHIMP_VALUE_TYPE_TASK;
    chimp_class_add_native_method (chimp_task_class, "send", _chimp_task_send);
    chimp_class_add_native_method (chimp_task_class, "recv", _chimp_task_recv);
    chimp_class_add_native_method (chimp_task_class, "wait", _chimp_task_wait);
    return CHIMP_TRUE;
}

ChimpTaskInternal *
chimp_task_new (ChimpRef *callable)
{
    ChimpTaskInternal *task = CHIMP_MALLOC(ChimpTaskInternal, sizeof(*task));
    if (task == NULL) {
        return NULL;
    }
    memset (task, 0, sizeof (*task));
    /* XXX nothing ensures that 'callable' won't get collected by the active GC */
    /*     do we want to make it a root of the current GC? something else? */
    task->parent = chimp_task_current ();
    task->impl = callable;
    task->is_main = CHIMP_FALSE;
    /* TODO error checking */
    pthread_create (&task->thread, NULL, chimp_task_thread_func, task);
    pthread_mutex_init (&task->lock, NULL);
    pthread_cond_init (&task->send_cond, NULL);
    pthread_cond_init (&task->recv_cond, NULL);
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
    task->parent = NULL;
    task->impl = NULL;
    task->is_main = CHIMP_TRUE;
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
    pthread_mutex_init (&task->lock, NULL);
    pthread_cond_init (&task->send_cond, NULL);
    pthread_cond_init (&task->recv_cond, NULL);
    return task;
}

void
chimp_task_delete (ChimpTaskInternal *task)
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

chimp_bool_t
chimp_task_send (ChimpTaskInternal *task, ChimpRef *msg)
{
    ChimpMsgInternal *temp;
    size_t i;

    temp = malloc (CHIMP_MSG(msg)->impl->size);
    if (temp == NULL) {
        pthread_mutex_unlock (&task->lock);
        return CHIMP_FALSE;
    }
    memcpy (temp, CHIMP_MSG(msg)->impl, CHIMP_MSG(msg)->impl->size);
    temp->cells = ((void *)temp) + sizeof(ChimpMsgInternal);
    for (i = 0; i < CHIMP_MSG(msg)->impl->num_cells; i++) {
        /* update pointers invalidated by the copy */
        temp->cells[i].str.data = ((void *)(temp->cells + i)) + sizeof(ChimpMsgCell);
    }

    if (pthread_mutex_lock (&task->lock) != 0) {
        return CHIMP_FALSE;
    }
    task->inbox = temp;
    pthread_cond_signal (&task->recv_cond);
    while (task->inbox != NULL) {
        pthread_cond_wait (&task->send_cond, &task->lock);
    }
    pthread_mutex_unlock (&task->lock);
    return CHIMP_TRUE;
}

ChimpRef *
chimp_task_recv (ChimpTaskInternal *task)
{
    ChimpMsgInternal *temp;
    ChimpRef *ref;

    if (pthread_mutex_lock (&task->lock) != 0) {
        return NULL;
    }
    while (task->inbox == NULL) {
        pthread_cond_wait (&task->recv_cond, &task->lock);
    }
    temp = task->inbox;
    task->inbox = NULL;
    pthread_cond_signal (&task->send_cond);
    pthread_mutex_unlock (&task->lock);

    ref = chimp_msg_unpack (temp);
    free (temp);
    return ref;
}

void
chimp_task_wait (ChimpTaskInternal *task)
{
    if (!task->is_main && !task->done) {
        pthread_join (task->thread, NULL);
        pthread_cond_destroy (&task->send_cond);
        pthread_cond_destroy (&task->recv_cond);
        pthread_mutex_destroy (&task->lock);
    }
    task->done = CHIMP_TRUE;
}

/*
ChimpRef *
chimp_task_push_frame (ChimpTaskInternal *task)
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
    frame = chimp_frame_new (task->gc);
    if (!chimp_array_push (task->stack, frame)) {
        return NULL;
    }
    return frame;
}

ChimpRef *
chimp_task_pop_frame (ChimpTaskInternal *task)
{
    if (task->stack != NULL) {
        return chimp_array_pop (task->stack);
    }
    else {
        return chimp_nil;
    }
}

ChimpRef *
chimp_task_get_frame (ChimpTaskInternal *task)
{
    if (task->stack != NULL) {
        return CHIMP_ARRAY_LAST (task->stack);
    }
    else {
        return chimp_nil;
    }
}
*/

ChimpTaskInternal *
chimp_task_current (void)
{
    return (ChimpTaskInternal *) pthread_getspecific (current_task_key);
}

ChimpGC *
chimp_task_get_gc (ChimpTaskInternal *task)
{
    CHIMP_ASSERT(task != NULL);

    return task->gc;
}

ChimpVM *
chimp_task_get_vm (ChimpTaskInternal *task)
{
    CHIMP_ASSERT(task != NULL);

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

