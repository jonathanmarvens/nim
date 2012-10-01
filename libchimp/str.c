#include "chimp/str.h"

#define CHIMP_STR_INIT(ref) \
    CHIMP_ANY(ref)->type = CHIMP_VALUE_TYPE_STR; \
    CHIMP_ANY(ref)->klass = chimp_str_class;

ChimpRef *
chimp_str_new (ChimpGC *gc, const char *data, size_t size)
{
    char *copy;
    ChimpRef *ref = chimp_gc_new_object (gc);
    if (ref == NULL) {
        return NULL;
    }
    CHIMP_STR_INIT(ref);
    copy = CHIMP_MALLOC(char, size + 1);
    memcpy (copy, data, size);
    copy[size] = '\0';
    CHIMP_STR(ref)->data = copy;
    CHIMP_STR(ref)->size = size;
    if (CHIMP_STR_DATA(ref) == NULL) {
        return NULL;
    }
    return ref;
}

