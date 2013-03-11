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

#include <pthread.h>

#include <stddef.h>
#include <inttypes.h>
#include <errno.h>
#include <signal.h>

#include "nim/gc.h"
#include "nim/task.h"
#include "nim/object.h"
#include "nim/array.h"
#include "nim/frame.h"
#include "nim/vm.h"

/* XXX this stuff leaks like a sieve */

NimRef *nim_task_class = NULL;
NimTaskInternal *main_task = NULL;

static pthread_once_t current_task_key_once = PTHREAD_ONCE_INIT;

enum {
    NIM_TASK_FLAG_MAIN        = 0x01,
    NIM_TASK_FLAG_READY       = 0x02,
    NIM_TASK_FLAG_INBOX_FULL  = 0x08,
    NIM_TASK_FLAG_DONE        = 0x80,
};

#define NIM_TASK_IS_MAIN(task) \
    ((((task)->flags) & NIM_TASK_FLAG_MAIN) == NIM_TASK_FLAG_MAIN)

#define NIM_TASK_IS_READY(task) \
    ((((task)->flags) & NIM_TASK_FLAG_READY) == NIM_TASK_FLAG_READY)

#define NIM_TASK_IS_DONE(task) \
    ((((task)->flags) & NIM_TASK_FLAG_DONE) == NIM_TASK_FLAG_DONE)

#define NIM_TASK_IS_INBOX_FULL(task) \
    ((((task)->flags) & NIM_TASK_FLAG_INBOX_FULL) \
        == NIM_TASK_FLAG_INBOX_FULL)

#define NIM_TASK_LOCK(task) \
    pthread_mutex_lock (&(task)->lock)

#define NIM_TASK_UNLOCK(task) \
    pthread_mutex_unlock (&(task)->lock)

static pthread_key_t current_task_key;

struct _NimTaskInternal {
    NimGC           *gc;
    NimVM           *vm;
    NimRef          *self;    /* NimTask */
    NimRef          *method;  /* NimMethod -- NULL for main */
    NimRef          *modules; /* NimHash */
    NimTaskInternal *next;
    pthread_cond_t     flags_cond;
    int                flags;
    pthread_t          thread;
    pthread_mutex_t    lock;
    int                refs;
    NimMsgInternal  *inbox;
};

static void
nim_task_cleanup (NimTaskInternal *task);

static void
nim_task_init_per_thread_key (void)
{
    pthread_key_create (&current_task_key, NULL);
}

static void
nim_task_init_per_thread_key_once (NimTaskInternal *task)
{
    pthread_once (&current_task_key_once, nim_task_init_per_thread_key);
    pthread_setspecific (current_task_key, task);
}

static NimRef *
nim_task_new_local (NimTaskInternal *task)
{
    NimRef *taskobj = nim_class_new_instance (nim_task_class, NULL);
    if (taskobj == NULL) {
        return NULL;
    }
    NIM_TASK(taskobj)->priv = task;
    NIM_TASK(taskobj)->local = NIM_TRUE;
    task->refs++;
    return taskobj;
}

