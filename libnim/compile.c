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

#include <sys/stat.h>
#include <sys/types.h>

#include "nim/compile.h"
#include "nim/str.h"
#include "nim/ast.h"
#include "nim/code.h"
#include "nim/array.h"
#include "nim/int.h"
#include "nim/method.h"
#include "nim/object.h"
#include "nim/task.h"
#include "nim/_parser.h"
#include "nim/symtable.h"
#include "nim/module_mgr.h"

typedef enum _NimUnitType {
    NIM_UNIT_TYPE_CODE,
    NIM_UNIT_TYPE_MODULE,
    NIM_UNIT_TYPE_CLASS
} NimUnitType;

typedef struct _NimCodeUnit {
    NimUnitType type;
    /* XXX not entirely sure why I made this a union. */
    union {
        NimRef *code;
        NimRef *module;
        NimRef *class;
        NimRef *value;
    };
    struct _NimCodeUnit *next;
    NimRef              *ste;
} NimCodeUnit;

/* track (possibly nested) loops so we can compute jumps for 'break' */

typedef struct _NimLoopStack {
    NimLabel *items;
    size_t      size;
    size_t      capacity;
} NimLoopStack;

typedef struct _NimCodeCompiler {
    NimCodeUnit  *current_unit;
    NimLoopStack  loop_stack;
    NimRef       *symtable;
} NimCodeCompiler;

/* the NimBind* structures are used for pattern matching */

typedef struct _NimBindPathItem {
    enum {
        NIM_BIND_PATH_ITEM_TYPE_SIMPLE,
        NIM_BIND_PATH_ITEM_TYPE_ARRAY,
        NIM_BIND_PATH_ITEM_TYPE_HASH
    } type;
    union {
        size_t    array_index;
        NimRef *hash_key;
    };
} NimBindPathItem;

#define MAX_BIND_PATH_SIZE 16

typedef struct _NimBindPath {
    NimBindPathItem items[16];
    size_t            size;
} NimBindPath;

typedef struct _NimBindVar {
    NimRef      *id;
    NimBindPath  path;
} NimBindVar;

#define MAX_BIND_VARS_SIZE 16

typedef struct _NimBindVars {
    NimBindVar    items[MAX_BIND_VARS_SIZE];
    size_t          size;
} NimBindVars;

#define NIM_BIND_PATH_ITEM_ARRAY(index) { NIM_BIND_PATH_ARRAY, (index) }
#define NIM_BIND_PATH_ITEM_HASH(key)    { NIM_BIND_PATH_HASH, (key) }

#define NIM_BIND_PATH_INIT(stack) memset ((stack), 0, sizeof(*(stack)))
#define NIM_BIND_PATH_PUSH_ARRAY(stack) \
    do { \
        if ((stack)->size >= MAX_BIND_PATH_SIZE) \
            NIM_BUG ("bindpath limit reached"); \
        (stack)->items[(stack)->size].type = NIM_BIND_PATH_ITEM_TYPE_ARRAY; \
        (stack)->items[(stack)->size].array_index = 0; \
        (stack)->size++; \
    } while (0)
#define NIM_BIND_PATH_PUSH_HASH(stack) \
    do { \
        if ((stack)->size >= MAX_BIND_PATH_SIZE) \
            NIM_BUG ("bindpath limit reached"); \
        (stack)->items[(stack)->size].type = NIM_BIND_PATH_ITEM_TYPE_HASH; \
        (stack)->items[(stack)->size].hash_key = NULL; \
        (stack)->size++; \
    } while (0)
#define NIM_BIND_PATH_POP(stack) (stack)->size--
#define NIM_BIND_PATH_ARRAY_INDEX(stack, i) \
    do { \
        if ((stack)->size == 0) { \
            NIM_BUG ("array index on empty bind path"); \
        } \
        if ((stack)->items[(stack)->size-1].type != NIM_BIND_PATH_ITEM_TYPE_ARRAY) { \
            NIM_BUG ("array index on path item that is not an array"); \
        } \
        (stack)->items[(stack)->size-1].array_index = (i); \
    } while (0)
#define NIM_BIND_PATH_HASH_KEY(stack, key) \
    do { \
        if ((stack)->size == 0) { \
            NIM_BUG ("hash key on empty bind path"); \
        } \
        if ((stack)->items[(stack)->size-1].type != NIM_BIND_PATH_ITEM_TYPE_HASH) { \
            NIM_BUG ("hash key on path item that is not a hash"); \
        } \
        (stack)->items[(stack)->size-1].hash_key = (key); \
    } while (0)
#define NIM_BIND_PATH_COPY(dest, src) memcpy ((dest), (src), sizeof(*dest));

#define NIM_BIND_VARS_INIT(vars) memset ((vars), 0, sizeof(*(vars)))
#define NIM_BIND_VARS_ADD(vars, id, p) \
    do { \
        if ((vars)->size >= MAX_BIND_VARS_SIZE) \
            NIM_BUG ("bindvar limit reached"); \
        (vars)->items[(vars)->size].id = id; \
        memcpy (&(vars)->items[(vars)->size].path, (p), sizeof(*p)); \
        /* XXX HACK path of zero elements means a top-level binding */ \
        if ((p)->size == 0) { \
            (vars)->items[(vars)->size].path.size++; \
            (vars)->items[(vars)->size].path.items[0].type = \
                NIM_BIND_PATH_ITEM_TYPE_SIMPLE; \
        } \
        (vars)->size++; \
    } while (0)

#define NIM_COMPILER_CODE(c) ((c)->current_unit)->code
#define NIM_COMPILER_MODULE(c) ((c)->current_unit)->module
#define NIM_COMPILER_CLASS(c) ((c)->current_unit)->class
#define NIM_COMPILER_SCOPE(c) ((c)->current_unit)->value

#define NIM_COMPILER_IN_MODULE(c) (((c)->current_unit)->type == NIM_UNIT_TYPE_MODULE)
#define NIM_COMPILER_IN_CODE(c) (((c)->current_unit)->type == NIM_UNIT_TYPE_CODE)
#define NIM_COMPILER_IN_CLASS(c) (((c)->current_unit)->type == NIM_UNIT_TYPE_CLASS)
#define NIM_COMPILER_SYMTABLE_ENTRY(c) (((c)->current_unit))->ste

static nim_bool_t
nim_compile_ast_decl (NimCodeCompiler *c, NimRef *decl);

static nim_bool_t
nim_compile_ast_stmt (NimCodeCompiler *c, NimRef *stmt);

static nim_bool_t
nim_compile_ast_expr (NimCodeCompiler *c, NimRef *expr);

static nim_bool_t
nim_compile_ast_expr_ident (NimCodeCompiler *c, NimRef *expr);

static nim_bool_t
nim_compile_ast_expr_array (NimCodeCompiler *c, NimRef *expr);

static nim_bool_t
nim_compile_ast_expr_hash (NimCodeCompiler *c, NimRef *expr);

static nim_bool_t
nim_compile_ast_expr_str (NimCodeCompiler *c, NimRef *expr);

static nim_bool_t
nim_compile_ast_expr_int_ (NimCodeCompiler *c, NimRef *expr);

static nim_bool_t
nim_compile_ast_expr_float_ (NimCodeCompiler *c, NimRef *expr);

static nim_bool_t
nim_compile_ast_expr_bool (NimCodeCompiler *c, NimRef *expr);

static nim_bool_t
nim_compile_ast_expr_call (NimCodeCompiler *c, NimRef *expr);

static nim_bool_t
nim_compile_ast_expr_fn (NimCodeCompiler *c, NimRef *expr);

static nim_bool_t
nim_compile_ast_expr_spawn (NimCodeCompiler *c, NimRef *expr);

