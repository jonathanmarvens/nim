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

#include "chimp/any.h"
#include "chimp/object.h"
#include "chimp/array.h"
#include "chimp/str.h"
#include "chimp/vm.h"

// ChimpTestRunner class we only use from within the ChimpUnit module
ChimpRef *chimp_test_runner_class = NULL;

typedef struct _ChimpTestRunner {
    ChimpAny base;
    ChimpRef *name;
    int64_t  passed;
    int64_t  failed;
} ChimpTestRunner;

#define CHIMP_TEST(ref) CHIMP_CHECK_CAST(ChimpTestRunner, (ref), chimp_test_runner_class)
#define CHIMP_TEST_NAME(ref) (CHIMP_TEST(ref)->name)

static ChimpRef *
_chimp_test_runner_init (ChimpRef *self, ChimpRef *args)
{
    CHIMP_TEST(self)->name = CHIMP_STR_NEW("");
    CHIMP_TEST(self)->passed = 0;
    CHIMP_TEST(self)->failed = 0;
    return self;
}
static void
_chimp_test_runner_output_stats (ChimpRef *self, FILE* dest)
{
    fprintf(dest, "\nPassed: %lld, Failed: %lld\n",
        CHIMP_TEST(self)->passed,
        CHIMP_TEST(self)->failed);
}

static void
_chimp_test_runner_dtor (ChimpRef *self)
{
    _chimp_test_runner_output_stats(self, stdout);
}

static void
_chimp_failed_test(ChimpRef *self)
{
    fprintf (stderr, "\nTest failed: %s",
        CHIMP_STR_DATA(CHIMP_TEST_NAME(self)));

    CHIMP_TEST(self)->failed++;
    _chimp_test_runner_output_stats(self, stderr);
}

static ChimpRef *
_chimp_test_runner_equals (ChimpRef *self, ChimpRef *args)
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
_chimp_test_runner_not_equals (ChimpRef *self, ChimpRef *args)
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
_chimp_test_runner_fail (ChimpRef *self, ChimpRef *args)
{
    _chimp_failed_test(self);
    CHIMP_BUG("%s\n", CHIMP_STR_DATA(CHIMP_ARRAY_ITEM(args, 0)));
    return NULL;
}

static ChimpRef *
_chimp_test_runner_is_nil (ChimpRef *self, ChimpRef *args)
{
    if (CHIMP_ARRAY_ITEM(args, 0) != chimp_nil) {
        CHIMP_BUG("assertion failed: expected nil, but got %s",
                CHIMP_STR_DATA(chimp_object_str(CHIMP_ARRAY_ITEM(args, 0))));
        return NULL;
    }
    return chimp_nil;
}

static ChimpRef *
_chimp_test_runner_is_not_nil (ChimpRef *self, ChimpRef *args)
{
    if (CHIMP_ARRAY_ITEM(args, 0) == chimp_nil) {
        CHIMP_BUG("assertion failed: expected non-nil value, but got nil");
        return NULL;
    }
    return chimp_nil;
}

chimp_bool_t
chimp_test_runner_class_bootstrap (void)
{
    chimp_test_runner_class =
        chimp_class_new (CHIMP_STR_NEW ("test"), NULL, sizeof(ChimpTestRunner));
    if (chimp_test_runner_class == NULL) {
        return CHIMP_FALSE;
    }
    CHIMP_CLASS(chimp_test_runner_class)->init = _chimp_test_runner_init;
    CHIMP_CLASS(chimp_test_runner_class)->dtor = _chimp_test_runner_dtor;
    chimp_gc_make_root (NULL, chimp_test_runner_class);
    chimp_class_add_native_method (chimp_test_runner_class, "equals",     _chimp_test_runner_equals);
    chimp_class_add_native_method (chimp_test_runner_class, "not_equals", _chimp_test_runner_not_equals);
    chimp_class_add_native_method (chimp_test_runner_class, "is_nil",     _chimp_test_runner_is_nil);
    chimp_class_add_native_method (chimp_test_runner_class, "is_not_nil", _chimp_test_runner_is_not_nil);
    chimp_class_add_native_method (chimp_test_runner_class, "fail",       _chimp_test_runner_fail);
    return CHIMP_TRUE;
}

ChimpRef *
chimp_test_runner_new (void)
{
    return chimp_class_new_instance (chimp_test_runner_class, NULL);
}

// ChimpUnit module implementation
static ChimpRef *test_runner = NULL;

static ChimpRef *
_chimp_unit_test(ChimpRef *self, ChimpRef *args)
{
    fprintf (stdout, ".");

    // TODO: Size/argument validation on the incoming args
    ChimpRef *name = chimp_object_str (CHIMP_ARRAY_ITEM(args, 0));
    ChimpRef *fn = CHIMP_ARRAY_ITEM(args, 1);

    if (test_runner == NULL) {
        test_runner = chimp_test_runner_new();
        /* XXX hack to avoid premature collection by the GC */
        chimp_gc_make_root (NULL, test_runner);
    }
    CHIMP_TEST(test_runner)->name = name;

    ChimpRef *fn_args = chimp_array_new();
    chimp_array_push(fn_args, test_runner);

    chimp_object_call (fn, fn_args);
    CHIMP_TEST(test_runner)->passed++;

    return chimp_nil;
}

ChimpRef *
chimp_init_unit_module (void)
{
    ChimpRef *mod;

    chimp_test_runner_class_bootstrap();

    mod = chimp_module_new_str ("chimpunit", NULL);
    if (mod == NULL) {
        return NULL;
    }

    if (!chimp_module_add_method_str (mod, "test", _chimp_unit_test)) {
        return NULL;
    }

    return mod;
}

