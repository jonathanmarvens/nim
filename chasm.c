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

#include <chimp.h>
#include <stdio.h>
#include <inttypes.h>
#include <ctype.h>

typedef enum _ChimpArgType {
    CHIMP_ARG_NONE,
    CHIMP_ARG_STR,
    CHIMP_ARG_INT,
    CHIMP_ARG_NAME,
    CHIMP_ARG_ANY
} ChimpArgType;

typedef struct _ChimpOpMap {
    const char  *name;
    ChimpOpcode  opcode;
    ChimpArgType arg;
} ChimpOpMap;

static const ChimpOpMap opmap[] = {
    { "pushconst",    CHIMP_OPCODE_PUSHCONST,   CHIMP_ARG_ANY  },
    { "pushname",     CHIMP_OPCODE_PUSHNAME,    CHIMP_ARG_NAME },
    { "pushnil",      CHIMP_OPCODE_PUSHNIL,     CHIMP_ARG_NONE },
    { "storename",    CHIMP_OPCODE_STORENAME,   CHIMP_ARG_NAME },
    { "getclass",     CHIMP_OPCODE_GETCLASS,    CHIMP_ARG_NONE },
    { "getattr",      CHIMP_OPCODE_GETATTR,     CHIMP_ARG_NAME },
    { "getitem",      CHIMP_OPCODE_GETITEM,     CHIMP_ARG_NONE },
    { "call",         CHIMP_OPCODE_CALL,        CHIMP_ARG_INT  },
    { "ret",          CHIMP_OPCODE_RET,         CHIMP_ARG_NONE },
    { "not",          CHIMP_OPCODE_NOT,         CHIMP_ARG_NONE },
    { "dup",          CHIMP_OPCODE_DUP,         CHIMP_ARG_NONE },
    { "spawn",        CHIMP_OPCODE_SPAWN,       CHIMP_ARG_NONE },
    { "makearray",    CHIMP_OPCODE_MAKEARRAY,   CHIMP_ARG_INT  },
    { "makehash",     CHIMP_OPCODE_MAKEHASH,    CHIMP_ARG_INT  },
    { "makeclosure",  CHIMP_OPCODE_MAKECLOSURE, CHIMP_ARG_NONE },
    { "jumpiftrue",   CHIMP_OPCODE_JUMPIFTRUE,  CHIMP_ARG_NAME },
    { "jumpiffalse",  CHIMP_OPCODE_JUMPIFFALSE, CHIMP_ARG_NAME },
    { "jump",         CHIMP_OPCODE_JUMP,        CHIMP_ARG_NAME },
    { "cmpeq",        CHIMP_OPCODE_CMPEQ,       CHIMP_ARG_NONE },
    { "cmpneq",       CHIMP_OPCODE_CMPNEQ,      CHIMP_ARG_NONE },
    { "cmpgt",        CHIMP_OPCODE_CMPGT,       CHIMP_ARG_NONE },
    { "cmpgte",       CHIMP_OPCODE_CMPGTE,      CHIMP_ARG_NONE },
    { "cmplt",        CHIMP_OPCODE_CMPLT,       CHIMP_ARG_NONE },
    { "pop",          CHIMP_OPCODE_POP,         CHIMP_ARG_NONE },
    { "add",          CHIMP_OPCODE_ADD,         CHIMP_ARG_NONE },
    { "sub",          CHIMP_OPCODE_SUB,         CHIMP_ARG_NONE },
    { "mul",          CHIMP_OPCODE_MUL,         CHIMP_ARG_NONE },
    { "div",          CHIMP_OPCODE_DIV,         CHIMP_ARG_NONE },
    { "getclass",     CHIMP_OPCODE_GETCLASS,    CHIMP_ARG_NONE },
    { NULL,           (ChimpOpcode)-1,          CHIMP_ARG_NONE }
};
static int
real_main (int argc, char **argv);

int
main (int argc, char **argv)
{
    int rc;

    if (!chimp_core_startup (getenv ("CHIMP_PATH"), (void *)&rc)) {
        fprintf (stderr, "error: unable to initialize chimp core\n");
        return 1;
    }

    rc = real_main(argc, argv);

    chimp_core_shutdown ();
    return rc;
}

static ChimpRef *
parse_args (int argc, char **argv)
{
    int i;
    ChimpRef *args;
    ChimpRef *argv_ = chimp_array_new ();
    for (i = 1; i < argc; i++) {
        ChimpRef *arg = chimp_str_new (argv[i], strlen(argv[i]));
        if (arg == NULL) {
            return NULL;
        }
        if (!chimp_array_push (argv_, arg)) {
            return NULL;
        }
    }
    args = chimp_array_new ();
    if (args == NULL) {
        return NULL;
    }
    if (!chimp_array_push (args, argv_)) {
        return NULL;
    }
    return args;
}

