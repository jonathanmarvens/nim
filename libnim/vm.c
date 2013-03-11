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

#include "nim/vm.h"
#include "nim/array.h"
#include "nim/code.h"
#include "nim/object.h"
#include "nim/task.h"

struct _NimVM {
    NimRef  *stack;
    NimRef  *frames;
};

static nim_bool_t
nim_vm_resolvename (
    NimVM *vm, NimRef *name, NimRef **value, nim_bool_t binding);

NimVM *
nim_vm_new (void)
{
    NimVM *vm = NIM_MALLOC(NimVM, sizeof(*vm));
    if (vm == NULL) {
        return NULL;
    }
    vm->stack = nim_array_new ();
    if (vm->stack == NULL) {
        NIM_FREE (vm);
        return NULL;
    }
    vm->frames = nim_array_new ();
    if (vm->frames == NULL) {
        NIM_FREE (vm);
        return NULL;
    }
    nim_gc_make_root (NULL, vm->stack);
    nim_gc_make_root (NULL, vm->frames);
    return vm;
}

void
nim_vm_delete (NimVM *vm)
{
    NIM_FREE(vm);
}

static NimRef *
nim_vm_pop (NimVM *vm)
{
    return nim_array_pop (vm->stack);
}

static NimRef *
nim_vm_top (NimVM *vm)
{
    return nim_array_last (vm->stack);
}

static nim_bool_t
nim_vm_push (NimVM *vm, NimRef *value)
{
    return nim_array_push (vm->stack, value);
}

