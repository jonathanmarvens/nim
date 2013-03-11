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

#ifndef _NIM_SYMTABLE_H_INCLUDED_
#define _NIM_SYMTABLE_H_INCLUDED_

#include <nim/any.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    NIM_SYM_DECL    = 0x0001, /* symbol explicitly introduced via a decl */
    NIM_SYM_BUILTIN = 0x0002, /* builtin */
    NIM_SYM_FREE    = 0x0004, /* free var */
    NIM_SYM_SPECIAL = 0x0008, /* special/virtual var (e.g. __file__) */
    NIM_SYM_MODULE  = 0x1000, /* symbol declared at the module level */
    NIM_SYM_CLASS   = 0x2000, /* symbol declared at the class level */
    NIM_SYM_SPAWN   = 0x4000, /* symbol declared inside a spawn block */
    NIM_SYM_FUNC    = 0x8000  /* symbol declared inside a function */
};

#define NIM_SYM_TYPE_MASK 0xf000

typedef struct _NimSymtable {
    NimAny base;
    NimRef *filename;
    NimRef *lookup;
    NimRef *stack;
    NimRef *ste;
} NimSymtable;

typedef struct _NimSymtableEntry {
    NimAny  base;
    int       flags;
    NimRef *symtable;
    NimRef *scope;
    NimRef *symbols;
    NimRef *varnames;
    NimRef *parent;
    NimRef *children;
} NimSymtableEntry;

nim_bool_t
nim_symtable_class_bootstrap (void);

nim_bool_t
nim_symtable_entry_class_bootstrap (void);

NimRef *
nim_symtable_new_from_ast (NimRef *filename, NimRef *ast);

NimRef *
nim_symtable_lookup (NimRef *self, NimRef *scope);

nim_bool_t
nim_symtable_entry_sym_flags (
    NimRef *self, NimRef *name, int64_t *flags);

nim_bool_t
nim_symtable_entry_sym_exists (NimRef *self, NimRef *name);

#define NIM_SYMTABLE(ref) \
    NIM_CHECK_CAST(NimSymtable, (ref), nim_symtable_class)

#define NIM_SYMTABLE_ENTRY(ref) \
    NIM_CHECK_CAST( \
        NimSymtableEntry, (ref), nim_symtable_entry_class)

#define NIM_SYMTABLE_ENTRY_CHILDREN(ref) NIM_SYMTABLE_ENTRY(ref)->children

NIM_EXTERN_CLASS(symtable);
NIM_EXTERN_CLASS(symtable_entry);

#ifdef __cplusplus
};
#endif

#endif

