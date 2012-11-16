#include <stdio.h>
#include "chimp/any.h"
#include "chimp/object.h"
#include "chimp/array.h"
#include "chimp/str.h"

static ChimpRef *
_getenv (ChimpRef *self, ChimpRef *args)
{
    const char *value = getenv(CHIMP_STR_DATA(CHIMP_ARRAY_ITEM(args, 0)));
    if (value == NULL) {
        return chimp_nil;
    }
    else {
        return chimp_str_new (value, strlen(value));
    }
}

ChimpRef *
chimp_init_os_module (void)
{
    ChimpRef *os;
    ChimpRef *exports;
    ChimpRef *getenv_method;

    getenv_method = chimp_method_new_native (NULL, _getenv);
    exports = chimp_hash_new ();
    chimp_hash_put_str (exports, "getenv", getenv_method);
    
    os = chimp_module_new_str ("os", exports);
    if (os == NULL) {
        return NULL;
    }

    /* XXX stupid hack because I can't think far enough ahead of myself */
    CHIMP_METHOD(getenv_method)->module = os;

    return os;
}

