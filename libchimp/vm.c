#include "chimp/vm.h"
#include "chimp/array.h"
#include "chimp/code.h"
#include "chimp/object.h"
#include "chimp/task.h"

struct _ChimpVM {
    ChimpRef  *stack;
};

ChimpVM *
chimp_vm_new (void)
{
    ChimpVM *vm = CHIMP_MALLOC(ChimpVM, sizeof(*vm));
    if (vm == NULL) {
        return NULL;
    }
    vm->stack = chimp_array_new (NULL);
    if (vm->stack == NULL) {
        CHIMP_FREE (vm);
        return NULL;
    }
    chimp_gc_make_root (NULL, vm->stack);
    return vm;
}

void
chimp_vm_delete (ChimpVM *vm)
{
    CHIMP_FREE(vm);
}

static ChimpRef *
chimp_vm_pop (ChimpVM *vm)
{
    return chimp_array_pop (vm->stack);
}

static chimp_bool_t
chimp_vm_push (ChimpVM *vm, ChimpRef *value)
{
    return chimp_array_push (vm->stack, value);
}

static chimp_bool_t
chimp_vm_pushconst (ChimpVM *vm, ChimpRef *code, ChimpRef *locals, size_t pc)
{
    ChimpRef *value = CHIMP_INSTR_CONST1(code, pc);
    if (value == NULL) {
        chimp_bug (__FILE__, __LINE__, "unknown or missing const at pc=%d", pc);
        return CHIMP_FALSE;
    }
    if (!chimp_vm_push(vm, value)) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_vm_storename (ChimpVM *vm, ChimpRef *code, ChimpRef *locals, size_t pc)
{
    ChimpRef *value;
    ChimpRef *target = CHIMP_INSTR_NAME1(code, pc);
    if (target == NULL) {
        chimp_bug (__FILE__, __LINE__, "unknown or missing name #%d at pc=%d", pc);
        return CHIMP_FALSE;
    }
    value = chimp_vm_pop (vm);
    if (value == NULL) {
        chimp_bug (__FILE__, __LINE__, "empty stack during assignment to %s", CHIMP_STR_DATA(target));
        return CHIMP_FALSE;
    }
    return chimp_hash_put (locals, target, value);
}

static chimp_bool_t
chimp_vm_pushname (ChimpVM *vm, ChimpRef *code, ChimpRef *locals, size_t pc)
{
    ChimpRef *value;
    ChimpRef *name = CHIMP_INSTR_NAME1(code, pc);
    if (name == NULL) {
        int n = CHIMP_INSTR_ARG1(code, pc);
        chimp_bug (__FILE__, __LINE__, "unknown or missing name #%d at pc=%d", n, pc);
        return CHIMP_FALSE;
    }
    /* TODO scan globals? */
    value = chimp_hash_get (locals, name);
    if (value == NULL || value == chimp_nil) {
        chimp_bug (__FILE__, __LINE__, "no such local: %s", CHIMP_STR_DATA(name));
        return CHIMP_FALSE;;
    }
    return chimp_vm_push (vm, value);
}

static chimp_bool_t
chimp_vm_getattr (ChimpVM *vm, ChimpRef *code, ChimpRef *locals, size_t pc)
{
    ChimpRef *attr;
    ChimpRef *target;
    ChimpRef *result;
    
    attr = chimp_vm_pop (vm);
    if (attr == NULL) {
        return CHIMP_FALSE;
    }
    target = chimp_vm_pop (vm);
    if (target == NULL) {
        return CHIMP_FALSE;
    }
    result = chimp_object_getattr (target, attr);
    if (result == NULL) {
        return CHIMP_FALSE;
    }
    if (!chimp_vm_push (vm, result)) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_vm_call (ChimpVM *vm, ChimpRef *code, ChimpRef *locals, size_t pc)
{
    ChimpRef *args;
    ChimpRef *target;
    ChimpRef *result;

    args = chimp_vm_pop (vm);
    if (args == NULL) {
        return CHIMP_FALSE;
    }
    target = chimp_vm_pop (vm);
    if (target == NULL) {
        return CHIMP_FALSE;
    }
    result = chimp_object_call (target, args);
    if (result == NULL) {
        chimp_bug (__FILE__, __LINE__, "target is not callable");
        return CHIMP_FALSE;
    }
    if (!chimp_vm_push (vm, result)) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_vm_makearray (ChimpVM *vm, ChimpRef *code, ChimpRef *locals, size_t pc)
{
    ChimpRef *array;
    int32_t nargs = CHIMP_INSTR_ARG1(code, pc);
    int32_t i;

    array = chimp_array_new (NULL);
    if (array == NULL) {
        return CHIMP_FALSE;
    }
    for (i = 0; i < nargs; i++) {
        ChimpRef *value = chimp_vm_pop (vm);
        if (value == NULL) {
            return CHIMP_FALSE;
        }
        if (!chimp_array_unshift (array, value)) {
            return CHIMP_FALSE;
        }
    }
    if (!chimp_vm_push (vm, array)) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_vm_makehash (ChimpVM *vm, ChimpRef *code, ChimpRef *locals, size_t pc)
{
    ChimpRef *hash;
    int32_t nargs = CHIMP_INSTR_ARG1(code, pc);
    int32_t i;

    hash = chimp_hash_new (NULL);
    if (hash == NULL) {
        return CHIMP_FALSE;
    }
    for (i = 0; i < nargs; i++) {
        ChimpRef *value;
        ChimpRef *key;
        
        value = chimp_vm_pop (vm);
        if (value == NULL) {
            return CHIMP_FALSE;
        }
        key = chimp_vm_pop (vm);
        if (key == NULL || key == chimp_nil) {
            return CHIMP_FALSE;
        }

        if (!chimp_hash_put (hash, key, value)) {
            return CHIMP_FALSE;
        }
    }
    if (!chimp_vm_push (vm, hash)) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_vm_pushtrue (ChimpVM *vm)
{
    return chimp_vm_push (vm, chimp_true);
}

static chimp_bool_t
chimp_vm_pushfalse (ChimpVM *vm)
{
    return chimp_vm_push (vm, chimp_false);
}

static ChimpCmpResult
chimp_vm_cmp (ChimpVM *vm)
{
    ChimpRef *right = chimp_vm_pop (vm);
    ChimpRef *left = chimp_vm_pop (vm);
    ChimpCmpResult actual;
    if (left == NULL || right == NULL) {
        chimp_bug (__FILE__, __LINE__, "NULL value on the stack");
        return CHIMP_FALSE;
    }
    actual = chimp_object_cmp (left, right);
    if (actual == CHIMP_CMP_ERROR) {
        chimp_bug (__FILE__, __LINE__, "TODO raise an exception");
    }
    return actual;
}

static ChimpRef *
chimp_vm_eval_frame (ChimpVM *vm, ChimpRef *frame)
{
    ChimpRef *code = CHIMP_FRAME(frame)->code;
    ChimpRef *locals = CHIMP_FRAME(frame)->locals;

    size_t pc = 0;
    while (pc < CHIMP_CODE_SIZE(code)) {
        switch (CHIMP_INSTR_OP(code, pc)) {
            case CHIMP_OPCODE_PUSHCONST:
            {
                if (!chimp_vm_pushconst (vm, code, locals, pc)) {
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_STORENAME:
            {
                if (!chimp_vm_storename (vm, code, locals, pc)) {
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_PUSHNAME:
            {
                if (!chimp_vm_pushname (vm, code, locals, pc)) {
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_GETATTR:
            {
                if (!chimp_vm_getattr (vm, code, locals, pc)) {
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_CALL:
            {
                if (!chimp_vm_call (vm, code, locals, pc)) {
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_MAKEARRAY:
            {
                if (!chimp_vm_makearray (vm, code, locals, pc)) {
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_MAKEHASH:
            {
                if (!chimp_vm_makehash (vm, code, locals, pc)) {
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_JUMPIFTRUE:
            {
                ChimpRef *value = chimp_vm_pop (vm);
                if (value == NULL) {
                    chimp_bug (__FILE__, __LINE__, "NULL value on the stack");
                    return NULL;
                }
                /* TODO test for truthiness */
                if (value == chimp_true) {
                    pc = CHIMP_INSTR_ADDR(code, pc);
                }
                else {
                    pc++;
                }
                break;
            }
            case CHIMP_OPCODE_JUMPIFFALSE:
            {
                ChimpRef *value = chimp_vm_pop (vm);
                if (value == NULL) {
                    chimp_bug (__FILE__, __LINE__, "NULL value on the stack");
                    return NULL;
                }
                /* TODO test for non-truthiness */
                if (value == chimp_false || value == chimp_nil) {
                    pc = CHIMP_INSTR_ADDR(code, pc);
                }
                else {
                    pc++;
                }
                break;
            }
            case CHIMP_OPCODE_JUMP:
            {
                pc = CHIMP_INSTR_ADDR(code, pc);
                break;
            }
            case CHIMP_OPCODE_CMPEQ:
            {
                ChimpCmpResult r = chimp_vm_cmp (vm);
                if (r == CHIMP_CMP_ERROR) {
                    return NULL;
                }
                else if (r == CHIMP_CMP_EQ) {
                    if (!chimp_vm_pushtrue (vm)) {
                        return NULL;
                    }
                }
                else {
                    if (!chimp_vm_pushfalse (vm)) {
                        return NULL;
                    }
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_CMPNEQ:
            {
                ChimpCmpResult r = chimp_vm_cmp (vm);
                if (r == CHIMP_CMP_ERROR) {
                    return NULL;
                }
                else if (r == CHIMP_CMP_EQ) {
                    if (!chimp_vm_pushfalse (vm)) {
                        return NULL;
                    }
                }
                else {
                    if (!chimp_vm_pushtrue (vm)) {
                        return NULL;
                    }
                }
                pc++;
                break;
            }
            default:
            {
                chimp_bug (__FILE__, __LINE__, "unknown opcode: %d", CHIMP_INSTR_OP(code, pc));
                return NULL;
            }
        };
    }

    return chimp_vm_pop(vm);
}

ChimpRef *
chimp_vm_eval (ChimpVM *vm, ChimpRef *code, ChimpRef *locals)
{
    ChimpRef *frame;
    if (vm == NULL) {
        vm = CHIMP_CURRENT_VM;
    }
    if (code == NULL) {
        chimp_bug (__FILE__, __LINE__, "NULL code object passed to chimp_vm_eval");
        return NULL;
    }
    frame = chimp_frame_new (NULL, code, locals);
    if (frame == NULL) {
        return NULL;
    }
    return chimp_vm_eval_frame (vm, frame);
}

