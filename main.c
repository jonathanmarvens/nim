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

    /* alright, how about an array? */
    ref = chimp_array_new (gc);
    chimp_gc_make_root (gc, ref);
    chimp_array_push (ref, CHIMP_STR_NEW(gc, "bar"));
    chimp_gc_collect (gc);
    printf ("%s\n", CHIMP_STR_DATA(CHIMP_ARRAY_ITEMS(ref)[0]));

    /* conversion from generic object to string? */
    ref = chimp_object_new (gc, NULL);
    ref = chimp_object_str (gc, ref);
    printf ("%s\n", CHIMP_STR_DATA(ref));

    ref = chimp_array_new (gc);
    ref = chimp_object_str (gc, ref);
    printf ("%s\n", CHIMP_STR_DATA(ref));

    chimp_gc_delete (gc);
    chimp_core_shutdown ();
    return 0;
}

