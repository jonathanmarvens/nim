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

#include <stdio.h>
#include <stdlib.h>

#include "chimp/array.h"
#include "chimp/object.h"
#include "chimp/test.h"
#include "chimp/str.h"
#include "chimp/class.h"
#include "chimp/ast.h"

ChimpRef *chimp_test_class = NULL;

static ChimpRef *
_chimp_test_init (ChimpRef *self, ChimpRef *args)
{
    CHIMP_TEST(self)->name = CHIMP_STR_NEW("");
    CHIMP_TEST(self)->passed = 0;
    CHIMP_TEST(self)->failed = 0;
    return self;
}
static void
_chimp_test_output_stats (ChimpRef *self, FILE* dest)
{
    fprintf(dest, "\nPassed: %lld, Failed: %lld\n",
        CHIMP_TEST(self)->passed,
        CHIMP_TEST(self)->failed);
}

static void
_chimp_test_dtor (ChimpRef *self)
{
    _chimp_test_output_stats(self, stdout);
}

static void
_chimp_failed_test(ChimpRef *self)
{
    fprintf (stderr, "\nTest failed: %s",
        CHIMP_STR_DATA(CHIMP_TEST_NAME(self)));

    CHIMP_TEST(self)->failed++;
    _chimp_test_output_stats(self, stderr);
}

static ChimpRef *
_chimp_test_equals (ChimpRef *self, ChimpRef *args)
{
    ChimpCmpResult r;
    ChimpRef *left = CHIMP_ARRAY_ITEM(args, 0);
    ChimpRef *right = CHIMP_ARRAY_ITEM(args, 1);

    r = chimp_object_cmp (left, right);

    if (r == CHIMP_CMP_ERROR) {
        return NULL;
    }
    if (r != CHIMP_CMP_EQ) {
        _chimp_failed_test(self);
        CHIMP_BUG("assertion failed: expected %s to be equal to %s\n",
            CHIMP_STR_DATA(chimp_object_str(left)),
            CHIMP_STR_DATA(chimp_object_str(right)));
        exit(1);
    }
    else {
        return chimp_nil;
    }
}

static ChimpRef *
_chimp_test_not_equals (ChimpRef *self, ChimpRef *args)
{
    ChimpCmpResult r;
    ChimpRef *left = CHIMP_ARRAY_ITEM(args, 0);
    ChimpRef *right = CHIMP_ARRAY_ITEM(args, 1);

    r = chimp_object_cmp (left, right);

    if (r == CHIMP_CMP_ERROR) {
        return NULL;
    }
    if (r == CHIMP_CMP_EQ) {
        _chimp_failed_test(self);
        CHIMP_BUG("assertion failed: expected %s to be not equal to %s\n",
            CHIMP_STR_DATA(chimp_object_str(left)),
            CHIMP_STR_DATA(chimp_object_str(right)));
        exit(1);
    }
    else {
        return chimp_nil;
    }
}

static ChimpRef *
_chimp_test_fail (ChimpRef *self, ChimpRef *args)
{
    _chimp_failed_test(self);
    CHIMP_BUG("%s\n", CHIMP_STR_DATA(CHIMP_ARRAY_ITEM(args, 0)));
    return NULL;
}

static ChimpRef *
_chimp_test_is_nil (ChimpRef *self, ChimpRef *args)
{
    if (CHIMP_ARRAY_ITEM(args, 0) != chimp_nil) {
        CHIMP_BUG("assertion failed: expected nil, but got %s",
                CHIMP_STR_DATA(chimp_object_str(CHIMP_ARRAY_ITEM(args, 0))));
        return NULL;
    }
    return chimp_nil;
}

static ChimpRef *
_chimp_test_is_not_nil (ChimpRef *self, ChimpRef *args)
{
    if (CHIMP_ARRAY_ITEM(args, 0) == chimp_nil) {
        CHIMP_BUG("assertion failed: expected non-nil value, but got nil");
        return NULL;
    }
    return chimp_nil;
}

chimp_bool_t
chimp_test_class_bootstrap (void)
{
    chimp_test_class =
        chimp_class_new (CHIMP_STR_NEW ("test"), NULL, sizeof(ChimpTest));
    if (chimp_test_class == NULL) {
        return CHIMP_FALSE;
    }
    CHIMP_CLASS(chimp_test_class)->init = _chimp_test_init;
    CHIMP_CLASS(chimp_test_class)->dtor = _chimp_test_dtor;
    chimp_gc_make_root (NULL, chimp_test_class);
    chimp_class_add_native_method (chimp_test_class, "equals",     _chimp_test_equals);
    chimp_class_add_native_method (chimp_test_class, "not_equals", _chimp_test_not_equals);
    chimp_class_add_native_method (chimp_test_class, "is_nil",     _chimp_test_is_nil);
    chimp_class_add_native_method (chimp_test_class, "is_not_nil", _chimp_test_is_not_nil);
    chimp_class_add_native_method (chimp_test_class, "fail",       _chimp_test_fail);
    return CHIMP_TRUE;
}

ChimpRef *
chimp_test_new (void)
{
    return chimp_class_new_instance (chimp_test_class, NULL);
}
