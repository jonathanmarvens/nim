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

    module = CHIMP_COMPILE_MODULE_FROM_FILE (NULL, argv[1]);
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

    return 0;
}

