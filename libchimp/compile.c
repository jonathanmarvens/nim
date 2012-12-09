#include "chimp/compile.h"
#include "chimp/str.h"
#include "chimp/ast.h"
#include "chimp/code.h"
#include "chimp/array.h"
#include "chimp/int.h"
#include "chimp/method.h"
#include "chimp/object.h"
#include "chimp/task.h"
#include "chimp/_parser.h"
#include "chimp/symtable.h"

typedef enum _ChimpUnitType {
    CHIMP_UNIT_TYPE_CODE,
    CHIMP_UNIT_TYPE_MODULE
} ChimpUnitType;

typedef struct _ChimpCodeUnit {
    ChimpUnitType type;
    union {
        ChimpRef *code;
        ChimpRef *module;
        ChimpRef *value;
    };
    struct _ChimpCodeUnit *next;
    ChimpRef              *ste;
} ChimpCodeUnit;

typedef struct _ChimpCodeCompiler {
    ChimpCodeUnit *current_unit;
    ChimpRef      *symtable;
} ChimpCodeCompiler;

#define CHIMP_COMPILER_CODE(c) ((c)->current_unit)->code
#define CHIMP_COMPILER_MODULE(c) ((c)->current_unit)->module
#define CHIMP_COMPILER_SCOPE(c) ((c)->current_unit)->value

#define CHIMP_COMPILER_IN_MODULE(c) (((c)->current_unit)->type == CHIMP_UNIT_TYPE_MODULE)
#define CHIMP_COMPILER_IN_CODE(c) (((c)->current_unit)->type == CHIMP_UNIT_TYPE_CODE)
#define CHIMP_COMPILER_SYMTABLE_ENTRY(c) (((c)->current_unit))->ste

static chimp_bool_t
chimp_compile_ast_decl (ChimpCodeCompiler *c, ChimpRef *decl);

static chimp_bool_t
chimp_compile_ast_stmt (ChimpCodeCompiler *c, ChimpRef *stmt);

static chimp_bool_t
chimp_compile_ast_expr (ChimpCodeCompiler *c, ChimpRef *expr);

static chimp_bool_t
chimp_compile_ast_expr_ident (ChimpCodeCompiler *c, ChimpRef *expr);

static chimp_bool_t
chimp_compile_ast_expr_array (ChimpCodeCompiler *c, ChimpRef *expr);

static chimp_bool_t
chimp_compile_ast_expr_hash (ChimpCodeCompiler *c, ChimpRef *expr);

static chimp_bool_t
chimp_compile_ast_expr_str (ChimpCodeCompiler *c, ChimpRef *expr);

static chimp_bool_t
chimp_compile_ast_expr_int_ (ChimpCodeCompiler *c, ChimpRef *expr);

static chimp_bool_t
chimp_compile_ast_expr_bool (ChimpCodeCompiler *c, ChimpRef *expr);

static chimp_bool_t
chimp_compile_ast_expr_call (ChimpCodeCompiler *c, ChimpRef *expr);

chimp_bool_t
chimp_compile_ast_expr_fn (ChimpCodeCompiler *c, ChimpRef *expr);

static chimp_bool_t
chimp_compile_ast_expr_getattr (ChimpCodeCompiler *c, ChimpRef *expr);

static chimp_bool_t
chimp_compile_ast_expr_binop (ChimpCodeCompiler *c, ChimpRef *binop);

