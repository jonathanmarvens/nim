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

#include "nim/any.h"
#include "nim/object.h"
#include "nim/array.h"
#include "nim/str.h"
#include "nim/vm.h"

// NimTestRunner class we only use from within the NimUnit module
NimRef *nim_test_runner_class = NULL;

typedef struct _NimTestRunner {
    NimAny base;
    NimRef *name;
    int64_t  passed;
    int64_t  failed;
} NimTestRunner;

#define NIM_TEST(ref) NIM_CHECK_CAST(NimTestRunner, (ref), nim_test_runner_class)
#define NIM_TEST_NAME(ref) (NIM_TEST(ref)->name)

static NimRef *
_nim_test_runner_init (NimRef *self, NimRef *args)
{
    NIM_TEST(self)->name = NIM_STR_NEW("");
    NIM_TEST(self)->passed = 0;
    NIM_TEST(self)->failed = 0;
    return self;
}
static void
_nim_test_runner_output_stats (NimRef *self, FILE* dest)
{
    fprintf(dest, "\nPassed: %ju, Failed: %ju\n",
        (intmax_t) NIM_TEST(self)->passed,
        (intmax_t) NIM_TEST(self)->failed);
}

static void
_nim_test_runner_dtor (NimRef *self)
{
    _nim_test_runner_output_stats(self, stdout);
}

static void
_nim_failed_test(NimRef *self)
{
    fprintf (stderr, "\nTest failed: %s",
        NIM_STR_DATA(NIM_TEST_NAME(self)));

    NIM_TEST(self)->failed++;
    _nim_test_runner_output_stats(self, stderr);
}

static NimRef *
_nim_test_runner_equals (NimRef *self, NimRef *args)
{
    NimCmpResult r;
    NimRef *left = NIM_ARRAY_ITEM(args, 0);
    NimRef *right = NIM_ARRAY_ITEM(args, 1);

    r = nim_object_cmp (left, right);

    if (r == NIM_CMP_ERROR) {
        return NULL;
    }
    if (r != NIM_CMP_EQ) {
        _nim_failed_test(self);
        NIM_BUG("assertion failed: expected %s to be equal to %s\n",
            NIM_STR_DATA(nim_object_str(left)),
            NIM_STR_DATA(nim_object_str(right)));
        exit(1);
    }
    else {
        return nim_nil;
    }
}

static NimRef *
_nim_test_runner_not_equals (NimRef *self, NimRef *args)
{
    NimCmpResult r;
    NimRef *left = NIM_ARRAY_ITEM(args, 0);
    NimRef *right = NIM_ARRAY_ITEM(args, 1);

    r = nim_object_cmp (left, right);

    if (r == NIM_CMP_ERROR) {
        return NULL;
    }
    if (r == NIM_CMP_EQ) {
        _nim_failed_test(self);
        NIM_BUG("assertion failed: expected %s to be not equal to %s\n",
            NIM_STR_DATA(nim_object_str(left)),
            NIM_STR_DATA(nim_object_str(right)));
        exit(1);
    }
    else {
        return nim_nil;
    }
}

static NimRef *
_nim_test_runner_fail (NimRef *self, NimRef *args)
{
    _nim_failed_test(self);
    NIM_BUG("%s\n", NIM_STR_DATA(NIM_ARRAY_ITEM(args, 0)));
    return NULL;
}

static NimRef *
_nim_test_runner_is_nil (NimRef *self, NimRef *args)
{
    if (NIM_ARRAY_ITEM(args, 0) != nim_nil) {
        NIM_BUG("assertion failed: expected nil, but got %s",
                NIM_STR_DATA(nim_object_str(NIM_ARRAY_ITEM(args, 0))));
        return NULL;
    }
    return nim_nil;
}

static NimRef *
_nim_test_runner_is_not_nil (NimRef *self, NimRef *args)
{
    if (NIM_ARRAY_ITEM(args, 0) == nim_nil) {
        NIM_BUG("assertion failed: expected non-nil value, but got nil");
        return NULL;
    }
    return nim_nil;
}

nim_bool_t
nim_test_runner_class_bootstrap (void)
{
    nim_test_runner_class =
        nim_class_new (NIM_STR_NEW ("test"), NULL, sizeof(NimTestRunner));
    if (nim_test_runner_class == NULL) {
        return NIM_FALSE;
    }
    NIM_CLASS(nim_test_runner_class)->init = _nim_test_runner_init;
    NIM_CLASS(nim_test_runner_class)->dtor = _nim_test_runner_dtor;
    nim_gc_make_root (NULL, nim_test_runner_class);
    nim_class_add_native_method (nim_test_runner_class, "equals",     _nim_test_runner_equals);
    nim_class_add_native_method (nim_test_runner_class, "not_equals", _nim_test_runner_not_equals);
    nim_class_add_native_method (nim_test_runner_class, "is_nil",     _nim_test_runner_is_nil);
    nim_class_add_native_method (nim_test_runner_class, "is_not_nil", _nim_test_runner_is_not_nil);
    nim_class_add_native_method (nim_test_runner_class, "fail",       _nim_test_runner_fail);
    return NIM_TRUE;
}

NimRef *
nim_test_runner_new (void)
{
    return nim_class_new_instance (nim_test_runner_class, NULL);
}

// NimUnit module implementation
static NimRef *test_runner = NULL;

static NimRef *
_nim_unit_test(NimRef *self, NimRef *args)
{
    fprintf (stdout, ".");

    // TODO: Size/argument validation on the incoming args
    NimRef *name = nim_object_str (NIM_ARRAY_ITEM(args, 0));
    NimRef *fn = NIM_ARRAY_ITEM(args, 1);

    if (test_runner == NULL) {
        test_runner = nim_test_runner_new();
        /* XXX hack to avoid premature collection by the GC */
        nim_gc_make_root (NULL, test_runner);
    }
    NIM_TEST(test_runner)->name = name;

    NimRef *fn_args = nim_array_new();
    nim_array_push(fn_args, test_runner);

    nim_object_call (fn, fn_args);
    NIM_TEST(test_runner)->passed++;

    return nim_nil;
}

NimRef *
nim_init_unit_module (void)
{
    NimRef *mod;

    nim_test_runner_class_bootstrap();

    mod = nim_module_new_str ("nimunit", NULL);
    if (mod == NULL) {
        return NULL;
    }

    if (!nim_module_add_method_str (mod, "test", _nim_unit_test)) {
        return NULL;
    }

    return mod;
}

