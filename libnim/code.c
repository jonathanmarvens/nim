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

#include "nim/code.h"
#include "nim/str.h"
#include "nim/class.h"
#include "nim/array.h"
#include "nim/int.h"

NimRef *nim_code_class = NULL;

void
nim_code_use_label (NimRef *self, NimLabel *label)
{
    if (!label->in_use) {
        size_t i;
        label->addr = NIM_CODE(self)->used;
        label->in_use = NIM_TRUE;
        for (i = 0; i < label->patchlist_size; i++) {
            NIM_CODE(self)->bytecode[label->patchlist[i]] |=
                                            (label->addr & 0xffffff);
        }
        nim_label_free (label);
    }
    else {
        NIM_BUG ("Label is already in use\n");
    }
}

void
nim_label_free (NimLabel *label)
{
    if (label->patchlist != NULL) {
        free (label->patchlist);
        label->patchlist = NULL;
    }
}

static nim_bool_t
nim_code_get_jump_addr (NimRef *self, NimLabel *label, size_t *dest_addr)
{
    if (label->in_use) {
        *dest_addr = label->addr;
    }
    else {
        const size_t size =
            sizeof(*label->patchlist) * (label->patchlist_size + 1);
        size_t *patchlist = realloc (label->patchlist, size);
        if (patchlist == NULL) {
            return NIM_FALSE;
        }
        label->patchlist = patchlist;
        label->patchlist[label->patchlist_size++] = NIM_CODE(self)->used;
        *dest_addr = 0;
    }
    return NIM_TRUE;
}

static void
_nim_code_dtor (NimRef *self)
{
    NIM_FREE (NIM_CODE(self)->bytecode);
}

static void
_nim_code_mark (NimGC *gc, NimRef *self)
{
    NIM_SUPER (self)->mark (gc, self);

    nim_gc_mark_ref (gc, NIM_CODE(self)->constants);
    nim_gc_mark_ref (gc, NIM_CODE(self)->names);
    nim_gc_mark_ref (gc, NIM_CODE(self)->vars);
    nim_gc_mark_ref (gc, NIM_CODE(self)->freevars);
}

static NimRef *
_nim_code_init (NimRef *self, NimRef *args)
{
    NimRef *temp;
    temp = nim_array_new ();
    if (temp == NULL) {
        NIM_BUG ("wtf");
        return NULL;
    }
    NIM_CODE(self)->constants = temp;
    temp = nim_array_new ();
    if (temp == NULL) {
        NIM_BUG ("wtf");
        return NULL;
    }
    NIM_CODE(self)->names = temp;
    NIM_CODE(self)->allocated = 256;
    NIM_CODE(self)->bytecode =
        NIM_MALLOC(uint32_t, sizeof(uint32_t) * NIM_CODE(self)->allocated);
    if (NIM_CODE(self)->bytecode == NULL) {
        return NULL;
    }
    temp = nim_array_new ();
    if (temp == NULL) {
        NIM_BUG ("could not allocate vars array");
        return NULL;
    }
    NIM_CODE(self)->vars = temp;
    temp = nim_array_new ();
    if (temp == NULL) {
        NIM_BUG ("could not allocate freevars array");
        return NULL;
    }
    NIM_CODE(self)->freevars = temp;
    return self;
}

nim_bool_t
nim_code_class_bootstrap (void)
{
    nim_code_class =
        nim_class_new (NIM_STR_NEW("code"), NULL, sizeof(NimCode));
    if (nim_code_class == NULL) {
        return NIM_FALSE;
    }
    NIM_CLASS(nim_code_class)->init = _nim_code_init;
    NIM_CLASS(nim_code_class)->dtor = _nim_code_dtor;
    NIM_CLASS(nim_code_class)->mark = _nim_code_mark;
    nim_gc_make_root (NULL, nim_code_class);
    return NIM_TRUE;
}

NimRef *
nim_code_new (void)
{
    return nim_class_new_instance (nim_code_class, NULL);
}

#define NIM_CURR_INSTR(co) NIM_CODE(co)->bytecode[NIM_CODE(co)->used]
#define NIM_NEXT_INSTR(co) NIM_CODE(co)->bytecode[NIM_CODE(co)->used++]

