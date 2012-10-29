#include <stdio.h>
#include "chimp/any.h"
#include "chimp/object.h"
#include "chimp/array.h"
#include "chimp/str.h"

static ChimpRef *
_print (ChimpRef *self, ChimpRef *args)
{
    size_t i;

    for (i = 0; i < CHIMP_ARRAY_SIZE(args); i++) {
        ChimpRef *str = chimp_object_str (CHIMP_ARRAY_ITEM (args, i));
        printf ("%s\n", CHIMP_STR_DATA(str));
    }

    return chimp_nil;
}

static ChimpRef *
_input (ChimpRef *self, ChimpRef *args)
{
    char buf[1024];
    size_t len;

    if (fgets (buf, sizeof(buf), stdin) == NULL) {
        return chimp_nil;
    }

    len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') {
        buf[--len] = '\0';
    }
    return chimp_str_new (buf, len);
}

ChimpRef *
chimp_init_io_module (void)
{
    ChimpRef *io;
    ChimpRef *exports;
    ChimpRef *print_method;
    ChimpRef *input_method;

    print_method = chimp_method_new_native (NULL, _print);
    input_method = chimp_method_new_native (NULL, _input);
    exports = chimp_hash_new (NULL);
    chimp_hash_put_str (exports, "print", print_method);
    chimp_hash_put_str (exports, "readline", input_method);
    
    io = chimp_module_new_str ("io", exports);
    if (io == NULL) {
        return NULL;
    }

    /* XXX stupid hack because I can't think far enough ahead of myself */
    CHIMP_METHOD(print_method)->module = io;
    CHIMP_METHOD(input_method)->module = io;

    return io;
}

