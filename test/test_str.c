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

void
test_str_setup (void)
{
    fail_unless (chimp_core_startup (NULL, stack_base), "core_startup failed");
}

void
test_str_teardown (void)
{
    chimp_core_shutdown ();
}

START_TEST(str_constructor_with_no_args_returns_empty_string)
{
    ChimpRef *obj = chimp_object_call (chimp_str_class, chimp_array_new ());
    fail_unless (strcmp("", CHIMP_STR_DATA(obj)) == 0, "expected empty string");
    fail_unless (CHIMP_STR_SIZE(obj) == 0, "empty strings should have a size of zero");
}
END_TEST

START_TEST(str_constructor_with_str_arg_returns_str_value)
{
    ChimpRef *obj = chimp_object_call (chimp_str_class, chimp_array_new_var (CHIMP_STR_NEW ("testing"), NULL));
    fail_unless (strcmp ("testing", CHIMP_STR_DATA(obj)) == 0,
                "expected a populated string");
}
END_TEST

START_TEST(str_constructor_with_int_arg_returns_int_value)
{
    ChimpRef *obj = chimp_object_call (chimp_str_class, chimp_array_new_var (chimp_int_new (4096), NULL));
    fail_unless (strcmp ("4096", CHIMP_STR_DATA(obj)) == 0,
                "expected the int to be converted to a string");
}
END_TEST

