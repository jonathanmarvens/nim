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

#include "chimp/var.h"
#include "chimp/str.h"
#include "chimp/class.h"
#include "chimp/object.h"

ChimpRef *chimp_var_class = NULL;

static void
_chimp_var_mark (ChimpGC *gc, ChimpRef *self)
{
    CHIMP_SUPER (self)->mark (gc, self);

    chimp_gc_mark_ref (gc, CHIMP_VAR(self)->value);
}

chimp_bool_t
chimp_var_class_bootstrap (void)
{
    chimp_var_class =
        chimp_class_new (CHIMP_STR_NEW("var"), NULL, sizeof(ChimpVar));
    if (chimp_var_class == NULL) {
        return CHIMP_FALSE;
    }
    CHIMP_CLASS(chimp_var_class)->mark = _chimp_var_mark;
    chimp_gc_make_root (NULL, chimp_var_class);
    return CHIMP_TRUE;
}

ChimpRef *
chimp_var_new (void)
{
    ChimpRef *var = chimp_class_new_instance (chimp_var_class, NULL);
    if (var == NULL) {
        return NULL;
    }
    CHIMP_VAR(var)->value = NULL;
    return var;
}

