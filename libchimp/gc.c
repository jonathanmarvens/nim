#include "chimp/gc.h"
#include "chimp/object.h"

ChimpGC *
chimp_gc_new (void)
{
    return NULL;
}

void
chimp_gc_delete (ChimpGC *gc)
{
}

ChimpRef *
chimp_gc_new_object (ChimpGC *gc)
{
    return NULL;
}

chimp_bool_t
chimp_gc_make_root (ChimpGC *gc, ChimpRef *ref)
{
    return CHIMP_FALSE;
}

ChimpValue *
chimp_gc_ref_check_cast (ChimpRef *ref, ChimpValueType type)
{
    return NULL;
}

chimp_bool_t
chimp_gc_collect (ChimpGC *gc)
{
    return CHIMP_FALSE;
}

