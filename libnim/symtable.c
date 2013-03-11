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

#include "nim/hash.h"
#include "nim/array.h"
#include "nim/class.h"
#include "nim/object.h"
#include "nim/symtable.h"

#define NIM_SYMTABLE_GET_CURRENT_ENTRY(ref) NIM_SYMTABLE(ref)->ste

#define NIM_SYMTABLE_ENTRY_CHECK_TYPE(ste, t) \
    ((NIM_SYMTABLE_ENTRY(ste)->flags & NIM_SYM_TYPE_MASK) == (t))

#define NIM_SYMTABLE_ENTRY_IS_MODULE(ste) \
    NIM_SYMTABLE_ENTRY_CHECK_TYPE(ste, NIM_SYM_MODULE)

NimRef *nim_symtable_class = NULL;
NimRef *nim_symtable_entry_class = NULL;

static nim_bool_t
nim_symtable_visit_decls (NimRef *self, NimRef *decls);

static nim_bool_t
nim_symtable_visit_decl (NimRef *self, NimRef *decl);

static nim_bool_t
nim_symtable_visit_stmt (NimRef *self, NimRef *stmt);

static nim_bool_t
nim_symtable_visit_expr (NimRef *self, NimRef *expr);

static nim_bool_t
nim_symtable_visit_mod (NimRef *self, NimRef *mod);

static NimRef *
_nim_symtable_init (NimRef *self, NimRef *args)
{
    NimRef *filename;
    NimRef *ast;

    if (!nim_method_parse_args (args, "oo", &filename, &ast)) {
        return NULL;
    }

    NIM_ANY(self)->klass = nim_symtable_class;
    NIM_SYMTABLE(self)->filename = filename;
    NIM_SYMTABLE(self)->ste = nim_nil;
    NIM_SYMTABLE(self)->lookup = nim_hash_new ();
    if (NIM_SYMTABLE(self)->lookup == NULL) {
        return NULL;
    }
    NIM_SYMTABLE(self)->stack = nim_array_new ();
    if (NIM_SYMTABLE(self)->stack == NULL) {
        return NULL;
    }

    if (NIM_ANY_CLASS(ast) == nim_ast_mod_class) {
        if (!nim_symtable_visit_mod (self, ast))
            return NULL;
    }
    else {
        NIM_BUG ("unknown top-level AST node type: %s",
                    NIM_STR_DATA(NIM_CLASS(NIM_ANY_CLASS(ast))->name));
        return NULL;
    }

    return self;
}

static void
_nim_symtable_mark (NimGC *gc, NimRef *self)
{
    NIM_SUPER (self)->mark (gc, self);

    nim_gc_mark_ref (gc, NIM_SYMTABLE(self)->filename);
    nim_gc_mark_ref (gc, NIM_SYMTABLE(self)->lookup);
    nim_gc_mark_ref (gc, NIM_SYMTABLE(self)->stack);
    nim_gc_mark_ref (gc, NIM_SYMTABLE(self)->ste);
}

nim_bool_t
nim_symtable_class_bootstrap (void)
{
    nim_symtable_class = nim_class_new (
        NIM_STR_NEW("symtable"), NULL, sizeof(NimSymtable));
    if (nim_symtable_class == NULL) {
        return NIM_FALSE;
    }
    NIM_CLASS(nim_symtable_class)->init = _nim_symtable_init;
    NIM_CLASS(nim_symtable_class)->mark = _nim_symtable_mark;
    nim_gc_make_root (NULL, nim_symtable_class);
    return NIM_TRUE;
}

