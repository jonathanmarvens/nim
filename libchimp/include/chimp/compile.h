#ifndef _CHIMP_COMPILE_H_INCLUDED_
#define _CHIMP_COMPILE_H_INCLUDED_

#include <chimp/ast.h>

#ifdef __cplusplus
extern "C" {
#endif

ChimpRef *
chimp_compile_ast (ChimpRef *ast);

#ifdef __cplusplus
};
#endif

#endif