static nim_bool_t
nim_vm_pushconst (NimVM *vm, NimRef *code, NimRef *locals, size_t pc)
{
    NimRef *value = NIM_INSTR_CONST1(code, pc);
    if (value == NULL) {
        NIM_BUG ("unknown or missing const at pc=%d", pc);
        return NIM_FALSE;
    }
#ifdef NIM_VM_DEBUG
    printf ("[%p] PUSHCONST %s\n", vm, NIM_STR_DATA (nim_object_str (value)));
#endif
    if (!nim_vm_push(vm, value)) {
        NIM_BUG ("failed to push value at pc=%d", pc);
        return NIM_FALSE;
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_vm_storename (NimVM *vm, NimRef *code, NimRef *locals, size_t pc)
{
    NimRef *value;
    NimRef *var;
    NimRef *target = NIM_INSTR_NAME1(code, pc);
    if (target == NULL) {
        NIM_BUG ("unknown or missing name #%d at pc=%d", pc);
        return NIM_FALSE;
    }
    value = nim_vm_pop (vm);
#ifdef NIM_VM_DEBUG
    printf ("[%p] STORENAME %s -> %s\n",
            vm, NIM_STR_DATA(target), NIM_STR_DATA(nim_object_str (value)));
#endif
    if (value == NULL) {
        NIM_BUG ("empty stack during assignment to %s", NIM_STR_DATA(target));
        return NIM_FALSE;
    }
    if (nim_hash_get (locals, target, &var) != 0) {
        NIM_BUG ("could not find local for %s", NIM_STR_DATA(target));
        return NIM_FALSE;
    }
    NIM_VAR(var)->value = value;
    return NIM_TRUE;
}

static nim_bool_t
nim_vm_resolvename (
    NimVM *vm, NimRef *name, NimRef **value, nim_bool_t binding)
{
    NimRef *frame;
    NimRef *module;
    int rc;
    /* 1. check locals */
    /* if we're binding a closure, we want to walk up the call stack */
    /* XXX this is potentially expensive. how do we do it better? */
    if (binding && NIM_ARRAY_SIZE(vm->frames) >= 1) {
        size_t i = NIM_ARRAY_SIZE(vm->frames);
        do {
            i--;
            frame = NIM_ARRAY_ITEM(vm->frames, i);
            rc = nim_hash_get (NIM_FRAME(frame)->locals, name, value);
            if (rc < 0) {
                return NIM_FALSE;
            }
            else if (rc == 0) {
                /* deref vars if we're not binding a closure */
                if (!binding && NIM_ANY_CLASS(*value) == nim_var_class) {
                    *value = NIM_VAR(*value)->value;
                }
                return NIM_TRUE;
            }
        } while (i > 0);
        frame = NIM_ARRAY_LAST (vm->frames);
    }
    else {
        frame = NIM_ARRAY_LAST (vm->frames);
        if (frame == NULL) {
            return NIM_FALSE;
        }
        rc = nim_hash_get (NIM_FRAME(frame)->locals, name, value);
        if (rc < 0) {
            return NIM_FALSE;
        }
        else if (rc == 0) {
            /* deref vars if we're not binding a closure */
            if (!binding && NIM_ANY_CLASS(*value) == nim_var_class) {
                *value = NIM_VAR(*value)->value;
            }
            return NIM_TRUE;
        }
    }

    /* 2. check module */
    module = NIM_METHOD(NIM_FRAME(frame)->method)->module;
    if (module != NULL) { /* builtins have no module */
        *value = nim_object_getattr (module, name);
        /* TODO discern between 'no-such-attr' and other errors */
        if (*value != NULL) {
            return NIM_TRUE;
        }
    }

    /* 3. check builtins */
    rc = nim_hash_get (nim_builtins, name, value);
    if (rc < 0) {
        return NIM_FALSE;
    }
    else if (rc == 0) {
        return NIM_TRUE;
    }

    NIM_BUG ("unknown name: %s", NIM_STR_DATA(name));
    return NIM_FALSE;
}

static nim_bool_t
nim_vm_pushname (NimVM *vm, NimRef *code, NimRef *locals, size_t pc)
{
    NimRef *value;
    NimRef *name = NIM_INSTR_NAME1(code, pc);
    if (name == NULL) {
        int n = NIM_INSTR_ARG1(code, pc);
        NIM_BUG ("unknown or missing name #%d at pc=%d", n, pc);
        return NIM_FALSE;
    }

    if (!nim_vm_resolvename (vm, name, &value, NIM_FALSE)) {
        return NIM_FALSE;
    }

#ifdef NIM_VM_DEBUG
    printf ("[%p] PUSHNAME %s = %s\n",
            vm, NIM_STR_DATA(name), NIM_STR_DATA(nim_object_str (value)));
#endif
    return nim_vm_push (vm, value);
}

static nim_bool_t
nim_vm_getattr (NimVM *vm, NimRef *code, NimRef *locals, size_t pc)
{
    NimRef *attr;
    NimRef *target;
    NimRef *result;
    
    attr = NIM_INSTR_NAME1 (code, pc);
    if (attr == NULL) {
        return NIM_FALSE;
    }
    target = nim_vm_pop (vm);
    if (target == NULL) {
        return NIM_FALSE;
    }
    result = nim_object_getattr (target, attr);
    if (result == NULL) {
        return NIM_FALSE;
    }
#ifdef NIM_VM_DEBUG
    printf ("[%p] GETATTR %s = %s\n",
            vm, NIM_STR_DATA(attr), NIM_STR_DATA (nim_object_str (result)));
#endif
    if (!nim_vm_push (vm, result)) {
        return NIM_FALSE;
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_vm_call (NimVM *vm, NimRef *code, NimRef *locals, size_t pc)
{
    NimRef *args;
    NimRef *target;
    NimRef *result;
    uint8_t nargs;
    uint8_t i;

    nargs = NIM_INSTR_ARG1 (code, pc);
    args = nim_array_new_with_capacity (nargs);
    if (args == NULL) {
        return NIM_FALSE;
    }
    for (i = 0; i < nargs; i++) {
        nim_array_unshift (args, nim_vm_pop (vm));
    }
    target = nim_vm_pop (vm);
    if (target == NULL) {
        return NIM_FALSE;
    }
#ifdef NIM_VM_DEBUG
    printf ("[%p] CALL %zu = ", vm, (intmax_t) nargs);
#endif
    result = nim_object_call (target, args);
    if (result == NULL) {
        NIM_BUG ("target (%s) is not callable",
            NIM_STR_DATA(nim_object_str (target)));
        return NIM_FALSE;
    }
#ifdef NIM_VM_DEBUG
    printf ("%s\n", NIM_STR_DATA(nim_object_str (result)));
#endif
    if (!nim_vm_push (vm, result)) {
        return NIM_FALSE;
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_vm_makearray (NimVM *vm, NimRef *code, NimRef *locals, size_t pc)
{
    NimRef *array;
    int32_t nargs = NIM_INSTR_ARG1(code, pc);
    int32_t i;

    array = nim_array_new_with_capacity (nargs);
    if (array == NULL) {
        return NIM_FALSE;
    }
    for (i = 0; i < nargs; i++) {
        NimRef *value = nim_vm_pop (vm);
        if (value == NULL) {
            return NIM_FALSE;
        }
        if (!nim_array_unshift (array, value)) {
            return NIM_FALSE;
        }
    }
    if (!nim_vm_push (vm, array)) {
        return NIM_FALSE;
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_vm_makehash (NimVM *vm, NimRef *code, NimRef *locals, size_t pc)
{
    NimRef *hash;
    int32_t nargs = NIM_INSTR_ARG1(code, pc);
    int32_t i;

    hash = nim_hash_new ();
    if (hash == NULL) {
        return NIM_FALSE;
    }
    for (i = 0; i < nargs; i++) {
        NimRef *value;
        NimRef *key;
        
        value = nim_vm_pop (vm);
        if (value == NULL) {
            return NIM_FALSE;
        }
        key = nim_vm_pop (vm);
        if (key == NULL || key == nim_nil) {
            return NIM_FALSE;
        }

        if (!nim_hash_put (hash, key, value)) {
            return NIM_FALSE;
        }
    }
    if (!nim_vm_push (vm, hash)) {
        return NIM_FALSE;
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_vm_makeclosure (NimVM *vm, NimRef *code, size_t pc)
{
    NimRef *freevars;
    size_t i;
    NimRef *bindings = nim_hash_new ();
    NimRef *method = nim_vm_pop (vm);
    if (method == NULL || method == nim_nil) {
        return NIM_FALSE;
    }
    freevars = NIM_CODE(NIM_METHOD(method)->bytecode.code)->freevars;
    for (i = 0; i < NIM_ARRAY_SIZE(freevars); i++) {
        NimRef *varname = NIM_ARRAY_ITEM(freevars, i);
        NimRef *value;
        if (!nim_vm_resolvename (vm, varname, &value, NIM_TRUE)) {
            return NIM_FALSE;
        }
        if (!nim_hash_put (bindings, varname, value)) {
            return NIM_FALSE;
        }
    }
    method = nim_method_new_closure (method, bindings);
    if (method == NULL) {
        return NIM_FALSE;
    }
    if (!nim_vm_push (vm, method)) {
        return NIM_FALSE;
    }
    return NIM_TRUE;
}

static nim_bool_t
nim_vm_pushtrue (NimVM *vm)
{
    return nim_vm_push (vm, nim_true);
}

static nim_bool_t
nim_vm_pushfalse (NimVM *vm)
{
    return nim_vm_push (vm, nim_false);
}

static nim_bool_t
nim_vm_truthy (NimRef *value)
{
    return value != NULL && value != nim_nil && value != nim_false;
}

static NimCmpResult
nim_vm_cmp (NimVM *vm)
{
    NimRef *right = nim_vm_pop (vm);
    NimRef *left = nim_vm_pop (vm);
    NimCmpResult actual;
    if (left == NULL || right == NULL) {
        NIM_BUG ("NULL value on the stack");
        return NIM_FALSE;
    }

#ifdef NIM_VM_DEBUG
    printf ("[%p] CMP %s, %s\n",
            vm,
            NIM_STR_DATA(NIM_CLASS_NAME(NIM_ANY_CLASS(left))),
            NIM_STR_DATA(NIM_CLASS_NAME(NIM_ANY_CLASS(right))));
#endif

    actual = nim_object_cmp (left, right);
    if (actual == NIM_CMP_ERROR) {
        NIM_BUG ("TODO raise an exception");
    }
    return actual;
}

static NimRef *
nim_vm_eval_frame (NimVM *vm, NimRef *frame)
{
    NimRef *code = NIM_FRAME_CODE(frame);
    NimRef *locals = NIM_FRAME(frame)->locals;

    if (!nim_array_push (vm->frames, frame)) {
        return NIM_FALSE;
    }

    size_t pc = 0;
    while (pc < NIM_CODE_SIZE(code)) {
        switch (NIM_INSTR_OP(code, pc)) {
            case NIM_OPCODE_PUSHCONST:
            {
                if (!nim_vm_pushconst (vm, code, locals, pc)) {
                    NIM_BUG ("PUSHCONST instruction failed");
                    return NULL;
                }
                pc++;
                break;
            }
            case NIM_OPCODE_STORENAME:
            {
                if (!nim_vm_storename (vm, code, locals, pc)) {
                    NIM_BUG ("STORENAME instruction failed");
                    return NULL;
                }
                pc++;
                break;
            }
            case NIM_OPCODE_PUSHNAME:
            {
                if (!nim_vm_pushname (vm, code, locals, pc)) {
                    NIM_BUG ("PUSHNAME instruction failed");
                    return NULL;
                }
                pc++;
                break;
            }
            case NIM_OPCODE_PUSHNIL:
            {
                if (!nim_vm_push (vm, nim_nil)) {
                    NIM_BUG ("PUSHNIL instruction failed");
                    return NULL;
                }
                pc++;
                break;
            }
            case NIM_OPCODE_GETCLASS:
            {
                NimRef *value = nim_vm_pop (vm);
#ifdef NIM_VM_DEBUG
                printf ("[%p] GETCLASS = %s\n",
                        vm, NIM_STR_DATA(NIM_CLASS_NAME(NIM_ANY_CLASS(value))));
#endif
                if (value == NULL) {
                    NIM_BUG ("GETCLASS instruction failed");
                    return NULL;
                }
                if (!nim_vm_push (vm, NIM_ANY_CLASS(value))) {
                    return NULL;
                }
                pc++;
                break;
            }
            case NIM_OPCODE_GETATTR:
            {
                if (!nim_vm_getattr (vm, code, locals, pc)) {
                    NIM_BUG ("GETATTR instruction failed");
                    return NULL;
                }
                pc++;
                break;
            }
            case NIM_OPCODE_GETITEM:
            {
                NimRef *result;
                NimRef *key;
                NimRef *target;
                
                key = nim_vm_pop (vm);
                if (key == NULL) {
                    NIM_BUG ("GETITEM instruction failed 1");
                    return NULL;
                }

                target = nim_vm_pop (vm);
                if (target == NULL) {
                    NIM_BUG ("GETITEM instruction failed 2");
                    return NULL;
                }

                result = nim_object_getitem (target, key);
                if (result == NULL) {
                    NIM_BUG ("GETITEM instruction failed 3");
                    return NULL;
                }

#ifdef NIM_VM_DEBUG
                printf ("[%p] GETITEM = %s\n",
                        vm,
                        NIM_STR_DATA(nim_object_str (result)));
#endif

                if (!nim_vm_push (vm, result)) {
                    NIM_BUG ("GETITEM instruction failed 4");
                    return NULL;
                }

                pc++;
                break;
            }
            case NIM_OPCODE_CALL:
            {
                if (!nim_vm_call (vm, code, locals, pc)) {
                    NIM_BUG ("CALL instruction failed");
                    return NULL;
                }
                pc++;
                break;
            }
            case NIM_OPCODE_RET:
            {
                goto done;
            }
            case NIM_OPCODE_SPAWN:
            {
                NimRef *task;
                NimRef *target;

                target = nim_vm_pop (vm);
                if (target == NULL) {
                    return NIM_FALSE;
                }
                if (NIM_METHOD_TYPE(target) == NIM_METHOD_TYPE_CLOSURE) {
                    NIM_BUG ("cannot use the spawn keyword with a closure");
                    return NIM_FALSE;
                }
                task = nim_task_new (target);
                if (task == NULL) {
                    return NIM_FALSE;
                }

                if (!nim_vm_push (vm, task)) {
                    return NIM_FALSE;
                }

                pc++;
                break;
            }
            case NIM_OPCODE_DUP:
            {
                NimRef *top = nim_vm_top (vm);

                if (top == NULL) {
                    return NIM_FALSE;
                }

#ifdef NIM_VM_DEBUG
                printf ("[%p] DUP = %s\n",
                        vm,
                        NIM_STR_DATA (nim_object_str (top)));
#endif

                if (!nim_vm_push (vm, top)) {
                    return NIM_FALSE;
                }

                pc++;
                break;
            }
            case NIM_OPCODE_NOT:
            {
                NimRef *value = nim_vm_pop (vm);
                if (value == NULL) {
                    return NIM_FALSE;
                }
                if (nim_vm_truthy (value)) {
                    if (!nim_vm_push (vm, nim_false)) {
                        return NIM_FALSE;
                    }
                }
                else {
                    if (!nim_vm_push (vm, nim_true)) {
                        return NIM_FALSE;
                    }
                }
                pc++;
                break;
            }
            case NIM_OPCODE_MAKEARRAY:
            {
                if (!nim_vm_makearray (vm, code, locals, pc)) {
                    NIM_BUG ("MAKEARRAY instruction failed");
                    return NULL;
                }
                pc++;
                break;
            }
            case NIM_OPCODE_MAKEHASH:
            {
                if (!nim_vm_makehash (vm, code, locals, pc)) {
                    NIM_BUG ("MAKEHASH instruction failed");
                    return NULL;
                }
                pc++;
                break;
            }
            case NIM_OPCODE_MAKECLOSURE:
            {
                if (!nim_vm_makeclosure (vm, code, pc)) {
                    NIM_BUG ("MAKECLOSURE instruction failed");
                    return NULL;
                }
                pc++;
                break;
            }
            case NIM_OPCODE_JUMPIFTRUE:
            {
                NimRef *value = nim_vm_pop (vm);
                if (value == NULL) {
                    NIM_BUG ("NULL value on the stack");
                    return NULL;
                }
                if (nim_vm_truthy (value)) {
#ifdef NIM_VM_DEBUG
                    printf ("[%p] JUMPIFTRUE %zu\n", vm, (intmax_t) pc);
#endif
                    pc = NIM_INSTR_ADDR(code, pc);
                }
                else {
                    pc++;
                }
                break;
            }
            case NIM_OPCODE_JUMPIFFALSE:
            {
                NimRef *value = nim_vm_pop (vm);
                if (value == NULL) {
                    NIM_BUG ("NULL value on the stack");
                    return NULL;
                }
                /* TODO test for non-truthiness */
                if (value == nim_false || value == nim_nil) {
#ifdef NIM_VM_DEBUG
                    printf ("[%p] JUMPIFFALSE %zu\n", vm, (intmax_t) pc);
#endif
                    pc = NIM_INSTR_ADDR(code, pc);
                }
                else {
                    pc++;
                }
                break;
            }
            case NIM_OPCODE_JUMP:
            {
                pc = NIM_INSTR_ADDR(code, pc);
                break;
            }
            case NIM_OPCODE_CMPEQ:
            {
                NimCmpResult r = nim_vm_cmp (vm);
                if (r == NIM_CMP_ERROR) {
                    return NULL;
                }
                else if (r == NIM_CMP_EQ) {
                    if (!nim_vm_pushtrue (vm)) {
                        return NULL;
                    }
                }
                else {
                    if (!nim_vm_pushfalse (vm)) {
                        return NULL;
                    }
                }
                pc++;
                break;
            }
            case NIM_OPCODE_CMPNEQ:
            {
                NimCmpResult r = nim_vm_cmp (vm);
                if (r == NIM_CMP_ERROR) {
                    return NULL;
                }
                else if (r == NIM_CMP_EQ) {
                    if (!nim_vm_pushfalse (vm)) {
                        return NULL;
                    }
                }
                else {
                    if (!nim_vm_pushtrue (vm)) {
                        return NULL;
                    }
                }
                pc++;
                break;
            }
            case NIM_OPCODE_CMPGT:
            {
                NimCmpResult r = nim_vm_cmp (vm);
                if (r == NIM_CMP_ERROR) {
                    return NULL;
                }
                else if (r == NIM_CMP_GT) {
                    if (!nim_vm_pushtrue (vm)) {
                        return NULL;
                    }
                }
                else {
                    if (!nim_vm_pushfalse (vm)) {
                        return NULL;
                    }
                }
                pc++;
                break;
            }
            case NIM_OPCODE_CMPGTE:
            {
                NimCmpResult r = nim_vm_cmp (vm);
                if (r == NIM_CMP_ERROR) {
                    return NULL;
                }
                else if (r == NIM_CMP_GT || r == NIM_CMP_EQ) {
                    if (!nim_vm_pushtrue (vm)) {
                        return NULL;
                    }
                }
                else {
                    if (!nim_vm_pushfalse (vm)) {
                        return NULL;
                    }
                }
                pc++;
                break;
            }
            case NIM_OPCODE_CMPLT:
            {
                NimCmpResult r = nim_vm_cmp (vm);
                if (r == NIM_CMP_ERROR) {
                    return NULL;
                }
                else if (r == NIM_CMP_LT) {
                    if (!nim_vm_pushtrue (vm)) {
                        return NULL;
                    }
                }
                else {
                    if (!nim_vm_pushfalse (vm)) {
                        return NULL;
                    }
                }
                pc++;
                break;
            }
            case NIM_OPCODE_CMPLTE:
            {
                NimCmpResult r = nim_vm_cmp (vm);
                if (r == NIM_CMP_ERROR) {
                    return NULL;
                }
                else if (r == NIM_CMP_LT || r == NIM_CMP_EQ) {
                    if (!nim_vm_pushtrue (vm)) {
                        return NULL;
                    }
                }
                else {
                    if (!nim_vm_pushfalse (vm)) {
                        return NULL;
                    }
                }
                pc++;
                break;
            }
            case NIM_OPCODE_POP:
            {
#ifdef NIM_VM_DEBUG
                printf ("[%p] POP = %s\n",
                        vm, NIM_STR_DATA (nim_object_str (nim_vm_top (vm))));
#endif
                if (!nim_vm_pop (vm)) {
                    return NULL;
                }
                pc++;
                break;
            }
            case NIM_OPCODE_ADD:
            {
                NimRef *left;
                NimRef *right;
                NimRef *result;

                right = nim_vm_pop (vm);
                if (right == NULL) {
                    return NULL;
                }

                left = nim_vm_pop (vm);
                if (left == NULL) {
                    return NULL;
                }

                result = nim_object_add (left, right);
#ifdef NIM_VM_DEBUG
                printf ("[%p] ADD = %s\n", vm, NIM_STR_DATA (nim_object_str (result)));
#endif
                if (!nim_vm_push (vm, result)) {
                    return NULL;
                }
                pc++;
                break;
            }
            case NIM_OPCODE_SUB:
            {
                NimRef *left;
                NimRef *right;
                NimRef *result;

                right = nim_vm_pop (vm);
                if (right == NULL) {
                    return NULL;
                }

                left = nim_vm_pop (vm);
                if (left == NULL) {
                    return NULL;
                }

                result = nim_object_sub (left, right);
                if (!nim_vm_push (vm, result)) {
                    return NULL;
                }
#ifdef NIM_VM_DEBUG
                printf ("[%p] SUB = %s\n", vm, NIM_STR_DATA (nim_object_str (result)));
#endif
                pc++;
                break;
            }
            case NIM_OPCODE_MUL:
            {
                NimRef *left;
                NimRef *right;
                NimRef *result;

                right = nim_vm_pop (vm);
                if (right == NULL) {
                    return NULL;
                }

                left = nim_vm_pop (vm);
                if (left == NULL) {
                    return NULL;
                }

                result = nim_object_mul (left, right);
                if (!nim_vm_push (vm, result)) {
                    return NULL;
                }
#ifdef NIM_VM_DEBUG
                printf ("[%p] MUL = %s\n", vm, NIM_STR_DATA (nim_object_str (result)));
#endif
                pc++;
                break;
            }
            case NIM_OPCODE_DIV:
            {
                NimRef *left;
                NimRef *right;
                NimRef *result;

                right = nim_vm_pop (vm);
                if (right == NULL) {
                    return NULL;
                }

                left = nim_vm_pop (vm);
                if (left == NULL) {
                    return NULL;
                }

                result = nim_object_div (left, right);
                if (!nim_vm_push (vm, result)) {
                    return NULL;
                }
#ifdef NIM_VM_DEBUG
                printf ("[%p] DIV = %s\n", vm, NIM_STR_DATA (nim_object_str (result)));
#endif
                pc++;
                break;
            }
            default:
            {
                NIM_BUG ("unknown opcode: %d", NIM_INSTR_OP(code, pc));
                return NULL;
            }
        };
    }

done:
    nim_array_pop (vm->frames);
    return nim_vm_pop(vm);
}

/*
NimRef *
nim_vm_eval (NimVM *vm, NimRef *code, NimRef *locals)
{
    NimRef *frame;
    if (vm == NULL) {
        vm = NIM_CURRENT_VM;
    }
    if (code == NULL) {
        NIM_BUG ("NULL code object passed to nim_vm_eval");
        return NULL;
    }
    frame = nim_frame_new (NULL, code, locals);
    if (frame == NULL) {
        return NULL;
    }
    return nim_vm_eval_frame (vm, frame);
}
*/

#define NIM_IS_BYTECODE_METHOD(ref) \
    (NIM_ANY_CLASS(ref) == nim_method_class && \
        ( \
            (NIM_METHOD(ref)->type == NIM_METHOD_TYPE_BYTECODE) || \
            (NIM_METHOD(ref)->type == NIM_METHOD_TYPE_CLOSURE) \
        ) \
    )

NimRef *
nim_vm_invoke (NimVM *vm, NimRef *method, NimRef *args)
{
    size_t i;
    size_t stack_size;
    NimRef *frame;
    NimRef *ret;

    if (!NIM_IS_BYTECODE_METHOD(method)) {
        return nim_object_call (method, args);
    }

    if (vm == NULL) {
        vm = NIM_CURRENT_VM;
    }

    /* save the stack */
    stack_size = NIM_ARRAY_SIZE(vm->stack);

    frame = nim_frame_new (method);
    if (frame == NULL) {
        NIM_BUG ("nim_vm_invoke failed to create a new execution frame");
        return NULL;
    }

    /* push args */
    for (i = 0; i < NIM_ARRAY_SIZE(args); i++) {
        if (!nim_vm_push (vm, NIM_ARRAY_ITEM(args, i))) {
            NIM_BUG ("nim_vm_invoke failed to append array item");
            return NULL;
        }
    }

    ret = nim_vm_eval_frame (vm, frame);

    /* restore the stack */
    NIM_ARRAY(vm->stack)->size = stack_size;

    return ret;
}

