#ifndef _CHIMP_STR_H_INCLUDED_
#define _CHIMP_STR_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpStr {
    ChimpAny  base;
    char     *data;
    size_t    size;
} ChimpStr;

ChimpRef *
chimp_str_new (ChimpGC *gc, const char *data, size_t size);

ChimpRef *
chimp_str_new_take (ChimpGC *gc, char *data, size_t size);

ChimpRef *
chimp_str_new_format (ChimpGC *gc, const char *fmt, ...);

ChimpRef *
chimp_str_new_concat (ChimpGC *gc, ...);

/* XXX these mutate string state ... revisit me */

chimp_bool_t
chimp_str_append (ChimpRef *str, ChimpRef *append_me);

chimp_bool_t
chimp_str_append_str (ChimpRef *str, const char *s);

#define CHIMP_STR(ref)    CHIMP_CHECK_CAST(ChimpStr, (ref), CHIMP_VALUE_TYPE_STR)

#define CHIMP_STR_DATA(ref) (CHIMP_STR(ref)->data)
#define CHIMP_STR_SIZE(ref) (CHIMP_STR(ref)->size)

#define CHIMP_STR_NEW(gc, data) chimp_str_new ((gc), (data), sizeof(data)-1)

CHIMP_EXTERN_CLASS(str);

#ifdef __cplusplus
};
#endif

#endif

