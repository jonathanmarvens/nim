/*****************************************************************************
 *                                                                           *
 * Copyright 2013 Thomas Lee                                                 *
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

#include <ctype.h>

// XXX - Windows compatibility?
#include <unistd.h>

#include "nim/core.h"
#include "nim/gc.h"
#include "nim/object.h"
#include "nim/lwhash.h"
#include "nim/class.h"
#include "nim/str.h"
#include "nim/int.h"
#include "nim/float.h"
#include "nim/array.h"
#include "nim/method.h"
#include "nim/task.h"
#include "nim/hash.h"
#include "nim/frame.h"
#include "nim/ast.h"
#include "nim/code.h"
#include "nim/module.h"
#include "nim/modules.h"
#include "nim/symtable.h"
#include "nim/compile.h"
#include "nim/module_mgr.h"

#define NIM_BOOTSTRAP_CLASS_L1(gc, c, n, sup) \
    do { \
        nim_gc_make_root ((gc), (c)); \
        NIM_ANY(c)->klass = nim_class_class; \
        NIM_CLASS(c)->super = (sup); \
        NIM_CLASS(c)->name = nim_gc_new_object ((gc)); \
        NIM_ANY(NIM_CLASS(c)->name)->klass = nim_str_class; \
        NIM_STR(NIM_CLASS(c)->name)->data = fake_strndup ((n), (sizeof(n)-1)); \
        if (NIM_STR(NIM_CLASS(c)->name)->data == NULL) { \
            nim_task_main_delete (); \
            main_task = NULL; \
            return NIM_FALSE; \
        } \
        NIM_STR(NIM_CLASS(c)->name)->size = sizeof(n)-1; \
    } while (0)

#define NIM_BOOTSTRAP_CLASS_L2(gc, c) \
    do { \
    } while (0)

#define NIM_BUILTIN_METHOD(method, method_name) \
    do { \
      NimRef* temp = nim_method_new_native (NULL, method); \
      if (temp == NULL) { \
          return NIM_FALSE; \
      } \
      nim_hash_put_str (nim_builtins, method_name, temp); \
    } while (0)

NimRef *nim_object_class = NULL;
NimRef *nim_class_class = NULL;
NimRef *nim_str_class = NULL;
NimRef *nim_nil_class = NULL;
NimRef *nim_bool_class = NULL;

NimRef *nim_nil = NULL;
NimRef *nim_true = NULL;
NimRef *nim_false = NULL;
NimRef *nim_builtins = NULL;
NimRef *nim_module_path = NULL;

static NimRef *
_nim_module_make_path (const char *path);

/* XXX clean me up -- strndup is a GNU extension */
static char *
fake_strndup (const char *s, size_t len)
{
    char *buf = malloc (len + 1);
    if (buf == NULL) {
        return NULL;
    }
    memcpy (buf, s, len);
    buf[len] = '\0';
    return buf;
}

static NimCmpResult
nim_str_cmp (NimRef *a, NimRef *b)
{
    NimStr *as;
    NimStr *bs;
    int r;

    if (a == b) {
        return NIM_CMP_EQ;
    }

    if (NIM_ANY_CLASS(a) != nim_str_class) {
        return NIM_CMP_LT;
    }

    /* TODO should probably be a subtype check? */
    if (NIM_ANY_CLASS(a) != NIM_ANY_CLASS(b)) {
        return NIM_CMP_NOT_IMPL;
    }

    as = NIM_STR(a);
    bs = NIM_STR(b);

    if (as->size != bs->size) {
        r = strcmp (as->data, bs->data);
        if (r < 0) {
            return NIM_CMP_LT;
        }
        else if (r > 0) {
            return NIM_CMP_GT;
        }
        else {
            return NIM_CMP_EQ;
        }
    }

    r = memcmp (NIM_STR(a)->data, NIM_STR(b)->data, as->size);
    if (r < 0) {
        return NIM_CMP_LT;
    }
    else if (r > 0) {
        return NIM_CMP_GT;
    }
    else {
        return NIM_CMP_EQ;
    }
}

static void
_nim_object_mark (NimGC *gc, NimRef *self)
{
}

static NimRef *
_nim_object_str (NimRef *self)
{
    char buf[32];
    NimRef *name = NIM_CLASS_NAME(NIM_ANY_CLASS(self));
    NimRef *str;
    
    snprintf (buf, sizeof(buf), " @ %p>", self);
    str = nim_str_new_concat ("<", NIM_STR_DATA(name), buf, NULL);
    if (str == NULL) {
        return NULL;
    }
    return str;
}

