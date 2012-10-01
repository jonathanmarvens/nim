#include <chimp.h>

int
main (int argc, char **argv)
{
    ChimpRef *ref;
    ChimpGC *gc = chimp_gc_new ();

    ref = chimp_gc_new_object (gc);
    CHIMP_ANY(ref)->type = CHIMP_VALUE_TYPE_OBJECT;
    CHIMP_ANY(ref)->klass = NULL;
    chimp_gc_make_root (gc, ref);

    chimp_gc_collect (gc);

    chimp_gc_delete (gc);
    return 0;
}

