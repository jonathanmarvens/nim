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

#include "chimp/any.h"
#include "chimp/object.h"
#include "chimp/array.h"
#include "chimp/str.h"
#include "chimp/vm.h"

static ChimpRef *
_chimp_unit_test(ChimpRef *self, ChimpRef *args)
{
    fprintf (stdout, ".");

    // TODO: Size/argument validation on the incoming args
    // ChimpRef *name = chimp_object_str (CHIMP_ARRAY_ITEM(args, 0));
    ChimpRef *fn = CHIMP_ARRAY_ITEM(args, 1);

    // TODO: What if fn takes arguments? We're never passing anything...
    // TODO: How do we get the assertion results from the test?
    if (chimp_object_call (fn, chimp_array_new()) == NULL) {
        return NULL;
    }

    return chimp_nil;
}

ChimpRef *
chimp_init_unit_module (void)
{
    ChimpRef *mod;

    mod = chimp_module_new_str ("chimpunit", NULL);
    if (mod == NULL) {
        return NULL;
    }

    if (!chimp_module_add_method_str (mod, "test", _chimp_unit_test)) {
        return NULL;
    }

    return mod;
}

