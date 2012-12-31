#include "chimp/var.h"
#include "chimp/str.h"
#include "chimp/class.h"
#include "chimp/object.h"

ChimpRef *chimp_var_class = NULL;

static void
_chimp_var_mark (ChimpGC *gc, ChimpRef *self)
{
    chimp_gc_mark_ref (gc, CHIMP_VAR(self)->value);
}

chimp_bool_t
chimp_var_class_bootstrap (void)
{
    chimp_var_class =
        chimp_class_new (CHIMP_STR_NEW("var"), chimp_object_class);
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

