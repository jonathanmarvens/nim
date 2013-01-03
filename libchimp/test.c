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
#include <stdlib.h>

#include "chimp/test.h"
#include "chimp/str.h"
#include "chimp/class.h"
#include "chimp/ast.h"

ChimpRef *chimp_test_class = NULL;

static ChimpRef *
_chimp_test_init (ChimpRef *self, ChimpRef *args)
{
    return self;
}

static ChimpRef *
chimp_test_fail (ChimpRef *self, ChimpRef *args)
{
    fprintf (stderr, "assertion failed: on purpose!\n");
    exit(1);
    return NULL;
}

chimp_bool_t
chimp_test_class_bootstrap (void)
{
    chimp_test_class =
        chimp_class_new (CHIMP_STR_NEW ("test"), NULL, sizeof(ChimpTest));
    if (chimp_test_class == NULL) {
        return CHIMP_FALSE;
    }
    chimp_gc_make_root (NULL, chimp_test_class);
    CHIMP_CLASS(chimp_test_class)->init = _chimp_test_init;
    chimp_class_add_native_method (chimp_test_class, "fail", chimp_test_fail);
    return CHIMP_TRUE;
}

ChimpRef *
chimp_test_new ()
{
    ChimpRef* ref = chimp_class_new_instance (chimp_test_class, NULL);
    return ref;
}
