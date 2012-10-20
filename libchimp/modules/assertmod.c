#include "chimp/any.h"
#include "chimp/object.h"
#include "chimp/array.h"
#include "chimp/str.h"
#include "chimp/vm.h"

static ChimpRef *
_chimp_assert_equal (ChimpRef *self, ChimpRef *args)
{
    ChimpCmpResult r;
    ChimpRef *left = CHIMP_ARRAY_ITEM(args, 0);
    ChimpRef *right = CHIMP_ARRAY_ITEM(args, 1);

    r = chimp_object_cmp (left, right);

    if (r == CHIMP_CMP_ERROR) {
        return NULL;
    }
    if (r != CHIMP_CMP_EQ) {
        chimp_vm_panic (
            NULL,
            chimp_str_new_format (
                NULL,
                "expected %s to be not equal to %s",
                CHIMP_STR_DATA(left),
                CHIMP_STR_DATA(right))
        );
        return NULL;
    }
    else {
        return chimp_nil;
    }
}

static ChimpRef *
_chimp_assert_not_equal (ChimpRef *self, ChimpRef *args)
{
    ChimpCmpResult r;
    ChimpRef *left = CHIMP_ARRAY_ITEM(args, 0);
    ChimpRef *right = CHIMP_ARRAY_ITEM(args, 1);

    r = chimp_object_cmp (left, right);

    if (r == CHIMP_CMP_ERROR) {
        return NULL;
    }
    if (r == CHIMP_CMP_EQ) {
        chimp_vm_panic (
            NULL,
            chimp_str_new_format (
                NULL,
                "expected %s to be not equal to %s",
                CHIMP_STR_DATA(left),
                CHIMP_STR_DATA(right))
        );
        return NULL;
    }
    else {
        return chimp_nil;
    }
}

ChimpRef *
chimp_init_assert_module (void)
{
    ChimpRef *mod;
    ChimpRef *exports;
    ChimpRef *eq_method;
    ChimpRef *neq_method;

    eq_method = chimp_method_new_native (NULL, NULL, _chimp_assert_equal);
    neq_method = chimp_method_new_native (NULL, NULL, _chimp_assert_not_equal);
    exports = chimp_hash_new (NULL);
    chimp_hash_put_str (exports, "equal", eq_method);
    chimp_hash_put_str (exports, "not_equal", neq_method);
    
    mod = chimp_module_new_str ("assert", exports);
    if (mod == NULL) {
        return NULL;
    }

    /* XXX stupid hack because I can't think far enough ahead of myself */
    CHIMP_METHOD(eq_method)->module = mod;
    CHIMP_METHOD(neq_method)->module = mod;

    return mod;
}

