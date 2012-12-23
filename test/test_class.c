/*****************************************************************************
 *                                                                           *
 * Copyright 2012 Thomas Lee                                                 *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License");           *
 * you may not use this file except in compliance with the License.          *
 * You may obtain a copy of the License at                                   *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 * Unless required by applicable law or agreed to in writing, software       *
 * distributed under the License is distributed on an "AS IS" BASIS,         *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 * See the License for the specific language governing permissions and       *
 * limitations under the License.                                            *
 *                                                                           *
 *****************************************************************************/

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

    instance = chimp_object_call (chimp_object_class, chimp_array_new ());
    CHIMP_TEST_INSTANCE_CHECK(instance, chimp_object_class, CHIMP_VALUE_TYPE_OBJECT);
}
END_TEST

START_TEST(calling_class_class_should_create_a_new_class)
{
    ChimpRef *instance;
    ChimpRef *args = chimp_array_new ();
    chimp_array_push (args, CHIMP_STR_NEW ("foo"));
    chimp_array_push (args, chimp_nil);

    instance = chimp_object_call (chimp_class_class, args);
    CHIMP_TEST_INSTANCE_CHECK(instance, chimp_class_class, CHIMP_VALUE_TYPE_CLASS);
}
END_TEST

START_TEST(calling_str_class_should_create_a_str_instance)
{
    ChimpRef *instance;

    instance = chimp_object_call (chimp_str_class, chimp_array_new ());
    CHIMP_TEST_INSTANCE_CHECK(instance, chimp_str_class, CHIMP_VALUE_TYPE_STR);
}
END_TEST

START_TEST(calling_hash_class_should_create_a_hash_instance)
{
    ChimpRef *instance;
    
    instance = chimp_object_call (chimp_hash_class, chimp_array_new ());
    CHIMP_TEST_INSTANCE_CHECK(instance, chimp_hash_class, CHIMP_VALUE_TYPE_HASH);
}
END_TEST

START_TEST(instances_of_new_classes_should_use_super_class)
{
    const char *instance_class_name;
    ChimpRef *klass = chimp_class_new (CHIMP_STR_NEW ("testing"), NULL);
    ChimpRef *instance = chimp_object_call (klass, chimp_array_new ());
    CHIMP_TEST_INSTANCE_CHECK(instance, klass, CHIMP_VALUE_TYPE_OBJECT);
    instance_class_name =
        CHIMP_STR_DATA(CHIMP_CLASS_NAME(CHIMP_ANY_CLASS(instance)));
    fail_unless (strcmp ("testing", instance_class_name) == 0,
            "expected name of instance class to be 'testing'");
    fail_unless (chimp_object_instance_of (instance, chimp_object_class),
                 "expected subclass instance to be 'instanceof' super class");
}
END_TEST

START_TEST(nil_value_should_have_nil_class)
{
    fail_unless (CHIMP_ANY_CLASS(chimp_nil) == chimp_nil_class,
                "expected nil value to have nil class");
}
END_TEST

