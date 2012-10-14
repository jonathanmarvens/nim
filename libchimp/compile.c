#include "chimp/compile.h"
#include "chimp/str.h"
#include "chimp/ast.h"
#include "chimp/code.h"
#include "chimp/array.h"
#include "chimp/int.h"
#include "chimp/method.h"
#include "chimp/object.h"

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
} ChimpCodeUnit;

typedef struct _ChimpCodeCompiler {
    ChimpCodeUnit *current_unit;
} ChimpCodeCompiler;

#define CHIMP_CURRENT_CODE_UNIT(c) (c)->current_unit
#define CHIMP_PUSH_CODE_UNIT(c, error_value) \
    if (!chimp_code_compiler_push_unit ((c), CHIMP_UNIT_TYPE_CODE)) { \
        return (error_value); \
    } \
    CHIMP_GC_MAKE_STACK_ROOT((c)->current_unit->code)

static chimp_bool_t
chimp_code_compiler_push_unit (ChimpCodeCompiler *c, ChimpUnitType type);

static ChimpRef *
chimp_code_compiler_pop_unit (ChimpCodeCompiler *c, ChimpUnitType type);

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
chimp_compile_ast_expr_bool (ChimpCodeCompiler *c, ChimpRef *expr);

static chimp_bool_t
chimp_compile_ast_expr_call (ChimpCodeCompiler *c, ChimpRef *expr);

static chimp_bool_t
chimp_compile_ast_expr_binop (ChimpCodeCompiler *c, ChimpRef *binop);

static chimp_bool_t
chimp_code_compiler_push_unit (ChimpCodeCompiler *c, ChimpUnitType type)
{
    ChimpCodeUnit *unit = CHIMP_MALLOC(ChimpCodeUnit, sizeof(*unit));
    if (unit == NULL) {
        return CHIMP_FALSE;
    }
    memset (unit, 0, sizeof(*unit));
    switch (type) {
        case CHIMP_UNIT_TYPE_CODE:
            unit->code = chimp_code_new ();
            break;
        case CHIMP_UNIT_TYPE_MODULE:
            unit->module = chimp_module_new_str ("main", NULL);
            break;
        default:
            chimp_bug (__FILE__, __LINE__, "unknown unit type: %d", type);
            return CHIMP_FALSE;
    };
    if (unit->value == NULL) {
        CHIMP_FREE(unit);
        return CHIMP_FALSE;
    }
    unit->type = type;
    unit->next = c->current_unit;
    c->current_unit = unit;
    return CHIMP_TRUE;
}

static ChimpRef *
chimp_code_compiler_pop_unit (ChimpCodeCompiler *c, ChimpUnitType expected)
{
    ChimpRef *value;
    ChimpCodeUnit *unit = CHIMP_CURRENT_CODE_UNIT(c);
    if (unit == NULL) {
        chimp_bug (__FILE__, __LINE__, "NULL code unit?");
        return NULL;
    }
    if (unit->type != expected) {
        chimp_bug (__FILE__, __LINE__, "unexpected unit type!");
        return NULL;
    }
    c->current_unit = unit->next;
    value = unit->value;
    CHIMP_FREE(unit);
    return value;
}

