#ifndef _CHIMP_CODE_H_INCLUDED_
#define _CHIMP_CODE_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _ChimpOpcode {
    CHIMP_OPCODE_PUSHCONST = 1,
    CHIMP_OPCODE_PUSHNAME  = 2,
    CHIMP_OPCODE_GETATTR   = 3,
    CHIMP_OPCODE_CALL      = 4,
    CHIMP_OPCODE_MAKEARRAY = 5,
} ChimpOpcode;

typedef struct _ChimpCode {
    ChimpAny base;
    ChimpRef *constants;
    ChimpRef *names;
    uint32_t *bytecode;
    size_t    used;
    size_t    allocated;
} ChimpCode;

chimp_bool_t
chimp_code_class_bootstrap (void);

ChimpRef *
chimp_code_new (void);

chimp_bool_t
chimp_code_pushconst (ChimpRef *self, ChimpRef *value);

chimp_bool_t
chimp_code_pushname (ChimpRef *self, ChimpRef *id);

chimp_bool_t
chimp_code_getattr (ChimpRef *self);

chimp_bool_t
chimp_code_call (ChimpRef *self);

chimp_bool_t
chimp_code_makearray (ChimpRef *self, uint8_t nargs);

#define CHIMP_CODE(ref)  CHIMP_CHECK_CAST(ChimpCode, (ref), CHIMP_VALUE_TYPE_CODE)

#define CHIMP_CODE_INSTR(ref, n) CHIMP_CODE(ref)->bytecode[n]

#define CHIMP_INSTR_OP(ref, n) ((ChimpOpcode)((CHIMP_CODE_INSTR(ref, n) & 0xff000000) >> 24))

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

