#include <stdio.h>
#include <unistd.h>

#include "chimp/any.h"
#include "chimp/object.h"
#include "chimp/array.h"
#include "chimp/str.h"

static ChimpRef *
_chimp_os_getenv (ChimpRef *self, ChimpRef *args)
{
    const char *value = getenv(CHIMP_STR_DATA(CHIMP_ARRAY_ITEM(args, 0)));
    if (value == NULL) {
        return chimp_nil;
    }
    else {
        return chimp_str_new (value, strlen(value));
    }
}

static ChimpRef *
_chimp_os_sleep (ChimpRef *ref, ChimpRef *args)
{
    ChimpRef *duration = CHIMP_ARRAY_ITEM(args, 0);
    if (duration == NULL) {
        sleep (0);
    }
    else {
        sleep ((time_t)CHIMP_INT(duration)->value);
    }
    return chimp_nil;
}

ChimpRef *
chimp_init_os_module (void)
{
    ChimpRef *os;

    os = chimp_module_new_str ("os", NULL);
    if (os == NULL) {
        return NULL;
    }

    if (!chimp_module_add_method_str (os, "getenv", _chimp_os_getenv)) {
        return NULL;
    }

    if (!chimp_module_add_method_str (os, "sleep", _chimp_os_sleep)) {
        return NULL;
    }

    return os;
}

