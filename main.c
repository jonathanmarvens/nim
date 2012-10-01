#include <chimp.h>

static ChimpRef *
print_self (ChimpRef *self, ChimpRef *args)
{
    printf ("print_self: %p\n", self);
    return NULL;
}

static ChimpRef *
some_other_method (ChimpRef *self, ChimpRef *args)
{
    printf ("from a separate thread\n");
    /* XXX */
    return NULL;
}

static ChimpRef *
some_native_method (ChimpRef *self, ChimpRef *args)
{
    return CHIMP_STR_NEW(CHIMP_CURRENT_GC, "Hello, World");
}

int
main (int argc, char **argv)
{
    ChimpTask *main_task;
    ChimpTask *task;
    ChimpRef *ref;
    ChimpGC *gc;

    if (!chimp_core_startup ()) {
        return 1;
    }

    main_task = chimp_task_new_main ();
    gc = CHIMP_CURRENT_GC;

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

    ref = chimp_method_new_native (gc, some_native_method);
    ref = chimp_object_call (ref, chimp_array_new (gc));
    printf ("%s\n", CHIMP_STR_DATA(ref));

    task = chimp_task_new (chimp_method_new_native (gc, some_other_method));
    /* XXX hrm. */
    chimp_task_delete (task);

    ref = chimp_class_new (gc, CHIMP_STR_NEW(gc, "foo"), chimp_object_class);
    chimp_gc_make_root (gc, ref);
    chimp_class_add_method (gc, ref, CHIMP_STR_NEW(gc, "prn"), chimp_method_new_native (gc, print_self));

    /* call prn as a class method for kicks ... */
    chimp_object_call (chimp_object_getattr (ref, CHIMP_STR_NEW(gc, "prn")), NULL);

    ref = chimp_object_new (gc, ref);
    ref = chimp_object_getattr (ref, CHIMP_STR_NEW(gc, "prn"));
    /* ref is now bound to our instance of 'foo' */
    chimp_object_call (ref, NULL);

    /* does the GC fuck with anything ? */
    chimp_gc_make_root (gc, ref);
    chimp_gc_collect (gc);
    chimp_object_call (ref, NULL);

    chimp_task_delete (main_task);
    chimp_core_shutdown ();
    return 0;
}

