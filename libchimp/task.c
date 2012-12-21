#include <pthread.h>

#include <stddef.h>
#include <inttypes.h>
#include <errno.h>

#include "chimp/gc.h"
#include "chimp/task.h"
#include "chimp/object.h"
#include "chimp/array.h"
#include "chimp/frame.h"
#include "chimp/vm.h"

/* XXX this stuff leaks like a sieve */

ChimpRef *chimp_task_class = NULL;
ChimpTaskInternal *main_task = NULL;

static pthread_once_t current_task_key_once = PTHREAD_ONCE_INIT;

enum {
    CHIMP_TASK_FLAG_MAIN        = 0x01,
    CHIMP_TASK_FLAG_READY       = 0x02,
    CHIMP_TASK_FLAG_INBOX_FULL  = 0x08,
    CHIMP_TASK_FLAG_DONE        = 0x80,
};

#define CHIMP_TASK_IS_MAIN(task) \
    ((((task)->flags) & CHIMP_TASK_FLAG_MAIN) == CHIMP_TASK_FLAG_MAIN)

#define CHIMP_TASK_IS_READY(task) \
    ((((task)->flags) & CHIMP_TASK_FLAG_READY) == CHIMP_TASK_FLAG_READY)

#define CHIMP_TASK_IS_DONE(task) \
    ((((task)->flags) & CHIMP_TASK_FLAG_DONE) == CHIMP_TASK_FLAG_DONE)

#define CHIMP_TASK_IS_INBOX_FULL(task) \
    ((((task)->flags) & CHIMP_TASK_FLAG_INBOX_FULL) \
        == CHIMP_TASK_FLAG_INBOX_FULL)

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
    ChimpTaskInternal *next;
    pthread_cond_t     flags_cond;
    int                flags;
    pthread_t          thread;
    pthread_mutex_t    lock;
    int                refs;
    ChimpMsgInternal  *inbox;
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

static ChimpRef *
chimp_task_new_local (ChimpTaskInternal *task)
{
    ChimpRef *taskobj = chimp_class_new_instance (chimp_task_class, NULL);
    if (taskobj == NULL) {
        return NULL;
    }
    CHIMP_TASK(taskobj)->priv = task;
    CHIMP_TASK(taskobj)->local = CHIMP_TRUE;
    task->refs++;
    return taskobj;
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

    /* take an extra ref to keep the task around after the GC dies */
    task->refs++;

    if (task->method != NULL) {
        ChimpRef *args;
        ChimpRef *taskobj = chimp_task_new_local (task);
        if (taskobj == NULL) {
            /* XXX we already have a lock, unref has its own locking code */
            chimp_task_unref (task);
            return NULL;
        }
        task->self = taskobj;
        args = chimp_array_new ();
        CHIMP_TASK_UNLOCK(task);
        if (chimp_vm_invoke (task->vm, task->method, args) == NULL) {
            chimp_task_unref (task);
            return NULL;
        }
    }

    /************************************************************************
     *                                                                      *
     * Cleaning up the VM and GC is a-OK here because the task is finished. *
     *                                                                      *
     * We can't kill the mutex nor free the internal task, because it might *
     * still be in use by other tasks.                                      *
     *                                                                      *
     * Bonus: this cleans up circular references & our 'self' ref.          *
     *                                                                      *
     ************************************************************************/

    chimp_vm_delete (task->vm);
    task->vm = NULL;
    chimp_gc_delete (task->gc);
    task->gc = NULL;

    CHIMP_TASK_LOCK(task);
    if (task->inbox != NULL) {
        CHIMP_FREE (task->inbox);
        task->inbox = NULL;
    }
    task->inbox = NULL;

    /* XXX pretty much a copy/paste of chimp_task_unref without locking crap */
    if (task->refs > 0) {
        task->refs--;
        if (task->refs == 0) {
            CHIMP_TASK_UNLOCK(task);
            chimp_task_cleanup (task);
            return NULL;
        }
    }

    task->flags |= CHIMP_TASK_FLAG_DONE;
    pthread_cond_broadcast (&task->flags_cond);
    CHIMP_TASK_UNLOCK(task);

    return NULL;
}

ChimpRef *
chimp_task_new (ChimpRef *callable)
{
    ChimpRef *taskobj;
    pthread_attr_t attrs;
    ChimpTaskInternal *task = CHIMP_MALLOC(ChimpTaskInternal, sizeof(*task));
    if (task == NULL) {
        return NULL;
    }
    memset (task, 0, sizeof (*task));

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
    if (pthread_attr_init (&attrs) != 0) {
        pthread_cond_destroy (&task->flags_cond);
        pthread_mutex_destroy (&task->lock);
        return NULL;
    }
    if (pthread_attr_setdetachstate (&attrs, PTHREAD_CREATE_DETACHED) != 0) {
        pthread_attr_destroy (&attrs);
        pthread_cond_destroy (&task->flags_cond);
        pthread_mutex_destroy (&task->lock);
        return NULL;
    }
    CHIMP_TASK_LOCK(task);
    if (pthread_create (&task->thread, &attrs, chimp_task_thread_func, task) != 0) {
        pthread_attr_destroy (&attrs);
        pthread_cond_destroy (&task->flags_cond);
        pthread_mutex_destroy (&task->lock);
        CHIMP_FREE (task);
        return NULL;
    }
    pthread_attr_destroy (&attrs);

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

    /* create a heap-local handle for this task */
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
    main_task = task;
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
    task->self = chimp_task_new_local (task);
    if (task->self == NULL) {
        chimp_vm_delete (task->vm);
        chimp_gc_delete (task->gc);
        CHIMP_FREE (task);
        return CHIMP_FALSE;
    }
    task->flags |= CHIMP_TASK_FLAG_READY;
    /* TODO init task->self */
    pthread_cond_broadcast (&task->flags_cond);
    CHIMP_TASK_UNLOCK(task);
    return CHIMP_TRUE;
}

