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

#include "nim/var.h"
#include "nim/str.h"
#include "nim/class.h"
#include "nim/object.h"

NimRef *nim_var_class = NULL;

static void
_nim_var_mark (NimGC *gc, NimRef *self)
{
    NIM_SUPER (self)->mark (gc, self);

    nim_gc_mark_ref (gc, NIM_VAR(self)->value);
}

nim_bool_t
nim_var_class_bootstrap (void)
{
    nim_var_class =
        nim_class_new (NIM_STR_NEW("var"), NULL, sizeof(NimVar));
    if (nim_var_class == NULL) {
        return NIM_FALSE;
    }
    NIM_CLASS(nim_var_class)->mark = _nim_var_mark;
    nim_gc_make_root (NULL, nim_var_class);
    return NIM_TRUE;
}

NimRef *
nim_var_new (void)
{
    NimRef *var = nim_class_new_instance (nim_var_class, NULL);
    if (var == NULL) {
        return NULL;
    }
    NIM_VAR(var)->value = NULL;
    return var;
}

