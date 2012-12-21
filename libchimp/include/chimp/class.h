#ifndef _CHIMP_CLASS_H_INCLUDED_
#define _CHIMP_CLASS_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/any.h>
#include <chimp/method.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _ChimpCmpResult {
    CHIMP_CMP_ERROR    = -3,
    CHIMP_CMP_NOT_IMPL = -2,
    CHIMP_CMP_LT       = -1,
    CHIMP_CMP_EQ       = 0,
    CHIMP_CMP_GT       = 1
} ChimpCmpResult;

typedef struct _ChimpClass {
    ChimpAny  base;
    ChimpRef *name;
    ChimpRef *super;
    ChimpValueType inst_type;
    ChimpCmpResult (*cmp)(ChimpRef *, ChimpRef *);
    ChimpRef *(*init)(ChimpRef *, ChimpRef *);
    void (*dtor)(ChimpRef *);
    ChimpRef *(*str)(ChimpRef *);
    ChimpRef *(*add)(ChimpRef *, ChimpRef *);
    ChimpRef *(*sub)(ChimpRef *, ChimpRef *);
    ChimpRef *(*mul)(ChimpRef *, ChimpRef *);
    ChimpRef *(*div)(ChimpRef *, ChimpRef *);
    ChimpRef *(*call)(ChimpRef *, ChimpRef *);
    ChimpRef *(*getattr)(ChimpRef *, ChimpRef *);
    ChimpRef *(*getitem)(ChimpRef *, ChimpRef *);
    struct _ChimpLWHash *methods;
} ChimpClass;

ChimpRef *
chimp_class_new (ChimpRef *name, ChimpRef *super);

ChimpRef *
chimp_class_new_instance (ChimpRef *klass, ...);

chimp_bool_t
chimp_class_add_method (ChimpRef *klass, ChimpRef *name, ChimpRef *method);

chimp_bool_t
chimp_class_add_native_method (ChimpRef *klass, const char *name, ChimpNativeMethodFunc func);

chimp_bool_t
_chimp_bootstrap_L3 (void);

#define CHIMP_CLASS(ref)  CHIMP_CHECK_CAST(ChimpClass, (ref), CHIMP_VALUE_TYPE_CLASS)

#define CHIMP_CLASS_NAME(ref) (CHIMP_CLASS(ref)->name)
#define CHIMP_CLASS_SUPER(ref) (CHIMP_CLASS(ref)->super)

CHIMP_EXTERN_CLASS(class);

#ifdef __cplusplus
};
#endif

#endif