#define NIM_JUMP_INSTR(co, op, label) \
    do { \
        size_t jump_addr; \
        if (!nim_code_get_jump_addr ((co), (label), &jump_addr)) { \
            nim_label_free (label); \
            return NIM_FALSE; \
        } \
        NIM_NEXT_INSTR(co) = \
            NIM_MAKE_INSTR0(op) | (jump_addr & 0xffffff); \
    } while (0)

#define NIM_MAKE_INSTR0(op) \
    (((NIM_OPCODE_ ## op) & 0xff) << 24)

#define NIM_MAKE_INSTR1(op, arg1) \
    ((((NIM_OPCODE_ ## op) & 0xff) << 24) | (((arg1) & 0xff) << 16))

#define NIM_MAKE_INSTR2(op, arg1, arg2) \
    ((((NIM_OPCODE_ ## op) & 0xff) << 24) | (((arg1) & 0xff) << 16) | (((arg2) & 0xff) << 8))

#define NIM_MAKE_INSTR3(op, arg1, arg2, arg3) \
    ((((NIM_OPCODE_ ## op) & 0xff) << 24) | (((arg1) & 0xff) << 16) | (((arg2) & 0xff) << 8) | ((arg3) & 0xff))

static nim_bool_t
nim_code_grow (NimRef *self)
{
    uint32_t *bytecode;
    if (NIM_CODE(self)->used < NIM_CODE(self)->allocated) {
        return NIM_TRUE;
    }
    bytecode = NIM_REALLOC(uint32_t, NIM_CODE(self)->bytecode, sizeof(uint32_t) * (NIM_CODE(self)->allocated + 32));
    if (bytecode == NULL) {
        return NIM_FALSE;
    }
    NIM_CODE(self)->bytecode = bytecode;
    NIM_CODE(self)->allocated += 32;
    return NIM_TRUE;
}

static int32_t
nim_code_add_const (NimRef *self, NimRef *value)
{
    int32_t n = nim_array_find (NIM_CODE(self)->constants, value);
    if (n >= 0) {
        return n;
    }
    if (!nim_array_push (NIM_CODE(self)->constants, value)) {
        return -1;
    }
    return NIM_ARRAY_SIZE(NIM_CODE(self)->constants) - 1;
}

static int32_t
nim_code_add_name (NimRef *self, NimRef *id)
{
    int32_t n = nim_array_find (NIM_CODE(self)->names, id);
    if (n >= 0) {
        return n;
    }
    if (!nim_array_push (NIM_CODE(self)->names, id)) {
        return -1;
    }
    return NIM_ARRAY_SIZE(NIM_CODE(self)->names) - 1;
}

nim_bool_t
nim_code_pushconst (NimRef *self, NimRef *value)
{
    int32_t arg;
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }
    arg = nim_code_add_const (self, value);
    if (arg < 0) {
        return NIM_FALSE;
    }
    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR1(PUSHCONST, arg);
    return NIM_TRUE;
}

nim_bool_t
nim_code_pushname (NimRef *self, NimRef *id)
{
    int32_t arg;
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }
    arg = nim_code_add_name (self, id);
    if (arg < 0) {
        return NIM_FALSE;
    }
    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR1(PUSHNAME, arg);
    return NIM_TRUE;
}

nim_bool_t
nim_code_pushnil (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }
    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(PUSHNIL);
    return NIM_TRUE;
}

nim_bool_t
nim_code_storename (NimRef *self, NimRef *id)
{
    int32_t arg;
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }

    arg = nim_code_add_name (self, id);
    if (arg < 0) {
        return NIM_FALSE;
    }

    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR1(STORENAME, arg);
    return NIM_TRUE;
}

nim_bool_t
nim_code_getclass (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }

    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(GETCLASS);

    return NIM_TRUE;
}


nim_bool_t
nim_code_getattr (NimRef *self, NimRef *id)
{
    int32_t arg;
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }

    arg = nim_code_add_name (self, id);
    if (arg < 0) {
        return NIM_FALSE;
    }

    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR1(GETATTR, arg);
    return NIM_TRUE;
}

nim_bool_t
nim_code_getitem (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }

    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(GETITEM);

    return NIM_TRUE;
}

nim_bool_t
nim_code_call (NimRef *self, uint8_t nargs)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }
    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR1(CALL, (int32_t)nargs);
    return NIM_TRUE;
}

nim_bool_t
nim_code_ret (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }
    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(RET);
    return NIM_TRUE;
}

nim_bool_t
nim_code_spawn (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }
    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(SPAWN);
    return NIM_TRUE;
}

nim_bool_t
nim_code_not (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }
    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(NOT);
    return NIM_TRUE;
}

nim_bool_t
nim_code_dup (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }
    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(DUP);
    return NIM_TRUE;
}

nim_bool_t
nim_code_makearray (NimRef *self, uint8_t nargs)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }
    /* XXX this cast should happen automatically in the make_instr macro */
    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR1(MAKEARRAY, (int32_t)nargs);
    return NIM_TRUE;
}

