#include "chimp/compile.h"
#include "chimp/str.h"
#include "chimp/ast.h"
#include "chimp/code.h"
#include "chimp/array.h"
#include "chimp/int.h"

static ChimpRef *
chimp_compile_ast_stmt (ChimpRef *code, ChimpRef *stmt);

static ChimpRef *
chimp_compile_ast_expr (ChimpRef *code, ChimpRef *expr);

static ChimpRef *
chimp_compile_ast_expr_ident (ChimpRef *code, ChimpRef *expr);

static ChimpRef *
chimp_compile_ast_expr_array (ChimpRef *code, ChimpRef *expr);

static ChimpRef *
chimp_compile_ast_expr_hash (ChimpRef *code, ChimpRef *expr);

static ChimpRef *
chimp_compile_ast_expr_str (ChimpRef *code, ChimpRef *expr);

static ChimpRef *
chimp_compile_ast_expr_bool (ChimpRef *code, ChimpRef *expr);

static ChimpRef *
chimp_compile_ast_expr_call (ChimpRef *code, ChimpRef *expr);

static ChimpRef *
chimp_compile_ast_mod (ChimpRef *code, ChimpRef *mod)
{
    size_t i;
    ChimpRef *body = CHIMP_AST_MOD(mod)->root.body;

    if (code == NULL) {
        code = chimp_code_new ();
        if (code == NULL) {
            return NULL;
        }
    }

    /* TODO check CHIMP_AST_MOD_* type */

    for (i = 0; i < CHIMP_ARRAY_SIZE(body); i++) {
        if (chimp_compile_ast_stmt (code, CHIMP_ARRAY_ITEM(body, i)) == NULL) {
            /* TODO error message? */
            return NULL;
        }
    }

    return code;
}

static ChimpRef *
chimp_compile_ast_stmt_assign (ChimpRef *code, ChimpRef *stmt)
{
    if (!chimp_compile_ast_expr (code, CHIMP_AST_STMT(stmt)->assign.value)) {
        return NULL;
    }

    if (!chimp_code_storename (code, CHIMP_AST_EXPR(CHIMP_AST_STMT(stmt)->assign.target)->ident.id)) {
        return NULL;
    }

    return code;
}

static ChimpRef *
chimp_compile_ast_stmt (ChimpRef *code, ChimpRef *stmt)
{
    if (code == NULL) {
        code = chimp_code_new ();
        if (code == NULL) {
            return NULL;
        }
    }

    switch (CHIMP_AST_STMT_TYPE(stmt)) {
        case CHIMP_AST_STMT_EXPR:
            return chimp_compile_ast_expr (code, CHIMP_AST_STMT(stmt)->expr.expr);
        case CHIMP_AST_STMT_ASSIGN:
            return chimp_compile_ast_stmt_assign (code, stmt);
        default:
            chimp_bug (__FILE__, __LINE__, "unknown AST stmt type: %d", CHIMP_AST_STMT_TYPE(stmt));
            return NULL;
    };
}

static ChimpRef *
chimp_compile_ast_expr (ChimpRef *code, ChimpRef *expr)
{
    if (code == NULL) {
        code = chimp_code_new ();
        if (code == NULL) {
            return NULL;
        }
    }

    switch (CHIMP_AST_EXPR_TYPE(expr)) {
        case CHIMP_AST_EXPR_CALL:
            return chimp_compile_ast_expr_call (code, expr);
        case CHIMP_AST_EXPR_ARRAY:
            return chimp_compile_ast_expr_array (code, expr);
        case CHIMP_AST_EXPR_HASH:
            return chimp_compile_ast_expr_hash (code, expr);
        case CHIMP_AST_EXPR_IDENT:
            return chimp_compile_ast_expr_ident (code, expr);
        case CHIMP_AST_EXPR_STR:
            return chimp_compile_ast_expr_str (code, expr);
        case CHIMP_AST_EXPR_BOOL:
            return chimp_compile_ast_expr_bool (code, expr);
        default:
            chimp_bug (__FILE__, __LINE__, "unknown AST expr type: %d", CHIMP_AST_EXPR_TYPE(expr));
            return NULL;
    };
}

