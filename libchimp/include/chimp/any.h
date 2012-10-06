#ifndef _CHIMP_ANY_H_INCLUDED_
#define _CHIMP_ANY_H_INCLUDED_

#include <chimp/gc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _ChimpValueType {
    CHIMP_VALUE_TYPE_ANY,
    CHIMP_VALUE_TYPE_OBJECT,
    CHIMP_VALUE_TYPE_CLASS,
    CHIMP_VALUE_TYPE_STR,
    CHIMP_VALUE_TYPE_ARRAY,
    CHIMP_VALUE_TYPE_METHOD,
    CHIMP_VALUE_TYPE_STACK_FRAME,
} ChimpValueType;

typedef struct _ChimpAny {
    ChimpValueType  type;
    ChimpRef       *klass;
} ChimpAny;

#define CHIMP_CHECK_CAST(struc, ref, type) ((struc *) chimp_gc_ref_check_cast ((ref), (type)))

#define CHIMP_ANY(ref)    CHIMP_CHECK_CAST(ChimpAny, (ref), CHIMP_VALUE_TYPE_ANY)

#define CHIMP_ANY_CLASS(ref) (CHIMP_ANY(ref)->klass)
#define CHIMP_ANY_TYPE(ref) (CHIMP_ANY(ref)->type)

#define CHIMP_EXTERN_CLASS(name) extern struct _ChimpRef *chimp_ ## name ## _class

#ifdef __cplusplus
};
#endif

#endif

