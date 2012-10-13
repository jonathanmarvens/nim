#ifndef _CHIMP_CODE_H_INCLUDED_
#define _CHIMP_CODE_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _ChimpOpcode {
    CHIMP_OPCODE_JUMPIFFALSE,
    CHIMP_OPCODE_JUMPIFTRUE,
    CHIMP_OPCODE_JUMP,
    CHIMP_OPCODE_PUSHCONST,
    CHIMP_OPCODE_STORENAME,
    CHIMP_OPCODE_PUSHNAME,
    CHIMP_OPCODE_GETATTR,
    CHIMP_OPCODE_CALL,
    CHIMP_OPCODE_MAKEARRAY,
    CHIMP_OPCODE_MAKEHASH,
} ChimpOpcode;

typedef struct _ChimpCode {
    ChimpAny base;
    ChimpRef *constants;
    ChimpRef *names;
    uint32_t *bytecode;
    size_t    used;
    size_t    allocated;
} ChimpCode;

/* XXX we'll probably need to do something less stupid one day. */
typedef size_t ChimpLabel;

chimp_bool_t
chimp_code_class_bootstrap (void);

ChimpRef *
chimp_code_new (void);

chimp_bool_t
chimp_code_pushconst (ChimpRef *self, ChimpRef *value);

chimp_bool_t
chimp_code_pushname (ChimpRef *self, ChimpRef *id);

chimp_bool_t
chimp_code_storename (ChimpRef *self, ChimpRef *id);

chimp_bool_t
chimp_code_getattr (ChimpRef *self);

chimp_bool_t
chimp_code_call (ChimpRef *self);

chimp_bool_t
chimp_code_makearray (ChimpRef *self, uint8_t nargs);

chimp_bool_t
chimp_code_makehash (ChimpRef *self, uint8_t nargs);

chimp_bool_t
chimp_code_jumpiftrue (ChimpRef *self, ChimpLabel *label);

chimp_bool_t
chimp_code_jumpiffalse (ChimpRef *self, ChimpLabel *label);

chimp_bool_t
chimp_code_jump (ChimpRef *self, ChimpLabel *label);

chimp_bool_t
chimp_code_patch_jump_location (ChimpRef *self, ChimpLabel label);

#define CHIMP_CODE(ref)  CHIMP_CHECK_CAST(ChimpCode, (ref), CHIMP_VALUE_TYPE_CODE)

#define CHIMP_CODE_INSTR(ref, n) CHIMP_CODE(ref)->bytecode[n]

#define CHIMP_INSTR_OP(ref, n) ((ChimpOpcode)((CHIMP_CODE_INSTR(ref, n) & 0xff000000) >> 24))
#define CHIMP_INSTR_ADDR(ref, n) (CHIMP_CODE_INSTR(ref, n) & 0x00ffffff)

#define CHIMP_INSTR_ARG1(ref, n) ((CHIMP_CODE_INSTR(ref, n) & 0x00ff0000) >> 16)
#define CHIMP_INSTR_ARG2(ref, n) ((CHIMP_CODE_INSTR(ref, n) & 0x0000ff00) >> 8)
#define CHIMP_INSTR_ARG3(ref, n) ((CHIMP_CODE_INSTR(ref, n) & 0x000000ff))

/* constants */

#define CHIMP_INSTR_CONST1(ref, n) \
    CHIMP_ARRAY_ITEM(CHIMP_CODE_CONSTANTS(ref), CHIMP_INSTR_ARG1(ref, n))

#define CHIMP_INSTR_CONST2(ref, n) \
    CHIMP_ARRAY_ITEM(CHIMP_CODE_CONSTANTS(ref), CHIMP_INSTR_ARG2(ref, n))

#define CHIMP_INSTR_CONST3(ref, n) \
    CHIMP_ARRAY_ITEM(CHIMP_CODE_CONSTANTS(ref), CHIMP_INSTR_ARG3(ref, n))

/* names */

#define CHIMP_INSTR_NAME1(ref, n) \
    CHIMP_ARRAY_ITEM(CHIMP_CODE_NAMES(ref), CHIMP_INSTR_ARG1(ref, n))

#define CHIMP_INSTR_NAME2(ref, n) \
    CHIMP_ARRAY_ITEM(CHIMP_CODE_NAMES(ref), CHIMP_INSTR_ARG2(ref, n))

#define CHIMP_INSTR_NAME3(ref, n) \
    CHIMP_ARRAY_ITEM(CHIMP_CODE_NAMES(ref), CHIMP_INSTR_ARG3(ref, n))

#define CHIMP_CODE_SIZE(ref) CHIMP_CODE(ref)->used
#define CHIMP_CODE_CONSTANTS(ref) CHIMP_CODE(ref)->constants
#define CHIMP_CODE_NAMES(ref) CHIMP_CODE(ref)->names

CHIMP_EXTERN_CLASS(code);

#ifdef __cplusplus
};
#endif

#endif

