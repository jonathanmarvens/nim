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
#include "nim/any.h"
#include "nim/object.h"
#include "nim/array.h"
#include "nim/str.h"

static NimRef *
_nim_gc_get_collection_count (NimRef *self, NimRef *args)
{
    return nim_int_new (nim_gc_collection_count (NULL));
}

static NimRef *
_nim_gc_get_live_count (NimRef *self, NimRef *args)
{
    return nim_int_new (nim_gc_num_live (NULL));
}

static NimRef *
_nim_gc_get_free_count (NimRef *self, NimRef *args)
{
    return nim_int_new (nim_gc_num_free (NULL));
}

static NimRef *
_nim_gc_collect (NimRef *self, NimRef *args)
{
    return nim_gc_collect (NULL) ? nim_true : nim_false;
}

NimRef *
nim_init_gc_module (void)
{
    NimRef *gc;

    gc = nim_module_new_str ("gc", NULL);
    if (gc == NULL) {
        return NULL;
    }

    if (!nim_module_add_method_str (
            gc, "get_collection_count", _nim_gc_get_collection_count)) {
        return NULL;
    }

    if (!nim_module_add_method_str (
            gc, "get_live_count", _nim_gc_get_live_count)) {
        return NULL;
    }

    if (!nim_module_add_method_str (
            gc, "get_free_count", _nim_gc_get_free_count)) {
        return NULL;
    }

    if (!nim_module_add_method_str (
            gc, "collect", _nim_gc_collect)) {
        return NULL;
    }

    return gc;
}