nim_bool_t
nim_code_makehash (NimRef *self, uint8_t nargs)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }
    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR1(MAKEHASH, (int32_t)nargs);
    return NIM_TRUE;
}

nim_bool_t
nim_code_makeclosure (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }
    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(MAKECLOSURE);
    return NIM_TRUE;
}

nim_bool_t
nim_code_jumpiftrue (NimRef *self, NimLabel *label)
{
    if (label == NULL) {
        NIM_BUG ("jump labels can't be null, fool.");
        return NIM_FALSE;
    }

    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }

    NIM_JUMP_INSTR(self, JUMPIFTRUE, label);

    return NIM_TRUE;
}

nim_bool_t
nim_code_jumpiffalse (NimRef *self, NimLabel *label)
{
    if (label == NULL) {
        NIM_BUG ("jump labels can't be null, fool.");
        return NIM_FALSE;
    }

    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }

    NIM_JUMP_INSTR(self, JUMPIFFALSE, label);

    return NIM_TRUE;
}

nim_bool_t
nim_code_jump (NimRef *self, NimLabel *label)
{
    if (label == NULL) {
        NIM_BUG ("jump labels can't be null, fool.");
        return NIM_FALSE;
    }

    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }

    NIM_JUMP_INSTR(self, JUMP, label);

    return NIM_TRUE;
}

nim_bool_t
nim_code_eq (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }

    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(CMPEQ);
    return NIM_TRUE;
}

nim_bool_t
nim_code_neq (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }

    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(CMPNEQ);
    return NIM_TRUE;
}

nim_bool_t
nim_code_gt (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }

    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(CMPGT);
    return NIM_TRUE;
}

nim_bool_t
nim_code_gte (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }

    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(CMPGTE);
    return NIM_TRUE;
}

nim_bool_t
nim_code_lt (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }

    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(CMPLT);
    return NIM_TRUE;
}

nim_bool_t
nim_code_lte (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }

    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(CMPLTE);
    return NIM_TRUE;
}

nim_bool_t
nim_code_add (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }

    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(ADD);
    return NIM_TRUE;
}

nim_bool_t
nim_code_sub (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }

    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(SUB);
    return NIM_TRUE;
}

nim_bool_t
nim_code_mul (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }

    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(MUL);
    return NIM_TRUE;
}

nim_bool_t
nim_code_div (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }

    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(DIV);
    return NIM_TRUE;
}

nim_bool_t
nim_code_pop (NimRef *self)
{
    if (!nim_code_grow (self)) {
        return NIM_FALSE;
    }

    NIM_NEXT_INSTR(self) = NIM_MAKE_INSTR0(POP);
    return NIM_TRUE;
}

