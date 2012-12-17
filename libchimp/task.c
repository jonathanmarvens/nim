#include <pthread.h>

#include <stddef.h>
#include <inttypes.h>

#include "chimp/gc.h"
#include "chimp/task.h"
#include "chimp/object.h"
#include "chimp/array.h"
#include "chimp/frame.h"
#include "chimp/vm.h"

ChimpRef *chimp_task_class = NULL;

static pthread_once_t current_task_key_once = PTHREAD_ONCE_INIT;

enum {
    CHIMP_TASK_FLAG_MAIN       = 0x01,
    CHIMP_TASK_FLAG_READY      = 0x02,
    CHIMP_TASK_FLAG_DETACHED   = 0x04,
    CHIMP_TASK_FLAG_INBOX_FULL = 0x08,
    CHIMP_TASK_FLAG_OUTBOX_FULL = 0x10,
    CHIMP_TASK_FLAG_DONE       = 0x80,
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

#define CHIMP_TASK_IS_INBOX_FULL(task) \
    ((((task)->flags) & CHIMP_TASK_FLAG_INBOX_FULL) \
        == CHIMP_TASK_FLAG_INBOX_FULL)

#define CHIMP_TASK_IS_OUTBOX_FULL(task) \
    ((((task)->flags) & CHIMP_TASK_FLAG_OUTBOX_FULL) \
        == CHIMP_TASK_FLAG_OUTBOX_FULL)

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
    int                refs;
    ChimpMsgInternal  *inbox;
    ChimpMsgInternal  *outbox;
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

    /* TODO better error handling */

    ChimpTaskInternal *task = (ChimpTaskInternal *) arg;
    /* printf ("[%p] started\n", task); */
    task->gc = chimp_gc_new ((void *)&task);
    if (task->gc == NULL) {
        return NULL;
    }

    chimp_task_init_per_thread_key_once (task);

    task->vm = chimp_vm_new ();
    if (task->vm == NULL) {
        return NULL;
    }

    task->flags |= CHIMP_TASK_FLAG_READY;
    pthread_cond_broadcast (&task->flags_cond);

    if (task->method != NULL) {
        ChimpRef *args;
        ChimpRef *taskobj = chimp_class_new_instance (chimp_task_class, NULL);
        if (taskobj == NULL) {
            return NULL;
        }
        CHIMP_TASK(taskobj)->priv = task;
        CHIMP_TASK(taskobj)->local = CHIMP_TRUE;
        task->refs++;
        task->self = taskobj;
        args = chimp_array_new_var (taskobj, NULL);
        CHIMP_TASK_UNLOCK(task);
        if (chimp_vm_invoke (task->vm, task->method, args) == NULL) {
            return NULL;
        }
    }

    /* if the current thread has been detached, we're now responsible
     * for cleaning ourselves up at this point (normally it's left to
     * a join on a task).
     */
    CHIMP_TASK_LOCK(task);
    if (CHIMP_TASK_IS_DETACHED(task)) {
        task->refs--;
        if (task->refs == 0) {
            chimp_task_cleanup (task);
        }
        else {
            CHIMP_TASK_UNLOCK(task);
        }
    }
    else {
        CHIMP_TASK_UNLOCK(task);
    }

    return NULL;
}

ChimpRef *
chimp_task_new (ChimpRef *callable)
{
    ChimpRef *taskobj;
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
        pthread_cond_destroy (&task->flags_cond);
        pthread_mutex_destroy (&task->lock);
        CHIMP_FREE (task);
        return NULL;
    }
    /* XXX error handling code is probably all kinds of wrong/unsafe from
     *     this point, but meh.
     */
    while (!CHIMP_TASK_IS_READY(task)) {
        if (pthread_cond_wait (&task->flags_cond, &task->lock) != 0) {
            CHIMP_TASK_UNLOCK(task);
            pthread_cond_destroy (&task->flags_cond);
            pthread_mutex_destroy (&task->lock);
            CHIMP_FREE (task);
            return NULL;
        }
    }
    taskobj = chimp_class_new_instance (chimp_task_class, NULL);
    if (taskobj == NULL) {
        CHIMP_TASK_UNLOCK (task);
        pthread_cond_destroy (&task->flags_cond);
        pthread_mutex_destroy (&task->lock);
        CHIMP_FREE (task);
        return NULL;
    }
    CHIMP_TASK(taskobj)->priv = task;
    CHIMP_TASK(taskobj)->local = CHIMP_FALSE;
    task->refs++;
    CHIMP_TASK_UNLOCK(task);
    return taskobj;
}