static chimp_bool_t
chimp_compile_ast_stmts (ChimpCodeCompiler *c, ChimpRef *stmts)
{
    size_t i;

    for (i = 0; i < CHIMP_ARRAY_SIZE(stmts); i++) {
        if (!chimp_compile_ast_stmt (c, CHIMP_ARRAY_ITEM(stmts, i))) {
            /* TODO error message? */
            return CHIMP_FALSE;
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
    ChimpRef *code = CHIMP_CURRENT_CODE_UNIT(c)->code;

    if (!chimp_compile_ast_expr (c, CHIMP_AST_STMT(stmt)->assign.value)) {
        return CHIMP_FALSE;
    }

    if (!chimp_code_storename (code, CHIMP_AST_EXPR(CHIMP_AST_STMT(stmt)->assign.target)->ident.id)) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_stmt_if_ (ChimpCodeCompiler *c, ChimpRef *stmt)
{
    ChimpRef *code = CHIMP_CURRENT_CODE_UNIT(c)->code;
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
chimp_compile_ast_decl_func (ChimpCodeCompiler *c, ChimpRef *decl)
{
    ChimpRef *func_code;
    ChimpRef *method;
    ChimpRef *args;
    ChimpRef *mod;
    size_t i;

    CHIMP_PUSH_CODE_UNIT(c, CHIMP_FALSE);

    func_code = CHIMP_CURRENT_CODE_UNIT(c)->code;

    /* unpack arguments */
    args = CHIMP_AST_DECL(decl)->func.args;
    for (i = 0; i < CHIMP_ARRAY_SIZE(args); i++) {
        ChimpRef *var_decl = CHIMP_ARRAY_ITEM(args, CHIMP_ARRAY_SIZE(args) - i - 1);
        if (!chimp_code_storename (func_code, CHIMP_AST_DECL(var_decl)->var.name)) {
            return CHIMP_FALSE;
        }
    }
    
    if (!chimp_compile_ast_stmts (c, CHIMP_AST_DECL(decl)->func.body)) {
        return CHIMP_FALSE;
    }

    func_code = chimp_code_compiler_pop_unit (c, CHIMP_UNIT_TYPE_CODE);
    if (func_code == NULL) {
        return CHIMP_FALSE;
    }

    if (CHIMP_CURRENT_CODE_UNIT(c)->type == CHIMP_UNIT_TYPE_MODULE) {
        mod = CHIMP_CURRENT_CODE_UNIT(c)->module;
    }
    else {
        /* XXX I think we want to go hunting for modules up the unit stack */
        mod = NULL;
    }

    method = chimp_method_new_bytecode (NULL, mod, func_code);
    if (method == NULL) {
        return CHIMP_FALSE;
    }

    if (CHIMP_CURRENT_CODE_UNIT(c)->type == CHIMP_UNIT_TYPE_CODE) {
        ChimpRef *code = CHIMP_CURRENT_CODE_UNIT(c)->code;
        if (!chimp_code_pushconst (code, method)) {
            return CHIMP_FALSE;
        }

        if (!chimp_code_storename (code, CHIMP_AST_DECL(decl)->func.name)) {
            return CHIMP_FALSE;
        }
    }
    else {
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
    /* TODO */
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
        case CHIMP_AST_EXPR_BINOP:
            return chimp_compile_ast_expr_binop (c, expr);
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
    ChimpRef *code = CHIMP_CURRENT_CODE_UNIT(c)->code;

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
chimp_compile_ast_expr_ident (ChimpCodeCompiler *c, ChimpRef *expr)
{
    ChimpRef *code = CHIMP_CURRENT_CODE_UNIT(c)->code;
    if (chimp_code_pushname (code, CHIMP_AST_EXPR(expr)->ident.id) < 0) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_expr_array (ChimpCodeCompiler *c, ChimpRef *expr)
{
    size_t i;
    ChimpRef *code = CHIMP_CURRENT_CODE_UNIT(c)->code;
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
    ChimpRef *code = CHIMP_CURRENT_CODE_UNIT(c)->code;
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
    ChimpRef *code = CHIMP_CURRENT_CODE_UNIT(c)->code;
    if (chimp_code_pushconst (code, CHIMP_AST_EXPR(expr)->str.value) < 0) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_expr_bool (ChimpCodeCompiler *c, ChimpRef *expr)
{
    ChimpRef *code = CHIMP_CURRENT_CODE_UNIT(c)->code;
    if (chimp_code_pushconst (code, CHIMP_AST_EXPR(expr)->bool.value) < 0) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_compile_ast_expr_binop (ChimpCodeCompiler *c, ChimpRef *expr)
{
    ChimpRef *code = CHIMP_CURRENT_CODE_UNIT(c)->code;
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
        default:
            chimp_bug (__FILE__, __LINE__, "unknown binop type: %d", CHIMP_AST_EXPR(expr)->binop.op);
            return CHIMP_FALSE;
    }

    return CHIMP_TRUE;
}

ChimpRef *
chimp_compile_ast (ChimpRef *ast)
{
    ChimpCodeUnit *unit;
    ChimpCodeCompiler c;
    memset (&c, 0, sizeof(c));

    if (!chimp_code_compiler_push_unit (&c, CHIMP_UNIT_TYPE_MODULE)) {
        return NULL;
    }
    CHIMP_GC_MAKE_STACK_ROOT(CHIMP_CURRENT_CODE_UNIT(&c)->module);

    switch (CHIMP_ANY_TYPE(ast)) {
        case CHIMP_VALUE_TYPE_AST_MOD:
            if (!chimp_compile_ast_mod (&c, ast)) goto error;
            break;
        default:
            chimp_bug (__FILE__, __LINE__, "unknown top-level AST node type: %d", CHIMP_ANY_TYPE(ast));
            return NULL;
    };

    return chimp_code_compiler_pop_unit (&c, CHIMP_UNIT_TYPE_MODULE);

error:
    unit = c.current_unit;
    while (unit != NULL) {
        ChimpCodeUnit *next = unit->next;
        CHIMP_FREE (unit);
        unit = next;
    }
    return NULL;
}

