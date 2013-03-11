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

#ifndef _NIM_CODE_H_INCLUDED_
#define _NIM_CODE_H_INCLUDED_

#include <nim/gc.h>
#include <nim/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _NimOpcode {
    NIM_OPCODE_JUMPIFFALSE,
    NIM_OPCODE_JUMPIFTRUE,
    NIM_OPCODE_JUMP,
    NIM_OPCODE_PUSHCONST,
    NIM_OPCODE_STORENAME,
    NIM_OPCODE_PUSHNAME,
    NIM_OPCODE_PUSHNIL,
    NIM_OPCODE_GETATTR,
    NIM_OPCODE_GETITEM,
    NIM_OPCODE_CALL,
    NIM_OPCODE_MAKEARRAY,
    NIM_OPCODE_MAKEHASH,

    NIM_OPCODE_CMPEQ,
    NIM_OPCODE_CMPNEQ,
    NIM_OPCODE_CMPGT,
    NIM_OPCODE_CMPGTE,
    NIM_OPCODE_CMPLT,
    NIM_OPCODE_CMPLTE,

    NIM_OPCODE_NOT,
    NIM_OPCODE_DUP,

    NIM_OPCODE_POP,

    NIM_OPCODE_RET,
    NIM_OPCODE_SPAWN,

    NIM_OPCODE_ADD,
    NIM_OPCODE_SUB,
    NIM_OPCODE_MUL,
    NIM_OPCODE_DIV,
    
    NIM_OPCODE_MAKECLOSURE,

    /* XXX temporary until we get a better way to do it */
    NIM_OPCODE_GETCLASS
} NimOpcode;

typedef enum _NimBinopType {
    NIM_BINOP_EQ,
    NIM_BINOP_NEQ,
    NIM_BINOP_OR,
    NIM_BINOP_GT,
    NIM_BINOP_GTE,
    NIM_BINOP_LT,
    NIM_BINOP_LTE,
    NIM_BINOP_AND,
    NIM_BINOP_ADD,
    NIM_BINOP_SUB,
    NIM_BINOP_MUL,
    NIM_BINOP_DIV
} NimBinopType;

typedef struct _NimCode {
    NimAny base;
    NimRef *constants;
    NimRef *names;
    uint32_t *bytecode;
    size_t    used;
    size_t    allocated;
    NimRef *vars;
    NimRef *freevars;
} NimCode;

typedef struct _NimLabel {
    size_t      *patchlist;
    size_t       patchlist_size;
    size_t       addr;
    nim_bool_t in_use;
} NimLabel;

nim_bool_t
nim_code_class_bootstrap (void);

NimRef *
nim_code_new (void);

nim_bool_t
nim_code_pushconst (NimRef *self, NimRef *value);

nim_bool_t
nim_code_pushname (NimRef *self, NimRef *id);

nim_bool_t
nim_code_pushnil (NimRef *self);

nim_bool_t
nim_code_storename (NimRef *self, NimRef *id);

nim_bool_t
nim_code_getclass (NimRef *self);

nim_bool_t
nim_code_getattr (NimRef *self, NimRef *id);

nim_bool_t
nim_code_getitem (NimRef *self);

nim_bool_t
nim_code_call (NimRef *self, uint8_t nargs);

nim_bool_t
nim_code_ret (NimRef *self);

nim_bool_t
nim_code_not (NimRef *self);

nim_bool_t
nim_code_dup (NimRef *self);

nim_bool_t
nim_code_spawn (NimRef *self);

nim_bool_t
nim_code_makearray (NimRef *self, uint8_t nargs);

nim_bool_t
nim_code_makehash (NimRef *self, uint8_t nargs);

nim_bool_t
nim_code_makeclosure (NimRef *self);

nim_bool_t
nim_code_jumpiftrue (NimRef *self, NimLabel *label);

nim_bool_t
nim_code_jumpiffalse (NimRef *self, NimLabel *label);

