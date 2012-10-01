#include <chimp.h>

int
main (int argc, char **argv)
{
    ChimpRef *ref;
    ChimpGC *gc;

    if (!chimp_core_startup ()) {
        return 1;
    }

    gc = chimp_gc_new ();
    if (gc == NULL) {
        chimp_core_shutdown ();
        return 1;
    }

    /* let's see if a string can survive a collection :) */

    ref = chimp_str_new (gc, "foo", 3);
    chimp_gc_make_root (gc, ref);

    chimp_gc_collect (gc);

    printf ("%s\n", CHIMP_STR_DATA(ref));

    chimp_gc_delete (gc);
    chimp_core_shutdown ();
    return 0;
}

