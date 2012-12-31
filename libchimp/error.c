#include "chimp/class.h"
#include "chimp/object.h"
#include "chimp/str.h"
#include "chimp/error.h"

ChimpRef *chimp_error_class = NULL;

static ChimpRef *
_chimp_error_init (ChimpRef *self, ChimpRef *args)
{
    ChimpRef *message = NULL;
    if (!chimp_method_parse_args (args, "|o", &message)) {
        return NULL;
    }
    CHIMP_ERROR(self)->message = message;
    return self;
}

static ChimpRef *
_chimp_error_str (ChimpRef *self)
{
    ChimpRef *message = CHIMP_ERROR(self)->message;
    if (message != NULL) {
        return chimp_str_new_format ("<error '%s'>", CHIMP_STR_DATA(message));
    }
    else {
        return CHIMP_STR_NEW ("<error>");
    }
}

static void
_chimp_error_mark (ChimpGC *gc, ChimpRef *self)
{
    chimp_gc_mark_ref (gc, CHIMP_ERROR(self)->message);
    chimp_gc_mark_ref (gc, CHIMP_ERROR(self)->cause);
    chimp_gc_mark_ref (gc, CHIMP_ERROR(self)->backtrace);
}

chimp_bool_t
chimp_error_class_bootstrap (void)
{
    chimp_error_class =
        chimp_class_new (CHIMP_STR_NEW("error"), chimp_object_class);
    if (chimp_error_class == NULL) {
        return CHIMP_FALSE;
    }
    CHIMP_CLASS(chimp_error_class)->str = _chimp_error_str;
    CHIMP_CLASS(chimp_error_class)->init = _chimp_error_init;
    CHIMP_CLASS(chimp_error_class)->inst_type = CHIMP_VALUE_TYPE_ERROR;
    CHIMP_CLASS(chimp_error_class)->mark = _chimp_error_mark;
    chimp_gc_make_root (NULL, chimp_error_class);
    return CHIMP_TRUE;
}

ChimpRef *
chimp_error_new (ChimpRef *message)
{
    return chimp_class_new_instance (chimp_error_class, message, NULL);
}

ChimpRef *
chimp_error_new_with_format (const char *fmt, ...)
{
    va_list args;
    ChimpRef *message;

    va_start (args, fmt);
    message = chimp_str_new_formatv (fmt, args);
    va_end (args);
    if (message == NULL) {
        return NULL;
    }

    return chimp_error_new (message);
}

