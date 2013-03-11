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

#include <sys/stat.h>

#include "nim/module.h"
#include "nim/array.h"
#include "nim/object.h"
#include "nim/str.h"
#include "nim/compile.h"

NimRef *nim_module_class = NULL;

static NimRef *
nim_module_init (NimRef *self, NimRef *args)
{
    NIM_MODULE(self)->name = NIM_ARRAY_ITEM(args, 0);
    if (NIM_ARRAY_SIZE(args) == 2) {
        NIM_MODULE(self)->locals = NIM_ARRAY_ITEM(args, 1);
    }
    else {
        NimRef *temp = nim_hash_new ();
        if (temp == NULL) {
            return NULL;
        }
        NIM_MODULE(self)->locals = temp;
    }
    return self;
}

static NimRef *
nim_module_getattr (NimRef *self, NimRef *name)
{
    NimRef *klass;
    /* XXX this check is needed because getattr is called by the class ctor */
    if (NIM_MODULE(self)->locals != NULL) {
        NimRef *result;
        int rc;
        
        rc = nim_hash_get (NIM_MODULE(self)->locals, name, &result);
        if (rc == 0) {
            return result;
        }
    }
    klass = NIM_CLASS(NIM_ANY_CLASS(self))->super;
    if (NIM_CLASS(klass)->getattr != NULL) {
        return NIM_CLASS(klass)->getattr (self, name);
    }
    return NULL;
}

static NimRef *
nim_module_str (NimRef *self)
{
    NimRef *name = NIM_STR_NEW ("<module \"");
    nim_str_append (name, NIM_MODULE(self)->name);
    nim_str_append_str (name, "\">");
    return name;
}

static void
_nim_module_mark (NimGC *gc, NimRef *self)
{
    NIM_SUPER(self)->mark (gc, self);

    nim_gc_mark_ref (gc, NIM_MODULE(self)->name);
    nim_gc_mark_ref (gc, NIM_MODULE(self)->locals);
}

nim_bool_t
nim_module_class_bootstrap (void)
{
    nim_module_class = nim_class_new (NIM_STR_NEW ("module"), NULL, sizeof(NimModule));
    if (nim_module_class == NULL) {
        return NIM_FALSE;
    }
    nim_gc_make_root (NULL, nim_module_class);
    NIM_CLASS(nim_module_class)->str = nim_module_str;
    NIM_CLASS(nim_module_class)->getattr = nim_module_getattr;
    NIM_CLASS(nim_module_class)->init = nim_module_init;
    NIM_CLASS(nim_module_class)->mark = _nim_module_mark;
    return NIM_TRUE;
}

NimRef *
nim_module_new (NimRef *name, NimRef *locals)
{
    if (locals == NULL) {
        locals = nim_hash_new ();
        if (locals == NULL) {
            return NULL;
        }
    }
    return nim_class_new_instance (nim_module_class, name, locals, NULL);
}

NimRef *
nim_module_new_str (const char *name, NimRef *locals)
{
    return nim_module_new (nim_str_new (name, strlen(name)), locals);
}

nim_bool_t
nim_module_add_local (NimRef *self, NimRef *name, NimRef *value)
{
    return nim_hash_put (NIM_MODULE(self)->locals, name, value);
}

nim_bool_t
nim_module_add_method_str (
    NimRef *self,
    const char *name,
    NimNativeMethodFunc impl
)
{
    NimRef *nameref;
    NimRef *method;
    
    nameref = nim_str_new (name, strlen(name));
    if (nameref == NULL) {
        return NIM_FALSE;
    }

    method = nim_method_new_native (self, impl);
    if (method == NULL) {
        return NIM_FALSE;
    }

    return nim_hash_put (NIM_MODULE(self)->locals, nameref, method);
}

nim_bool_t
nim_module_add_local_str (
    NimRef *self,
    const char *name,
    NimRef *value
)
{
    NimRef *nameref;

    nameref = nim_str_new (name, strlen (name));
    if (nameref == NULL) {
        return NIM_FALSE;
    }

    return nim_module_add_local (self, nameref, value);
}