nim_bool_t
nim_code_jump (NimRef *self, NimLabel *label);

nim_bool_t
nim_code_eq (NimRef *self);

nim_bool_t
nim_code_neq (NimRef *self);

nim_bool_t
nim_code_gt (NimRef *self);

nim_bool_t
nim_code_gte (NimRef *self);

nim_bool_t
nim_code_lt (NimRef *self);

nim_bool_t
nim_code_lte (NimRef *self);

nim_bool_t
nim_code_add (NimRef *self);

nim_bool_t
nim_code_sub (NimRef *self);

nim_bool_t
nim_code_mul (NimRef *self);

nim_bool_t
nim_code_div (NimRef *self);

nim_bool_t
nim_code_pop (NimRef *self);

NimRef *
nim_code_dump (NimRef *self);

void
nim_code_use_label (NimRef *self, NimLabel *label);

void
nim_label_free (NimLabel *self);

#define NIM_LABEL_INIT { NULL, 0, 0, NIM_FALSE }

#define NIM_CODE(ref)  NIM_CHECK_CAST(NimCode, (ref), nim_code_class)

#define NIM_CODE_INSTR(ref, n) NIM_CODE(ref)->bytecode[n]

#define NIM_INSTR_OP(ref, n) ((NimOpcode)((NIM_CODE_INSTR(ref, n) & 0xff000000) >> 24))
#define NIM_INSTR_ADDR(ref, n) (NIM_CODE_INSTR(ref, n) & 0x00ffffff)

#define NIM_INSTR_ARG1(ref, n) ((NIM_CODE_INSTR(ref, n) & 0x00ff0000) >> 16)
#define NIM_INSTR_ARG2(ref, n) ((NIM_CODE_INSTR(ref, n) & 0x0000ff00) >> 8)
#define NIM_INSTR_ARG3(ref, n) ((NIM_CODE_INSTR(ref, n) & 0x000000ff))

/* constants */

#define NIM_INSTR_CONST1(ref, n) \
    NIM_ARRAY_ITEM(NIM_CODE_CONSTANTS(ref), NIM_INSTR_ARG1(ref, n))

#define NIM_INSTR_CONST2(ref, n) \
    NIM_ARRAY_ITEM(NIM_CODE_CONSTANTS(ref), NIM_INSTR_ARG2(ref, n))

#define NIM_INSTR_CONST3(ref, n) \
    NIM_ARRAY_ITEM(NIM_CODE_CONSTANTS(ref), NIM_INSTR_ARG3(ref, n))

/* names */

#define NIM_INSTR_NAME1(ref, n) \
    NIM_ARRAY_ITEM(NIM_CODE_NAMES(ref), NIM_INSTR_ARG1(ref, n))

#define NIM_INSTR_NAME2(ref, n) \
    NIM_ARRAY_ITEM(NIM_CODE_NAMES(ref), NIM_INSTR_ARG2(ref, n))

#define NIM_INSTR_NAME3(ref, n) \
    NIM_ARRAY_ITEM(NIM_CODE_NAMES(ref), NIM_INSTR_ARG3(ref, n))

#define NIM_CODE_SIZE(ref) NIM_CODE(ref)->used
#define NIM_CODE_CONSTANTS(ref) NIM_CODE(ref)->constants
#define NIM_CODE_NAMES(ref) NIM_CODE(ref)->names

#define NIM_DEBUG_INSPECT(obj) NIM_STR_DATA(nim_object_str(obj))

#define NIM_DEBUG_CLASS_NAME(klass) \
    NIM_STR_DATA(NIM_CLASS(klass)->name)

#define NIM_DEBUG_SUPERCLASS_NAME(klass) \
    NIM_DEBUG_CLASS_NAME(NIM_CLASS(klass)->super)

NIM_EXTERN_CLASS(code);

#ifdef __cplusplus
};
#endif

#endif

