#ifndef _CHIMP_INT_H_INCLUDED_
#define _CHIMP_INT_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpInt {
    ChimpAny base;
    int64_t  value;
} ChimpInt;

chimp_bool_t
chimp_int_class_bootstrap (ChimpGC *gc);

ChimpRef *
chimp_int_new (ChimpGC *gc, int64_t value);

#define CHIMP_INT(ref) CHIMP_CHECK_CAST(ChimpInt, (ref), CHIMP_VALUE_TYPE_INT)

#define CHIMP_INT_VALUE(ref) CHIMP_INT(ref)->value

CHIMP_EXTERN_CLASS(int);

#ifdef __cplusplus
};
#endif

#endif

