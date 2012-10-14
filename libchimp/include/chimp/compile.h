#ifndef _CHIMP_COMPILE_H_INCLUDED_
#define _CHIMP_COMPILE_H_INCLUDED_

#include <chimp/ast.h>

#ifdef __cplusplus
extern "C" {
#endif

ChimpRef *
chimp_compile_ast (ChimpRef *name, ChimpRef *ast);

#define CHIMP_COMPILE_MODULE_AST(name, ast) \
    chimp_compile_ast (chimp_str_new (NULL, (name), strlen(name)), (ast))

#ifdef __cplusplus
};
#endif

#endif

