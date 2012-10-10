#include <check.h>

#include <chimp.h>

#define CHIMP_TEST_INSTANCE_CHECK(inst, klass, gc_type) \
    fail_unless ((inst) != NULL, "calling %s constructor failed", CHIMP_STR_DATA(CHIMP_CLASS_NAME(klass))); \
    fail_unless (CHIMP_ANY_TYPE((inst)) == (gc_type), "object should be GC type #%d", (gc_type)); \
    fail_unless (CHIMP_ANY_CLASS(inst) == (klass), "object is not an instance of %s", CHIMP_STR_DATA(CHIMP_CLASS_NAME(klass)));

void
test_class_setup (void)
{
    fail_unless (chimp_core_startup (stack_base), "core_startup failed");
}

void
test_class_teardown (void)
{
    chimp_core_shutdown ();
}

START_TEST(calling_object_class_should_create_an_object_instance)
{
    ChimpRef *instance;

    instance = chimp_object_call (chimp_object_class, chimp_array_new (NULL));
    CHIMP_TEST_INSTANCE_CHECK(instance, chimp_object_class, CHIMP_VALUE_TYPE_OBJECT);
}
END_TEST

START_TEST(calling_class_class_should_create_a_new_class)
{
    ChimpRef *instance;

    instance = chimp_object_call (chimp_class_class, chimp_array_new (NULL));
    CHIMP_TEST_INSTANCE_CHECK(instance, chimp_class_class, CHIMP_VALUE_TYPE_CLASS);
}
END_TEST

START_TEST(calling_str_class_should_create_a_str_instance)
{
    ChimpRef *instance;

    instance = chimp_object_call (chimp_str_class, chimp_array_new (NULL));
    CHIMP_TEST_INSTANCE_CHECK(instance, chimp_str_class, CHIMP_VALUE_TYPE_STR);
}
END_TEST

START_TEST(calling_hash_class_should_create_a_hash_instance)
{
    ChimpRef *instance;
    
    instance = chimp_object_call (chimp_hash_class, chimp_array_new (NULL));
    CHIMP_TEST_INSTANCE_CHECK(instance, chimp_hash_class, CHIMP_VALUE_TYPE_HASH);
}
END_TEST

START_TEST(instances_of_new_classes_should_use_super_class)
{
    const char *instance_class_name;
    ChimpRef *klass = chimp_class_new (NULL, CHIMP_STR_NEW (NULL, "testing"), NULL);
    ChimpRef *instance = chimp_object_call (klass, chimp_array_new (NULL));
    CHIMP_TEST_INSTANCE_CHECK(instance, klass, CHIMP_VALUE_TYPE_OBJECT);
    instance_class_name =
        CHIMP_STR_DATA(CHIMP_CLASS_NAME(CHIMP_ANY_CLASS(instance)));
    fail_unless (strcmp ("testing", instance_class_name) == 0,
            "expected name of instance class to be 'testing'");
    fail_unless (chimp_object_instance_of (instance, chimp_object_class),
                 "expected subclass instance to be 'instanceof' super class");
}
END_TEST


