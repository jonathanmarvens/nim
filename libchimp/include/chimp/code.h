#ifndef _CHIMP_CODE_H_INCLUDED_
#define _CHIMP_CODE_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _ChimpOpcode {
    CHIMP_OPCODE_PUSHCONST = 1,
    CHIMP_OPCODE_GETATTR   = 2,
    CHIMP_OPCODE_CALL      = 3,
    CHIMP_OPCODE_MAKEARRAY = 4,
} ChimpOpcode;

typedef struct _ChimpCode {
    ChimpAny base;
    ChimpRef *constants;
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
chimp_code_getattr (ChimpRef *self);

chimp_bool_t
chimp_code_call (ChimpRef *self);

chimp_bool_t
chimp_code_makearray (ChimpRef *self, uint8_t nargs);

#define CHIMP_CODE(ref)  CHIMP_CHECK_CAST(ChimpCode, (ref), CHIMP_VALUE_TYPE_CODE)

#define CHIMP_CODE_OPCODE(ref, n) CHIMP_CODE(ref)->bytecode[n]
#define CHIMP_CODE_SIZE(ref) CHIMP_CODE(ref)->used
#define CHIMP_CODE_CONSTANTS(ref) CHIMP_CODE(ref)->constants

CHIMP_EXTERN_CLASS(code);

#ifdef __cplusplus
};
#endif

#endif

