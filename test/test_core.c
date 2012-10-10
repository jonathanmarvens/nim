START_TEST (test_startup_shutdown)
{
    int stack;

    fail_unless (chimp_core_startup ((void *)&stack), "expected startup to succeed");
    chimp_core_shutdown ();
}
END_TEST