ChimpRef *
chimp_task_new_from_internal (ChimpTaskInternal *priv)
{
    ChimpRef *ref = chimp_class_new_instance (chimp_task_class, NULL);
    if (ref == NULL) {
        return NULL;
    }
    /* XXX blatant copy/paste from chimp_task_new */
    CHIMP_TASK_LOCK(priv);
    while (!CHIMP_TASK_IS_READY(priv)) {
        if (pthread_cond_wait (&priv->flags_cond, &priv->lock) != 0) {
            CHIMP_TASK_UNLOCK(priv);
            return NULL;
        }
    }
    CHIMP_TASK(ref)->priv = priv;
    CHIMP_TASK(ref)->local = CHIMP_FALSE;
    priv->refs++;
    CHIMP_TASK_UNLOCK(priv);
    return ref;
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
    task->vm = chimp_vm_new ();
    if (task->vm == NULL) {
        chimp_gc_delete (task->gc);
        CHIMP_FREE (task);
        return CHIMP_FALSE;
    }
    task->flags |= CHIMP_TASK_FLAG_READY;
    pthread_cond_broadcast (&task->flags_cond);
    CHIMP_TASK_UNLOCK(task);
    return CHIMP_TRUE;
}

void
chimp_task_main_delete ()
{
    ChimpTaskInternal *task = CHIMP_CURRENT_TASK;
    CHIMP_TASK_LOCK(task);
    chimp_task_cleanup(task);
}

void
chimp_task_ref (ChimpTaskInternal *task)
{
    CHIMP_TASK_LOCK(task);
    task->refs++;
    CHIMP_TASK_UNLOCK(task);
}

void
chimp_task_unref (ChimpTaskInternal *task)
{
    CHIMP_TASK_LOCK(task);
    task->refs--;
    CHIMP_TASK_UNLOCK(task);
}

chimp_bool_t
chimp_task_send (ChimpRef *self, ChimpRef *value)
{
    ChimpMsgInternal *msg;
    ChimpTaskInternal *task = CHIMP_TASK(self)->priv;

    msg = chimp_msg_pack (value);
    if (msg == NULL) {
        return CHIMP_FALSE;
    }

    CHIMP_TASK_LOCK(task);
    while ((CHIMP_TASK(self)->local && CHIMP_TASK_IS_OUTBOX_FULL(task)) ||
            (!CHIMP_TASK(self)->local && CHIMP_TASK_IS_INBOX_FULL(task))) {
        if (pthread_cond_wait (&task->flags_cond, &task->lock) != 0) {
            CHIMP_FREE(msg);
            CHIMP_TASK_UNLOCK(task);
            return CHIMP_FALSE;
        }
    }
    if (!CHIMP_TASK(self)->local) {
        task->inbox = msg;
        task->flags |= CHIMP_TASK_FLAG_INBOX_FULL;
    }
    else {
        task->outbox = msg;
        task->flags |= CHIMP_TASK_FLAG_OUTBOX_FULL;
    }
    if (pthread_cond_signal (&task->flags_cond) != 0) {
        CHIMP_TASK_UNLOCK(task);
        return CHIMP_FALSE;
    }
    CHIMP_TASK_UNLOCK(task);
    return CHIMP_TRUE;
}

