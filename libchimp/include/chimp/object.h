#ifndef _CHIMP_OBJECT_H_INCLUDED_
#define _CHIMP_OBJECT_H_INCLUDED_

#include <chimp/core.h>
#include <chimp/gc.h>
#include <chimp/class.h>
#include <chimp/str.h>
#include <chimp/array.h>
#include <chimp/stackframe.h>
#include <chimp/method.h>
#include <chimp/hash.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpObject {
    ChimpAny     base;
} ChimpObject;

typedef union _ChimpValue {
    ChimpAny        any;
    ChimpClass      klass;
    ChimpObject     object;
    ChimpStr        str;
    ChimpArray      array;
    ChimpHash       hash;
    ChimpMethod     method;
    ChimpStackFrame stack_frame;
} ChimpValue;

ChimpRef *
chimp_object_new (ChimpGC *gc, ChimpRef *klass);

ChimpCmpResult
chimp_object_cmp (ChimpRef *a, ChimpRef *b);

ChimpRef *
chimp_object_str (ChimpGC *gc, ChimpRef *self);

ChimpRef *
chimp_object_call (ChimpRef *target, ChimpRef *args);

ChimpRef *
chimp_object_getattr (ChimpRef *self, ChimpRef *name);

#define CHIMP_OBJECT(ref) CHIMP_CHECK_CAST(ChimpObject, (ref), CHIMP_VALUE_TYPE_OBJECT)

CHIMP_EXTERN_CLASS(object);
CHIMP_EXTERN_CLASS(bool);
CHIMP_EXTERN_CLASS(nil);

extern struct _ChimpRef *chimp_nil;
extern struct _ChimpRef *chimp_true;
extern struct _ChimpRef *chimp_false;

#define CHIMP_BOOL_REF(b) ((b) ? chimp_true : chimp_false)

#ifdef __cplusplus
};
#endif

#endif

