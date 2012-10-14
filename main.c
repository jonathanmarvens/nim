#include <chimp.h>
#include <stdio.h>
#include <inttypes.h>

#if 0
static ChimpRef *
print_self (ChimpRef *self, ChimpRef *args)
{
    printf ("print_self: %p\n", self);
    return chimp_nil;
}

static ChimpRef *
some_other_method (ChimpRef *self, ChimpRef *args)
{
    printf ("from a separate thread\n");
    return chimp_nil;
}

static ChimpRef *
some_native_method (ChimpRef *self, ChimpRef *args)
{
    return CHIMP_STR_NEW(NULL, "Hello, World");
}

#endif

static int
real_main (int argc, char **argv);

int
main (int argc, char **argv)
{
    int rc;

    if (!chimp_core_startup ((void *)&rc)) {
        fprintf (stderr, "error: unable to initialize chimp core\n");
        return 1;
    }

    rc = real_main(argc, argv);

    chimp_core_shutdown ();
    return rc;
}

static ChimpRef *
parse_args (int argc, char **argv)
{
    int i;
    ChimpRef *args;
    ChimpRef *argv_ = chimp_array_new (NULL);
    for (i = 1; i < argc; i++) {
        ChimpRef *arg = chimp_str_new (NULL, argv[i], strlen(argv[i]));
        if (arg == NULL) {
            return NULL;
        }
        if (!chimp_array_push (argv_, arg)) {
            return NULL;
        }
    }
    args = chimp_array_new (NULL);
    if (args == NULL) {
        return NULL;
    }
    if (!chimp_array_push (args, argv_)) {
        return NULL;
    }
    return args;
}

static int
real_main (int argc, char **argv)
{
    ChimpRef *result;
    ChimpRef *module;
    ChimpRef *main_method;
    ChimpRef *args;
    if (argc < 2) {
        fprintf (stderr, "usage: %s <file>\n", argv[0]);
        return 1;
    }

    module = CHIMP_COMPILE_MODULE_FROM_FILE ("main", argv[1]);
    if (module == NULL) {
        fprintf (stderr, "error: failed to compile %s\n", argv[0]);
        return 1;
    }

    main_method = chimp_object_getattr_str (module, "main");
    if (main_method == NULL) {
        fprintf (stderr, "error: could not find main method in this module\n");
        return 1;
    }

    args = parse_args (argc, argv);
    result = chimp_vm_invoke (NULL, main_method, args);
    if (result == NULL) {
        fprintf (stderr, "error: chimp_vm_eval () returned NULL\n");
        return 1;
    }

    return 0;
}

#if 0
static int
real_main (int argc, char **argv)
{
    ChimpTask *task;
    ChimpRef *ref;
    ChimpRef *args;
    size_t i;
    ChimpRef *a, *b, *c;

    /* 25 ChimpValues used by chimp_core_startup: push it up furthr so we're
     * just before a GC at 57
     */
    for (i = 0; i < 29; i++) {
        CHIMP_STR_NEW(NULL, "foo");
    }

    printf ("a @ %p\n", &a);
    printf ("b @ %p\n", &b);
    printf ("c @ %p\n", &c);
    /* allocations 55 & 56 work fine but notice they're ripe for collection
     * since we don't walk the C stack.
     */
    a = CHIMP_STR_NEW(NULL, "a");
    b = CHIMP_STR_NEW(NULL, "b");

    /* GC occurs. a & b now point to freed memory. */
    printf ("GC should occur at this point ...\n");
    c = CHIMP_STR_NEW(NULL, "c");
    printf ("\n");

    /* a & b may contain garbage here if the GC is broken */
    printf ("a & b may contain garbage here if the GC is broken\n");
    printf ("a=%s, b=%s, c=%s\n", CHIMP_STR_DATA(a), CHIMP_STR_DATA(b), CHIMP_STR_DATA(c));
    printf ("\n");

    /* this should free nothing since we just GCed */
    printf ("The next collection should free nothing...\n");
    chimp_gc_collect (NULL);
    printf ("\n");

    /* free a ref on the local stack */
    b = NULL;

    /* this should free one value */
    printf ("This should free exactly one value\n");
    chimp_gc_collect (NULL);
    printf ("\n");

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

    /* empty array to string? */
    ref = chimp_array_new (NULL);
    ref = chimp_object_str (NULL, ref);
    printf ("%s\n", CHIMP_STR_DATA(ref));

    /* populated array? */
    ref = chimp_array_new (NULL);
    chimp_array_push (ref, CHIMP_STR_NEW(NULL, "testing :)"));
    chimp_array_push (ref, chimp_object_new (NULL, NULL));
    chimp_array_push (ref, chimp_nil);
    ref = chimp_object_str (NULL, ref);
    printf ("%s\n", CHIMP_STR_DATA(ref));

    /* int? */
    ref = chimp_int_new (NULL, 1024);
    ref = chimp_object_str (NULL, ref);
    printf ("int: %s\n", CHIMP_STR_DATA(ref));

    /* empty hash to string? */
    ref = chimp_hash_new (NULL);
    ref = chimp_object_str (NULL, ref);
    printf ("%s\n", CHIMP_STR_DATA(ref));

    /* populated hash to string? */
    ref = chimp_hash_new (NULL);
    CHIMP_HASH_PUT(ref, "a", CHIMP_STR_NEW(NULL, "Hello"));
    CHIMP_HASH_PUT(ref, "foo", chimp_array_new(NULL));
    ref = chimp_object_str (NULL, ref);
    printf ("%s\n", CHIMP_STR_DATA(ref));

    ref = chimp_method_new_native (NULL, some_native_method);
    ref = chimp_object_call (ref, chimp_array_new (NULL));
    printf ("%s\n", CHIMP_STR_DATA(ref));

    task = chimp_task_new (chimp_method_new_native (NULL, some_other_method));
    chimp_task_wait (task);
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

    return 0;
}
#endif
