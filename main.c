#include <chimp.h>

#if 0
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
    return CHIMP_STR_NEW(NULL, "Hello, World");
}

#endif

int
main (int argc, char **argv)
{
#if 0
    ChimpTask *task;
    ChimpRef *ref;
    ChimpRef *args;
#endif
    size_t i;
    ChimpRef *a, *b, *c;

    if (!chimp_core_startup ()) {
        return 1;
    }

    /* 25 ChimpValues used by chimp_core_startup: push it up furthr so we're
     * just before a GC at 57
     */
    for (i = 0; i < 29; i++) {
        CHIMP_STR_NEW(NULL, "foo");
    }

    /* allocations 55 & 56 work fine but notice they're ripe for collection
     * since we don't walk the C stack.
     */
    a = CHIMP_STR_NEW(NULL, "a");
    b = CHIMP_STR_NEW(NULL, "b");

    /* GC occurs. a & b now point to freed memory. */
    c = CHIMP_STR_NEW(NULL, "c");

    /* a & b may contain garbage here */
    printf ("a=%s, b=%s, c=%s\n", CHIMP_STR_DATA(a), CHIMP_STR_DATA(b), CHIMP_STR_DATA(c));

#if 0
    CHIMP_PUSH_STACK_FRAME();

    /* let's see if a string can survive a collection :) */
    ref = chimp_str_new (NULL, "foo", 3);
    chimp_gc_make_root (NULL, ref);
    chimp_gc_collect (NULL);
    printf ("%s\n", CHIMP_STR_DATA(ref));

    /* alright, how about an array? */
    ref = chimp_array_new (NULL);
    chimp_gc_make_root (NULL, ref);
    chimp_array_push (ref, CHIMP_STR_NEW(NULL, "bar"));
    chimp_gc_collect (NULL);
    printf ("%s\n", CHIMP_STR_DATA(CHIMP_ARRAY_ITEMS(ref)[0]));

    /* conversion from generic object to string? */
    ref = chimp_object_new (NULL, NULL);
    ref = chimp_object_str (NULL, ref);
    printf ("%s\n", CHIMP_STR_DATA(ref));

    ref = chimp_array_new (NULL);
    ref = chimp_object_str (NULL, ref);
    printf ("%s\n", CHIMP_STR_DATA(ref));

    ref = chimp_method_new_native (NULL, some_native_method);
    ref = chimp_object_call (ref, chimp_array_new (NULL));
    printf ("%s\n", CHIMP_STR_DATA(ref));

    task = chimp_task_new (chimp_method_new_native (NULL, some_other_method));
    /* XXX hrm. */
    chimp_task_delete (task);

    ref = chimp_class_new (NULL, CHIMP_STR_NEW(NULL, "foo"), chimp_object_class);
    chimp_gc_make_root (NULL, ref);
    {
        ChimpRef *name = CHIMP_STR_NEW(NULL, "prn");
        ChimpRef *method = chimp_method_new_native (NULL, print_self);

        chimp_class_add_method (NULL, ref, name, method);

        /* call prn as a class method for kicks ... */
        chimp_object_call (chimp_object_getattr (ref, name), NULL);
    }

    ref = chimp_object_new (NULL, ref);
    ref = chimp_object_getattr (ref, CHIMP_STR_NEW(NULL, "prn"));
    /* ref is now bound to our instance of 'foo' */
    chimp_object_call (ref, NULL);

    /* does the GC fuck with anything ? */
    chimp_gc_make_root (NULL, ref);
    chimp_gc_collect (NULL);
    chimp_object_call (ref, NULL);
    chimp_gc_collect (NULL);

    /* does nil format itself well? */
    ref = chimp_object_str (NULL, chimp_nil);
    printf ("%s\n", CHIMP_STR_DATA(ref));

    /* push a value onto an array indirectly, then check the result */
    ref = chimp_array_new (NULL);
    args = chimp_array_new (NULL);
    chimp_array_push (args, CHIMP_STR_NEW(NULL, "hello"));
    chimp_object_call (chimp_object_getattr (ref, CHIMP_STR_NEW(NULL, "push")), args);
    ref = chimp_object_str (NULL, chimp_array_get (ref, 0));
    printf ("%s\n", CHIMP_STR_DATA(ref));

    /* booleans to string */
    ref = chimp_object_str (NULL, chimp_true);
    printf ("%s\n", CHIMP_STR_DATA(ref));
    ref = chimp_object_str (NULL, chimp_false);
    printf ("%s\n", CHIMP_STR_DATA(ref));
    printf ("%s\n", CHIMP_STR_DATA(CHIMP_CLASS_NAME(chimp_bool_class)));

    CHIMP_POP_STACK_FRAME ();
#endif

    chimp_core_shutdown ();
    return 0;
}

