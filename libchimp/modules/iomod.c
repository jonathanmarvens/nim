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

#include <stdio.h>
#include "chimp/any.h"
#include "chimp/object.h"
#include "chimp/array.h"
#include "chimp/str.h"

static ChimpRef *
_chimp_io_output (ChimpRef *self, ChimpRef *args, char* separator)
{
    size_t i;

    for (i = 0; i < CHIMP_ARRAY_SIZE(args); i++) {
        ChimpRef *str = chimp_object_str (CHIMP_ARRAY_ITEM (args, i));
        printf ("%s%s", CHIMP_STR_DATA(str), separator);
    }

    return chimp_nil;
}

static ChimpRef *
_chimp_io_print (ChimpRef *self, ChimpRef *args)
{
    return _chimp_io_output(self, args, "\n");
}


static ChimpRef *
_chimp_io_write (ChimpRef *self, ChimpRef *args)
{
    return _chimp_io_output(self, args, "");
}


static ChimpRef *
_chimp_io_readline (ChimpRef *self, ChimpRef *args)
{
    char buf[1024];
    size_t len;

    if (fgets (buf, sizeof(buf), stdin) == NULL) {
        return chimp_nil;
    }

    len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') {
        buf[--len] = '\0';
    }
    return chimp_str_new (buf, len);
}

ChimpRef *
chimp_init_io_module (void)
{
    ChimpRef *io;

    io = chimp_module_new_str ("io", NULL);
    if (io == NULL) {
        return NULL;
    }

    if (!chimp_module_add_method_str (io, "print", _chimp_io_print)) {
        return NULL;
    }

    if (!chimp_module_add_method_str (io, "write", _chimp_io_write)) {
        return NULL;
    }

    if (!chimp_module_add_method_str (io, "readline", _chimp_io_readline)) {
        return NULL;
    }

    return io;
}

