#include "chimp/module.h"
#include "chimp/array.h"
#include "chimp/object.h"

ChimpRef *chimp_module_class = NULL;

static ChimpRef *
chimp_module_init (ChimpRef *self, ChimpRef *args)
{
    CHIMP_MODULE(self)->name = CHIMP_ARRAY_ITEM(args, 0);
    if (CHIMP_ARRAY_SIZE(args) == 2) {
        CHIMP_MODULE(self)->locals = CHIMP_ARRAY_ITEM(args, 1);
    }
    else {
        ChimpRef *temp = chimp_hash_new (NULL);
        CHIMP_MODULE(self)->locals = temp;
    }
    return chimp_nil;
}

static ChimpRef *
chimp_module_str (ChimpGC *gc, ChimpRef *self)
{
    ChimpRef *name = CHIMP_STR_NEW (NULL, "<module \"");
    chimp_str_append (name, CHIMP_MODULE(self)->name);
    chimp_str_append_str (name, "\">");
    return name;
}

chimp_bool_t
chimp_module_class_bootstrap (void)
{
    chimp_module_class = chimp_class_new (NULL, CHIMP_STR_NEW (NULL, "module"), NULL);
    if (chimp_module_class == NULL) {
        return CHIMP_FALSE;
    }
    chimp_gc_make_root (NULL, chimp_module_class);
    CHIMP_CLASS(chimp_module_class)->inst_type = CHIMP_VALUE_TYPE_MODULE;
    CHIMP_CLASS(chimp_module_class)->str = chimp_module_str;
    if (!chimp_class_add_native_method (NULL, chimp_module_class, "init", chimp_module_init)) {
        return CHIMP_FALSE;
    }
    return CHIMP_TRUE;
}

ChimpRef *
chimp_module_new (ChimpRef *name, ChimpRef *locals)
{
    if (locals == NULL) {
        locals = chimp_hash_new (NULL);
        if (locals == NULL) {
            return NULL;
        }
    }
    return chimp_class_new_instance (chimp_module_class, name, locals, NULL);
}

chimp_bool_t
chimp_module_add_local (ChimpRef *self, ChimpRef *name, ChimpRef *value)
{
    return chimp_hash_put (CHIMP_MODULE(self)->locals, name, value);
}