ChimpRef *
chimp_task_recv (ChimpRef *self)
{
    ChimpMsgInternal *msg;
    ChimpRef *value;
    ChimpTaskInternal *task = CHIMP_TASK(self)->priv;

    CHIMP_TASK_LOCK(task);
    while ((!CHIMP_TASK(self)->local && !CHIMP_TASK_IS_OUTBOX_FULL(task)) ||
            (CHIMP_TASK(self)->local && !CHIMP_TASK_IS_INBOX_FULL(task))) {
        if (pthread_cond_wait (&task->flags_cond, &task->lock) != 0) {
            CHIMP_TASK_UNLOCK(task);
            return NULL;
        }
    }
    if (CHIMP_TASK(self)->local) {
        msg = task->inbox;
        task->inbox = NULL;
        task->flags &= ~CHIMP_TASK_FLAG_INBOX_FULL;
    }
    else {
        msg = task->outbox;
        task->outbox = NULL;
        task->flags &= ~CHIMP_TASK_FLAG_OUTBOX_FULL;
    }
    if (pthread_cond_signal (&task->flags_cond) != 0) {
        CHIMP_FREE (msg);
        CHIMP_TASK_UNLOCK(task);
        return NULL;
    }
    CHIMP_TASK_UNLOCK(task);

    value = chimp_msg_unpack (msg);
    CHIMP_FREE (msg);
    if (value == NULL) {
        return NULL;
    }
    return value;
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

    CHIMP_TASK_UNLOCK (task);
    chimp_vm_delete (task->vm);
    chimp_gc_delete (task->gc);
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

        task->refs--;
        if (task->refs == 0) {
            chimp_task_cleanup (task);
        }
        else {
            CHIMP_TASK_UNLOCK(task);
        }
    }
    else {
        CHIMP_TASK_UNLOCK(task);
    }
}

void
chimp_task_mark (ChimpGC *gc, ChimpTaskInternal *task)
{
    chimp_gc_mark_ref (gc, task->self);
    chimp_gc_mark_ref (gc, task->method);
    chimp_gc_mark_ref (gc, task->modules);
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

static ChimpRef *
_chimp_task_init (ChimpRef *self, ChimpRef *args)
{
    return self;
}

static void
_chimp_task_dtor (ChimpRef *self)
{
    CHIMP_TASK_LOCK(CHIMP_TASK(self)->priv);
    CHIMP_TASK(self)->priv->refs--;
    if (CHIMP_TASK(self)->priv->refs == 0) {
        chimp_task_cleanup (CHIMP_TASK(self)->priv);
    }
    else {
        CHIMP_TASK_UNLOCK(CHIMP_TASK(self)->priv);
    }
}

static ChimpRef *
_chimp_task_send (ChimpRef *self, ChimpRef *args)
{
    return chimp_task_send (self, CHIMP_ARRAY_ITEM(args, 0)) ? chimp_true : chimp_false;
}

static ChimpRef *
_chimp_task_recv (ChimpRef *self, ChimpRef *args)
{
    return chimp_task_recv (self);
}

static ChimpRef *
_chimp_task_join (ChimpRef *self, ChimpRef *args)
{
    chimp_task_join (CHIMP_TASK(self)->priv);
    return chimp_nil;
}

static ChimpRef *
_chimp_task_str (ChimpRef *self)
{
    return chimp_str_new_format ("<task id:0x%X>", CHIMP_TASK(self)->priv);
}

chimp_bool_t
chimp_task_class_bootstrap (void)
{
    chimp_task_class =
        chimp_class_new (CHIMP_STR_NEW("task"), chimp_object_class);
    if (chimp_task_class == NULL) {
        return CHIMP_FALSE;
    }
    CHIMP_CLASS(chimp_task_class)->init = _chimp_task_init;
    CHIMP_CLASS(chimp_task_class)->dtor = _chimp_task_dtor;
    CHIMP_CLASS(chimp_task_class)->str  = _chimp_task_str;
    CHIMP_CLASS(chimp_task_class)->inst_type = CHIMP_VALUE_TYPE_TASK;
    chimp_gc_make_root (NULL, chimp_array_class);
    chimp_class_add_native_method (chimp_task_class, "send", _chimp_task_send);
    chimp_class_add_native_method (chimp_task_class, "recv", _chimp_task_recv);
    chimp_class_add_native_method (chimp_task_class, "join", _chimp_task_join);
    return CHIMP_TRUE;
}