static NimRef *
nim_bool_str (NimRef *self)
{
    if (self == nim_true) {
        return NIM_STR_NEW("true");
    }
    else if (self == nim_false) {
        return NIM_STR_NEW("false");
    }
    else {
        return NULL;
    }
}

static NimRef *
nim_str_str (NimRef *self)
{
    return self;
}

static NimRef *
_nim_object_getattr (NimRef *self, NimRef *name)
{
    /* TODO check NIM_ANY(self)->attributes ? */
    NimRef *class = NIM_ANY_CLASS(self);
    while (class != NULL) {
        NimLWHash *methods = NIM_CLASS(class)->methods;
        NimRef *method;
        if (nim_lwhash_get (methods, name, &method)) {
            if (method == NULL) {
                class = NIM_CLASS(class)->super;
                continue;
            }
            /* XXX binding on every access is probably dumb/slow */
            return nim_method_new_bound (method, self);
        }
        else break;
    }
    return NULL;
}

static NimRef *
nim_class_getattr (NimRef *self, NimRef *name)
{
    if (strcmp (NIM_STR_DATA(name), "name") == 0) {
        return NIM_CLASS(self)->name;
    }
    else if (strcmp (NIM_STR_DATA(name), "super") == 0) {
        if (NIM_CLASS(self)->super == NULL) {
            return nim_nil;
        }
        else {
            return NIM_CLASS(self)->super;
        }
    }
    else {
        while (self != NULL) {
            NimLWHash *methods = NIM_CLASS(self)->methods;
            NimRef *method;
            if (nim_lwhash_get (methods, name, &method)) {
                if (method == NULL) {
                    self = NIM_CLASS(self)->super;
                    continue;
                }
                return method;
            }
            else break;
        }
        return NULL;
    }
}

static NimRef *
nim_nil_str (NimRef *self)
{
    return NIM_CLASS_NAME(NIM_ANY_CLASS(self));
}

static NimRef *
_nim_task_recv (NimRef *self, NimRef *args)
{
    NimRef *result = nim_task_recv (nim_task_get_self (NIM_CURRENT_TASK));
    return result;
}

static NimRef *
_nim_task_self (NimRef *self, NimRef *args)
{
    return nim_task_get_self (NIM_CURRENT_TASK);
}

static NimRef *
_nim_array_range(NimRef* a, NimRef *args)
{
    int64_t start = 0;
    int64_t stop;
    int64_t step = 1;
    int64_t i;
    NimRef *result;

    switch (NIM_ARRAY(args)->size) {
        case 0:
            NIM_BUG("range(): not enough args");
            return NULL;
        case 1:
            if (!nim_method_parse_args (args, "I", &stop)) {
                return NULL;
            }
            break;
        case 2:
            if (!nim_method_parse_args (args, "II", &start, &stop)) {
                return NULL;
            }
            break;
        case 3:
            if (!nim_method_parse_args (args, "III", &start, &stop, &step)) {
                return NULL;
            }
            if (0 == step) {
                NIM_BUG("range(): step argument can't be zero.");
                return NULL;
            }
            break;
        default:
            NIM_BUG("range(): too many args.");
            return NULL;
    }

    result = nim_array_new();
    for (i = start; SIGN(step)*(stop - i) > 0; i += step) {
        if (!nim_array_push(result, nim_int_new(i))) {
            return NULL;
        }
    }
    return result;
}

static NimTaskInternal *main_task = NULL;

static NimRef *
_nim_compile (NimRef *self, NimRef *args)
{
    NimRef *name;
    NimRef *filename;
    
    if (NIM_ARRAY_SIZE(args) == 2) {
        name = NIM_ARRAY_ITEM(args, 0);
        filename = NIM_ARRAY_ITEM(args, 1);
    }
    else if (NIM_ARRAY_SIZE(args) == 1) {
        name = filename = NIM_ARRAY_ITEM(args, 0);
    }

    NimRef *module = nim_module_mgr_compile (name, filename);
    if (module == NULL) {
        fprintf (stderr, "error: failed to compile %s\n", NIM_STR_DATA(filename));
        return nim_nil;
    }
    return module;
}