static void *
nim_task_thread_func (void *arg)
{
    /* NOTE: task->lock will be held by nim_task_new at this point.    */
    /*       this will prevent other threads fiddling before we're ready */

    /* TODO better error handling */

    NimTaskInternal *task = (NimTaskInternal *) arg;

    /* printf ("[%p] started\n", task); */
    task->gc = nim_gc_new ((void *)&task);
    if (task->gc == NULL) {
        return NULL;
    }

    nim_task_init_per_thread_key_once (task);

    task->vm = nim_vm_new ();
    if (task->vm == NULL) {
        return NULL;
    }

    task->flags |= NIM_TASK_FLAG_READY;
    pthread_cond_broadcast (&task->flags_cond);

    /* take an extra ref to keep the task around after the GC dies */
    task->refs++;

    if (task->method != NULL) {
        NimRef *args;
        NimRef *taskobj = nim_task_new_local (task);
        if (taskobj == NULL) {
            /* XXX we already have a lock, unref has its own locking code */
            nim_task_unref (task);
            return NULL;
        }
        task->self = taskobj;
        NIM_TASK_UNLOCK(task);
        args = nim_task_recv (task->self);
        if (nim_vm_invoke (task->vm, task->method, args) == NULL) {
            nim_task_unref (task);
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

    nim_vm_delete (task->vm);
    task->vm = NULL;
    nim_gc_delete (task->gc);
    task->gc = NULL;

    NIM_TASK_LOCK(task);
    if (task->inbox != NULL) {
        NIM_FREE (task->inbox);
        task->inbox = NULL;
    }
    task->inbox = NULL;

    /* XXX pretty much a copy/paste of nim_task_unref without locking crap */
    if (task->refs > 0) {
        task->refs--;
        if (task->refs == 0) {
            NIM_TASK_UNLOCK(task);
            nim_task_cleanup (task);
            return NULL;
        }
    }

    task->flags |= NIM_TASK_FLAG_DONE;
    pthread_cond_broadcast (&task->flags_cond);
    NIM_TASK_UNLOCK(task);

    return NULL;
}

NimRef *
nim_task_new (NimRef *callable)
{
    NimRef *taskobj;
    pthread_attr_t attrs;
    NimTaskInternal *task = NIM_MALLOC(NimTaskInternal, sizeof(*task));
    if (task == NULL) {
        return NULL;
    }
    memset (task, 0, sizeof (*task));

    /* XXX can we guarantee callable won't be collected? think so ... */
    task->method = callable;
    task->flags = 0;
    /* !!! important to incref *BEFORE* we start the task thread !!! */
    /* (otherwise, short-lived tasks can prematurely kill the TaskInternal) */
    task->refs++;
    if (pthread_mutex_init (&task->lock, NULL) != 0) {
        NIM_FREE (task);
        return NULL;
    }
    if (pthread_cond_init (&task->flags_cond, NULL) != 0) {
        pthread_mutex_destroy (&task->lock);
        NIM_FREE (task);
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
    NIM_TASK_LOCK(task);
    if (pthread_create (&task->thread, &attrs, nim_task_thread_func, task) != 0) {
        pthread_attr_destroy (&attrs);
        pthread_cond_destroy (&task->flags_cond);
        pthread_mutex_destroy (&task->lock);
        NIM_FREE (task);
        return NULL;
    }
    pthread_attr_destroy (&attrs);

    /* XXX error handling code is probably all kinds of wrong/unsafe from
     *     this point, but meh.
     */
    while (!NIM_TASK_IS_READY(task)) {
        if (pthread_cond_wait (&task->flags_cond, &task->lock) != 0) {
            NIM_TASK_UNLOCK(task);
            pthread_cond_destroy (&task->flags_cond);
            pthread_mutex_destroy (&task->lock);
            NIM_FREE (task);
            return NULL;
        }
    }

    /* create a heap-local handle for this task */
    taskobj = nim_class_new_instance (nim_task_class, NULL);
    if (taskobj == NULL) {
        NIM_TASK_UNLOCK (task);
        pthread_cond_destroy (&task->flags_cond);
        pthread_mutex_destroy (&task->lock);
        NIM_FREE (task);
        return NULL;
    }
    NIM_TASK(taskobj)->priv = task;
    NIM_TASK(taskobj)->local = NIM_FALSE;
    NIM_TASK_UNLOCK(task);
    return taskobj;
}

NimRef *
nim_task_new_from_internal (NimTaskInternal *priv)
{
    NimRef *ref = nim_class_new_instance (nim_task_class, NULL);
    if (ref == NULL) {
        return NULL;
    }
    /* XXX blatant copy/paste from nim_task_new */
    NIM_TASK_LOCK(priv);
    while (!NIM_TASK_IS_READY(priv)) {
        if (pthread_cond_wait (&priv->flags_cond, &priv->lock) != 0) {
            NIM_TASK_UNLOCK(priv);
            return NULL;
        }
    }
    NIM_TASK(ref)->priv = priv;
    NIM_TASK(ref)->local = NIM_FALSE;
    priv->refs++;
    NIM_TASK_UNLOCK(priv);
    return ref;
}

NimTaskInternal *
nim_task_new_main (void *stack_start)
{
    sigset_t set;
    NimTaskInternal *task;
    
    sigemptyset (&set);
    sigaddset (&set, SIGPIPE);

    if (pthread_sigmask (SIG_BLOCK, &set, NULL) != 0) {
        NIM_BUG ("could not set signal mask on task thread");
        return NULL;
    }

    signal (SIGPIPE, SIG_IGN);

    task = NIM_MALLOC(NimTaskInternal, sizeof(*task));
    if (task == NULL) {
        return NULL;
    }

    main_task = task;
    memset (task, 0, sizeof (*task));
    task->flags = NIM_TASK_FLAG_MAIN;
    task->gc = nim_gc_new (stack_start);
    if (task->gc == NULL) {
        NIM_FREE (task);
        return NULL;
    }

    nim_task_init_per_thread_key_once (task);

    if (pthread_mutex_init (&task->lock, NULL) != 0) {
        nim_gc_delete (task->gc);
        nim_vm_delete (task->vm);
        NIM_FREE (task);
        return NULL;
    }
    if (pthread_cond_init (&task->flags_cond, NULL) != 0) {
        pthread_mutex_destroy (&task->lock);
        nim_gc_delete (task->gc);
        nim_vm_delete (task->vm);
        NIM_FREE (task);
        return NULL;
    }
    NIM_TASK_LOCK(task);
    return task;
}

nim_bool_t
nim_task_main_ready (void)
{
    /* NOTE: lock still held by nim_task_main_new here */

    NimTaskInternal *task = NIM_CURRENT_TASK;
    task->vm = nim_vm_new ();
    if (task->vm == NULL) {
        nim_gc_delete (task->gc);
        NIM_FREE (task);
        return NIM_FALSE;
    }
    task->self = nim_task_new_local (task);
    if (task->self == NULL) {
        nim_vm_delete (task->vm);
        nim_gc_delete (task->gc);
        NIM_FREE (task);
        return NIM_FALSE;
    }
    task->flags |= NIM_TASK_FLAG_READY;
    /* TODO init task->self */
    pthread_cond_broadcast (&task->flags_cond);
    NIM_TASK_UNLOCK(task);
    return NIM_TRUE;
}

void
nim_task_main_delete ()
{
    NimTaskInternal *task = NIM_CURRENT_TASK;
    if (task != NULL) {
        nim_task_unref (task);
        main_task = NULL;
        pthread_setspecific (current_task_key, NULL);
    }
}

void
nim_task_ref (NimTaskInternal *task)
{
    NIM_TASK_LOCK(task);
    task->refs++;
    NIM_TASK_UNLOCK(task);
}

void
nim_task_unref (NimTaskInternal *task)
{
    NIM_TASK_LOCK(task);
    if (task->refs > 0) {
        task->refs--;
        if (task->refs == 0) {
            /* last ref: safe to unlock aggressively */
            NIM_TASK_UNLOCK(task);
            nim_task_cleanup (task);
            return;
        }
    }
    NIM_TASK_UNLOCK(task);
}

nim_bool_t
nim_task_send (NimRef *self, NimRef *value)
{
    NimMsgInternal *msg;
    NimTaskInternal *task = NIM_TASK(self)->priv;

    msg = nim_msg_pack (value);
    if (msg == NULL) {
        return NIM_FALSE;
    }

    NIM_TASK_LOCK(task);

    if (NIM_TASK(self)->local) {
        NIM_BUG ("cannot send using local task object");
        NIM_FREE (msg);
        return NIM_FALSE;
    }
    else if (NIM_TASK_IS_DONE(task)) {
        /* this is *not* a bug: we can fail gracefully if recipient died */
        NIM_FREE (msg);
        return NIM_FALSE;
    }

    while (NIM_TASK_IS_INBOX_FULL(task)) {
        if (pthread_cond_wait (&task->flags_cond, &task->lock) != 0) {
            NIM_FREE(msg);
            NIM_TASK_UNLOCK(task);
            return NIM_FALSE;
        }
    }
    task->inbox = msg;
    task->flags |= NIM_TASK_FLAG_INBOX_FULL;
    if (pthread_cond_broadcast (&task->flags_cond) != 0) {
        NIM_TASK_UNLOCK(task);
        return NIM_FALSE;
    }
    NIM_TASK_UNLOCK(task);
    return NIM_TRUE;
}

NimRef *
nim_task_recv (NimRef *self)
{
    NimMsgInternal *msg;
    NimRef *value;
    NimTaskInternal *task;
    
    if (self == NULL) {
        self = nim_task_current ()->self;
    }

    task = NIM_TASK(self)->priv;

    NIM_TASK_LOCK(task);

    if (!NIM_TASK(self)->local) {
        NIM_BUG ("cannot recv from a non-local task object");
        return NULL;
    }
    else if (NIM_TASK_IS_DONE(task)) {
        NIM_BUG ("attempt to recv in a task marked DONE");
        return NULL;
    }

    while (!NIM_TASK_IS_DONE(task) && !NIM_TASK_IS_INBOX_FULL(task)) {
        if (pthread_cond_wait (&task->flags_cond, &task->lock) != 0) {
            NIM_TASK_UNLOCK(task);
            return NULL;
        }
    }
    if (NIM_TASK_IS_DONE(task)) {
        NIM_TASK_UNLOCK(task);
        return NULL;
    }
    msg = task->inbox;
    task->inbox = NULL;
    task->flags &= ~NIM_TASK_FLAG_INBOX_FULL;
    if (pthread_cond_broadcast (&task->flags_cond) != 0) {
        NIM_FREE (msg);
        NIM_TASK_UNLOCK(task);
        return NULL;
    }
    NIM_TASK_UNLOCK(task);

    value = nim_msg_unpack (msg);
    NIM_FREE (msg);
    if (value == NULL) {
        return NULL;
    }
    return value;
}

static void
nim_task_cleanup (NimTaskInternal *task)
{
    /* NOTE: we assume the task lock is held here */

    if (task->vm != NULL) {
        nim_vm_delete (task->vm);
        task->vm = NULL;
    }
    if (task->gc != NULL) {
        nim_gc_delete (task->gc);
        task->gc = NULL;
    }
    if (task->inbox != NULL) {
        NIM_FREE (task->inbox);
        task->inbox = NULL;
    }
    pthread_cond_destroy (&task->flags_cond);
    pthread_mutex_destroy (&task->lock);
    NIM_FREE (task);
}

void
nim_task_join (NimTaskInternal *task)
{
    NIM_TASK_LOCK(task);
    if (!NIM_TASK_IS_MAIN(task)) {
        while (!NIM_TASK_IS_DONE(task)) {
            if (pthread_cond_wait (&task->flags_cond, &task->lock) != 0) {
                NIM_TASK_UNLOCK(task);
                return;
            }
        }
    }
    NIM_TASK_UNLOCK(task);
}

static void
_nim_task_mark (NimGC *gc, NimRef *self)
{
    NimTaskInternal *task = NIM_TASK(self)->priv;
    nim_gc_mark_ref (gc, task->method);
    nim_gc_mark_ref (gc, task->modules);
}

void
nim_task_mark (NimGC *gc, NimTaskInternal *task)
{
    if (task->self != NULL) {
        nim_gc_mark_ref (gc, task->self);
    }
}

NimTaskInternal *
nim_task_current (void)
{
    return (NimTaskInternal *) pthread_getspecific (current_task_key);
}

NimTaskInternal *
nim_task_main (void)
{
    return main_task;
}

NimRef *
nim_task_get_self (NimTaskInternal *task)
{
    return task->self;
}

NimGC *
nim_task_get_gc (NimTaskInternal *task)
{
    if (task == NULL) task = NIM_CURRENT_TASK;

    return task->gc;
}

NimVM *
nim_task_get_vm (NimTaskInternal *task)
{
    if (task == NULL) task = NIM_CURRENT_TASK;

    return task->vm;
}

nim_bool_t
nim_task_add_module (NimTaskInternal *task, NimRef *module)
{
    if (task == NULL) {
        task = nim_task_current ();
    }

    if (task->modules == NULL) {
        task->modules = nim_hash_new ();
        if (task->modules == NULL) {
            return NIM_FALSE;
        }
        nim_gc_make_root (NULL, task->modules);
    }

    return nim_hash_put (task->modules, NIM_MODULE(module)->name, module);
}

NimRef *
nim_task_find_module (NimTaskInternal *task, NimRef *name)
{
    /* XXX modules are not really per-task atm. */

    NimRef *ref;
    int rc;

    rc = nim_hash_get (main_task->modules, name, &ref);
    if (rc < 0) {
        return NULL;
    }
    return ref;
}

static NimRef *
_nim_task_init (NimRef *self, NimRef *args)
{
    return self;
}

static void
_nim_task_dtor (NimRef *self)
{
    nim_task_unref (NIM_TASK(self)->priv);
}

static NimRef *
_nim_task_send (NimRef *self, NimRef *args)
{
    NimRef *arg;
    if (!nim_method_parse_args (args, "o", &arg)) {
        return NIM_FALSE;
    }
    return nim_task_send (self, NIM_ARRAY_ITEM(args, 0)) ? nim_true : nim_false;
}

static NimRef *
_nim_task_join (NimRef *self, NimRef *args)
{
    if (!nim_method_no_args (args)) {
        return NULL;
    }
    nim_task_join (NIM_TASK(self)->priv);
    return nim_nil;
}

static NimRef *
_nim_task_str (NimRef *self)
{
    return nim_str_new_format ("<task id:0x%X>", NIM_TASK(self)->priv);
}

static NimCmpResult
_nim_task_cmp (NimRef *self, NimRef *other)
{
    if (NIM_ANY_CLASS(self) == NIM_ANY_CLASS(other)) {
        if (NIM_TASK(self)->priv == NIM_TASK(other)->priv) {
            return NIM_CMP_EQ;
        }
        else if (NIM_TASK(self)->priv < NIM_TASK(other)->priv) {
            return NIM_CMP_LT;
        }
        else /* if (NIM_TASK(self)->priv > NIM_TASK(other)->priv) */ {
            return NIM_CMP_GT;
        }
    }
    else {
        return NIM_CMP_NOT_IMPL;
    }
}

nim_bool_t
nim_task_class_bootstrap (void)
{
    nim_task_class =
        nim_class_new (NIM_STR_NEW("task"), NULL, sizeof(NimTask));
    if (nim_task_class == NULL) {
        return NIM_FALSE;
    }
    NIM_CLASS(nim_task_class)->init = _nim_task_init;
    NIM_CLASS(nim_task_class)->dtor = _nim_task_dtor;
    NIM_CLASS(nim_task_class)->str  = _nim_task_str;
    NIM_CLASS(nim_task_class)->cmp  = _nim_task_cmp;
    NIM_CLASS(nim_task_class)->mark = _nim_task_mark;
    nim_gc_make_root (NULL, nim_task_class);
    nim_class_add_native_method (nim_task_class, "send", _nim_task_send);
    nim_class_add_native_method (nim_task_class, "join", _nim_task_join);
    return NIM_TRUE;
}