static nim_bool_t
nim_compile_ast_expr_not (NimCodeCompiler *c, NimRef *expr);

static nim_bool_t
nim_compile_ast_expr_getattr (NimCodeCompiler *c, NimRef *expr);

static nim_bool_t
nim_compile_ast_expr_getitem (NimCodeCompiler *c, NimRef *expr);

static nim_bool_t
nim_compile_ast_expr_binop (NimCodeCompiler *c, NimRef *binop);

static nim_bool_t
nim_compile_ast_stmt_pattern_test (
    NimCodeCompiler *c,
    NimRef *test,
    NimBindPath *path,
    NimBindVars *vars,
    NimLabel *next_label
);

static void
nim_code_compiler_cleanup (NimCodeCompiler *c)
{
    while (c->current_unit != NULL) {
        NimCodeUnit *unit = c->current_unit;
        NimCodeUnit *next = unit->next;
        NIM_FREE (unit);
        unit = next;
        c->current_unit = NULL;
    }
    if (c->loop_stack.items != NULL) {
        free (c->loop_stack.items);
        c->loop_stack.items = NULL;
    }
}

static NimLabel *
nim_code_compiler_begin_loop (NimCodeCompiler *c)
{
    NimLoopStack *stack = &c->loop_stack;
    if (stack->capacity < stack->size + 1) {
        const size_t new_capacity =
            (stack->capacity == 0 ? 8 : stack->capacity * 2);
        NimLabel *labels = realloc (
            stack->items, new_capacity * sizeof(*stack->items));
        if (labels == NULL) {
            return NULL;
        }
        stack->items = labels;
        stack->capacity = new_capacity;
    }
    /* TODO NIM_LABEL_INIT (rename macro to NIM_LABEL_INIT_STATIC) */
    memset (stack->items + stack->size, 0, sizeof(*stack->items));
    stack->size++;
    return stack->items + (stack->size - 1);
}

static void
nim_code_compiler_end_loop (NimCodeCompiler *c)
{
    NimLoopStack *stack = &c->loop_stack;
    if (stack->size == 0) {
        NIM_BUG ("end_loop with zero stack size");
        return;
    }
    /* TODO ensure we're in a code block */
    stack->size--;
    nim_code_use_label (NIM_COMPILER_CODE(c), stack->items + stack->size);
}

static NimLabel *
nim_code_compiler_get_loop_label (NimCodeCompiler *c)
{
    NimLoopStack *stack = &c->loop_stack;
    if (stack->size == 0) {
        NIM_BUG ("get_loop_label with zero stack size");
        return NULL;
    }
    return &stack->items[stack->size-1];
}

static NimRef *
nim_code_compiler_push_unit (
    NimCodeCompiler *c, NimUnitType type, NimRef *scope, NimRef *value)
{
    NimCodeUnit *unit = NIM_MALLOC(NimCodeUnit, sizeof(*unit));
    if (unit == NULL) {
        return NULL;
    }
    memset (unit, 0, sizeof(*unit));
    switch (type) {
        case NIM_UNIT_TYPE_CODE:
            unit->code = nim_code_new ();
            break;
        case NIM_UNIT_TYPE_MODULE:
            unit->module = nim_module_new (nim_nil, NULL);
            break;
        case NIM_UNIT_TYPE_CLASS:
            /* XXX the value parameter is a total hack */
            unit->class = value;
            break;
        default:
            NIM_BUG ("unknown unit type: %d", type);
            NIM_FREE (unit);
            return NULL;
    };
    if (unit->value == NULL) {
        NIM_FREE (unit);
        return NULL;
    }
    unit->type = type;
    unit->next = c->current_unit;
    unit->ste = nim_symtable_lookup (c->symtable, scope);
    if (unit->ste == NULL) {
        NIM_FREE (unit);
        NIM_BUG ("symtable lookup error for scope %p", scope);
        return NULL;
    }
    if (unit->ste == nim_nil) {
        NIM_FREE (unit);
        NIM_BUG ("symtable lookup failed for scope %p", scope);
        return NULL;
    }
    c->current_unit = unit;
    return unit->value;
}

inline static NimRef *
nim_code_compiler_push_code_unit (NimCodeCompiler *c, NimRef *scope)
{
    return nim_code_compiler_push_unit (c, NIM_UNIT_TYPE_CODE, scope, NULL);
}

inline static NimRef *
nim_code_compiler_push_module_unit (NimCodeCompiler *c, NimRef *scope)
{
    return nim_code_compiler_push_unit (c, NIM_UNIT_TYPE_MODULE, scope, NULL);
}

inline static NimRef *
nim_code_compiler_push_class_unit (NimCodeCompiler *c, NimRef *scope, NimRef *klass)
{
    return nim_code_compiler_push_unit (c, NIM_UNIT_TYPE_CLASS, scope, klass);
}

static NimRef *
nim_code_compiler_pop_unit (NimCodeCompiler *c, NimUnitType expected)
{
    NimCodeUnit *unit = c->current_unit;
    if (unit == NULL) {
        NIM_BUG ("NULL code unit?");
        return NULL;
    }
    if (unit->type != expected) {
        NIM_BUG ("unexpected unit type!");
        return NULL;
    }
    c->current_unit = unit->next;
    NIM_FREE(unit);
    if (c->current_unit != NULL) {
        return c->current_unit->value;
    }
    else {
        return nim_true;
    }
}

inline static NimRef *
nim_code_compiler_pop_code_unit (NimCodeCompiler *c)
{
    return nim_code_compiler_pop_unit (c, NIM_UNIT_TYPE_CODE);
}

inline static NimRef *
nim_code_compiler_pop_module_unit (NimCodeCompiler *c)
{
    return nim_code_compiler_pop_unit (c, NIM_UNIT_TYPE_MODULE);
}

inline static NimRef *
nim_code_compiler_pop_class_unit (NimCodeCompiler *c)
{
    return nim_code_compiler_pop_unit (c, NIM_UNIT_TYPE_CLASS);
}