static nim_bool_t
nim_core_init_builtins (void)
{
    nim_builtins = nim_hash_new ();
    if (nim_builtins == NULL) {
        return NIM_FALSE;
    }
    nim_gc_make_root (NULL, nim_builtins);

    /* TODO error checking */

    nim_hash_put_str (nim_builtins, "hash",   nim_hash_class);
    nim_hash_put_str (nim_builtins, "array",  nim_array_class);
    nim_hash_put_str (nim_builtins, "int",    nim_int_class);
    nim_hash_put_str (nim_builtins, "float",  nim_float_class);
    nim_hash_put_str (nim_builtins, "str",    nim_str_class);
    nim_hash_put_str (nim_builtins, "bool",   nim_bool_class);
    nim_hash_put_str (nim_builtins, "module", nim_module_class);
    nim_hash_put_str (nim_builtins, "object", nim_object_class);
    nim_hash_put_str (nim_builtins, "class",  nim_class_class);
    nim_hash_put_str (nim_builtins, "method", nim_method_class);
    nim_hash_put_str (nim_builtins, "error",  nim_error_class);

    NIM_BUILTIN_METHOD(_nim_task_recv, "recv");
    NIM_BUILTIN_METHOD(_nim_task_self, "self");
    NIM_BUILTIN_METHOD(_nim_array_range, "range");
    NIM_BUILTIN_METHOD(_nim_compile, "compile");

    return NIM_TRUE;
}

static void
_nim_class_mark (NimGC *gc, NimRef *self)
{
    NIM_SUPER (self)->mark (gc, self);

    nim_gc_mark_ref (gc, NIM_CLASS(self)->super);
    nim_gc_mark_ref (gc, NIM_CLASS(self)->name);
    nim_gc_mark_lwhash (gc, NIM_CLASS(self)->methods);
}

static NimRef *
_nim_str_add (NimRef *self, NimRef *other)
{
    if (NIM_ANY_CLASS(other) != nim_str_class) {
        NIM_BUG ("rhs of str `+` operator must also be a str");
        return NULL;
    }

    return nim_str_new_concat (
        NIM_STR_DATA(self), NIM_STR_DATA(other), NULL);
}

static NimRef *
_nim_str_nonzero (NimRef *self)
{
    return NIM_BOOL_REF(NIM_STR_SIZE(self) > 0);
}

static NimRef *
nim_bool_init (NimRef *self, NimRef *args)
{
    if (NIM_ARRAY_SIZE(args) > 0) {
        NimRef *arg;
        if (!nim_method_parse_args (args, "o", &arg)) {
            return NULL;
        }
        NimRef *klass = NIM_ANY_CLASS(arg);

        while (klass != NULL) {
            if (NIM_CLASS(klass)->nonzero != NULL) {
                return NIM_CLASS(klass)->nonzero (arg);
            }
            klass = NIM_CLASS(klass)->super;
        }
    }
    NIM_BUG("The type does not have a nonzero method");
    return NULL;
}

