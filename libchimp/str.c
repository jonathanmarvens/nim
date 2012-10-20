#include "chimp/object.h"
#include "chimp/str.h"

#define CHIMP_STR_INIT(ref) \
    CHIMP_ANY(ref)->type = CHIMP_VALUE_TYPE_STR; \
    CHIMP_ANY(ref)->klass = chimp_str_class;

ChimpRef *
chimp_str_new (ChimpGC *gc, const char *data, size_t size)
{
    char *copy;
    copy = CHIMP_MALLOC(char, size + 1);
    if (copy == NULL) return NULL;
    memcpy (copy, data, size);
    copy[size] = '\0';
    return chimp_str_new_take (gc, copy, size);
}

ChimpRef *
chimp_str_new_take (ChimpGC *gc, char *data, size_t size)
{
    ChimpRef *ref = chimp_gc_new_object (gc);
    if (ref == NULL) {
        return NULL;
    }
    CHIMP_STR_INIT(ref);
    CHIMP_STR(ref)->data = data;
    CHIMP_STR(ref)->size = size;
    return ref;
}

ChimpRef *
chimp_str_new_format (ChimpGC *gc, const char *fmt, ...)
{
    va_list args;
    int size;
    char *buf;

    va_start(args, fmt);
    size = vsnprintf (NULL, 0, fmt, args);
    va_end(args);

    if (size >= 0) {
        buf = CHIMP_MALLOC(char, size + 1);
        if (buf == NULL) {
            return NULL;
        }

        va_start(args, fmt);
        vsnprintf (buf, size + 1, fmt, args);
        va_end(args);
        return chimp_str_new_take (gc, buf, size);
    }
    else {
        return NULL;
    }
}

ChimpRef *
chimp_str_new_concat (ChimpGC *gc, ...)
{
    va_list args;
    const char *arg;
    size_t size = 0;
    char *ptr, *temp;

    va_start (args, gc);
    while ((arg = va_arg(args, const char *)) != NULL) {
        size += strlen (arg);
    }
    va_end (args);

    temp = ptr = CHIMP_MALLOC(char, size + 1);
    if (temp == NULL) return NULL;

    va_start (args, gc);
    while ((arg = va_arg(args, const char *)) != NULL) {
        size_t len = strlen (arg);
        memcpy (temp, arg, len);
        temp += len;
    }
    va_end (args);
    *temp = '\0';

    return chimp_str_new_take (gc, ptr, size);
}

chimp_bool_t
chimp_str_append (ChimpRef *self, ChimpRef *append_me)
{
    ChimpRef *append_str = chimp_object_str (NULL, append_me);
    /* TODO error checking */
    char *data = CHIMP_REALLOC (char, CHIMP_STR(self)->data, CHIMP_STR_SIZE(self) + CHIMP_STR_SIZE(append_str) + 1);
    if (data == NULL) {
        return CHIMP_FALSE;
    }
    CHIMP_STR(self)->data = data;
    memcpy (CHIMP_STR_DATA(self) + CHIMP_STR_SIZE(self), CHIMP_STR_DATA(append_str), CHIMP_STR_SIZE(append_str));
    CHIMP_STR(self)->size += CHIMP_STR_SIZE(append_str);
    CHIMP_STR(self)->data[CHIMP_STR(self)->size] = '\0';
    return CHIMP_TRUE;
}

chimp_bool_t
chimp_str_append_str (ChimpRef *self, const char *s)
{
    return chimp_str_append (self, chimp_str_new (NULL, s, strlen(s)));
}