static const char *
nim_code_opcode_str (NimOpcode op)
{
    switch (op) {
        case NIM_OPCODE_JUMPIFFALSE:
            return "JUMP_IF_FALSE";
        case NIM_OPCODE_JUMPIFTRUE:
             return "JUMP_IF_TRUE";
        case NIM_OPCODE_JUMP:
             return "JUMP";
        case NIM_OPCODE_PUSHCONST:
             return "PUSHCONST";
        case NIM_OPCODE_STORENAME:
             return "STORENAME";
        case NIM_OPCODE_PUSHNAME:
             return "PUSHNAME";
        case NIM_OPCODE_PUSHNIL:
             return "PUSHNIL";
        case NIM_OPCODE_GETATTR:
             return "GETATTR";
        case NIM_OPCODE_GETITEM:
             return "GETITEM";
        case NIM_OPCODE_CALL:
             return "CALL";
        case NIM_OPCODE_MAKEARRAY:
             return "MAKEARRAY";
        case NIM_OPCODE_MAKEHASH:
             return "MAKEHASH";
        case NIM_OPCODE_CMPEQ:
             return "CMP_EQ";
        case NIM_OPCODE_CMPNEQ:
             return "CMP_NEQ";
        case NIM_OPCODE_CMPGT:
             return "CMP_GT";
        case NIM_OPCODE_CMPGTE:
             return "CMP_GTE";
        case NIM_OPCODE_CMPLT:
             return "CMP_LT";
        case NIM_OPCODE_CMPLTE:
             return "CMP_LTE";
        case NIM_OPCODE_POP:
             return "POP";
        case NIM_OPCODE_RET:
             return "RET";
        case NIM_OPCODE_ADD:
             return "ADD";
        case NIM_OPCODE_SUB:
             return "SUB";
        case NIM_OPCODE_MUL:
             return "MUL";
        case NIM_OPCODE_DIV:
             return "DIV";
        case NIM_OPCODE_SPAWN:
             return "SPAWN";
        case NIM_OPCODE_NOT:
             return "NOT";
        case NIM_OPCODE_DUP:
             return "DUP";
        case NIM_OPCODE_GETCLASS:
             return "GETCLASS";
        case NIM_OPCODE_MAKECLOSURE:
             return "MAKECLOSURE";
        default:
             return "???OPCODE???";
    };
}

NimRef *
nim_code_dump (NimRef *self)
{
    /* XXX super duper inefficient & sloppy. */
    /*     mostly for diagnostic purposes atm. */
    size_t i;
    NimRef *str = NIM_STR_NEW ("");
    for (i = 0; i < NIM_CODE(self)->used; i++) {
        int32_t instr = NIM_CODE_INSTR(self, i);
        NimOpcode op = ((NimOpcode)((instr & 0xff000000) >> 24));
        const char *op_str = nim_code_opcode_str (op);
        if (!nim_str_append (str, nim_int_new (i))) {
            return NULL;
        }
        if (!nim_str_append_str (str, " ")) {
            return NULL;
        }
        if (!nim_str_append_str (str, op_str)) {
            return NULL;
        }
        if (op == NIM_OPCODE_PUSHNAME || op == NIM_OPCODE_STORENAME || op == NIM_OPCODE_GETATTR) {
            if (!nim_str_append_str (str, " ")) {
                return NULL;
            }
            if (!nim_str_append (str, NIM_INSTR_NAME1(self, i))) {
                return NULL;
            }
        }
        else if (op == NIM_OPCODE_PUSHCONST) {
            if (!nim_str_append_str (str, " ")) {
                return NULL;
            }
            if (!nim_str_append (str, NIM_INSTR_CONST1(self, i))) {
                return NULL;
            }
        }
        else if (op == NIM_OPCODE_MAKEARRAY || op == NIM_OPCODE_MAKEHASH || op == NIM_OPCODE_CALL) {
            if (!nim_str_append_str (str, " ")) {
                return NULL;
            }
            if (!nim_str_append (str, nim_int_new (NIM_INSTR_ARG1(self, i)))) {
                return NULL;
            }
        }
        else if (op == NIM_OPCODE_JUMP || op == NIM_OPCODE_JUMPIFTRUE || op == NIM_OPCODE_JUMPIFFALSE) {
            if (!nim_str_append_str (str, " ")) {
                return NULL;
            }
            if (!nim_str_append (str, nim_int_new ((instr & 0xffffff)))) {
                return NULL;
            }
        }
        if (!nim_str_append_str (str, "\n")) {
            return NULL;
        }
    }
    return str;
}