nim_bool_t
nim_core_startup (const char *path, void *stack_start)
{
    main_task = nim_task_new_main (stack_start);
    if (main_task == NULL) {
        return NIM_FALSE;
    }

    nim_object_class = nim_gc_new_object (NULL);
    nim_class_class  = nim_gc_new_object (NULL);
    nim_str_class    = nim_gc_new_object (NULL);

    NIM_BOOTSTRAP_CLASS_L1(NULL, nim_object_class, "object", NULL);
    NIM_CLASS(nim_object_class)->str = _nim_object_str;
    NIM_CLASS(nim_object_class)->mark = _nim_object_mark;
    NIM_CLASS(nim_object_class)->getattr = _nim_object_getattr;
    NIM_BOOTSTRAP_CLASS_L1(NULL, nim_class_class, "class", nim_object_class);
    NIM_CLASS(nim_class_class)->getattr = nim_class_getattr;
    NIM_CLASS(nim_class_class)->mark = _nim_class_mark;
    NIM_BOOTSTRAP_CLASS_L1(NULL, nim_str_class, "str", nim_object_class);
    NIM_CLASS(nim_str_class)->cmp = nim_str_cmp;
    NIM_CLASS(nim_str_class)->str = nim_str_str;
    NIM_CLASS(nim_str_class)->mark = _nim_object_mark;
    NIM_CLASS(nim_str_class)->add  = _nim_str_add;
    NIM_CLASS(nim_str_class)->nonzero  = _nim_str_nonzero;

    NIM_BOOTSTRAP_CLASS_L2(NULL, nim_object_class);
    NIM_BOOTSTRAP_CLASS_L2(NULL, nim_class_class);
    NIM_BOOTSTRAP_CLASS_L2(NULL, nim_str_class);

    if (!nim_error_class_bootstrap ()) goto error;

    if (!nim_method_class_bootstrap ()) goto error;

    if (!_nim_bootstrap_L3 ()) {
        return NIM_FALSE;
    }

    if (!nim_int_class_bootstrap ()) goto error;
    if (!nim_float_class_bootstrap ()) goto error;
    if (!nim_array_class_bootstrap ()) goto error;
    if (!nim_hash_class_bootstrap ()) goto error;

    nim_nil_class = nim_class_new (
        NIM_STR_NEW("nil"), nim_object_class, sizeof(NimAny));
    if (nim_nil_class == NULL) goto error;
    nim_gc_make_root (NULL, nim_nil_class);
    NIM_CLASS(nim_nil_class)->str = nim_nil_str;
    nim_nil = nim_class_new_instance (nim_nil_class, NULL);
    if (nim_nil == NULL) goto error;
    nim_gc_make_root (NULL, nim_nil);

    nim_bool_class = nim_class_new (
        NIM_STR_NEW("bool"), nim_object_class, sizeof(NimAny));
    if (nim_bool_class == NULL) goto error;
    nim_gc_make_root (NULL, nim_bool_class);
    NIM_CLASS(nim_bool_class)->str = nim_bool_str;

    nim_true = nim_class_new_instance (nim_bool_class, NULL);
    if (nim_true == NULL) goto error;
    nim_gc_make_root (NULL, nim_true);
    nim_false = nim_class_new_instance (nim_bool_class, NULL);
    if (nim_false == NULL) goto error;
    nim_gc_make_root (NULL, nim_false);

    NIM_CLASS(nim_bool_class)->init = nim_bool_init;

    if (!nim_frame_class_bootstrap ()) goto error;
    if (!nim_code_class_bootstrap ()) goto error;
    if (!nim_module_class_bootstrap ()) goto error;
    if (!nim_symtable_class_bootstrap ()) goto error;
    if (!nim_symtable_entry_class_bootstrap ()) goto error;
    if (!nim_task_class_bootstrap ()) goto error;
    if (!nim_var_class_bootstrap ()) goto error;

    /*
    if (!nim_task_push_frame (main_task)) goto error;
    */
    if (!nim_ast_class_bootstrap ()) goto error;

    if (!nim_core_init_builtins ()) goto error;

    if (!nim_str_class_init_2 ()) goto error;

    nim_module_path = _nim_module_make_path (path);
    if (nim_module_path == NULL)
        goto error;
    if (!nim_gc_make_root (NULL, nim_module_path))
        goto error;

    if (!nim_task_main_ready ()) goto error;

    if (!nim_module_mgr_init ()) goto error;

    return NIM_TRUE;

error:

    nim_task_main_delete ();
    main_task = NULL;
    return NIM_FALSE;
}

void
nim_core_shutdown (void)
{
    if (main_task != NULL) {
        nim_module_mgr_shutdown ();
        nim_task_main_delete ();
        main_task = NULL;
        nim_object_class = NULL;
        nim_class_class = NULL;
        nim_str_class = NULL;
    }
}

void
nim_bug (const char *filename, int lineno, const char *format, ...)
{
    va_list args;

    fprintf (stderr, "bug: %s:%d: ", filename, lineno);

    va_start (args, format);
    vfprintf (stderr, format, args);
    va_end (args);

    fprintf (stderr, ". aborting ...\n");

    abort ();
}

nim_bool_t
nim_is_builtin (NimRef *name)
{
    int rc;
    
    rc = nim_hash_get (nim_builtins, name, NULL);
    if (rc < 0) {
        NIM_BUG ("could not determine if %s is a builtin",
                    NIM_STR_DATA(name));
        return NIM_FALSE;
    }
    else if (rc > 0) {
        return NIM_FALSE;
    }
    else {
        return NIM_TRUE;
    }
}