static int
_str_to_opcode (const char *s)
{
    const ChimpOpMap *op;
    for (op = opmap; op->name != NULL; op++) {
        if (strcasecmp (s, op->name) == 0) {
            return op->opcode;
        }
    }
    return -1;
}

static void
_skip_whitespace (const char **ptr)
{
    const char *p = *ptr;
    while (*p && isspace(*p)) {
        p++;
    }
    *ptr = p;
}

static chimp_bool_t
_parse_name (const char **ptr, ChimpRef **op)
{
    const char *begin;
    const char *p = *ptr;
    size_t len = 0;
    _skip_whitespace (&p);
    begin = p;
    if (*p != '_' && !isalpha(*p)) {
        return CHIMP_FALSE;
    }
    while (*p && (*p == '_' || isalnum(*p))) {
        len++;
        p++;
    }
    if (len == 0) return CHIMP_FALSE;
    *op = chimp_str_new (begin, len);
    if (*op == NULL) {
        return CHIMP_FALSE;
    }
    *ptr = p;
    return CHIMP_TRUE;
}

static chimp_bool_t
_parse_const_str (const char **ptr, ChimpRef **value)
{
    const char *begin;
    const char *p = *ptr;
    size_t len = 0;
    _skip_whitespace (&p);
    p++;
    begin = p;
    while (*p && *p != '"') {
        len++;
        p++;
    }
    if (!*p) {
        return CHIMP_FALSE;
    }
    p++;
    *value = chimp_str_new (begin, len);
    if (!*value) {
        return CHIMP_FALSE;
    }
    *ptr = p;
    return CHIMP_TRUE;
}

static chimp_bool_t
_parse_const_int (const char **ptr, ChimpRef **value)
{
    int64_t i = 0;
    const char *p = *ptr;
    chimp_bool_t neg;
    _skip_whitespace (&p);
    neg = (*p == '-');
    if (*p == '-' || *p == '+') p++;
    while (*p && isdigit (*p)) {
        i *= (int64_t) 10;
        i += (*p) - '0';
        p++;
    }
    *value = chimp_int_new (i);
    *ptr = p;
    return *value != NULL;
}

static chimp_bool_t
_parse_const (const char **ptr, ChimpRef **value)
{
    const char *p = *ptr;
    _skip_whitespace (&p);
    if (*p == '"') {
        if (_parse_const_str (&p, value)) {
            *ptr = p;
            return CHIMP_TRUE;
        }
        return CHIMP_FALSE;
    }
    else if (isdigit (*p)) {
        if (_parse_const_int (&p, value)) {
            *ptr = p;
            return CHIMP_TRUE;
        }
        return CHIMP_FALSE;
    }
    else {
        return CHIMP_FALSE;
    }
}