static nim_bool_t
nim_compile_ast_stmts (NimCodeCompiler *c, NimRef *stmts)
{
    size_t i;

    for (i = 0; i < NIM_ARRAY_SIZE(stmts); i++) {
        if (NIM_ANY_CLASS(NIM_ARRAY_ITEM(stmts, i)) == nim_ast_stmt_class) {
            if (!nim_compile_ast_stmt (c, NIM_ARRAY_ITEM(stmts, i))) {
                /* TODO error message? */
                return NIM_FALSE;
            }
        }
        else if (NIM_ANY_CLASS(NIM_ARRAY_ITEM(stmts, i)) == nim_ast_decl_class) {
            if (!nim_compile_ast_decl (c, NIM_ARRAY_ITEM(stmts, i))) {
                return NIM_FALSE;
            }
        }
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_decls (NimCodeCompiler *c, NimRef *decls)
{
    size_t i;

    for (i = 0; i < NIM_ARRAY_SIZE(decls); i++) {
        if (!nim_compile_ast_decl (c, NIM_ARRAY_ITEM(decls, i))) {
            /* TODO error message? */
            return NIM_FALSE;
        }
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_mod (NimCodeCompiler *c, NimRef *mod)
{
    NimRef *uses = NIM_AST_MOD(mod)->root.uses;
    NimRef *body = NIM_AST_MOD(mod)->root.body;

    if (!nim_compile_ast_decls (c, uses)) {
        return NIM_FALSE;
    }

    /* TODO check NIM_AST_MOD_* type */
    if (!nim_compile_ast_decls (c, body)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_stmt_assign (NimCodeCompiler *c, NimRef *stmt)
{
    NimRef *name =
        NIM_AST_EXPR(NIM_AST_STMT(stmt)->assign.target)->ident.id;
    NimRef *code = NIM_COMPILER_CODE(c);

    if (!nim_compile_ast_expr (c, NIM_AST_STMT(stmt)->assign.value)) {
        return NIM_FALSE;
    }

    if (!nim_code_storename (code, name)) {
        return NIM_FALSE;
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_stmt_while_ (NimCodeCompiler *c, NimRef *stmt)
{
    NimRef *code = NIM_COMPILER_CODE(c);
    NimLabel start_body = NIM_LABEL_INIT;
    NimLabel *end_body = NULL;
    
    nim_code_use_label (code, &start_body);

    if (!nim_compile_ast_expr (c, NIM_AST_STMT(stmt)->while_.expr))
        goto error;

    end_body = nim_code_compiler_begin_loop (c);

    if (!nim_code_jumpiffalse (code, end_body))
        goto error;

    if (!nim_compile_ast_stmts (c, NIM_AST_STMT(stmt)->while_.body))
        goto error;

    if (!nim_code_jump (code, &start_body))
        goto error;

    nim_code_compiler_end_loop (c);

    return NIM_TRUE;

error:
    nim_label_free (&start_body);
    if (end_body != NULL) {
        nim_code_compiler_end_loop (c);
    }
    return NIM_FALSE;
}

static nim_bool_t
nim_compile_ast_stmt_array_pattern_test (
    NimCodeCompiler *c,
    NimRef *array,
    NimBindPath *path,
    NimBindVars *vars,
    NimLabel *next_label
)
{
    NimRef *size;
    size_t i;
    NimRef *code = NIM_COMPILER_CODE(c);
    NimLabel pop_label = NIM_LABEL_INIT;
    NimLabel end_label = NIM_LABEL_INIT;

    /* XXX need to free labels when things go bad */

    /* is the value an array ? */
    if (!nim_code_dup (code)) {
        return NIM_FALSE;
    }

    if (!nim_code_getclass (code)) {
        return NIM_FALSE;
    }

    if (!nim_code_pushname (code, NIM_STR_NEW("array"))) {
        return NIM_FALSE;
    }

    if (!nim_code_eq (code)) {
        return NIM_FALSE;
    }

    if (!nim_code_jumpiffalse (code, next_label)) {
        return NIM_FALSE;
    }

    if (!nim_code_dup (code)) {
        return NIM_FALSE;
    }

    if (!nim_code_getattr (code, NIM_STR_NEW("size"))) {
        return NIM_FALSE;
    }

    if (!nim_code_call (code, 0)) {
        return NIM_FALSE;
    }

    size = nim_int_new (NIM_ARRAY_SIZE(array));
    if (size == NULL) {
        return NIM_FALSE;
    }

    if (!nim_code_pushconst (code, size)) {
        return NIM_FALSE;
    }

    if (!nim_code_eq (code)) {
        return NIM_FALSE;
    }

    if (!nim_code_jumpiffalse (code, next_label)) {
        return NIM_FALSE;
    }

    for (i = 0; i < NIM_ARRAY_SIZE(array); i++) {
        NimRef *item = NIM_ARRAY_ITEM(array, i);

        if (!nim_code_dup (code)) {
            return NIM_FALSE;
        }

        if (!nim_code_pushconst (code, nim_int_new (i))) {
            return NIM_FALSE;
        }

        if (!nim_code_getitem (code)) {
            return NIM_FALSE;
        }

        NIM_BIND_PATH_ARRAY_INDEX(path, i);

        if (!nim_compile_ast_stmt_pattern_test (
                    c, item, path, vars, &pop_label)) {
            return NIM_FALSE;
        }

        if (!nim_code_pop (code)) {
            return NIM_FALSE;
        }
    }

    if (!nim_code_jump (code, &end_label)) {
        return NIM_FALSE;
    }

    nim_code_use_label (code, &pop_label);

    if (!nim_code_pop (code)) {
        return NIM_FALSE;
    }

    if (!nim_code_jump (code, next_label)) {
        return NIM_FALSE;
    }

    nim_code_use_label (code, &end_label);

    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_stmt_hash_pattern_test (
    NimCodeCompiler *c,
    NimRef *hash,
    NimBindPath *path,
    NimBindVars *vars,
    NimLabel *next_label
)
{
    NimRef *size;
    size_t i;
    NimRef *code = NIM_COMPILER_CODE(c);

    /* is the value a hash ? */
    if (!nim_code_dup (code)) {
        return NIM_FALSE;
    }

    if (!nim_code_getclass (code)) {
        return NIM_FALSE;
    }

    if (!nim_code_pushname (code, NIM_STR_NEW("hash"))) {
        return NIM_FALSE;
    }

    if (!nim_code_eq (code)) {
        return NIM_FALSE;
    }

    if (!nim_code_jumpiffalse (code, next_label)) {
        return NIM_FALSE;
    }

    if (!nim_code_dup (code)) {
        return NIM_FALSE;
    }

    if (!nim_code_getattr (code, NIM_STR_NEW("size"))) {
        return NIM_FALSE;
    }

    if (!nim_code_call (code, 0)) {
        return NIM_FALSE;
    }

    /* divide by two because we have an array repr of a hash:
     * [key1, value1, key2, value2]
     */
    size = nim_int_new (NIM_ARRAY_SIZE(hash) / 2);
    if (size == NULL) {
        return NIM_FALSE;
    }

    if (!nim_code_pushconst (code, size)) {
        return NIM_FALSE;
    }

    if (!nim_code_eq (code)) {
        return NIM_FALSE;
    }

    if (!nim_code_jumpiffalse (code, next_label)) {
        return NIM_FALSE;
    }

    /* XXX hashes are encoded as arrays in the AST */
    for (i = 0; i < NIM_ARRAY_SIZE(hash); i += 2) {
        NimRef *key = NIM_ARRAY(hash)->items[i];
        NimRef *value = NIM_ARRAY(hash)->items[i+1];
        NimRef *realkey;

        switch (NIM_AST_EXPR_TYPE(key)) {
            case NIM_AST_EXPR_IDENT:
                {
                    NIM_BUG (
                        "pattern matcher does not support unpacking by key");
                    return NIM_FALSE;
                }
            case NIM_AST_EXPR_ARRAY:
                {
                    NIM_BUG ("pattern matcher does not support array keys");
                    return NIM_FALSE;
                }
            case NIM_AST_EXPR_HASH:
                {
                    NIM_BUG ("pattern matcher does not support hash keys");
                    return NIM_FALSE;
                }
            case NIM_AST_EXPR_STR:
                {
                    realkey = NIM_AST_EXPR(key)->str.value;
                    break;
                }
            case NIM_AST_EXPR_INT_:
                {
                    realkey = NIM_AST_EXPR(key)->int_.value;
                    break;
                }
            case NIM_AST_EXPR_FLOAT_:
                {
                    realkey = NIM_AST_EXPR(key)->float_.value;
                    break;
                }
            case NIM_AST_EXPR_BOOL:
                {
                    realkey = NIM_AST_EXPR(key)->bool.value;
                    break;
                }
            case NIM_AST_EXPR_NIL:
                {
                    realkey = nim_nil;
                    break;
                }
            case NIM_AST_EXPR_WILDCARD:
                {
                    NIM_BUG (
                        "pattern matcher doesn't support wildcard in hash key");
                    return NIM_FALSE;
                }
            default:
                NIM_BUG (
                    "pattern matcher found unknown AST expr type in hash key");
                return NIM_FALSE;
        }

        if (!nim_code_dup (code)) {
            return NIM_FALSE;
        }

        if (!nim_code_pushconst (code, realkey)) {
            return NIM_FALSE;
        }

        if (!nim_code_getitem (code)) {
            return NIM_FALSE;
        }

        NIM_BIND_PATH_HASH_KEY(path, realkey);

        if (!nim_compile_ast_stmt_pattern_test (
                    c, value, path, vars, next_label)) {
            return NIM_FALSE;
        }

        if (!nim_code_pop (code)) {
            return NIM_FALSE;
        }
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_stmt_simple_pattern_test (
    NimCodeCompiler *c,
    const char *class_name,
    NimRef *value,
    NimLabel *next_label
)
{
    NimRef *code = NIM_COMPILER_CODE(c);

    /* XXX the class comparison code path should be moved elsewhere:
     *     pretty sure it's only used by a pattern test for `nil`
     */
    if (value == NULL) {
        /*
         * no value provided. do the types match?
         */
        if (!nim_code_dup (code)) {
            return NIM_FALSE;
        }

        if (!nim_code_getclass (code)) {
            return NIM_FALSE;
        }

        /* XXX the `nil` class isn't exposed at the language level */
        if (strcmp (class_name, "nil") == 0) {
            if (!nim_code_pushconst (code, nim_nil_class)) {
                return NIM_FALSE;
            }
        }
        else if (!nim_code_pushname (code,
                nim_str_new (class_name, strlen(class_name)))) {
            return NIM_FALSE;
        }

        if (!nim_code_eq (code)) {
            return NIM_FALSE;
        }

        if (!nim_code_jumpiffalse (code, next_label)) {
            return NIM_FALSE;
        }

    }
    else /* if (value != NULL) */ {
        /*
         * okay, so the types match. are the values equivalent?
         */
        if (!nim_code_dup (code)) {
            return NIM_FALSE;
        }

        if (!nim_code_pushconst (code, value)) {
            return NIM_FALSE;
        }

        if (!nim_code_eq (code)) {
            return NIM_FALSE;
        }

        if (!nim_code_jumpiffalse (code, next_label)) {
            return NIM_FALSE;
        }
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_stmt_pattern_test (
    NimCodeCompiler *c,
    NimRef *test,
    NimBindPath *path,
    NimBindVars *vars,
    NimLabel *next_label
)
{
    switch (NIM_AST_EXPR(test)->type) {
        case NIM_AST_EXPR_INT_:
            {
                NimRef *value = NIM_AST_EXPR(test)->int_.value;
                if (!nim_compile_ast_stmt_simple_pattern_test (
                        c, "int", value, next_label)) {
                    return NIM_FALSE;
                }

                break;
            }
        case NIM_AST_EXPR_FLOAT_:
            {
                NimRef *value = NIM_AST_EXPR(test)->float_.value;
                if (!nim_compile_ast_stmt_simple_pattern_test(
                        c, "float", value, next_label )) {
                    return NIM_FALSE;
                }

               break; 
            }
        case NIM_AST_EXPR_STR:
            {
                NimRef *value = NIM_AST_EXPR(test)->str.value;
                if (!nim_compile_ast_stmt_simple_pattern_test (
                        c, "str", value, next_label)) {
                    return NIM_FALSE;
                }

                break;
            }
        case NIM_AST_EXPR_BOOL:
            {
                NimRef *value = NIM_AST_EXPR(test)->bool.value;
                if (!nim_compile_ast_stmt_simple_pattern_test (
                        c, "bool", value, next_label)) {
                    return NIM_FALSE;
                }

                break;
            }
        case NIM_AST_EXPR_NIL:
            {
                if (!nim_compile_ast_stmt_simple_pattern_test (
                        c, "nil", NULL, next_label)) {
                    return NIM_FALSE;
                }

                break;
            }
        case NIM_AST_EXPR_IDENT:
            {
                NimRef *id = NIM_AST_EXPR(test)->ident.id;

                NIM_BIND_VARS_ADD(vars, id, path);

                break;
            }
        case NIM_AST_EXPR_ARRAY:
            {
                NimRef *value = NIM_AST_EXPR(test)->array.value;

                NIM_BIND_PATH_PUSH_ARRAY(path);

                if (!nim_compile_ast_stmt_array_pattern_test (
                        c, value, path, vars, next_label)) {
                    return NIM_FALSE;
                }

                NIM_BIND_PATH_POP(path);

                break;
            }
        case NIM_AST_EXPR_HASH:
            {
                NimRef *value = NIM_AST_EXPR(test)->hash.value;

                NIM_BIND_PATH_PUSH_HASH(path);

                if (!nim_compile_ast_stmt_hash_pattern_test (
                        c, value, path, vars, next_label)) {
                    return NIM_FALSE;
                }

                NIM_BIND_PATH_POP(path);

                break;
            }
        case NIM_AST_EXPR_WILDCARD:
            {
                break;
            }
        default:
            NIM_BUG ("TODO");
            return NIM_FALSE;
    };

    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_stmt_match (NimCodeCompiler *c, NimRef *stmt)
{
    size_t i;
    NimLabel end_label = NIM_LABEL_INIT;
    NimRef *code = NIM_COMPILER_CODE(c);
    NimRef *expr = NIM_AST_STMT(stmt)->match.expr;
    NimRef *patterns = NIM_AST_STMT(stmt)->match.body;
    const size_t size = NIM_ARRAY_SIZE(patterns);

    if (!nim_compile_ast_expr (c, expr)) {
        nim_label_free (&end_label);
        return NIM_FALSE;
    }

    for (i = 0; i < size; i++) {
        NimLabel next_label = NIM_LABEL_INIT;
        NimRef *pattern = NIM_ARRAY_ITEM(patterns, i);
        NimRef *test = NIM_AST_STMT(pattern)->pattern.test;
        NimRef *body = NIM_AST_STMT(pattern)->pattern.body;
        size_t j, k;
        NimBindPath path;
        NimBindVars bound;

        NIM_BIND_PATH_INIT(&path);
        NIM_BIND_VARS_INIT(&bound);

        /* try to match the value against this test. if we fail, jump to next_label */
        if (!nim_compile_ast_stmt_pattern_test (c, test, &path, &bound, &next_label)) {
            nim_label_free (&end_label);
            return NIM_FALSE;
        }

        /* pattern test succeeded: bind pattern vars (if any) */
        for (j = 0; j < bound.size; j++) {
            const NimBindPath *var_path = &bound.items[j].path;

            if (!nim_code_dup (code)) {
                return NIM_FALSE;
            }

            for (k = 0; k < var_path->size; k++) {
                if (var_path->items[k].type == NIM_BIND_PATH_ITEM_TYPE_ARRAY) {
                    size_t index = var_path->items[k].array_index;

                    if (!nim_code_pushconst (code, nim_int_new (index))) {
                        return NIM_FALSE;
                    }

                    if (!nim_code_getitem (code)) {
                        return NIM_FALSE;
                    }
                }
                else if (var_path->items[k].type == NIM_BIND_PATH_ITEM_TYPE_HASH) {
                    NimRef *key = var_path->items[k].hash_key;

                    if (!nim_code_pushconst (code, key)) {
                        return NIM_FALSE;
                    }

                    if (!nim_code_getitem (code)) {
                        return NIM_FALSE;
                    }
                }
                else {
                    /* simple binding: we're storing the matched value itself */
                }
            }
            if (!nim_code_storename (code, bound.items[j].id)) {
                return NIM_FALSE;
            }
        }

        /* pop the value we're testing off the stack */
        if (!nim_code_pop (code)) {
            return NIM_FALSE;
        }

        /* successful match: execute the body */
        if (!nim_compile_ast_stmts (c, body)) {
            return NIM_FALSE;
        }

        if (!nim_code_jump (code, &end_label)) {
            return NIM_FALSE;
        }

        nim_code_use_label (code, &next_label);
    }

    nim_code_use_label (code, &end_label);

    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_stmt_if_ (NimCodeCompiler *c, NimRef *stmt)
{
    NimRef *code = NIM_COMPILER_CODE(c);
    NimLabel end_body = NIM_LABEL_INIT;
    NimLabel end_orelse = NIM_LABEL_INIT;

    if (!nim_compile_ast_expr (c, NIM_AST_STMT(stmt)->if_.expr))
        goto error;

    if (!nim_code_jumpiffalse (code, &end_body))
        goto error;

    if (!nim_compile_ast_stmts (c, NIM_AST_STMT(stmt)->if_.body))
        goto error;

    if (NIM_AST_STMT(stmt)->if_.orelse != NULL) {

        if (!nim_code_jump (code, &end_orelse))
            goto error;

        nim_code_use_label (code, &end_body);

        if (!nim_compile_ast_stmts (c, NIM_AST_STMT(stmt)->if_.orelse))
            goto error;

        nim_code_use_label (code, &end_orelse);
    }
    else {
        nim_code_use_label (code, &end_body);
    }

    return NIM_TRUE;

error:
    nim_label_free (&end_body);
    nim_label_free (&end_orelse);
    return NIM_FALSE;
}

static nim_bool_t
nim_compile_ast_stmt_ret (NimCodeCompiler *c, NimRef *stmt)
{
    NimRef *code = NIM_COMPILER_CODE(c);
    NimRef *expr = NIM_AST_STMT(stmt)->ret.expr;
    if (expr != NULL) {
        if (!nim_compile_ast_expr (c, expr)) {
            return NIM_FALSE;
        }
    }
    else if (!nim_code_pushnil (code)) {
        return NIM_FALSE;
    }

    if (!nim_code_ret (code)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_stmt_break_ (NimCodeCompiler *c, NimRef *stmt)
{
    NimRef *code = NIM_COMPILER_CODE(c);
    NimLabel *label = nim_code_compiler_get_loop_label (c);
    if (label == NULL) {
        return NIM_FALSE;
    }

    if (!nim_code_jump (code, label)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

static NimRef *
nim_compile_get_nearest_unit_with_type (
        NimCodeCompiler *c, NimUnitType type)
{
    if (c->current_unit->type == type) {
        return NIM_COMPILER_SCOPE(c);
    }
    else {
        NimCodeUnit *unit = c->current_unit;
        if (unit == NULL) return NULL;
        unit = unit->next;
        while (unit != NULL) {
            if (unit->type == type) {
                return unit->value;
            }
            unit = unit->next;
        }
        return NULL;
    }
}

static NimRef *
nim_compile_get_current_class (NimCodeCompiler *c)
{
    return nim_compile_get_nearest_unit_with_type (
            c, NIM_UNIT_TYPE_CLASS);
}

static NimRef *
nim_compile_get_current_module (NimCodeCompiler *c)
{
    return nim_compile_get_nearest_unit_with_type (
            c, NIM_UNIT_TYPE_MODULE);
}

static NimRef *
nim_compile_bytecode_method (NimCodeCompiler *c, NimRef *fn, NimRef *args, NimRef *body)
{
    NimRef *mod;
    NimRef *func_code;
    NimRef *symbols;
    NimRef *method;
    NimRef *ste;
    size_t i;

    func_code = nim_code_compiler_push_code_unit (c, fn);
    if (func_code == NULL) {
        return NULL;
    }

    ste = c->current_unit->ste;
    symbols = NIM_SYMTABLE_ENTRY(ste)->symbols;
    for (i = 0; i < NIM_HASH_SIZE(symbols); i++) {
        NimRef *key = NIM_HASH(symbols)->keys[i];
        NimRef *value = NIM_HASH(symbols)->values[i];
        if (NIM_INT(value)->value & NIM_SYM_DECL) {
            if (!nim_array_push (NIM_CODE(func_code)->vars, key)) {
                NIM_BUG ("failed to push var name");
                return NULL;
            }
        }
        else if (NIM_INT(value)->value & NIM_SYM_FREE) {
            if (!nim_array_push (NIM_CODE(func_code)->freevars, key)) {
                NIM_BUG ("failed to push freevar name");
                return NULL;
            }
        }
    }

    /* unpack arguments */
    for (i = 0; i < NIM_ARRAY_SIZE(args); i++) {
        NimRef *var_decl = NIM_ARRAY_ITEM(args, NIM_ARRAY_SIZE(args) - i - 1);
        if (!nim_code_storename (func_code, NIM_AST_DECL(var_decl)->var.name)) {
            NIM_BUG ("failed to emit STORENAME instruction");
            return NULL;
        }
    }
    
    if (!nim_compile_ast_stmts (c, body)) {
        return NULL;
    }

    if (!nim_code_compiler_pop_code_unit (c)) {
        return NULL;
    }

    mod = nim_compile_get_current_module (c);

    if (getenv ("NIM_DEBUG_MODE")) {
        fprintf (stderr, "%s\n", NIM_STR_DATA(nim_code_dump (func_code)));
    }
    method = nim_method_new_bytecode (mod, func_code);
    if (method == NULL) {
        return NULL;
    }

    return method;
}

static nim_bool_t
nim_compile_ast_decl_func (NimCodeCompiler *c, NimRef *decl)
{
    NimRef *method;
    NimRef *mod;

    method = nim_compile_bytecode_method (
        c,
        decl,
        NIM_AST_DECL(decl)->func.args,
        NIM_AST_DECL(decl)->func.body
    );
    if (method == NULL) {
        return NIM_FALSE;
    }

    if (NIM_COMPILER_IN_CODE(c)) {
        NimRef *code = NIM_COMPILER_CODE(c);
        if (!nim_code_pushconst (code, method)) {
            return NIM_FALSE;
        }

        if (!nim_code_storename (code, NIM_AST_DECL(decl)->func.name)) {
            return NIM_FALSE;
        }
    }
    else if (NIM_COMPILER_IN_CLASS(c)) {
        NimRef *name = NIM_AST_DECL(decl)->func.name;
        mod = nim_compile_get_current_class (c);
        if (!nim_class_add_method (mod, name, method)) {
            return NIM_FALSE;
        }
    }
    else {
        NimRef *name = NIM_AST_DECL(decl)->func.name;
        mod = nim_compile_get_current_module (c);
        if (!nim_module_add_local (mod, name, method)) {
            return NIM_FALSE;
        }
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_decl_class (NimCodeCompiler *c, NimRef *decl)
{
    NimRef *mod;
    NimRef *name = NIM_AST_DECL(decl)->class.name;
    NimRef *base = NIM_AST_DECL(decl)->class.base;
    NimRef *body = NIM_AST_DECL(decl)->class.body;
    NimRef *base_ref = NULL;
    NimRef *klass;
    size_t i;

    if (NIM_COMPILER_IN_CODE(c)) {
        NIM_BUG ("Cannot declare classes inside functions");
        return NIM_FALSE;
    }
    else if (NIM_COMPILER_IN_CLASS(c)) {
        NIM_BUG ("Cannot declare nested classes");
    }

    mod = nim_compile_get_current_module (c);
    if (mod == NULL) {
        NIM_BUG ("get_current_module failed");
        return NIM_FALSE;
    }

    /* resolve the base class */
    /* XXX this feels kind of hacky */
    if (NIM_ARRAY_SIZE(base) > 0) {
        if (nim_is_builtin (nim_array_first (base))) {
            int rc = nim_hash_get (
                        nim_builtins, nim_array_first (base), &base_ref);
            if (rc < 0) {
                NIM_BUG ("is_builtin/hash_get failed");
                return NIM_FALSE;
            }
            else if (rc > 0) {
                return NIM_FALSE;
            }
        }
        else {
            base_ref = mod;
            for (i = 0; i < NIM_ARRAY_SIZE(base); i++) {
                int rc;
                NimRef *child;
                NimRef *name;
                if (i != NIM_ARRAY_SIZE(base)-1 && 
                    NIM_ANY_CLASS(base_ref) != nim_module_class) {
                    NIM_BUG ("Cannot resolve class on non-module type: %s",
                        NIM_STR_DATA(name));
                    return NIM_FALSE;
                }
                name = NIM_ARRAY_ITEM(base, i);
                rc = nim_hash_get (
                        NIM_MODULE_LOCALS(base_ref), name, &child);
                if (rc < 0) {
                    NIM_BUG ("Error");
                    return NIM_FALSE;
                }
                else if (rc > 0) {
                    NIM_BUG ("Module does not expose `%s`",
                        NIM_STR_DATA(NIM_ARRAY_ITEM(base_ref, i)));
                    return NIM_FALSE;
                }
                base_ref = child;
            }
        }
    }

    klass = nim_class_new (name, base_ref, sizeof(NimObject));
    if (klass == NULL) {
        return NIM_FALSE;
    }

    if (!nim_code_compiler_push_class_unit (c, decl, klass)) {
        return NIM_FALSE;
    }
    if (!nim_compile_ast_decls (c, body)) {
        return NIM_FALSE;
    }
    if (!nim_code_compiler_pop_class_unit (c)) {
        return NIM_FALSE;
    }

    if (!nim_module_add_local (mod, name, klass)) {
        NIM_BUG ("Failed to add new class to module");
        return NIM_FALSE;
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_decl_use (NimCodeCompiler *c, NimRef *decl)
{
    NimRef *module = NIM_COMPILER_MODULE(c);
    NimRef *import;
    NimRef *name;

    name = NIM_AST_DECL(decl)->use.name;
    import = nim_module_mgr_load (name);

    if (import == NULL) {
        return NIM_FALSE;
    }

    if (!nim_module_add_local (module, name, import)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_decl_var (NimCodeCompiler *c, NimRef *decl)
{
    NimRef *scope = NIM_COMPILER_SCOPE(c);
    NimRef *name = NIM_AST_DECL(decl)->var.name;
    NimRef *value = NIM_AST_DECL(decl)->var.value;

    if (NIM_ANY_CLASS(scope) == nim_module_class) {
        return nim_module_add_local (scope, name, value == NULL ? nim_nil : value);
    }
    else {
        /* TODO it's really about time we get ourselves a symbol table */
        if (value != NULL) {
            NimRef *code = NIM_COMPILER_CODE(c);

            if (!nim_compile_ast_expr (c, value)) {
                return NIM_FALSE;
            }

            if (!nim_code_storename (code, name)) {
                return NIM_FALSE;
            }
        }
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_decl (NimCodeCompiler *c, NimRef *decl)
{
    switch (NIM_AST_DECL_TYPE(decl)) {
        case NIM_AST_DECL_FUNC:
            return nim_compile_ast_decl_func (c, decl);
        case NIM_AST_DECL_CLASS:
            return nim_compile_ast_decl_class (c, decl);
        case NIM_AST_DECL_USE:
            return nim_compile_ast_decl_use (c, decl);
        case NIM_AST_DECL_VAR:
            return nim_compile_ast_decl_var (c, decl);
        default:
            NIM_BUG ("unknown AST stmt type: %d", NIM_AST_DECL_TYPE(decl));
            return NIM_FALSE;
    }
}

static nim_bool_t
nim_compile_ast_stmt (NimCodeCompiler *c, NimRef *stmt)
{
    switch (NIM_AST_STMT_TYPE(stmt)) {
        case NIM_AST_STMT_EXPR:
            if (!nim_compile_ast_expr (c, NIM_AST_STMT(stmt)->expr.expr)) {
                return NIM_FALSE;
            }
            /* clean up the stack when we're not saving the result */
            if (!nim_code_pop (NIM_COMPILER_CODE(c))) {
                return NIM_FALSE;
            }
            return NIM_TRUE;
        case NIM_AST_STMT_ASSIGN:
            return nim_compile_ast_stmt_assign (c, stmt);
        case NIM_AST_STMT_IF_:
            return nim_compile_ast_stmt_if_ (c, stmt);
        case NIM_AST_STMT_WHILE_:
            return nim_compile_ast_stmt_while_ (c, stmt);
        case NIM_AST_STMT_MATCH:
            return nim_compile_ast_stmt_match (c, stmt);
        case NIM_AST_STMT_RET:
            return nim_compile_ast_stmt_ret (c, stmt);
        case NIM_AST_STMT_BREAK_:
            return nim_compile_ast_stmt_break_ (c, stmt);
        default:
            NIM_BUG ("unknown AST stmt type: %d", NIM_AST_STMT_TYPE(stmt));
            return NIM_FALSE;
    };
}

static nim_bool_t
nim_compile_ast_expr (NimCodeCompiler *c, NimRef *expr)
{
    switch (NIM_AST_EXPR_TYPE(expr)) {
        case NIM_AST_EXPR_CALL:
            return nim_compile_ast_expr_call (c, expr);
        case NIM_AST_EXPR_GETATTR:
            return nim_compile_ast_expr_getattr (c, expr);
        case NIM_AST_EXPR_GETITEM:
            return nim_compile_ast_expr_getitem (c, expr);
        case NIM_AST_EXPR_ARRAY:
            return nim_compile_ast_expr_array (c, expr);
        case NIM_AST_EXPR_HASH:
            return nim_compile_ast_expr_hash (c, expr);
        case NIM_AST_EXPR_IDENT:
            return nim_compile_ast_expr_ident (c, expr);
        case NIM_AST_EXPR_STR:
            return nim_compile_ast_expr_str (c, expr);
        case NIM_AST_EXPR_BOOL:
            return nim_compile_ast_expr_bool (c, expr);
        case NIM_AST_EXPR_NIL:
            return nim_code_pushnil (NIM_COMPILER_CODE(c));
        case NIM_AST_EXPR_BINOP:
            return nim_compile_ast_expr_binop (c, expr);
        case NIM_AST_EXPR_INT_:
            return nim_compile_ast_expr_int_ (c, expr);
        case NIM_AST_EXPR_FLOAT_:
            return nim_compile_ast_expr_float_ (c, expr);
        case NIM_AST_EXPR_FN:
            return nim_compile_ast_expr_fn (c, expr);
        case NIM_AST_EXPR_SPAWN:
            return nim_compile_ast_expr_spawn (c, expr);
        case NIM_AST_EXPR_NOT:
            return nim_compile_ast_expr_not (c, expr);
        case NIM_AST_EXPR_WILDCARD:
            {
                NIM_BUG (
                    "wildcard can't be used outside of a match statement");
                return NIM_FALSE;
            }
        default:
            NIM_BUG ("unknown AST expr type: %d", NIM_AST_EXPR_TYPE(expr));
            return NIM_FALSE;
    };
}

static nim_bool_t
nim_compile_ast_expr_call (NimCodeCompiler *c, NimRef *expr)
{
    NimRef *target;
    NimRef *args;
    size_t i;
    NimRef *code = NIM_COMPILER_CODE(c);

    target = NIM_AST_EXPR(expr)->call.target;
    if (!nim_compile_ast_expr (c, target)) {
        return NIM_FALSE;
    }

    args = NIM_AST_EXPR(expr)->call.args;
    for (i = 0; i < NIM_ARRAY_SIZE(args); i++) {
        if (!nim_compile_ast_expr (c, NIM_ARRAY_ITEM(args, i))) {
            return NIM_FALSE;
        }
    }

    if (!nim_code_call (code, NIM_ARRAY_SIZE(args))) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_expr_getattr (NimCodeCompiler *c, NimRef *expr)
{
    NimRef *target;
    NimRef *attr;
    NimRef *code = NIM_COMPILER_CODE(c);

    target = NIM_AST_EXPR(expr)->getattr.target;
    if (!nim_compile_ast_expr (c, target)) {
        return NIM_FALSE;
    }

    attr = NIM_AST_EXPR(expr)->getattr.attr;
    if (attr == NULL) {
        return NIM_FALSE;
    }

    if (!nim_code_getattr (code, attr)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_expr_getitem (NimCodeCompiler *c, NimRef *expr)
{
    NimRef *target;
    NimRef *key;
    NimRef *code = NIM_COMPILER_CODE(c);

    target = NIM_AST_EXPR(expr)->getitem.target;
    if (!nim_compile_ast_expr (c, target)) {
        return NIM_FALSE;
    }

    key = NIM_AST_EXPR(expr)->getitem.key;
    if (key == NULL) {
        return NIM_FALSE;
    }

    if (!nim_compile_ast_expr (c, key)) {
        return NIM_FALSE;
    }

    if (!nim_code_getitem (code)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_expr_ident (NimCodeCompiler *c, NimRef *expr)
{
    NimRef *id = NIM_AST_EXPR(expr)->ident.id;
    const NimAstNodeLocation *loc = &NIM_AST_EXPR(expr)->location;
    NimRef *code = NIM_COMPILER_CODE(c);
    if (strcmp (NIM_STR_DATA (id), "__file__") == 0) {
        /* XXX loc->filename == NULL here. why?!? */
        if (!nim_code_pushconst (code, NIM_SYMTABLE(c->symtable)->filename)) {
            return NIM_FALSE;
        }
    }
    else if (strcmp (NIM_STR_DATA (id), "__line__") == 0) {
        if (!nim_code_pushconst (code, nim_int_new (loc->first_line))) {
            return NIM_FALSE;
        }
    }
    else {
        if (!nim_code_pushname (code, id)) {
            return NIM_FALSE;
        }
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_expr_array (NimCodeCompiler *c, NimRef *expr)
{
    size_t i;
    NimRef *code = NIM_COMPILER_CODE(c);
    NimRef *arr = NIM_AST_EXPR(expr)->array.value;
    for (i = 0; i < NIM_ARRAY_SIZE(arr); i++) {
        if (!nim_compile_ast_expr (c, NIM_ARRAY_ITEM(arr, i))) {
            return NIM_FALSE;
        }
    }
    if (!nim_code_makearray (code, NIM_ARRAY_SIZE(arr))) {
        return NIM_FALSE;
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_expr_hash (NimCodeCompiler *c, NimRef *expr)
{
    size_t i;
    NimRef *code = NIM_COMPILER_CODE(c);
    NimRef *arr = NIM_AST_EXPR(expr)->hash.value;
    /* TODO ensure array size is even (key/value pairs) */
    for (i = 0; i < NIM_ARRAY_SIZE(arr); i++) {
        if (!nim_compile_ast_expr (c, NIM_ARRAY_ITEM(arr, i))) {
            return NIM_FALSE;
        }
    }
    if (!nim_code_makehash (code, NIM_ARRAY_SIZE(arr) / 2)) {
        return NIM_FALSE;
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_expr_str (NimCodeCompiler *c, NimRef *expr)
{
    NimRef *code = NIM_COMPILER_CODE(c);
    if (nim_code_pushconst (code, NIM_AST_EXPR(expr)->str.value) < 0) {
        return NIM_FALSE;
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_expr_bool (NimCodeCompiler *c, NimRef *expr)
{
    NimRef *code = NIM_COMPILER_CODE(c);
    if (nim_code_pushconst (code, NIM_AST_EXPR(expr)->bool.value) < 0) {
        return NIM_FALSE;
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_expr_int_ (NimCodeCompiler *c, NimRef *expr)
{
    NimRef *code = NIM_COMPILER_CODE(c);
    if (nim_code_pushconst (code, NIM_AST_EXPR(expr)->int_.value) < 0) {
        return NIM_FALSE;
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_expr_float_ (NimCodeCompiler *c, NimRef *expr)
{
    NimRef *code = NIM_COMPILER_CODE(c);
    if (nim_code_pushconst (code, NIM_AST_EXPR(expr)->float_.value) < 0) {
        return NIM_FALSE;
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_expr_binop (NimCodeCompiler *c, NimRef *expr)
{
    NimRef *code = NIM_COMPILER_CODE(c);
    if (!nim_compile_ast_expr (c, NIM_AST_EXPR(expr)->binop.left)) {
        return NIM_FALSE;
    }

    switch (NIM_AST_EXPR(expr)->binop.op) {
        case NIM_BINOP_EQ:
            {
                if (!nim_compile_ast_expr (c, NIM_AST_EXPR(expr)->binop.right)) {
                    return NIM_FALSE;
                }

                if (!nim_code_eq (code)) {
                    return NIM_FALSE;
                }
                break;
            }
        case NIM_BINOP_NEQ:
            {
                if (!nim_compile_ast_expr (c, NIM_AST_EXPR(expr)->binop.right)) {
                    return NIM_FALSE;
                }

                if (!nim_code_neq (code)) {
                    return NIM_FALSE;
                }
                break;
            }
        case NIM_BINOP_GT:
            {
                if (!nim_compile_ast_expr (c, NIM_AST_EXPR(expr)->binop.right)) {
                    return NIM_FALSE;
                }

                if (!nim_code_gt (code)) {
                    return NIM_FALSE;
                }
                break;
            }
        case NIM_BINOP_GTE:
            {
                if (!nim_compile_ast_expr (c, NIM_AST_EXPR(expr)->binop.right)) {
                    return NIM_FALSE;
                }

                if (!nim_code_gte (code)) {
                    return NIM_FALSE;
                }
                break;
            }
        case NIM_BINOP_LT:
            {
                if (!nim_compile_ast_expr (c, NIM_AST_EXPR(expr)->binop.right)) {
                    return NIM_FALSE;
                }

                if (!nim_code_lt (code)) {
                    return NIM_FALSE;
                }
                break;
            }
        case NIM_BINOP_LTE:
            {
                if (!nim_compile_ast_expr (c, NIM_AST_EXPR(expr)->binop.right)) {
                    return NIM_FALSE;
                }

                if (!nim_code_lte (code)) {
                    return NIM_FALSE;
                }
                break;
            }
        case NIM_BINOP_OR:
            {
                /* short-circuited logical OR */
                NimLabel right_label = NIM_LABEL_INIT;
                NimLabel end_label = NIM_LABEL_INIT;

                if (!nim_code_dup (code))
                    goto or_error;

                if (!nim_code_jumpiffalse (code, &right_label))
                    goto or_error;

                if (!nim_code_jump (code, &end_label))
                    goto or_error;

                nim_code_use_label (code, &right_label);

                if (!nim_code_pop (code))
                    goto or_error;

                if (!nim_compile_ast_expr (c, NIM_AST_EXPR(expr)->binop.right))
                    goto or_error;

                nim_code_use_label (code, &end_label);

                break;

or_error:
                nim_label_free (&right_label);
                nim_label_free (&end_label);
                return NIM_FALSE;
            }
        case NIM_BINOP_AND:
            {
                /* short-circuited logical AND */

                NimLabel right_label = NIM_LABEL_INIT;
                NimLabel end_label = NIM_LABEL_INIT;

                if (!nim_code_dup (code))
                    goto and_error;

                if (!nim_code_jumpiftrue (code, &right_label))
                    goto and_error;

                if (!nim_code_jump (code, &end_label))
                    goto and_error;

                nim_code_use_label (code, &right_label);

                if (!nim_code_pop (code))
                    goto and_error;

                if (!nim_compile_ast_expr (c, NIM_AST_EXPR(expr)->binop.right))
                    goto and_error;

                nim_code_use_label (code, &end_label);

                break;
and_error:
                nim_label_free (&right_label);
                nim_label_free (&end_label);
                return NIM_FALSE;
            }
        case NIM_BINOP_ADD:
            {
                if (!nim_compile_ast_expr (c, NIM_AST_EXPR(expr)->binop.right)) {
                    return NIM_FALSE;
                }
                
                if (!nim_code_add (code)) {
                    return NIM_FALSE;
                }

                break;
            }
        case NIM_BINOP_SUB:
            {
                if (!nim_compile_ast_expr (c, NIM_AST_EXPR(expr)->binop.right)) {
                    return NIM_FALSE;
                }

                if (!nim_code_sub (code)) {
                    return NIM_FALSE;
                }

                break;
            }
        case NIM_BINOP_MUL:

            {
                if (!nim_compile_ast_expr (c, NIM_AST_EXPR(expr)->binop.right)) {
                    return NIM_FALSE;
                }

                if (!nim_code_mul (code)) {
                    return NIM_FALSE;
                }

                break;
            }
        case NIM_BINOP_DIV:
            {
                if (!nim_compile_ast_expr (c, NIM_AST_EXPR(expr)->binop.right)) {
                    return NIM_FALSE;
                }

                if (!nim_code_div (code)) {
                    return NIM_FALSE;
                }

                break;
            }
        default:
            NIM_BUG ("unknown binop type: %d", NIM_AST_EXPR(expr)->binop.op);
            return NIM_FALSE;
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_expr_fn (NimCodeCompiler *c, NimRef *expr)
{
    NimRef *method;
    NimRef *code = NIM_COMPILER_CODE(c);

    method = nim_compile_bytecode_method (
        c,
        expr,
        NIM_AST_EXPR(expr)->fn.args,
        NIM_AST_EXPR(expr)->fn.body
    );
    if (method == NULL) {
        return NIM_FALSE;
    }
    
    if (!nim_code_pushconst (code, method)) {
        return NIM_FALSE;
    }

    if (NIM_METHOD(method)->type == NIM_METHOD_TYPE_BYTECODE) {
        NimRef *method_code = NIM_METHOD(method)->bytecode.code;
        if (NIM_ARRAY_SIZE(NIM_CODE(method_code)->freevars) > 0) {
            if (!nim_code_makeclosure (code)) {
                return NIM_FALSE;
            }
        }
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_expr_spawn (NimCodeCompiler *c, NimRef *expr)
{
    size_t i;
    NimRef *code = NIM_COMPILER_CODE(c);
    NimRef *target = NIM_AST_EXPR(expr)->spawn.target;
    NimRef *args = NIM_AST_EXPR(expr)->spawn.args;

    if (!nim_compile_ast_expr (c, target)) {
        return NIM_FALSE;
    }

    if (!nim_code_spawn (code)) {
        return NIM_FALSE;
    }

    if (!nim_code_dup (code)) {
        return NIM_FALSE;
    }

    if (!nim_code_getattr (code, NIM_STR_NEW ("send"))) {
        return NIM_FALSE;
    }

    for (i = 0; i < NIM_ARRAY_SIZE(args); i++) {
        if (!nim_compile_ast_expr (c, NIM_ARRAY_ITEM(args, i))) {
            return NIM_FALSE;
        }
    }

    if (!nim_code_makearray (code, NIM_ARRAY_SIZE(args))) {
        return NIM_FALSE;
    }

    if (!nim_code_call (code, 1)) {
        return NIM_FALSE;
    }

    if (!nim_code_pop (code)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_compile_ast_expr_not (NimCodeCompiler *c, NimRef *expr)
{
    NimRef *code = NIM_COMPILER_CODE(c);
    if (!nim_compile_ast_expr (c, NIM_AST_EXPR(expr)->not.value)) {
        return NIM_FALSE;
    }

    if (!nim_code_not (code)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

NimRef *
nim_compile_ast (NimRef *name, const char *filename, NimRef *ast)
{
    NimRef *module;
    NimCodeCompiler c;
    NimRef *filename_obj;
    memset (&c, 0, sizeof(c));

    filename_obj = nim_str_new (filename, strlen (filename));
    if (filename_obj == NULL) {
        return NULL;
    }

    c.symtable = nim_symtable_new_from_ast (filename_obj, ast);
    if (c.symtable == NULL) {
        goto error;
    }

    module = nim_code_compiler_push_module_unit (&c, ast);
    if (module == NULL) {
        goto error;
    }
    NIM_MODULE(module)->name = name;

    if (NIM_ANY_CLASS(ast) == nim_ast_mod_class) {
        if (!nim_compile_ast_mod (&c, ast)) {
            goto error;
        }
    }
    else {
        NIM_BUG ("unknown top-level AST node type: %s",
                    NIM_STR_DATA(NIM_CLASS(NIM_ANY_CLASS(ast))->name));
        goto error;
    }

    if (!nim_code_compiler_pop_unit (&c, NIM_UNIT_TYPE_MODULE)) {
        goto error;
    }

    nim_code_compiler_cleanup (&c);

    return module;

error:
    nim_code_compiler_cleanup (&c);
    return NULL;
}

extern int yyparse(void *scanner, NimRef *filename, NimRef **mod);
extern void yylex_init (void **scanner);
extern void yyset_in (FILE *stream, void *scanner);
extern void yyset_debug (int enable, void *scanner);
extern void yylex_destroy(void *scanner);
#ifdef NIM_PARSER_DEBUG
extern int yydebug;
#endif

static nim_bool_t
is_file (const char *filename, nim_bool_t *result)
{
    struct stat st;

    if (stat (filename, &st) != 0) {
        NIM_BUG ("stat() failed");
        return NIM_FALSE;
    }

    *result = S_ISREG(st.st_mode);

    return NIM_TRUE;
}

NimRef *
nim_compile_file (NimRef *name, const char *filename)
{
    int rc;
    NimRef *filename_obj;
    NimRef *mod;
    nim_bool_t isreg;
    void *scanner;
    FILE *input;

    if (!is_file (filename, &isreg)) {
        return NULL;
    }

    if (!isreg) {
        NIM_BUG ("not a regular file: %s", filename);
        return NULL;
    }

    input = fopen (filename, "r");
    if (input == NULL) {
        return NULL;
    }
    filename_obj = nim_str_new (filename, strlen (filename));
    if (filename_obj == NULL) {
        fclose (input);
        return NULL;
    }
    yylex_init (&scanner);
    yyset_in (input, scanner);
#ifdef NIM_PARSER_DEBUG
    yyset_debug (1, scanner);
    yydebug = 1;
#endif
    rc = yyparse(scanner, filename_obj, &mod);
    fclose (input);
    yylex_destroy (scanner);
    if (rc == 0) {
        /* keep a ptr to main_mod on the stack so it doesn't get collected */
        if (name == NULL) {
            ssize_t slash = -1;
            ssize_t dot = -1;
            size_t i;
            size_t len = strlen (filename);

            /* XXX this is just a shit O(n) basename() impl */
            for (i = 0; i < len; i++) {
                if (filename[i] == '/') {
                    slash = i + 1;
                    dot = -1;
                }
                else if (filename[i] == '.' && dot == -1) {
                    dot = i;
                }
            }
            if (dot == -1) dot = len;
            if (slash == -1) slash = 0;

            name = nim_str_new (filename + slash, dot - slash);
            if (name == NULL) {
                return NULL;
            }
        }
        return nim_compile_ast (name, filename, mod);
    }
    else {
        return NULL;
    }
}

