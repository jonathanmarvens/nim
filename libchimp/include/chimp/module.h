#ifndef _CHIMP_MODULE_H_INCLUDED_
#define _CHIMP_MODULE_H_INCLUDED_

#include <chimp/any.h>
#include <chimp/method.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpModule {
    ChimpAny base;
    ChimpRef *name;
    ChimpRef *locals;
} ChimpModule;

chimp_bool_t
chimp_module_class_bootstrap (void);

ChimpRef *
chimp_module_new (ChimpRef *name, ChimpRef *locals);

ChimpRef *
chimp_module_new_str (const char *name, ChimpRef *locals);

chimp_bool_t
chimp_module_add_local (
    ChimpRef *self,
    ChimpRef *name,
    ChimpRef *value
);

chimp_bool_t
chimp_module_add_method_str (
    ChimpRef *self,
    const char *name,
    ChimpNativeMethodFunc impl
);

#define CHIMP_MODULE(ref) CHIMP_CHECK_CAST(ChimpModule, (ref), CHIMP_VALUE_TYPE_MODULE)
#define CHIMP_MODULE_NAME(ref) (CHIMP_METHOD(ref)->name)
#define CHIMP_MODULE_LOCALS(ref) (CHIMP_METHOD(ref)->locals)

CHIMP_EXTERN_CLASS(module);

#ifdef __cplusplus
};
#endif

#endif

