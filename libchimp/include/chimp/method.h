#ifndef _CHIMP_METHOD_H_INCLUDED_
#define _CHIMP_METHOD_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/object.h>

#ifdef __cplusplus
extern "C" {
#endif

ChimpRef *
chimp_method_new_native (ChimpGC *gc, ChimpNativeMethodFunc func);

/* XXX no method type check here */
#define CHIMP_NATIVE_METHOD(ref) (&(CHIMP_METHOD(ref)->native))

#ifdef __cplusplus
};
#endif

#endif

