#include <sys/stat.h>

#include "nim/module_mgr.h"
#include "nim/object.h"
#include "nim/compile.h"
#include "nim/array.h"
#include "nim/str.h"
#include "nim/task.h"
#include "nim/modules.h"

static NimRef *module_mgr_task = NULL;
static NimRef *func = NULL;
static NimRef *cache = NULL;
static NimRef *builtins = NULL;

#define IS_MODULE_MGR_TASK() \
    (nim_task_current() == NIM_TASK(module_mgr_task)->priv)

static nim_bool_t
_nim_module_mgr_add_builtin (NimRef *module)
{
    if (!nim_hash_put (
            builtins, NIM_MODULE_NAME(module), module)) {
        return NIM_FALSE;
    }
    return NIM_TRUE;
}

static NimRef *
_nim_module_mgr_compile (
    NimRef *name, NimRef *filename, nim_bool_t check)
{
    NimRef *mod;
    
    if (check) {
        if (nim_hash_get (cache, name, NULL) == 0) {
            NIM_BUG ("attempt to bind the same module twice: %s",
                    NIM_STR_DATA(name));
            return NULL;
        }
    }

    mod = nim_compile_file (name, NIM_STR_DATA(filename));
    if (mod == NULL) {
        return NULL;
    }
    if (!nim_hash_put (cache, name, mod)) {
        return NULL;
    }
    return mod;
}

static NimRef *
_nim_module_mgr_load (NimRef *name, NimRef *path)
{
    size_t i;
    int rc;
    NimRef *mod;

    /* TODO circular references ... ? */

    rc = nim_hash_get (cache, name, &mod);
    if (rc < 0) {
        NIM_BUG ("error looking up module %s in cache",
                NIM_STR_DATA(name));
        return NULL;
    }
    else if (rc == 0) {
        return mod;
    }

    if (path != NULL) {
        for (i = 0; i < NIM_ARRAY_SIZE(path); i++) {
            NimRef *element = NIM_ARRAY_ITEM(path, i);
            NimRef *filename = nim_str_new_format (
                "%s/%s.nim", NIM_STR_DATA(element), NIM_STR_DATA(name));
            struct stat st;

            if (stat (NIM_STR_DATA(filename), &st) != 0) {
                continue;
            }

            if (S_ISREG(st.st_mode)) {
                return _nim_module_mgr_compile (name, filename, NIM_FALSE);
            }
        }
    }

    rc = nim_hash_get (builtins, name, &mod);

    if (rc == 0) {
        if (!nim_hash_put (cache, name, mod)) {
            return NULL;
        }
        return mod;
    }
    else if (rc == 1) {
        NIM_BUG ("module not found: %s", NIM_STR_DATA(name));
        return NULL;
    }
    else {
        NIM_BUG ("bad key in builtin_modules?");
        return NULL;
    }
}

static NimRef *
_nim_module_mgr_func (NimRef *self, NimRef *args)
{
    cache = nim_hash_new ();
    if (cache == NULL) {
        return NULL;
    }
    nim_gc_make_root (NULL, cache);
    builtins = nim_hash_new ();
    if (builtins == NULL) {
        return NULL;
    }
    nim_gc_make_root (NULL, builtins);

    if (!_nim_module_mgr_add_builtin (nim_init_io_module ())) {
        return NIM_FALSE;
    }

    if (!_nim_module_mgr_add_builtin (nim_init_assert_module ())) {
        return NIM_FALSE;
    }

    if (!_nim_module_mgr_add_builtin (nim_init_unit_module ())) {
        return NIM_FALSE;
    }

    if (!_nim_module_mgr_add_builtin (nim_init_os_module ())) {
        return NIM_FALSE;
    }

    if (!_nim_module_mgr_add_builtin (nim_init_gc_module ())) {
        return NIM_FALSE;
    }

    if (!_nim_module_mgr_add_builtin (nim_init_net_module ())) {
        return NIM_FALSE;
    }

    if (!_nim_module_mgr_add_builtin (nim_init_http_module ())) {
        return NIM_FALSE;
    }

    if (!nim_task_send (
            NIM_ARRAY_ITEM(args, 0), NIM_STR_NEW ("ready"))) {
        return NULL;
    }

    for (;;) {
        NimRef *msg = nim_task_recv (NULL);
        if (msg == NULL) {
            return NULL;
        }

        if (NIM_ANY_CLASS(msg) == nim_array_class) {
            NimRef *action = NIM_ARRAY_ITEM(msg, 0);

            if (NIM_ANY_CLASS(action) != nim_str_class) {
                NIM_BUG ("module managed received unexpected action: %s",
                    NIM_STR_DATA(nim_object_str (action)));
                return NULL;
            }

            if (strcmp (NIM_STR_DATA(action), "load") == 0) {
                NimRef *sender = NIM_ARRAY_ITEM(msg, 1);
                NimRef *name = NIM_ARRAY_ITEM(msg, 2);
                NimRef *mod;
                mod = _nim_module_mgr_load (name, nim_module_path);
                if (mod == NULL) {
                    return NULL;
                }
                if (!nim_task_send (sender, mod)) {
                    return NULL;
                }
            }
            else if (strcmp (NIM_STR_DATA(action), "compile") == 0) {
                NimRef *sender = NIM_ARRAY_ITEM(msg, 1);
                NimRef *name = NIM_ARRAY_ITEM(msg, 2);
                NimRef *filename = NIM_ARRAY_ITEM(msg, 3);
                NimRef *mod =
                    _nim_module_mgr_compile (name, filename, NIM_TRUE);
                if (mod == NULL) {
                    return NULL;
                }
                if (!nim_task_send (sender, mod)) {
                    return NULL;
                }
            }
            else if (strcmp (NIM_STR_DATA(action), "exit") == 0) {
                break;
            }
            else {
                NIM_BUG ("module manager received unknown command: %s",
                    NIM_STR_DATA(nim_object_str (action)));
                return NULL;
            }
        }
        else {
            NIM_BUG ("module manager received unexpected message: %s",
                NIM_STR_DATA(nim_object_str (msg)));
            return NULL;
        }
    }

    return nim_nil;
}