void
chimp_task_main_delete ()
{
    ChimpTaskInternal *task = CHIMP_CURRENT_TASK;
    if (task != NULL) {
        chimp_task_unref (task);
        main_task = NULL;
        pthread_setspecific (current_task_key, NULL);
    }
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
    if (task->refs > 0) {
        task->refs--;
        if (task->refs == 0) {
            /* last ref: safe to unlock aggressively */
            CHIMP_TASK_UNLOCK(task);
            chimp_task_cleanup (task);
            return;
        }
    }
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

    if (CHIMP_TASK(self)->local) {
        chimp_bug (__FILE__, __LINE__, "cannot send using local task object");
        CHIMP_FREE (msg);
        return CHIMP_FALSE;
    }
    else if (CHIMP_TASK_IS_DONE(task)) {
        /* this is *not* a bug: we can fail gracefully if recipient died */
        CHIMP_FREE (msg);
        return CHIMP_FALSE;
    }

    while (CHIMP_TASK_IS_INBOX_FULL(task)) {
        if (pthread_cond_wait (&task->flags_cond, &task->lock) != 0) {
            CHIMP_FREE(msg);
            CHIMP_TASK_UNLOCK(task);
            return CHIMP_FALSE;
        }
    }
    task->inbox = msg;
    task->flags |= CHIMP_TASK_FLAG_INBOX_FULL;
    if (pthread_cond_broadcast (&task->flags_cond) != 0) {
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

    if (!CHIMP_TASK(self)->local) {
        chimp_bug (__FILE__, __LINE__,
            "cannot recv from a non-local task object");
        return NULL;
    }
    else if (CHIMP_TASK_IS_DONE(task)) {
        chimp_bug (__FILE__, __LINE__,
            "attempt to recv in a task marked DONE");
        return NULL;
    }

    while (!CHIMP_TASK_IS_DONE(task) && !CHIMP_TASK_IS_INBOX_FULL(task)) {
        if (pthread_cond_wait (&task->flags_cond, &task->lock) != 0) {
            CHIMP_TASK_UNLOCK(task);
            return NULL;
        }
    }
    if (CHIMP_TASK_IS_DONE(task)) {
        CHIMP_TASK_UNLOCK(task);
        return NULL;
    }
    msg = task->inbox;
    task->inbox = NULL;
    task->flags &= ~CHIMP_TASK_FLAG_INBOX_FULL;
    if (pthread_cond_broadcast (&task->flags_cond) != 0) {
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
    /* NOTE: we assume the task lock is held here */

    if (task->vm != NULL) {
        chimp_vm_delete (task->vm);
        task->vm = NULL;
    }
    if (task->gc != NULL) {
        chimp_gc_delete (task->gc);
        task->gc = NULL;
    }
    if (task->inbox != NULL) {
        CHIMP_FREE (task->inbox);
        task->inbox = NULL;
    }
    pthread_cond_destroy (&task->flags_cond);
    pthread_mutex_destroy (&task->lock);
    CHIMP_FREE (task);
}

static void
chimp_task_join (ChimpTaskInternal *task)
{
    CHIMP_TASK_LOCK(task);
    if (!CHIMP_TASK_IS_MAIN(task)) {
        while (!CHIMP_TASK_IS_DONE(task)) {
            if (pthread_cond_wait (&task->flags_cond, &task->lock) != 0) {
                CHIMP_TASK_UNLOCK(task);
                return;
            }
        }
    }
    CHIMP_TASK_UNLOCK(task);
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

ChimpRef *
chimp_task_get_self (ChimpTaskInternal *task)
{
    return task->self;
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

    ChimpRef *ref = chimp_hash_get (main_task->modules, name);
    if (ref == chimp_nil) {
        return NULL;
    }
    return ref;
}

static ChimpRef *
_chimp_task_init (ChimpRef *self, ChimpRef *args)
{
    return self;
}

static void
_chimp_task_dtor (ChimpRef *self)
{
    chimp_task_unref (CHIMP_TASK(self)->priv);
}

static ChimpRef *
_chimp_task_send (ChimpRef *self, ChimpRef *args)
{
    return chimp_task_send (self, CHIMP_ARRAY_ITEM(args, 0)) ? chimp_true : chimp_false;
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
    chimp_class_add_native_method (chimp_task_class, "join", _chimp_task_join);
    return CHIMP_TRUE;
}