static ChimpRef *
chimp_code_compiler_push_unit (ChimpCodeCompiler *c, ChimpUnitType type, ChimpRef *scope)
{
    ChimpCodeUnit *unit = CHIMP_MALLOC(ChimpCodeUnit, sizeof(*unit));
    if (unit == NULL) {
        return NULL;
    }
    memset (unit, 0, sizeof(*unit));
    switch (type) {
        case CHIMP_UNIT_TYPE_CODE:
            unit->code = chimp_code_new ();
            break;
        case CHIMP_UNIT_TYPE_MODULE:
            /* XXX name hard-coded */
            unit->module = chimp_module_new (chimp_nil, NULL);
            break;
        default:
            chimp_bug (__FILE__, __LINE__, "unknown unit type: %d", type);
            CHIMP_FREE (unit);
            return NULL;
    };
    if (unit->value == NULL) {
        CHIMP_FREE (unit);
        return NULL;
    }
    unit->type = type;
    unit->next = c->current_unit;
    unit->ste = chimp_symtable_lookup (c->symtable, scope);
    if (unit->ste == NULL) {
        CHIMP_FREE (unit);
        chimp_bug (__FILE__, __LINE__, "symtable lookup error for scope %p", scope);
        return NULL;
    }
    if (unit->ste == chimp_nil) {
        CHIMP_FREE (unit);
        chimp_bug (__FILE__, __LINE__, "symtable lookup failed for scope %p", scope);
        return NULL;
    }
    c->current_unit = unit;
    return unit->value;
}

inline static ChimpRef *
chimp_code_compiler_push_code_unit (ChimpCodeCompiler *c, ChimpRef *scope)
{
    return chimp_code_compiler_push_unit (c, CHIMP_UNIT_TYPE_CODE, scope);
}

inline static ChimpRef *
chimp_code_compiler_push_module_unit (ChimpCodeCompiler *c, ChimpRef *scope)
{
    return chimp_code_compiler_push_unit (c, CHIMP_UNIT_TYPE_MODULE, scope);
}

static ChimpRef *
chimp_code_compiler_pop_unit (ChimpCodeCompiler *c, ChimpUnitType expected)
{
    ChimpCodeUnit *unit = c->current_unit;
    if (unit == NULL) {
        chimp_bug (__FILE__, __LINE__, "NULL code unit?");
        return NULL;
    }
    if (unit->type != expected) {
        chimp_bug (__FILE__, __LINE__, "unexpected unit type!");
        return NULL;
    }
    c->current_unit = unit->next;
    CHIMP_FREE(unit);
    if (c->current_unit != NULL) {
        return c->current_unit->value;
    }
    else {
        return chimp_true;
    }
}

inline static ChimpRef *
chimp_code_compiler_pop_code_unit (ChimpCodeCompiler *c)
{
    return chimp_code_compiler_pop_unit (c, CHIMP_UNIT_TYPE_CODE);
}

inline static ChimpRef *
chimp_code_compiler_pop_module_unit (ChimpCodeCompiler *c)
{
    return chimp_code_compiler_pop_unit (c, CHIMP_UNIT_TYPE_MODULE);
}