static NimRef *
_nim_module_make_path (const char *path)
{
    size_t i, j;
    size_t len;
    NimRef *result = nim_array_new ();
    
    // We only add the current working directory if no paths were passed via the environment
    if (path == NULL) {
        char current_dir[FILENAME_MAX];
        if (getcwd(current_dir, sizeof(current_dir))) {
          NimRef *entry = nim_str_new(current_dir, sizeof(current_dir));
          nim_array_push(result, entry);
        }
        return result;
    }

    len = strlen (path);

    /* XXX this works, but this is totally crap */
    for (i = j = 0; i < len; j++) {
        if (path[j] == ':' || path[j] == '\0') {
            char *s;
            size_t l;
            NimRef *entry;

            l = j - i;
            if (l == 0) continue;

            s = fake_strndup (path + i, l);
            if (s == NULL) {
                return NULL;
            }

            entry = nim_str_new_take (s, l);
            if (entry == NULL) {
                free (s);
                return NULL;
            }
            if (!nim_array_push (result, entry)) {
                return NULL;
            }
            i = j + 1;
        }
    }

    return result;
}

static int 
_is_nim_float(NimRef* var)
{
    return (NIM_ANY_CLASS(var) == nim_float_class);
}

static int 
_is_nim_int(NimRef* var)
{
    return (NIM_ANY_CLASS(var) == nim_int_class);
}

static int 
_is_nim_num(NimRef* var)
{
    return (_is_nim_float(var) || _is_nim_int(var));
}

NimRef *
nim_num_add (NimRef *left, NimRef *right)
{
    // Check if both operands are numbers
    if (!(_is_nim_num(left) && _is_nim_num(right))) {
        return NULL;
    }

    // Check if one of the operands is float. This would mean promoting the
    // other operand to a float.
    if (_is_nim_float(left) || _is_nim_float(right)) {

        double left_value, right_value;

        left_value = _is_nim_float(left)?NIM_FLOAT(left)->value:NIM_INT(left)->value;
        right_value = _is_nim_float(right)?NIM_FLOAT(right)->value:NIM_INT(right)->value;
        return nim_float_new (left_value + right_value);
    }
    else {

        int left_value, right_value;

        left_value = NIM_INT(left)->value;
        right_value = NIM_INT(right)->value;
        return nim_int_new (left_value + right_value);
    }
}

NimRef *
nim_num_sub (NimRef *left, NimRef *right)
{
    if (!(_is_nim_num(left) && _is_nim_num(right))) {
        return NULL;
    }

    if (_is_nim_float(left) || _is_nim_float(right)) {

        double left_value, right_value;

        left_value = _is_nim_float(left)?NIM_FLOAT(left)->value:(double)NIM_INT(left)->value;
        right_value = _is_nim_float(right)?NIM_FLOAT(right)->value:(double)NIM_INT(right)->value;
        return nim_float_new (left_value - right_value);
    }
    else {

        int left_value, right_value;

        left_value = NIM_INT(left)->value;
        right_value = NIM_INT(right)->value;
        return nim_int_new (left_value - right_value);
    }
}

NimRef *
nim_num_mul (NimRef *left, NimRef *right)
{
    if (!(_is_nim_num(left) && _is_nim_num(right))) {
        return NULL;
    }

    if (_is_nim_float(left) || _is_nim_float(right)) {

        double left_value, right_value;

        left_value = _is_nim_float(left)?NIM_FLOAT(left)->value:NIM_INT(left)->value;
        right_value = _is_nim_float(right)?NIM_FLOAT(right)->value:NIM_INT(right)->value;
        return nim_float_new (left_value * right_value);
    }
    else {

        int left_value, right_value;

        left_value = NIM_INT(left)->value;
        right_value = NIM_INT(right)->value;
        return nim_int_new (left_value * right_value);
    }
}

NimRef *
nim_num_div (NimRef *left, NimRef *right)
{
    if (!(_is_nim_num(left) && _is_nim_num(right))) {
        return NULL;
    }

    if (_is_nim_float(left) || _is_nim_float(right)) {

        double left_value, right_value;

        left_value = _is_nim_float(left)?NIM_FLOAT(left)->value:NIM_INT(left)->value;
        right_value = _is_nim_float(right)?NIM_FLOAT(right)->value:NIM_INT(right)->value;
        return nim_float_new (left_value / right_value);
    }
    else {

        int left_value, right_value;

        left_value = NIM_INT(left)->value;
        right_value = NIM_INT(right)->value;
        return nim_int_new (left_value / right_value);
    }
}

