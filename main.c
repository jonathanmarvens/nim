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

#include <nim.h>
#include <stdio.h>
#include <inttypes.h>

static int
real_main (int argc, char **argv);

int
main (int argc, char **argv)
{
    int rc;

    if (!nim_core_startup (getenv ("NIM_PATH"), (void *)&rc)) {
        fprintf (stderr, "error: unable to initialize nim core\n");
        return 1;
    }

    rc = real_main(argc, argv);

    nim_core_shutdown ();
    return rc;
}

static NimRef *
parse_args (int argc, char **argv)
{
    int i;
    NimRef *args;
    NimRef *argv_ = nim_array_new ();
    for (i = 1; i < argc; i++) {
        NimRef *arg = nim_str_new (argv[i], strlen(argv[i]));
        if (arg == NULL) {
            return NULL;
        }
        if (!nim_array_push (argv_, arg)) {
            return NULL;
        }
    }
    args = nim_array_new ();
    if (args == NULL) {
        return NULL;
    }
    if (!nim_array_push (args, argv_)) {
        return NULL;
    }
    return args;
}

static int
real_main (int argc, char **argv)
{
    NimRef *result;
    NimRef *module;
    NimRef *main_method;
    NimRef *args;
    if (argc < 2) {
        fprintf (stderr, "nim v%s [%s/%s]\n", NIM_VERSION, NIM_OS, NIM_ARCH);
        fprintf (stderr, "usage: %s <file>\n", argv[0]);
        return 1;
    }

    module = NIM_COMPILE_MODULE_FROM_FILE (NULL, argv[1]);
    if (module == NULL) {
        fprintf (stderr, "error: failed to compile %s\n", argv[1]);
        return 1;
    }

    main_method = nim_object_getattr_str (module, "main");
    if (main_method == NULL) {
        fprintf (stderr, "error: could not find main method in this module\n");
        return 1;
    }

    args = parse_args (argc, argv);
    result = nim_vm_invoke (NULL, main_method, args);
    if (result == NULL) {
        fprintf (stderr, "error: nim_vm_invoke () returned NULL\n");
        return 1;
    }

    return 0;
}