static chimp_bool_t
chimp_compile_ast_stmts (ChimpCodeCompiler *c, ChimpRef *stmts)
{
    size_t i;

    for (i = 0; i < CHIMP_ARRAY_SIZE(stmts); i++) {
        if (CHIMP_ANY_TYPE(CHIMP_ARRAY_ITEM(stmts, i)) == CHIMP_VALUE_TYPE_AST_STMT) {
            if (!chimp_compile_ast_stmt (c, CHIMP_ARRAY_ITEM(stmts, i))) {
                /* TODO error message? */
                return CHIMP_FALSE;
            }
        }
        else if (CHIMP_ANY_TYPE(CHIMP_ARRAY_ITEM(stmts, i)) == CHIMP_VALUE_TYPE_AST_DECL) {
            if (!chimp_compile_ast_decl (c, CHIMP_ARRAY_ITEM(stmts, i))) {
                return CHIMP_FALSE;
            }
        }
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_decls (ChimpCodeCompiler *c, ChimpRef *decls)
{
    size_t i;

    for (i = 0; i < CHIMP_ARRAY_SIZE(decls); i++) {
        if (!chimp_compile_ast_decl (c, CHIMP_ARRAY_ITEM(decls, i))) {
            /* TODO error message? */
            return CHIMP_FALSE;
        }
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_mod (ChimpCodeCompiler *c, ChimpRef *mod)
{
    ChimpRef *uses = CHIMP_AST_MOD(mod)->root.uses;
    ChimpRef *body = CHIMP_AST_MOD(mod)->root.body;

    if (!chimp_compile_ast_decls (c, uses)) {
        return CHIMP_FALSE;
    }

    /* TODO check CHIMP_AST_MOD_* type */
    if (!chimp_compile_ast_decls (c, body)) {
        return CHIMP_FALSE;
    }

    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_stmt_assign (ChimpCodeCompiler *c, ChimpRef *stmt)
{
    ChimpRef *name =
        CHIMP_AST_EXPR(CHIMP_AST_STMT(stmt)->assign.target)->ident.id;
    ChimpRef *code = CHIMP_COMPILER_CODE(c);

    if (!chimp_compile_ast_expr (c, CHIMP_AST_STMT(stmt)->assign.value)) {
        return CHIMP_FALSE;
    }

    if (!chimp_code_storename (code, name)) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_stmt_while_ (ChimpCodeCompiler *c, ChimpRef *stmt)
{
    ChimpRef *code = CHIMP_COMPILER_CODE(c);
    ChimpLabel end_body;
    ChimpLabel start_body;
    ChimpLabel p = CHIMP_CODE(code)->used;

    if (!chimp_compile_ast_expr (c, CHIMP_AST_STMT(stmt)->while_.expr)) {
        return CHIMP_FALSE;
    }

    if (!chimp_code_jumpiffalse (code, &end_body)) {
        return CHIMP_FALSE;
    }

    if (!chimp_compile_ast_stmts (c, CHIMP_AST_STMT(stmt)->while_.body)) {
        return CHIMP_FALSE;
    }

    if (!chimp_code_jump (code, &start_body)) {
        return CHIMP_FALSE;
    }

    if (!chimp_code_set_jump_location (code, start_body, p)) {
        return CHIMP_FALSE;
    }

    if (!chimp_code_patch_jump_location (code, end_body)) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_stmt_if_ (ChimpCodeCompiler *c, ChimpRef *stmt)
{
    ChimpRef *code = CHIMP_COMPILER_CODE(c);
    ChimpLabel end_body;

    if (!chimp_compile_ast_expr (c, CHIMP_AST_STMT(stmt)->if_.expr)) {
        return CHIMP_FALSE;
    }

    if (!chimp_code_jumpiffalse (code, &end_body)) {
        return CHIMP_FALSE;
    }

    if (!chimp_compile_ast_stmts (c, CHIMP_AST_STMT(stmt)->if_.body)) {
        return CHIMP_FALSE;
    }

    if (CHIMP_AST_STMT(stmt)->if_.orelse != NULL) {
        ChimpLabel end_orelse;

        if (!chimp_code_jump (code, &end_orelse)) {
            return CHIMP_FALSE;
        }

        if (!chimp_code_patch_jump_location (code, end_body)) {
            return CHIMP_FALSE;
        }

        if (!chimp_compile_ast_stmts (c, CHIMP_AST_STMT(stmt)->if_.orelse)) {
            return CHIMP_FALSE;
        }

        if (!chimp_code_patch_jump_location (code, end_orelse)) {
            return CHIMP_FALSE;
        }
    }
    else {
        if (!chimp_code_patch_jump_location (code, end_body)) {
            return CHIMP_FALSE;
        }
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_stmt_ret (ChimpCodeCompiler *c, ChimpRef *stmt)
{
    ChimpRef *code = CHIMP_COMPILER_CODE(c);
    ChimpRef *expr = CHIMP_AST_STMT(stmt)->ret.expr;
    if (expr != NULL) {
        if (!chimp_compile_ast_expr (c, expr)) {
            return CHIMP_FALSE;
        }
    }
    else if (!chimp_code_pushnil (code)) {
        return CHIMP_FALSE;
    }

    if (!chimp_code_ret (code)) {
        return CHIMP_FALSE;
    }

    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_stmt_panic (ChimpCodeCompiler *c, ChimpRef *stmt)
{
    ChimpRef *code = CHIMP_COMPILER_CODE(c);
    ChimpRef *expr = CHIMP_AST_STMT(stmt)->panic.expr;
    if (expr != NULL) {
        if (!chimp_compile_ast_expr (c, expr)) {
            return CHIMP_FALSE;
        }
    }
    else if (!chimp_code_pushnil (code)) {
        return CHIMP_FALSE;
    }

    if (!chimp_code_panic (code)) {
        return CHIMP_FALSE;
    }

    return CHIMP_TRUE;
}

static ChimpRef *
chimp_compile_get_current_module (ChimpCodeCompiler *c)
{
    if (CHIMP_COMPILER_IN_MODULE(c)) {
        return CHIMP_COMPILER_MODULE(c);
    }
    else {
        ChimpCodeUnit *unit = c->current_unit;
        if (unit == NULL) return NULL;
        unit = unit->next;
        while (unit != NULL) {
            if (unit->type == CHIMP_UNIT_TYPE_MODULE) {
                return unit->module;
            }
            unit = unit->next;
        }
        return NULL;
    }
}

static ChimpRef *
chimp_compile_bytecode_method (ChimpCodeCompiler *c, ChimpRef *fn, ChimpRef *args, ChimpRef *body)
{
    ChimpRef *mod;
    ChimpRef *func_code;
    ChimpRef *method;
    size_t i;

    func_code = chimp_code_compiler_push_code_unit (c, fn);
    if (func_code == NULL) {
        return NULL;
    }

    /* unpack arguments */
    for (i = 0; i < CHIMP_ARRAY_SIZE(args); i++) {
        ChimpRef *var_decl = CHIMP_ARRAY_ITEM(args, CHIMP_ARRAY_SIZE(args) - i - 1);
        if (!chimp_code_storename (func_code, CHIMP_AST_DECL(var_decl)->var.name)) {
            return NULL;
        }
    }
    
    if (!chimp_compile_ast_stmts (c, body)) {
        return NULL;
    }

    if (!chimp_code_compiler_pop_code_unit (c)) {
        return NULL;
    }

    mod = chimp_compile_get_current_module (c);

    method = chimp_method_new_bytecode (mod, func_code);
    if (method == NULL) {
        return NULL;
    }

    return method;
}

static chimp_bool_t
chimp_compile_ast_decl_func (ChimpCodeCompiler *c, ChimpRef *decl)
{
    ChimpRef *method;
    ChimpRef *mod;

    method = chimp_compile_bytecode_method (
        c,
        decl,
        CHIMP_AST_DECL(decl)->func.args,
        CHIMP_AST_DECL(decl)->func.body
    );
    if (method == NULL) {
        return CHIMP_FALSE;
    }

    if (CHIMP_COMPILER_IN_CODE(c)) {
        ChimpRef *code = CHIMP_COMPILER_CODE(c);
        if (!chimp_code_pushconst (code, method)) {
            return CHIMP_FALSE;
        }

        if (!chimp_code_storename (code, CHIMP_AST_DECL(decl)->func.name)) {
            return CHIMP_FALSE;
        }
    }
    else {
        mod = chimp_compile_get_current_module (c);
        ChimpRef *name = CHIMP_AST_DECL(decl)->func.name;
        if (!chimp_module_add_local (mod, name, method)) {
            return CHIMP_FALSE;
        }
    }

    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_decl_use (ChimpCodeCompiler *c, ChimpRef *decl)
{
    ChimpRef *module = CHIMP_COMPILER_MODULE(c);
    ChimpRef *import;
    ChimpRef *name;

    name = CHIMP_AST_DECL(decl)->use.name;
    import = chimp_task_find_module (NULL, name);
    if (import == NULL) {
        /* TODO load new module from file */
        return CHIMP_FALSE;
    }

    if (!chimp_module_add_local (module, name, import)) {
        return CHIMP_FALSE;
    }

    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_decl_var (ChimpCodeCompiler *c, ChimpRef *decl)
{
    ChimpRef *scope = CHIMP_COMPILER_SCOPE(c);
    ChimpRef *name = CHIMP_AST_DECL(decl)->var.name;
    ChimpRef *value = CHIMP_AST_DECL(decl)->var.value;

    if (CHIMP_ANY_TYPE(scope) == CHIMP_VALUE_TYPE_MODULE) {
        return chimp_module_add_local (scope, name, value == NULL ? chimp_nil : value);
    }
    else {
        /* TODO it's really about time we get ourselves a symbol table */
        if (value != NULL) {
            ChimpRef *code = CHIMP_COMPILER_CODE(c);

            if (!chimp_compile_ast_expr (c, value)) {
                return CHIMP_FALSE;
            }

            if (!chimp_code_storename (code, name)) {
                return CHIMP_FALSE;
            }
        }
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_decl (ChimpCodeCompiler *c, ChimpRef *decl)
{
    switch (CHIMP_AST_DECL_TYPE(decl)) {
        case CHIMP_AST_DECL_FUNC:
            return chimp_compile_ast_decl_func (c, decl);
        case CHIMP_AST_DECL_USE:
            return chimp_compile_ast_decl_use (c, decl);
        case CHIMP_AST_DECL_VAR:
            return chimp_compile_ast_decl_var (c, decl);
        default:
            chimp_bug (__FILE__, __LINE__, "unknown AST stmt type: %d", CHIMP_AST_DECL_TYPE(decl));
            return CHIMP_FALSE;
    }
}

static chimp_bool_t
chimp_compile_ast_stmt (ChimpCodeCompiler *c, ChimpRef *stmt)
{
    switch (CHIMP_AST_STMT_TYPE(stmt)) {
        case CHIMP_AST_STMT_EXPR:
            return chimp_compile_ast_expr (c, CHIMP_AST_STMT(stmt)->expr.expr);
        case CHIMP_AST_STMT_ASSIGN:
            return chimp_compile_ast_stmt_assign (c, stmt);
        case CHIMP_AST_STMT_IF_:
            return chimp_compile_ast_stmt_if_ (c, stmt);
        case CHIMP_AST_STMT_WHILE_:
            return chimp_compile_ast_stmt_while_ (c, stmt);
        case CHIMP_AST_STMT_RET:
            return chimp_compile_ast_stmt_ret (c, stmt);
        case CHIMP_AST_STMT_PANIC:
            return chimp_compile_ast_stmt_panic (c, stmt);
        default:
            chimp_bug (__FILE__, __LINE__, "unknown AST stmt type: %d", CHIMP_AST_STMT_TYPE(stmt));
            return CHIMP_FALSE;
    };
}

static chimp_bool_t
chimp_compile_ast_expr (ChimpCodeCompiler *c, ChimpRef *expr)
{
    switch (CHIMP_AST_EXPR_TYPE(expr)) {
        case CHIMP_AST_EXPR_CALL:
            return chimp_compile_ast_expr_call (c, expr);
        case CHIMP_AST_EXPR_GETATTR:
            return chimp_compile_ast_expr_getattr (c, expr);
        case CHIMP_AST_EXPR_ARRAY:
            return chimp_compile_ast_expr_array (c, expr);
        case CHIMP_AST_EXPR_HASH:
            return chimp_compile_ast_expr_hash (c, expr);
        case CHIMP_AST_EXPR_IDENT:
            return chimp_compile_ast_expr_ident (c, expr);
        case CHIMP_AST_EXPR_STR:
            return chimp_compile_ast_expr_str (c, expr);
        case CHIMP_AST_EXPR_BOOL:
            return chimp_compile_ast_expr_bool (c, expr);
        case CHIMP_AST_EXPR_NIL:
            return chimp_code_pushnil (CHIMP_COMPILER_CODE(c));
        case CHIMP_AST_EXPR_BINOP:
            return chimp_compile_ast_expr_binop (c, expr);
        case CHIMP_AST_EXPR_INT_:
            return chimp_compile_ast_expr_int_ (c, expr);
        case CHIMP_AST_EXPR_FN:
            return chimp_compile_ast_expr_fn (c, expr);
        default:
            chimp_bug (__FILE__, __LINE__, "unknown AST expr type: %d", CHIMP_AST_EXPR_TYPE(expr));
            return CHIMP_FALSE;
    };
}

static chimp_bool_t
chimp_compile_ast_expr_call (ChimpCodeCompiler *c, ChimpRef *expr)
{
    ChimpRef *target;
    ChimpRef *args;
    size_t i;
    ChimpRef *code = CHIMP_COMPILER_CODE(c);

    target = CHIMP_AST_EXPR(expr)->call.target;
    if (!chimp_compile_ast_expr (c, target)) {
        return CHIMP_FALSE;
    }

    args = CHIMP_AST_EXPR(expr)->call.args;
    for (i = 0; i < CHIMP_ARRAY_SIZE(args); i++) {
        if (!chimp_compile_ast_expr (c, CHIMP_ARRAY_ITEM(args, i))) {
            return CHIMP_FALSE;
        }
    }

    if (!chimp_code_makearray (code, CHIMP_ARRAY_SIZE(args))) {
        return CHIMP_FALSE;
    }

    if (!chimp_code_call (code)) {
        return CHIMP_FALSE;
    }

    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_expr_getattr (ChimpCodeCompiler *c, ChimpRef *expr)
{
    ChimpRef *target;
    ChimpRef *attr;
    ChimpRef *code = CHIMP_COMPILER_CODE(c);

    target = CHIMP_AST_EXPR(expr)->getattr.target;
    if (!chimp_compile_ast_expr (c, target)) {
        return CHIMP_FALSE;
    }

    attr = CHIMP_AST_EXPR(expr)->getattr.attr;
    if (attr == NULL) {
        return CHIMP_FALSE;
    }

    if (!chimp_code_getattr (code, attr)) {
        return CHIMP_FALSE;
    }

    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_expr_ident (ChimpCodeCompiler *c, ChimpRef *expr)
{
    ChimpRef *code = CHIMP_COMPILER_CODE(c);
    if (chimp_code_pushname (code, CHIMP_AST_EXPR(expr)->ident.id) < 0) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_expr_array (ChimpCodeCompiler *c, ChimpRef *expr)
{
    size_t i;
    ChimpRef *code = CHIMP_COMPILER_CODE(c);
    ChimpRef *arr = CHIMP_AST_EXPR(expr)->array.value;
    for (i = 0; i < CHIMP_ARRAY_SIZE(arr); i++) {
        if (!chimp_compile_ast_expr (c, CHIMP_ARRAY_ITEM(arr, i))) {
            return CHIMP_FALSE;
        }
    }
    if (!chimp_code_makearray (code, CHIMP_ARRAY_SIZE(arr))) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_expr_hash (ChimpCodeCompiler *c, ChimpRef *expr)
{
    size_t i;
    ChimpRef *code = CHIMP_COMPILER_CODE(c);
    ChimpRef *arr = CHIMP_AST_EXPR(expr)->hash.value;
    /* TODO ensure array size is even (key/value pairs) */
    for (i = 0; i < CHIMP_ARRAY_SIZE(arr); i++) {
        if (!chimp_compile_ast_expr (c, CHIMP_ARRAY_ITEM(arr, i))) {
            return CHIMP_FALSE;
        }
    }
    if (!chimp_code_makehash (code, CHIMP_ARRAY_SIZE(arr) / 2)) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_expr_str (ChimpCodeCompiler *c, ChimpRef *expr)
{
    ChimpRef *code = CHIMP_COMPILER_CODE(c);
    if (chimp_code_pushconst (code, CHIMP_AST_EXPR(expr)->str.value) < 0) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_expr_bool (ChimpCodeCompiler *c, ChimpRef *expr)
{
    ChimpRef *code = CHIMP_COMPILER_CODE(c);
    if (chimp_code_pushconst (code, CHIMP_AST_EXPR(expr)->bool.value) < 0) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_expr_int_ (ChimpCodeCompiler *c, ChimpRef *expr)
{
    ChimpRef *code = CHIMP_COMPILER_CODE(c);
    if (chimp_code_pushconst (code, CHIMP_AST_EXPR(expr)->int_.value) < 0) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_expr_binop (ChimpCodeCompiler *c, ChimpRef *expr)
{
    ChimpRef *code = CHIMP_COMPILER_CODE(c);
    if (!chimp_compile_ast_expr (c, CHIMP_AST_EXPR(expr)->binop.left)) {
        return CHIMP_FALSE;
    }

    switch (CHIMP_AST_EXPR(expr)->binop.op) {
        case CHIMP_BINOP_EQ:
            {
                if (!chimp_compile_ast_expr (c, CHIMP_AST_EXPR(expr)->binop.right)) {
                    return CHIMP_FALSE;
                }

                if (!chimp_code_eq (code)) {
                    return CHIMP_FALSE;
                }
                break;
            }
        case CHIMP_BINOP_NEQ:
            {
                if (!chimp_compile_ast_expr (c, CHIMP_AST_EXPR(expr)->binop.right)) {
                    return CHIMP_FALSE;
                }

                if (!chimp_code_neq (code)) {
                    return CHIMP_FALSE;
                }
                break;
            }
        case CHIMP_BINOP_GT:
            {
                if (!chimp_compile_ast_expr (c, CHIMP_AST_EXPR(expr)->binop.right)) {
                    return CHIMP_FALSE;
                }

                if (!chimp_code_gt (code)) {
                    return CHIMP_FALSE;
                }
                break;
            }
        case CHIMP_BINOP_GTE:
            {
                if (!chimp_compile_ast_expr (c, CHIMP_AST_EXPR(expr)->binop.right)) {
                    return CHIMP_FALSE;
                }

                if (!chimp_code_gte (code)) {
                    return CHIMP_FALSE;
                }
                break;
            }
        case CHIMP_BINOP_LT:
            {
                if (!chimp_compile_ast_expr (c, CHIMP_AST_EXPR(expr)->binop.right)) {
                    return CHIMP_FALSE;
                }

                if (!chimp_code_lt (code)) {
                    return CHIMP_FALSE;
                }
                break;
            }
        case CHIMP_BINOP_LTE:
            {
                if (!chimp_compile_ast_expr (c, CHIMP_AST_EXPR(expr)->binop.right)) {
                    return CHIMP_FALSE;
                }

                if (!chimp_code_lte (code)) {
                    return CHIMP_FALSE;
                }
                break;
            }
        case CHIMP_BINOP_OR:
            {
                /* short-circuited logical OR */
                ChimpLabel right_label;
                ChimpLabel end_label;

                if (!chimp_code_jumpiffalse (code, &right_label)) {
                    return CHIMP_FALSE;
                }
                if (!chimp_code_jump (code, &end_label)) {
                    return CHIMP_FALSE;
                }
                if (!chimp_code_patch_jump_location (code, right_label)) {
                    return CHIMP_FALSE;
                }
                if (!chimp_compile_ast_expr (c, CHIMP_AST_EXPR(expr)->binop.right)) {
                    return CHIMP_FALSE;
                }
                if (!chimp_code_patch_jump_location (code, end_label)) {
                    return CHIMP_FALSE;
                }

                break;
            }
        case CHIMP_BINOP_AND:
            {
                /* short-circuited logical AND */

                ChimpLabel right_label;
                ChimpLabel end_label;

                if (!chimp_code_jumpiftrue (code, &right_label)) {
                    return CHIMP_FALSE;
                }
                if (!chimp_code_jump (code, &end_label)) {
                    return CHIMP_FALSE;
                }
                if (!chimp_code_patch_jump_location (code, right_label)) {
                    return CHIMP_FALSE;
                }
                /* JUMPIFTRUE will pop the value on our behalf */
                /*
                if (!chimp_code_pop (code)) {
                    return NULL;
                }
                */
                if (!chimp_compile_ast_expr (c, CHIMP_AST_EXPR(expr)->binop.right)) {
                    return CHIMP_FALSE;
                }
                if (!chimp_code_patch_jump_location (code, end_label)) {
                    return CHIMP_FALSE;
                }

                break;
            }
        case CHIMP_BINOP_ADD:
            {
                if (!chimp_compile_ast_expr (c, CHIMP_AST_EXPR(expr)->binop.right)) {
                    return CHIMP_FALSE;
                }
                
                if (!chimp_code_add (code)) {
                    return CHIMP_FALSE;
                }

                break;
            }
        case CHIMP_BINOP_SUB:
            {
                if (!chimp_compile_ast_expr (c, CHIMP_AST_EXPR(expr)->binop.right)) {
                    return CHIMP_FALSE;
                }

                if (!chimp_code_sub (code)) {
                    return CHIMP_FALSE;
                }

                break;
            }
        case CHIMP_BINOP_MUL:

            {
                if (!chimp_compile_ast_expr (c, CHIMP_AST_EXPR(expr)->binop.right)) {
                    return CHIMP_FALSE;
                }

                if (!chimp_code_mul (code)) {
                    return CHIMP_FALSE;
                }

                break;
            }
        case CHIMP_BINOP_DIV:
            {
                if (!chimp_compile_ast_expr (c, CHIMP_AST_EXPR(expr)->binop.right)) {
                    return CHIMP_FALSE;
                }

                if (!chimp_code_div (code)) {
                    return CHIMP_FALSE;
                }

                break;
            }
        default:
            chimp_bug (__FILE__, __LINE__, "unknown binop type: %d", CHIMP_AST_EXPR(expr)->binop.op);
            return CHIMP_FALSE;
    }

    return CHIMP_TRUE;
}

chimp_bool_t
chimp_compile_ast_expr_fn (ChimpCodeCompiler *c, ChimpRef *expr)
{
    ChimpRef *method;
    ChimpRef *code = CHIMP_COMPILER_CODE(c);

    method = chimp_compile_bytecode_method (
        c,
        expr,
        CHIMP_AST_EXPR(expr)->fn.args,
        CHIMP_AST_EXPR(expr)->fn.body
    );
    if (method == NULL) {
        return CHIMP_FALSE;
    }
    
    return chimp_code_pushconst (code, method);
}

ChimpRef *
chimp_compile_ast (ChimpRef *name, const char *filename, ChimpRef *ast)
{
    ChimpCodeUnit *unit;
    ChimpRef *module;
    ChimpCodeCompiler c;
    ChimpRef *filename_obj;
    memset (&c, 0, sizeof(c));

    filename_obj = chimp_str_new (filename, strlen (filename));
    if (filename_obj == NULL) {
        return NULL;
    }

    c.symtable = chimp_symtable_new_from_ast (filename_obj, ast);
    if (c.symtable == NULL) {
        return NULL;
    }

    module = chimp_code_compiler_push_module_unit (&c, ast);
    if (module == NULL) {
        return NULL;
    }
    CHIMP_MODULE(module)->name = name;

    switch (CHIMP_ANY_TYPE(ast)) {
        case CHIMP_VALUE_TYPE_AST_MOD:
            if (!chimp_compile_ast_mod (&c, ast)) goto error;
            break;
        default:
            chimp_bug (__FILE__, __LINE__, "unknown top-level AST node type: %d", CHIMP_ANY_TYPE(ast));
            return NULL;
    };

    if (!chimp_code_compiler_pop_unit (&c, CHIMP_UNIT_TYPE_MODULE)) {
        return NULL;
    }
    return module;

error:
    unit = c.current_unit;
    while (unit != NULL) {
        ChimpCodeUnit *next = unit->next;
        CHIMP_FREE (unit);
        unit = next;
    }
    return NULL;
}

extern int yyparse(ChimpRef **mod);
extern void yylex_destroy(void);
extern FILE *yyin;
extern ChimpRef *chimp_source_file;

ChimpRef *
chimp_compile_file (ChimpRef *name, const char *filename)
{
    int rc;
    ChimpRef *mod;
    yyin = fopen (filename, "r");
    if (yyin == NULL) {
        return NULL;
    }
    chimp_source_file = chimp_str_new (filename, strlen (filename));
    if (chimp_source_file == NULL) {
        fclose (yyin);
        return NULL;
    }
    rc = yyparse(&mod);
    fclose (yyin);
    yylex_destroy ();
    chimp_source_file = NULL;
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

            name = chimp_str_new (filename + slash, dot - slash);
            if (name == NULL) {
                return NULL;
            }
        }
        return chimp_compile_ast (name, filename, mod);
    }
    else {
        return NULL;
    }
}