static ChimpRef *
assemble_module (const char *filename)
{
    char buf[8192];
    ChimpRef *code;
    ChimpRef *mod;
    ChimpRef *method;
    FILE *stream = fopen (filename, "r");
    if (stream == NULL) {
        fprintf (stderr, "error: failed to open %s\n", filename);
        return NULL;
    }
    code = chimp_code_new ();
    if (code == NULL) {
        fprintf (stderr, "error: failed to allocate code object\n");
        return NULL;
    }
    while (!feof (stream)) {
        const char *ptr;
        ChimpRef *opname;
        ChimpOpcode opcode;
        if (fgets (buf, sizeof(buf)-1, stream) == NULL) {
            break;
        }
        ptr = buf;
        _skip_whitespace (&ptr);
        if (!*ptr) continue;
        if (*ptr == '#') continue;
        if (!_parse_name (&ptr, &opname)) {
            fprintf (stderr, "error: expected name\n");
            goto error;
        }
        opcode = _str_to_opcode (CHIMP_STR_DATA (opname));
        switch (opcode) {
            case CHIMP_OPCODE_PUSHNIL:
                {
                    if (!chimp_code_pushnil (code)) {
                        goto error;
                    }
                    break;
                }
            case CHIMP_OPCODE_DUP:
                {
                    if (!chimp_code_dup (code)) {
                        goto error;
                    }
                    break;
                }
            case CHIMP_OPCODE_NOT:
                {
                    if (!chimp_code_not (code)) {
                        goto error;
                    }
                    break;
                }
            case CHIMP_OPCODE_RET:
                {
                    if (!chimp_code_ret (code)) {
                        goto error;
                    }
                    break;
                }
            case CHIMP_OPCODE_SPAWN:
                {
                    if (!chimp_code_spawn (code)) {
                        goto error;
                    }
                    break;
                }
            case CHIMP_OPCODE_MAKEARRAY:
                {
                    ChimpRef *nargs;

                    if (!_parse_const_int (&ptr, &nargs)) {
                        goto error;
                    }

                    if (!chimp_code_makearray (
                            code, CHIMP_INT(nargs)->value)) {
                        goto error;
                    }

                    break;
                }
            case CHIMP_OPCODE_MAKECLOSURE:
                {
                    if (!chimp_code_makeclosure (code)) {
                        goto error;
                    }
                    break;
                }
            case CHIMP_OPCODE_POP:
                {
                    if (!chimp_code_pop (code)) {
                        goto error;
                    }
                    break;
                }
            case CHIMP_OPCODE_PUSHNAME:
                {
                    ChimpRef *name;

                    if (!_parse_name (&ptr, &name)) {
                        goto error;
                    }

                    if (!chimp_code_pushname (code, name)) {
                        goto error;
                    }

                    break;
                }
            case CHIMP_OPCODE_STORENAME:
                {
                    ChimpRef *name;

                    if (!_parse_name (&ptr, &name)) {
                        goto error;
                    }

                    if (!chimp_code_storename (code, name)) {
                        goto error;
                    }

                    break;
                }
            case CHIMP_OPCODE_GETATTR:
                {
                    ChimpRef *name;

                    if (!_parse_name (&ptr, &name)) {
                        goto error;
                    }

                    if (!chimp_code_getattr (code, name)) {
                        goto error;
                    }

                    break;
                }
            case CHIMP_OPCODE_GETITEM:
                {
                    if (!chimp_code_getitem (code)) {
                        goto error;
                    }

                    break;
                }
            case CHIMP_OPCODE_PUSHCONST:
                {
                    ChimpRef *value;

                    if (!_parse_const (&ptr, &value)) {
                        goto error;
                    }

                    if (!chimp_code_pushconst (code, value)) {
                        goto error;
                    }
                    break;
                }
            case CHIMP_OPCODE_ADD:
                {
                    if (!chimp_code_add (code)) {
                        goto error;
                    }
                    break;
                }
            case CHIMP_OPCODE_SUB:
                {
                    if (!chimp_code_sub (code)) {
                        goto error;
                    }
                    break;
                }
            case CHIMP_OPCODE_MUL:
                {
                    if (!chimp_code_mul (code)) {
                        goto error;
                    }
                    break;
                }
            case CHIMP_OPCODE_DIV:
                {
                    if (!chimp_code_div (code)) {
                        goto error;
                    }
                    break;
                }
            case CHIMP_OPCODE_GETCLASS:
                {
                    if (!chimp_code_getclass (code)) {
                        goto error;
                    }
                    break;
                }
            case CHIMP_OPCODE_CALL:
                {
                    ChimpRef *nargs;

                    if (!_parse_const_int (&ptr, &nargs)) {
                        goto error;
                    }

                    if (!chimp_code_call (
                            code, (uint8_t) CHIMP_INT(nargs)->value)) {
                        goto error;
                    }
                    break;
                }
            default:
                fprintf (stderr, "error: unknown or unsupported opname: %s\n", CHIMP_STR_DATA (opname));
                goto error;
        };
        _skip_whitespace (&ptr);
        if (*ptr) {
            fprintf (stderr,
                "error: too many arguments for op: %s\n", CHIMP_STR_DATA(opname));
            goto error;
        }
    }
    fclose (stream);
    /* printf ("%s\n", CHIMP_STR_DATA (chimp_code_dump (code))); */
    mod = chimp_module_new_str ("main", NULL);
    if (mod == NULL) {
        return NULL;
    }
    method = chimp_method_new_bytecode (mod, code);
    if (method == NULL) {
        return NULL;
    }
    if (!chimp_module_add_local (mod, CHIMP_STR_NEW("main"), method)) {
        return NULL;
    }
    return mod;

error:
    fclose (stream);
    return NULL;
}

static int
real_main (int argc, char **argv)
{
    ChimpRef *result;
    ChimpRef *module;
    ChimpRef *main_method;
    ChimpRef *args;
    if (argc < 2) {
        fprintf (stderr, "usage: %s <file>\n", argv[0]);
        return 1;
    }

    module = assemble_module (argv[1]);
    if (module == NULL) {
        fprintf (stderr, "error: failed to compile %s\n", argv[1]);
        return 1;
    }

    main_method = chimp_object_getattr_str (module, "main");
    if (main_method == NULL) {
        fprintf (stderr, "error: could not find main method in this module\n");
        return 1;
    }

    args = parse_args (argc, argv);
    result = chimp_vm_invoke (NULL, main_method, args);
    if (result == NULL) {
        fprintf (stderr, "error: chimp_vm_invoke () returned NULL\n");
        return 1;
    }

    printf ("%s\n", CHIMP_STR_DATA (chimp_object_str (result)));

    return 0;
}