static ChimpRef *
chimp_compile_ast_expr_call (ChimpRef *code, ChimpRef *expr)
{
    ChimpRef *target;
    ChimpRef *args;
    size_t i;

    target = CHIMP_AST_EXPR(expr)->call.target;
    if (chimp_compile_ast_expr (code, target) == NULL) {
        return NULL;
    }

    args = CHIMP_AST_EXPR(expr)->call.args;
    for (i = 0; i < CHIMP_ARRAY_SIZE(args); i++) {
        if (chimp_compile_ast_expr (code, CHIMP_ARRAY_ITEM(args, i)) == NULL) {
            return NULL;
        }
    }

    if (!chimp_code_makearray (code, CHIMP_ARRAY_SIZE(args))) {
        return NULL;
    }

    if (!chimp_code_call (code)) {
        return NULL;
    }

    return code;
}

static ChimpRef *
chimp_compile_ast_expr_ident (ChimpRef *code, ChimpRef *expr)
{
    if (chimp_code_pushname (code, CHIMP_AST_EXPR(expr)->ident.id) < 0) {
        return NULL;
    }
    return code;
}

static ChimpRef *
chimp_compile_ast_expr_array (ChimpRef *code, ChimpRef *expr)
{
    size_t i;
    ChimpRef *arr = CHIMP_AST_EXPR(expr)->array.value;
    for (i = 0; i < CHIMP_ARRAY_SIZE(arr); i++) {
        if (!chimp_compile_ast_expr (code, CHIMP_ARRAY_ITEM(arr, i))) {
            return NULL;
        }
    }
    if (!chimp_code_makearray (code, CHIMP_ARRAY_SIZE(arr))) {
        return NULL;
    }
    return code;
}

static ChimpRef *
chimp_compile_ast_expr_hash (ChimpRef *code, ChimpRef *expr)
{
    size_t i;
    ChimpRef *arr = CHIMP_AST_EXPR(expr)->hash.value;
    /* TODO ensure array size is even (key/value pairs) */
    for (i = 0; i < CHIMP_ARRAY_SIZE(arr); i++) {
        if (!chimp_compile_ast_expr (code, CHIMP_ARRAY_ITEM(arr, i))) {
            return NULL;
        }
    }
    if (!chimp_code_makehash (code, CHIMP_ARRAY_SIZE(arr) / 2)) {
        return NULL;
    }
    return code;
}

static ChimpRef *
chimp_compile_ast_expr_str (ChimpRef *code, ChimpRef *expr)
{
    if (chimp_code_pushconst (code, CHIMP_AST_EXPR(expr)->str.value) < 0) {
        return NULL;
    }
    return code;
}

static ChimpRef *
chimp_compile_ast_expr_bool (ChimpRef *code, ChimpRef *expr)
{
    if (chimp_code_pushconst (code, CHIMP_AST_EXPR(expr)->bool.value) < 0) {
        return NULL;
    }
    return code;
}

ChimpRef *
chimp_compile_ast (ChimpRef *ast)
{
    ChimpRef *code = chimp_code_new ();
    if (code == NULL) {
        return NULL;
    }

    switch (CHIMP_ANY_TYPE(ast)) {
        case CHIMP_VALUE_TYPE_AST_MOD:
            return chimp_compile_ast_mod (code, ast);
        case CHIMP_VALUE_TYPE_AST_STMT:
            return chimp_compile_ast_stmt (code, ast);
        case CHIMP_VALUE_TYPE_AST_EXPR:
            return chimp_compile_ast_expr (code, ast);
        default:
            chimp_bug (__FILE__, __LINE__, "unknown AST node type: %d", CHIMP_ANY_TYPE(ast));
            return NULL;
    };
}

