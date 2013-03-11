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

#ifndef _NIM_COMPILE_H_INCLUDED_
#define _NIM_COMPILE_H_INCLUDED_

#include <nim/ast.h>

#ifdef __cplusplus
extern "C" {
#endif

NimRef *
nim_compile_ast (NimRef *name, const char *filename, NimRef *ast);

NimRef *
nim_compile_file (NimRef *name, const char *filename);

#define NIM_COMPILE_MODULE_FROM_AST(name, ast) \
    nim_compile_ast (nim_str_new ((name), strlen(name)), (ast))

/* XXX passing a NULL name generates a warning in GCC for no apparent reason */
#define NIM_COMPILE_MODULE_FROM_FILE(name, filename) \
    nim_compile_file ((((name) != NULL) ? nim_str_new ((name), strlen(name)) : NULL), (filename))

#ifdef __cplusplus
};
#endif

#endif