static NimRef *
_nim_symtable_entry_init (NimRef *self, NimRef *args)
{
    int64_t flags;
    NimRef *symtable;
    NimRef *scope;
    NimRef *parent;

    if (!nim_method_parse_args (
            args, "oooI", &symtable, &parent, &scope, &flags)) {
        return NULL;
    }

    NIM_SYMTABLE_ENTRY(self)->flags = flags;
    NIM_SYMTABLE_ENTRY(self)->symtable = symtable;
    NIM_SYMTABLE_ENTRY(self)->scope = scope;
    NIM_SYMTABLE_ENTRY(self)->parent = parent;
    NIM_SYMTABLE_ENTRY(self)->symbols = nim_hash_new ();
    if (NIM_SYMTABLE_ENTRY(self)->symbols == NULL) {
        NIM_BUG ("failed to allocate symbols hash");
        return NULL;
    }
    NIM_SYMTABLE_ENTRY(self)->children = nim_array_new ();
    if (NIM_SYMTABLE_ENTRY(self)->children == NULL) {
        NIM_BUG ("failed to allocate children hash");
        return NULL;
    }
    return self;
}

static void
_nim_symtable_entry_mark (NimGC *gc, NimRef *self)
{
    NIM_SUPER (self)->mark (gc, self);

    nim_gc_mark_ref (gc, NIM_SYMTABLE_ENTRY(self)->symtable);
    nim_gc_mark_ref (gc, NIM_SYMTABLE_ENTRY(self)->scope);
    nim_gc_mark_ref (gc, NIM_SYMTABLE_ENTRY(self)->symbols);
    nim_gc_mark_ref (gc, NIM_SYMTABLE_ENTRY(self)->parent);
    nim_gc_mark_ref (gc, NIM_SYMTABLE_ENTRY(self)->children);
}

static void
_nim_symtable_entry_dtor (NimRef *dtor)
{
}

nim_bool_t
nim_symtable_entry_class_bootstrap (void)
{
    nim_symtable_entry_class = nim_class_new (
        NIM_STR_NEW("symtable_entry"), NULL, sizeof(NimSymtableEntry));
    if (nim_symtable_entry_class == NULL) {
        return NIM_FALSE;
    }
    NIM_CLASS(nim_symtable_entry_class)->init = _nim_symtable_entry_init;
    NIM_CLASS(nim_symtable_entry_class)->mark = _nim_symtable_entry_mark;
    NIM_CLASS(nim_symtable_entry_class)->dtor = _nim_symtable_entry_dtor;
    nim_gc_make_root (NULL, nim_symtable_entry_class);
    return NIM_TRUE;
}

static NimRef *
nim_symtable_entry_new (
    NimRef *symtable, NimRef *parent, NimRef *scope, int flags)
{
    NimRef *flags_obj = nim_int_new (flags);
    if (flags_obj == NULL) {
        return NULL;
    }
    symtable = symtable ? symtable : nim_nil;
    parent = parent ? parent : nim_nil;
    scope = scope ? scope : nim_nil;
    return nim_class_new_instance (
        nim_symtable_entry_class, symtable, parent, scope, flags_obj, NULL);
}

