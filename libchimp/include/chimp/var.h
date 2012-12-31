#ifndef _CHIMP_VAR_H_INCLUDED_
#define _CHIMP_VAR_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpVar {
    ChimpAny  base;
    ChimpRef *value;
} ChimpVar;

chimp_bool_t
chimp_var_class_bootstrap (void);

ChimpRef *
chimp_var_new (void);

#define CHIMP_VAR(ref)  CHIMP_CHECK_CAST(ChimpVar, (ref), chimp_var_class)
#define CHIMP_VAR_VALUE(ref) CHIMP_VAR(ref)->value

CHIMP_EXTERN_CLASS(var);

#ifdef __cplusplus
};
#endif

#endif

