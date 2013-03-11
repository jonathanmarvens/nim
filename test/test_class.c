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

#define NIM_TEST_INSTANCE_CHECK(inst, klass) \
    fail_unless ((inst) != NULL, "calling %s constructor failed", NIM_STR_DATA(NIM_CLASS_NAME(klass))); \
    fail_unless (NIM_ANY_CLASS(inst) == (klass), "object is not an instance of %s", NIM_STR_DATA(NIM_CLASS_NAME(klass)));

void
test_class_setup (void)
{
    fail_unless (nim_core_startup (NULL, stack_base), "core_startup failed");
}

void
test_class_teardown (void)
{
    nim_core_shutdown ();
}

START_TEST(calling_object_class_should_create_an_object_instance)
{
    NimRef *instance;

    instance = nim_object_call (nim_object_class, nim_array_new ());
    NIM_TEST_INSTANCE_CHECK(instance, nim_object_class);
}
END_TEST

START_TEST(calling_class_class_should_create_a_new_class)
{
    NimRef *instance;
    NimRef *args = nim_array_new ();
    nim_array_push (args, NIM_STR_NEW ("foo"));
    nim_array_push (args, nim_nil);

    instance = nim_object_call (nim_class_class, args);
    NIM_TEST_INSTANCE_CHECK(instance, nim_class_class);
}
END_TEST

START_TEST(calling_str_class_should_create_a_str_instance)
{
    NimRef *instance;

    instance = nim_object_call (nim_str_class, nim_array_new ());
    NIM_TEST_INSTANCE_CHECK(instance, nim_str_class);
}
END_TEST

START_TEST(calling_hash_class_should_create_a_hash_instance)
{
    NimRef *instance;
    
    instance = nim_object_call (nim_hash_class, nim_array_new ());
    NIM_TEST_INSTANCE_CHECK(instance, nim_hash_class);
}
END_TEST

START_TEST(instances_of_new_classes_should_use_super_class)
{
    const char *instance_class_name;
    NimRef *klass = nim_class_new (NIM_STR_NEW ("testing"), NULL, 128);
    NimRef *instance = nim_object_call (klass, nim_array_new ());
    NIM_TEST_INSTANCE_CHECK(instance, klass);
    instance_class_name =
        NIM_STR_DATA(NIM_CLASS_NAME(NIM_ANY_CLASS(instance)));
    fail_unless (strcmp ("testing", instance_class_name) == 0,
            "expected name of instance class to be 'testing'");
    fail_unless (nim_object_instance_of (instance, nim_object_class),
                 "expected subclass instance to be 'instanceof' super class");
}
END_TEST

START_TEST(nil_value_should_have_nil_class)
{
    fail_unless (NIM_ANY_CLASS(nim_nil) == nim_nil_class,
                "expected nil value to have nil class");
}
END_TEST