nim_bool_t
nim_module_mgr_init (void)
{
    NimRef *self = nim_task_get_self (nim_task_current ());
    func = nim_method_new_native (NULL, _nim_module_mgr_func);
    if (func == NULL) {
        return NIM_FALSE;
    }
    nim_gc_make_root (NULL, func);
    module_mgr_task = nim_task_new (func);
    if (module_mgr_task == NULL) {
        return NIM_FALSE;
    }
    nim_gc_make_root (NULL, module_mgr_task);

    /* send args */
    if (!nim_task_send (
            module_mgr_task, nim_array_new_var (self, NULL))) {
        return NIM_FALSE;
    }
    /* XXX wait to be notified the module mgr task is ready */
    if (!nim_task_recv (NULL)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

void
nim_module_mgr_shutdown (void)
{
    if (!nim_task_send (
            module_mgr_task,
            nim_array_new_var (NIM_STR_NEW ("exit"), NULL))) {
        NIM_BUG ("failed to shutdown module manager task");
    }
    nim_task_join (NIM_TASK(module_mgr_task)->priv);

    module_mgr_task = NULL;
    func = NULL;
}

static NimRef *
_nim_module_mgr_send_cmd (
    const char *action, size_t actionlen, ...)
{
    va_list args;
    size_t num_args = 0;
    NimRef *cmd;
    NimRef *arg;
    NimRef *self = nim_task_get_self (nim_task_current ());
    
    va_start (args, actionlen);
    while ((arg = va_arg (args, NimRef *)) != NULL) {
        num_args++;
    }
    va_end (args);

    cmd = nim_array_new_with_capacity (2 + num_args);
    if (cmd == NULL) {
        return NULL;
    }

    if (!nim_array_push (cmd, nim_str_new (action, actionlen))) {
        return NULL;
    }
    if (!nim_array_push (cmd, self)) {
        return NULL;
    }

    va_start (args, actionlen);
    while ((arg = va_arg (args, NimRef *)) != NULL) {
        if (!nim_array_push (cmd, arg)) {
            return NULL;
        }
    }
    va_end (args);

    if (!nim_task_send (module_mgr_task, cmd)) {
        NIM_BUG ("failed to send %s request to module manager", action);
        return NULL;
    }

    return nim_task_recv (NULL);
}

NimRef *
nim_module_mgr_load (NimRef *name)
{
    if (IS_MODULE_MGR_TASK ()) {
        return _nim_module_mgr_load (name, nim_module_path);
    }
    else {
        return _nim_module_mgr_send_cmd ("load", sizeof("load")-1, name, NULL);
    }
}

NimRef *
nim_module_mgr_compile (NimRef *name, NimRef *filename)
{
    if (IS_MODULE_MGR_TASK ()) {
        return _nim_module_mgr_compile (name, filename, NIM_TRUE);
    }
    else {
        return _nim_module_mgr_send_cmd (
                "compile", sizeof("compile")-1, name, filename, NULL);
    }
}

