#ifndef _CHIMP_COMPILE_H_INCLUDED_
#define _CHIMP_COMPILE_H_INCLUDED_

#include <chimp/ast.h>

#ifdef __cplusplus
extern "C" {
#endif

ChimpRef *
chimp_compile_ast (ChimpRef *name, ChimpRef *ast);

ChimpRef *
chimp_compile_file (ChimpRef *name, const char *filename);

#define CHIMP_COMPILE_MODULE_FROM_AST(name, ast) \
    chimp_compile_ast (chimp_str_new (NULL, (name), strlen(name)), (ast))

#define CHIMP_COMPILE_MODULE_FROM_FILE(name, filename) \
    chimp_compile_file (chimp_str_new (NULL, (name), strlen(name)), (filename))

#ifdef __cplusplus
};
#endif

#endif

