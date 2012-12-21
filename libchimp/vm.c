#include "chimp/vm.h"
#include "chimp/array.h"
#include "chimp/code.h"
#include "chimp/object.h"
#include "chimp/task.h"

struct _ChimpVM {
    ChimpRef  *stack;
    ChimpRef  *frames;
};

ChimpVM *
chimp_vm_new (void)
{
    ChimpVM *vm = CHIMP_MALLOC(ChimpVM, sizeof(*vm));
    if (vm == NULL) {
        return NULL;
    }
    vm->stack = chimp_array_new ();
    if (vm->stack == NULL) {
        CHIMP_FREE (vm);
        return NULL;
    }
    vm->frames = chimp_array_new ();
    if (vm->frames == NULL) {
        CHIMP_FREE (vm);
        return NULL;
    }
    chimp_gc_make_root (NULL, vm->stack);
    chimp_gc_make_root (NULL, vm->frames);
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

static ChimpRef *
chimp_vm_top (ChimpVM *vm)
{
    return chimp_array_last (vm->stack);
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
        chimp_bug (__FILE__, __LINE__, "failed to push value at pc=%d", pc);
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
    ChimpRef *frame;
    ChimpRef *module;
    ChimpRef *name = CHIMP_INSTR_NAME1(code, pc);
    if (name == NULL) {
        int n = CHIMP_INSTR_ARG1(code, pc);
        chimp_bug (__FILE__, __LINE__, "unknown or missing name #%d at pc=%d", n, pc);
        return CHIMP_FALSE;
    }

    /* 1. check locals */
    frame = CHIMP_ARRAY_LAST (vm->frames);
    if (frame == NULL) {
        return CHIMP_FALSE;
    }
    value = chimp_hash_get (CHIMP_FRAME(frame)->locals, name);
    if (value == NULL) {
        return CHIMP_FALSE;
    }
    else if (value != chimp_nil) {
        return chimp_vm_push (vm, value);
    }

    /* 2. check module */
    module = CHIMP_METHOD(CHIMP_FRAME(frame)->method)->module;
    if (module != NULL) { /* builtins have no module */
        value = chimp_object_getattr (module, name);
        /* TODO discern between 'no-such-attr' and other errors */
        if (value == NULL) {
            return CHIMP_FALSE;
        }
        else if (value != chimp_nil) {
            return chimp_vm_push (vm, value);
        }
    }

    /* 3. check builtins */
    value = chimp_hash_get (chimp_builtins, name);
    if (value == NULL) {
        return CHIMP_FALSE;
    }
    else if (value != chimp_nil) {
        return chimp_vm_push (vm, value);
    }

    chimp_bug (__FILE__, __LINE__, "unknown name: %s", CHIMP_STR_DATA(name));
    return CHIMP_FALSE;
}

static chimp_bool_t
chimp_vm_getattr (ChimpVM *vm, ChimpRef *code, ChimpRef *locals, size_t pc)
{
    ChimpRef *attr;
    ChimpRef *target;
    ChimpRef *result;
    
    attr = CHIMP_INSTR_NAME1 (code, pc);
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
    uint8_t nargs;
    uint8_t i;

    nargs = CHIMP_INSTR_ARG1 (code, pc);
    args = chimp_array_new_with_capacity (nargs);
    if (args == NULL) {
        return CHIMP_FALSE;
    }
    for (i = 0; i < nargs; i++) {
        chimp_array_unshift (args, chimp_vm_pop (vm));
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

    array = chimp_array_new_with_capacity (nargs);
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

    hash = chimp_hash_new ();
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

static chimp_bool_t
chimp_vm_truthy (ChimpRef *value)
{
    return value != NULL && value != chimp_nil && value != chimp_false;
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
    ChimpRef *code = CHIMP_FRAME_CODE(frame);
    ChimpRef *locals = CHIMP_FRAME(frame)->locals;

    if (!chimp_array_push (vm->frames, frame)) {
        return CHIMP_FALSE;
    }

    size_t pc = 0;
    while (pc < CHIMP_CODE_SIZE(code)) {
        switch (CHIMP_INSTR_OP(code, pc)) {
            case CHIMP_OPCODE_PUSHCONST:
            {
                if (!chimp_vm_pushconst (vm, code, locals, pc)) {
                    chimp_bug (__FILE__, __LINE__, "PUSHCONST instruction failed");
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_STORENAME:
            {
                if (!chimp_vm_storename (vm, code, locals, pc)) {
                    chimp_bug (__FILE__, __LINE__, "STORENAME instruction failed");
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_PUSHNAME:
            {
                if (!chimp_vm_pushname (vm, code, locals, pc)) {
                    chimp_bug (__FILE__, __LINE__, "PUSHNAME instruction failed");
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_PUSHNIL:
            {
                if (!chimp_vm_push (vm, chimp_nil)) {
                    chimp_bug (__FILE__, __LINE__, "PUSHNIL instruction failed");
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_GETCLASS:
            {
                ChimpRef *value = chimp_vm_pop (vm);
                if (value == NULL) {
                    chimp_bug (__FILE__, __LINE__, "GETCLASS instruction failed");
                    return NULL;
                }
                if (!chimp_vm_push (vm, CHIMP_ANY_CLASS(value))) {
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_GETATTR:
            {
                if (!chimp_vm_getattr (vm, code, locals, pc)) {
                    chimp_bug (__FILE__, __LINE__, "GETATTR instruction failed");
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_GETITEM:
            {
                ChimpRef *result;
                ChimpRef *key;
                ChimpRef *target;
                
                key = chimp_vm_pop (vm);
                if (key == NULL) {
                    chimp_bug (__FILE__, __LINE__, "GETITEM instruction failed 1");
                    return NULL;
                }

                target = chimp_vm_pop (vm);
                if (target == NULL) {
                    chimp_bug (__FILE__, __LINE__, "GETITEM instruction failed 2");
                    return NULL;
                }

                result = chimp_object_getitem (target, key);
                if (result == NULL) {
                    chimp_bug (__FILE__, __LINE__, "GETITEM instruction failed 3");
                    return NULL;
                }

                if (!chimp_vm_push (vm, result)) {
                    chimp_bug (__FILE__, __LINE__, "GETITEM instruction failed 4");
                    return NULL;
                }

                pc++;
                break;
            }
            case CHIMP_OPCODE_CALL:
            {
                if (!chimp_vm_call (vm, code, locals, pc)) {
                    chimp_bug (__FILE__, __LINE__, "CALL instruction failed");
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_RET:
            {
                goto done;
            }
            case CHIMP_OPCODE_PANIC:
            {
                ChimpRef *value;
                value = chimp_vm_pop (vm);
                chimp_vm_panic (vm, value);
                break;
            }
            case CHIMP_OPCODE_SPAWN:
            {
                ChimpRef *task;
                ChimpRef *target;

                target = chimp_vm_pop (vm);
                if (target == NULL) {
                    return CHIMP_FALSE;
                }
                task = chimp_task_new (target);
                if (task == NULL) {
                    return CHIMP_FALSE;
                }

                if (!chimp_vm_push (vm, task)) {
                    return CHIMP_FALSE;
                }

                pc++;
                break;
            }
            case CHIMP_OPCODE_RECEIVE:
            {
                ChimpRef *task = chimp_task_get_self (CHIMP_CURRENT_TASK);
                ChimpRef *value = chimp_task_recv (task);

                if (!chimp_vm_push (vm, value)) {
                    return CHIMP_FALSE;
                }

                pc++;
                break;
            }
            case CHIMP_OPCODE_DUP:
            {
                ChimpRef *top = chimp_vm_top (vm);

                if (top == NULL) {
                    return CHIMP_FALSE;
                }

                if (!chimp_vm_push (vm, top)) {
                    return CHIMP_FALSE;
                }

                pc++;
                break;
            }
            case CHIMP_OPCODE_NOT:
            {
                ChimpRef *value = chimp_vm_pop (vm);
                if (value == NULL) {
                    return CHIMP_FALSE;
                }
                if (chimp_vm_truthy (value)) {
                    if (!chimp_vm_push (vm, chimp_false)) {
                        return CHIMP_FALSE;
                    }
                }
                else {
                    if (!chimp_vm_push (vm, chimp_true)) {
                        return CHIMP_FALSE;
                    }
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_MAKEARRAY:
            {
                if (!chimp_vm_makearray (vm, code, locals, pc)) {
                    chimp_bug (__FILE__, __LINE__, "MAKEARRAY instruction failed");
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_MAKEHASH:
            {
                if (!chimp_vm_makehash (vm, code, locals, pc)) {
                    chimp_bug (__FILE__, __LINE__, "MAKEHASH instruction failed");
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
                if (chimp_vm_truthy (value)) {
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
            case CHIMP_OPCODE_CMPGT:
            {
                ChimpCmpResult r = chimp_vm_cmp (vm);
                if (r == CHIMP_CMP_ERROR) {
                    return NULL;
                }
                else if (r == CHIMP_CMP_GT) {
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
            case CHIMP_OPCODE_CMPGTE:
            {
                ChimpCmpResult r = chimp_vm_cmp (vm);
                if (r == CHIMP_CMP_ERROR) {
                    return NULL;
                }
                else if (r == CHIMP_CMP_GT || r == CHIMP_CMP_EQ) {
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
            case CHIMP_OPCODE_CMPLT:
            {
                ChimpCmpResult r = chimp_vm_cmp (vm);
                if (r == CHIMP_CMP_ERROR) {
                    return NULL;
                }
                else if (r == CHIMP_CMP_LT) {
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
            case CHIMP_OPCODE_CMPLTE:
            {
                ChimpCmpResult r = chimp_vm_cmp (vm);
                if (r == CHIMP_CMP_ERROR) {
                    return NULL;
                }
                else if (r == CHIMP_CMP_LT || r == CHIMP_CMP_EQ) {
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
            case CHIMP_OPCODE_POP:
            {
                if (!chimp_vm_pop (vm)) {
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_ADD:
            {
                ChimpRef *left;
                ChimpRef *right;
                ChimpRef *result;

                right = chimp_vm_pop (vm);
                if (right == NULL) {
                    return NULL;
                }

                left = chimp_vm_pop (vm);
                if (left == NULL) {
                    return NULL;
                }

                result = chimp_object_add (left, right);
                if (!chimp_vm_push (vm, result)) {
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_SUB:
            {
                ChimpRef *left;
                ChimpRef *right;
                ChimpRef *result;

                right = chimp_vm_pop (vm);
                if (right == NULL) {
                    return NULL;
                }

                left = chimp_vm_pop (vm);
                if (left == NULL) {
                    return NULL;
                }

                result = chimp_object_sub (left, right);
                if (!chimp_vm_push (vm, result)) {
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_MUL:
            {
                ChimpRef *left;
                ChimpRef *right;
                ChimpRef *result;

                right = chimp_vm_pop (vm);
                if (right == NULL) {
                    return NULL;
                }

                left = chimp_vm_pop (vm);
                if (left == NULL) {
                    return NULL;
                }

                result = chimp_object_mul (left, right);
                if (!chimp_vm_push (vm, result)) {
                    return NULL;
                }
                pc++;
                break;
            }
            case CHIMP_OPCODE_DIV:
            {
                ChimpRef *left;
                ChimpRef *right;
                ChimpRef *result;

                right = chimp_vm_pop (vm);
                if (right == NULL) {
                    return NULL;
                }

                left = chimp_vm_pop (vm);
                if (left == NULL) {
                    return NULL;
                }

                result = chimp_object_div (left, right);
                if (!chimp_vm_push (vm, result)) {
                    return NULL;
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

done:
    chimp_array_pop (vm->frames);
    return chimp_vm_pop(vm);
}

/*
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
*/

#define CHIMP_IS_BYTECODE_METHOD(ref) \
    (CHIMP_ANY(ref)->type == CHIMP_VALUE_TYPE_METHOD && \
        CHIMP_METHOD(ref)->type == CHIMP_METHOD_TYPE_BYTECODE)

ChimpRef *
chimp_vm_invoke (ChimpVM *vm, ChimpRef *method, ChimpRef *args)
{
    size_t i;
    size_t stack_size;
    ChimpRef *frame;
    ChimpRef *ret;

    if (vm == NULL) {
        vm = CHIMP_CURRENT_VM;
    }

    /* save the stack */
    stack_size = CHIMP_ARRAY_SIZE(vm->stack);

    if (!CHIMP_IS_BYTECODE_METHOD(method)) {
        chimp_bug (__FILE__, __LINE__,
            "chimp_vm_invoke called on a non-method (or native method)");
        return NULL;
    }

    frame = chimp_frame_new (method);
    if (frame == NULL) {
        chimp_bug (__FILE__, __LINE__,
            "chimp_vm_invoke failed to create a new execution frame");
        return NULL;
    }

    /* push args */
    for (i = 0; i < CHIMP_ARRAY_SIZE(args); i++) {
        if (!chimp_vm_push (vm, CHIMP_ARRAY_ITEM(args, i))) {
            chimp_bug (__FILE__, __LINE__,
                "chimp_vm_invoke failed to append array item");
            return NULL;
        }
    }

    ret = chimp_vm_eval_frame (vm, frame);

    /* restore the stack */
    CHIMP_ARRAY(vm->stack)->size = stack_size;

    return ret;
}

void
chimp_vm_panic (ChimpVM *vm, ChimpRef *value)
{
    /* TODO make panics rescuable */
    if (value == NULL || value == chimp_nil) {
        fprintf (stderr, "panic: no message provided\n");
    }
    else {
        value = chimp_object_str (value);
        if (value == NULL) {
            fprintf (stderr, "panic: no message available\n");
        }
        else {
            fprintf (stderr, "panic: %s\n", CHIMP_STR_DATA(value));
        }
    }
    exit(1);
}

