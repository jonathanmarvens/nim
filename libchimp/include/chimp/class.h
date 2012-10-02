#ifndef _CHIMP_CLASS_H_INCLUDED_
#define _CHIMP_CLASS_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/object.h>

#ifdef __cplusplus
extern "C" {
#endif

ChimpRef *
chimp_class_new (ChimpGC *gc, ChimpRef *name, ChimpRef *super);

chimp_bool_t
chimp_class_add_method (ChimpGC *gc, ChimpRef *klass, ChimpRef *name, ChimpRef *method);

chimp_bool_t
chimp_class_add_native_method (ChimpGC *gc, ChimpRef *klass, const char *name, ChimpNativeMethodFunc func);

#define CHIMP_CLASS_NAME(ref) (CHIMP_CLASS(ref)->name)
#define CHIMP_CLASS_SUPER(ref) (CHIMP_CLASS(ref)->super)

#ifdef __cplusplus
};
#endif

#endif

