#include "chimp/code.h"
#include "chimp/str.h"
#include "chimp/class.h"
#include "chimp/array.h"

ChimpRef *chimp_code_class = NULL;

chimp_bool_t
chimp_code_class_bootstrap (void)
{
    chimp_code_class =
        chimp_class_new (NULL, CHIMP_STR_NEW(NULL, "code"), NULL);
    if (chimp_code_class == NULL) {
        return CHIMP_FALSE;
    }
    chimp_gc_make_root (NULL, chimp_code_class);
    return CHIMP_TRUE;
}

ChimpRef *
chimp_code_new (void)
{
    ChimpRef *ref = chimp_gc_new_object (NULL);
    if (ref == NULL) {
        return NULL;
    }
    CHIMP_ANY(ref)->type = CHIMP_VALUE_TYPE_CODE;
    CHIMP_ANY(ref)->klass = chimp_code_class;
    CHIMP_CODE(ref)->constants = chimp_array_new (NULL);
    CHIMP_CODE(ref)->bytecode = CHIMP_MALLOC(uint32_t, 32);
    if (CHIMP_CODE(ref)->bytecode == NULL) {
        return NULL;
    }
    CHIMP_CODE(ref)->allocated = 256;
    return ref;
}

#define CHIMP_NEXT_INSTR(co) CHIMP_CODE(co)->bytecode[CHIMP_CODE(co)->used++]

#define CHIMP_MAKE_INSTR0(op) \
    (((CHIMP_OPCODE_ ## op) & 0xff) << 24)

#define CHIMP_MAKE_INSTR1(op, arg1) \
    ((((CHIMP_OPCODE_ ## op) & 0xff) << 24) | (((arg1) & 0xff) << 16))

#define CHIMP_MAKE_INSTR2(op, arg1, arg2) \
    ((((CHIMP_OPCODE_ ## op) & 0xff) << 24) | (((arg1) & 0xff) << 16) | (((arg2) & 0xff) << 8))

#define CHIMP_MAKE_INSTR3(op, arg1, arg2, arg3) \
    ((((CHIMP_OPCODE_ ## op) & 0xff) << 24) | (((arg1) & 0xff) << 16) | (((arg2) & 0xff) << 8) | ((arg3) & 0xff))

static chimp_bool_t
chimp_code_grow (ChimpRef *self)
{
    uint32_t *bytecode;
    if (CHIMP_CODE(self)->used < CHIMP_CODE(self)->allocated) {
        return CHIMP_TRUE;
    }
    bytecode = CHIMP_REALLOC(uint32_t, CHIMP_CODE(self)->bytecode, sizeof(uint32_t) * (CHIMP_CODE(self)->allocated + 32));
    if (bytecode == NULL) {
        return CHIMP_FALSE;
    }
    CHIMP_CODE(self)->bytecode = bytecode;
    CHIMP_CODE(self)->allocated += 32;
    return CHIMP_TRUE;
}

static int32_t
chimp_code_add_const (ChimpRef *self, ChimpRef *value)
{
    int32_t n = chimp_array_find (CHIMP_CODE(self)->constants, value);
    if (n >= 0) {
        return n;
    }
    if (!chimp_array_push (CHIMP_CODE(self)->constants, value)) {
        return -1;
    }
    return CHIMP_ARRAY_SIZE(CHIMP_CODE(self)->constants);
}

chimp_bool_t
chimp_code_pushconst (ChimpRef *self, ChimpRef *value)
{
    int32_t arg;
    if (!chimp_code_grow (self)) {
        return CHIMP_FALSE;
    }
    arg = chimp_code_add_const (self, value);
    if (arg < 0) {
        return CHIMP_FALSE;
    }
    CHIMP_NEXT_INSTR(self) = CHIMP_MAKE_INSTR1(PUSHCONST, arg);
    return CHIMP_TRUE;
}

chimp_bool_t
chimp_code_getattr (ChimpRef *self)
{
    if (!chimp_code_grow (self)) {
        return CHIMP_FALSE;
    }
    CHIMP_NEXT_INSTR(self) = CHIMP_MAKE_INSTR0(GETATTR);
    return CHIMP_TRUE;
}

chimp_bool_t
chimp_code_call (ChimpRef *self)
{
    if (!chimp_code_grow (self)) {
        return CHIMP_FALSE;
    }
    CHIMP_NEXT_INSTR(self) = CHIMP_MAKE_INSTR0(CALL);
    return CHIMP_TRUE;
}

chimp_bool_t
chimp_code_makearray (ChimpRef *self, uint8_t nargs)
{
    if (!chimp_code_grow (self)) {
        return CHIMP_FALSE;
    }
    /* XXX this cast should happen automatically in the make_instr macro */
    CHIMP_NEXT_INSTR(self) = CHIMP_MAKE_INSTR1(MAKEARRAY, (int32_t)nargs);
    return CHIMP_TRUE;
}