static nim_bool_t
nim_symtable_enter_scope (NimRef *self, NimRef *scope, int flags)
{
    NimRef *ste = NIM_SYMTABLE_GET_CURRENT_ENTRY(self);
    NimRef *new_ste = nim_symtable_entry_new (self, ste, scope, flags);
    if (new_ste == NULL) {
        return NIM_FALSE;
    }
    if (!nim_hash_put (NIM_SYMTABLE(self)->lookup, scope, new_ste)) {
        return NIM_FALSE;
    }
    if (ste != nim_nil) {
        if (!nim_array_push (NIM_SYMTABLE(self)->stack, ste)) {
            return NIM_FALSE;
        }
        if (!nim_array_push (NIM_SYMTABLE_ENTRY_CHILDREN(ste), new_ste)) {
            return NIM_FALSE;
        }
    }
    NIM_SYMTABLE(self)->ste = new_ste;
    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_leave_scope (NimRef *self)
{
    NimRef *stack = NIM_SYMTABLE(self)->stack;
    if (NIM_ARRAY_SIZE(stack) > 0) {
        NIM_SYMTABLE(self)->ste = nim_array_pop (stack);
    }
    return NIM_SYMTABLE_GET_CURRENT_ENTRY(self) != nim_nil;
}

static nim_bool_t
nim_symtable_add (NimRef *self, NimRef *name, int64_t flags)
{
    NimRef *ste = NIM_SYMTABLE_GET_CURRENT_ENTRY(self);
    NimRef *symbols = NIM_SYMTABLE_ENTRY(ste)->symbols;
    NimRef *fl = nim_int_new (flags);
    if (fl == NULL) {
        return NIM_FALSE;
    }
    if (!nim_hash_put (symbols, name, fl)) {
        return NIM_FALSE;
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_stmts_or_decls (NimRef *self, NimRef *arr)
{
    size_t i;

    for (i = 0; i < NIM_ARRAY_SIZE(arr); i++) {
        NimRef *item = NIM_ARRAY_ITEM(arr, i);
        if (NIM_ANY_CLASS(item) == nim_ast_stmt_class) {
            if (!nim_symtable_visit_stmt (self, item)) {
                return NIM_FALSE;
            }
        }
        else if (NIM_ANY_CLASS(item) == nim_ast_decl_class) {
            if (!nim_symtable_visit_decl (self, item)) {
                return NIM_FALSE;
            }
        }
        else {
            NIM_BUG ("dude");
            return NIM_FALSE;
        }
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_decl_func (NimRef *self, NimRef *decl)
{
    NimRef *name = NIM_AST_DECL(decl)->func.name;
    NimRef *args = NIM_AST_DECL(decl)->func.args;
    NimRef *body = NIM_AST_DECL(decl)->func.body;
    NimRef *ste = NIM_SYMTABLE_GET_CURRENT_ENTRY(self);
    int type = NIM_SYMTABLE_ENTRY(ste)->flags & NIM_SYM_TYPE_MASK;

    if (!nim_symtable_add (self, name, NIM_SYM_DECL | type)) {
        return NIM_FALSE;
    }

    if (!nim_symtable_enter_scope (self, decl, NIM_SYM_FUNC)) {
        return NIM_FALSE;
    }

    if (!nim_symtable_visit_decls (self, args)) {
        return NIM_FALSE;
    }

    if (!nim_symtable_visit_stmts_or_decls (self, body)) {
        return NIM_FALSE;
    }

    if (!nim_symtable_leave_scope (self)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_decl_class (NimRef *self, NimRef *decl)
{
    NimRef *name = NIM_AST_DECL(decl)->class.name;
    NimRef *base = NIM_AST_DECL(decl)->class.base;
    NimRef *body = NIM_AST_DECL(decl)->class.body;
    NimRef *ste = NIM_SYMTABLE_GET_CURRENT_ENTRY(self);
    int type = NIM_SYMTABLE_ENTRY(ste)->flags & NIM_SYM_TYPE_MASK;

    if (!nim_symtable_add (self, name, NIM_SYM_DECL | type)) {
        return NIM_FALSE;
    }

    if (!nim_symtable_enter_scope (self, decl, NIM_SYM_CLASS)) {
        return NIM_FALSE;
    }

    if (base != NULL) {
        /* XXX this sucks. */
        if (!nim_symtable_add (self, nim_array_first (base), 0)) {
            return NIM_FALSE;
        }
    }

    if (!nim_symtable_visit_decls (self, body)) {
        return NIM_FALSE;
    }

    if (!nim_symtable_leave_scope (self)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_decl_use (NimRef *self, NimRef *decl)
{
    NimRef *name = NIM_AST_DECL(decl)->use.name;
    NimRef *ste = NIM_SYMTABLE_GET_CURRENT_ENTRY(self);
    int type = NIM_SYMTABLE_ENTRY(ste)->flags & NIM_SYM_TYPE_MASK;

    if (!nim_symtable_add (self, name, NIM_SYM_DECL | type)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_decl_var (NimRef *self, NimRef *decl)
{
    NimRef *name = NIM_AST_DECL(decl)->var.name;
    NimRef *value = NIM_AST_DECL(decl)->var.value;
    NimRef *ste = NIM_SYMTABLE_GET_CURRENT_ENTRY(self);
    int type = NIM_SYMTABLE_ENTRY(ste)->flags & NIM_SYM_TYPE_MASK;

    if (!nim_symtable_add (self, name, NIM_SYM_DECL | type)) {
        return NIM_FALSE;
    }

    if (value != NULL) {
        if (!nim_symtable_visit_expr (self, value)) {
            return NIM_FALSE;
        }
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_decl (NimRef *self, NimRef *decl)
{
    switch (NIM_AST_DECL_TYPE(decl)) {
        case NIM_AST_DECL_FUNC:
            return nim_symtable_visit_decl_func (self, decl);
        case NIM_AST_DECL_CLASS:
            return nim_symtable_visit_decl_class (self, decl);
        case NIM_AST_DECL_USE:
            return nim_symtable_visit_decl_use (self, decl);
        case NIM_AST_DECL_VAR:
            return nim_symtable_visit_decl_var (self, decl);
        default:
            NIM_BUG ("unknown AST decl type: %d", NIM_AST_DECL_TYPE(decl));
            return NIM_FALSE;
    }
}

static nim_bool_t
nim_symtable_visit_expr_call (NimRef *self, NimRef *expr)
{
    NimRef *target = NIM_AST_EXPR(expr)->call.target;
    NimRef *args = NIM_AST_EXPR(expr)->call.args;
    size_t i;

    if (!nim_symtable_visit_expr (self, target)) {
        return NIM_FALSE;
    }

    for (i = 0; i < NIM_ARRAY_SIZE(args); i++) {
        NimRef *item = NIM_ARRAY_ITEM(args, i);
        if (!nim_symtable_visit_expr (self, item)) {
            return NIM_FALSE;
        }
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_expr_getattr (NimRef *self, NimRef *expr)
{
    NimRef *target = NIM_AST_EXPR(expr)->getattr.target;

    if (!nim_symtable_visit_expr (self, target)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_expr_getitem (NimRef *self, NimRef *expr)
{
    NimRef *target = NIM_AST_EXPR(expr)->getitem.target;

    if (!nim_symtable_visit_expr (self, target)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_expr_array (NimRef *self, NimRef *expr)
{
    size_t i;
    NimRef *arr = NIM_AST_EXPR(expr)->array.value;
    for (i = 0; i < NIM_ARRAY_SIZE(arr); i++) {
        NimRef *item = NIM_ARRAY_ITEM(arr, i);
        if (!nim_symtable_visit_expr (self, item)) {
            return NIM_FALSE;
        }
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_expr_hash (NimRef *self, NimRef *expr)
{
    size_t i;
    NimRef *arr = NIM_AST_EXPR(expr)->hash.value;
    for (i = 0; i < NIM_ARRAY_SIZE(arr); i++) {
        NimRef *item = NIM_ARRAY_ITEM(arr, i);
        if (!nim_symtable_visit_expr (self, item)) {
            return NIM_FALSE;
        }
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_expr_ident (NimRef *self, NimRef *expr)
{
    NimRef *ste = NIM_SYMTABLE_GET_CURRENT_ENTRY(self);
    NimRef *name = NIM_AST_EXPR(expr)->ident.id;

    while (ste != nim_nil) {
        int rc;
        rc = nim_hash_get (NIM_SYMTABLE_ENTRY(ste)->symbols, name, NULL);
        if (rc == 0) {
            int type = NIM_SYMTABLE_ENTRY(ste)->flags &
                        NIM_SYM_TYPE_MASK;
            if (ste != NIM_SYMTABLE_GET_CURRENT_ENTRY(self) &&
                !NIM_SYMTABLE_ENTRY_IS_MODULE(ste)) {
                return nim_symtable_add (self, name, NIM_SYM_FREE | type);
            }
            return NIM_TRUE;
        }
        ste = NIM_SYMTABLE_ENTRY(ste)->parent;
    }

    /* XXX is this really the best way to handle builtins? */
    if (nim_is_builtin (name)) {
        return nim_symtable_add (self, name, NIM_SYM_BUILTIN);
    }

    if (strcmp (NIM_STR_DATA(name), "__file__") == 0 ||
        strcmp (NIM_STR_DATA(name), "__line__") == 0) {
        return nim_symtable_add (self, name, NIM_SYM_SPECIAL);
    }

    NIM_BUG ("unknown symbol: %s", NIM_STR_DATA(name));
    return NIM_FALSE;
}

static nim_bool_t
nim_symtable_visit_expr_binop (NimRef *self, NimRef *expr)
{
    NimRef *left = NIM_AST_EXPR(expr)->binop.left;
    NimRef *right = NIM_AST_EXPR(expr)->binop.right;
    if (!nim_symtable_visit_expr (self, left)) {
        return NIM_FALSE;
    }
    if (!nim_symtable_visit_expr (self, right)) {
        return NIM_FALSE;
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_expr_fn (NimRef *self, NimRef *expr)
{
    NimRef *args = NIM_AST_EXPR(expr)->fn.args;
    NimRef *body = NIM_AST_EXPR(expr)->fn.body;

    if (!nim_symtable_enter_scope (self, expr, NIM_SYM_FUNC)) {
        return NIM_FALSE;
    }

    if (!nim_symtable_visit_decls (self, args)) {
        return NIM_FALSE;
    }

    if (!nim_symtable_visit_stmts_or_decls (self, body)) {
        return NIM_FALSE;
    }

    if (!nim_symtable_leave_scope (self)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_expr_spawn (NimRef *self, NimRef *expr)
{
    size_t i;
    NimRef *target = NIM_AST_EXPR(expr)->spawn.target;
    NimRef *args = NIM_AST_EXPR(expr)->spawn.args;

    if (!nim_symtable_visit_expr (self, target)) {
        return NIM_FALSE;
    }

    for (i = 0; i < NIM_ARRAY_SIZE(args); i++) {
        if (!nim_symtable_visit_expr (self, NIM_ARRAY_ITEM(args, i))) {
            return NIM_FALSE;
        }
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_expr_not (NimRef *self, NimRef *expr)
{
    return nim_symtable_visit_expr (self, NIM_AST_EXPR(expr)->not.value);
}

static nim_bool_t
nim_symtable_visit_expr (NimRef *self, NimRef *expr)
{
    switch (NIM_AST_EXPR_TYPE(expr)) {
        case NIM_AST_EXPR_CALL:
            return nim_symtable_visit_expr_call (self, expr);
        case NIM_AST_EXPR_GETATTR:
            return nim_symtable_visit_expr_getattr (self, expr);
        case NIM_AST_EXPR_GETITEM:
            return nim_symtable_visit_expr_getitem (self, expr);
        case NIM_AST_EXPR_ARRAY:
            return nim_symtable_visit_expr_array (self, expr);
        case NIM_AST_EXPR_HASH:
            return nim_symtable_visit_expr_hash (self, expr);
        case NIM_AST_EXPR_IDENT:
            return nim_symtable_visit_expr_ident (self, expr);
        case NIM_AST_EXPR_STR:
            return NIM_TRUE;
        case NIM_AST_EXPR_BOOL:
            return NIM_TRUE;
        case NIM_AST_EXPR_NIL:
            return NIM_TRUE;
        case NIM_AST_EXPR_BINOP:
            return nim_symtable_visit_expr_binop (self, expr);
        case NIM_AST_EXPR_INT_:
            return NIM_TRUE;
        case NIM_AST_EXPR_FLOAT_:
            return NIM_TRUE;
        case NIM_AST_EXPR_FN:
            return nim_symtable_visit_expr_fn (self, expr);
        case NIM_AST_EXPR_SPAWN:
            return nim_symtable_visit_expr_spawn (self, expr);
        case NIM_AST_EXPR_NOT:
            return nim_symtable_visit_expr_not (self, expr);
        default:
            NIM_BUG ("unknown AST expr type: %d", NIM_AST_EXPR_TYPE(expr));
            return NIM_FALSE;
    }
}

static nim_bool_t
nim_symtable_visit_stmt_assign (NimRef *self, NimRef *stmt)
{
    NimRef *target = NIM_AST_STMT(stmt)->assign.target;
    NimRef *value = NIM_AST_STMT(stmt)->assign.value;

    if (!nim_symtable_visit_expr (self, target)) {
        return NIM_FALSE;
    }

    if (!nim_symtable_visit_expr (self, value)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_stmt_if_ (NimRef *self, NimRef *stmt)
{
    NimRef *expr = NIM_AST_STMT(stmt)->if_.expr;
    NimRef *body = NIM_AST_STMT(stmt)->if_.body;
    NimRef *orelse = NIM_AST_STMT(stmt)->if_.orelse;

    if (!nim_symtable_visit_expr (self, expr)) {
        return NIM_FALSE;
    }

    if (!nim_symtable_visit_stmts_or_decls (self, body)) {
        return NIM_FALSE;
    }

    if (orelse != NULL) {
        if (!nim_symtable_visit_stmts_or_decls (self, orelse)) {
            return NIM_FALSE;
        }
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_stmt_while_ (NimRef *self, NimRef *stmt)
{
    NimRef *expr = NIM_AST_STMT(stmt)->while_.expr;
    NimRef *body = NIM_AST_STMT(stmt)->while_.body;

    if (!nim_symtable_visit_expr (self, expr)) {
        return NIM_FALSE;
    }

    if (!nim_symtable_visit_stmts_or_decls (self, body)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_stmt_pattern_test (NimRef *self, NimRef *test)
{
    switch (NIM_AST_EXPR(test)->type) {
        case NIM_AST_EXPR_IDENT:
            {
                NimRef *id = NIM_AST_EXPR(test)->ident.id;
                NimRef *ste = NIM_SYMTABLE_GET_CURRENT_ENTRY(self);
                int type = NIM_SYMTABLE_ENTRY(ste)->flags &
                            NIM_SYM_TYPE_MASK;
                if (!nim_symtable_add (self, id, NIM_SYM_DECL | type)) {
                    return NIM_FALSE;
                }

                break;
            }
        case NIM_AST_EXPR_ARRAY:
            {
                NimRef *array = NIM_AST_EXPR(test)->array.value;
                size_t i;
                for (i = 0; i < NIM_ARRAY_SIZE(array); i++) {
                    NimRef *item = NIM_ARRAY_ITEM(array, i);
                    if (!nim_symtable_visit_stmt_pattern_test (self, item)) {
                        return NIM_FALSE;
                    }
                }

                break;
            }
        case NIM_AST_EXPR_HASH:
            {
                NimRef *hash = NIM_AST_EXPR(test)->hash.value;
                size_t i;
                /* XXX hashes are encoded as arrays in the AST */
                for (i = 0; i < NIM_ARRAY_SIZE(hash); i += 2) {
                    NimRef *key = NIM_ARRAY(hash)->items[i];
                    NimRef *value = NIM_ARRAY(hash)->items[i+1];
                    if (!nim_symtable_visit_stmt_pattern_test (self, key)) {
                        return NIM_FALSE;
                    }
                    if (!nim_symtable_visit_stmt_pattern_test (self, value)) {
                        return NIM_FALSE;
                    }
                }
            }
        default:
            break;
    };

    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_stmt_patterns (NimRef *self, NimRef *patterns)
{
    size_t size = NIM_ARRAY_SIZE(patterns);
    size_t i;

    for (i = 0; i < size; i++) {
        NimRef *pattern = NIM_ARRAY_ITEM(patterns, i);
        NimRef *test = NIM_AST_STMT(pattern)->pattern.test;
        NimRef *body = NIM_AST_STMT(pattern)->pattern.body;

        if (!nim_symtable_visit_stmt_pattern_test (self, test)) {
            return NIM_FALSE;
        }

        if (!nim_symtable_visit_stmts_or_decls (self, body)) {
            return NIM_FALSE;
        }
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_stmt_match (NimRef *self, NimRef *stmt)
{
    NimRef *expr = NIM_AST_STMT(stmt)->match.expr;
    NimRef *body = NIM_AST_STMT(stmt)->match.body;
    
    if (!nim_symtable_visit_expr (self, expr)) {
        return NIM_FALSE;
    }

    if (!nim_symtable_visit_stmt_patterns (self, body)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_stmt_ret (NimRef *self, NimRef *stmt)
{
    NimRef *expr = NIM_AST_STMT(stmt)->ret.expr;
    if (expr != NULL) {
        if (!nim_symtable_visit_expr (self, expr)) {
            return NIM_FALSE;
        }
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_stmt (NimRef *self, NimRef *stmt)
{
    switch (NIM_AST_STMT_TYPE(stmt)) {
        case NIM_AST_STMT_EXPR:
            return nim_symtable_visit_expr (self, NIM_AST_STMT(stmt)->expr.expr);
        case NIM_AST_STMT_ASSIGN:
            return nim_symtable_visit_stmt_assign (self, stmt);
        case NIM_AST_STMT_IF_:
            return nim_symtable_visit_stmt_if_ (self, stmt);
        case NIM_AST_STMT_WHILE_:
            return nim_symtable_visit_stmt_while_ (self, stmt);
        case NIM_AST_STMT_MATCH:
            return nim_symtable_visit_stmt_match (self, stmt);
        case NIM_AST_STMT_RET:
            return nim_symtable_visit_stmt_ret (self, stmt);
        case NIM_AST_STMT_BREAK_:
            return NIM_TRUE;
        default:
            NIM_BUG ("unknown AST stmt type: %d", NIM_AST_STMT_TYPE(stmt));
            return NIM_FALSE;
    }
}

static nim_bool_t
nim_symtable_visit_decls (NimRef *self, NimRef *decls)
{
    size_t i;

    for (i = 0; i < NIM_ARRAY_SIZE(decls); i++) {
        if (!nim_symtable_visit_decl (self, NIM_ARRAY_ITEM(decls, i))) {
            /* TODO error message? */
            return NIM_FALSE;
        }
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_symtable_visit_mod (NimRef *self, NimRef *mod)
{
    NimRef *uses = NIM_AST_MOD(mod)->root.uses;
    NimRef *body = NIM_AST_MOD(mod)->root.body;

    if (!nim_symtable_enter_scope (self, mod, NIM_SYM_MODULE)) {
        return NIM_FALSE;
    }

    if (!nim_symtable_visit_decls (self, uses)) {
        return NIM_FALSE;
    }

    if (!nim_symtable_visit_decls (self, body)) {
        return NIM_FALSE;
    }

    if (!nim_symtable_leave_scope (self)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

NimRef *
nim_symtable_new_from_ast (NimRef *filename, NimRef *ast)
{
    return nim_class_new_instance (nim_symtable_class, filename, ast, NULL);
}

NimRef *
nim_symtable_lookup (NimRef *self, NimRef *scope)
{
    NimRef *sym;
    int rc = nim_hash_get (NIM_SYMTABLE(self)->lookup, scope, &sym);
    if (rc < 0) {
        NIM_BUG ("Error looking up symtable entry for scope %p", scope);
        return NULL;
    }
    else if (rc > 0) {
        NIM_BUG ("No symtable entry for scope %p", scope);
        return NULL;
    }
    return sym;
}

nim_bool_t
nim_symtable_entry_sym_flags (
    NimRef *self, NimRef *name, int64_t *flags)
{
    NimRef *ste = self;
    while (ste != nim_nil) {
        NimRef *symbols = NIM_SYMTABLE_ENTRY(ste)->symbols;
        NimRef *ref;
        int rc;
        
        rc = nim_hash_get (symbols, name, &ref);
        if (rc == 0) {
            if (flags != NULL) {
                *flags = NIM_INT(ref)->value;
            }
            return NIM_TRUE;
        }
        ste = NIM_SYMTABLE_ENTRY(ste)->parent;
    }
    return NIM_FALSE;
}

nim_bool_t
nim_symtable_entry_sym_exists (NimRef *self, NimRef *name)
{
    return nim_symtable_entry_sym_flags (self, name, NULL);
}

