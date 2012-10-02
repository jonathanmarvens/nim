#ifndef _CHIMP_OBJECT_H_INCLUDED_
#define _CHIMP_OBJECT_H_INCLUDED_

#include <chimp/core.h>
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
    ChimpCmpResult (*cmp)(ChimpRef *, ChimpRef *);
    ChimpRef *(*str)(ChimpGC *, ChimpRef *);
    ChimpRef *(*call)(ChimpRef *, ChimpRef *);
    ChimpRef *(*getattr)(ChimpRef *, ChimpRef *);
    struct _ChimpLWHash *methods;
} ChimpClass;

typedef struct _ChimpObject {
    ChimpAny     base;
} ChimpObject;

typedef struct _ChimpStr {
    ChimpAny  base;
    char     *data;
    size_t    size;
} ChimpStr;

typedef struct _ChimpArray {
    ChimpAny   base;
    ChimpRef **items;
    size_t     size;
} ChimpArray;

typedef struct _ChimpStackFrame {
    ChimpAny   base;
} ChimpStackFrame;

typedef enum _ChimpMethodType {
    CHIMP_METHOD_NATIVE,
    CHIMP_METHOD_BYTECODE
} ChimpMethodType;

typedef ChimpRef *(*ChimpNativeMethodFunc)(ChimpRef *, ChimpRef *);

typedef struct _ChimpMethod {
    ChimpAny         base;
    ChimpRef        *self; /* NULL for unbound/function */
    ChimpMethodType  type;
    union {
        struct {
            ChimpNativeMethodFunc func;
        } native;
        struct {
            /* TODO */
        } bytecode;
    };
} ChimpMethod;

typedef union _ChimpValue {
    ChimpAny        any;
    ChimpClass      klass;
    ChimpObject     object;
    ChimpStr        str;
    ChimpArray      array;
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

#define CHIMP_CHECK_CAST(struc, ref, type) ((struc *) chimp_gc_ref_check_cast ((ref), (type)))

#define CHIMP_ANY(ref)    CHIMP_CHECK_CAST(ChimpAny, (ref), CHIMP_VALUE_TYPE_ANY)
#define CHIMP_CLASS(ref)  CHIMP_CHECK_CAST(ChimpClass, (ref), CHIMP_VALUE_TYPE_CLASS)
#define CHIMP_OBJECT(ref) CHIMP_CHECK_CAST(ChimpObject, (ref), CHIMP_VALUE_TYPE_OBJECT)
#define CHIMP_STR(ref)    CHIMP_CHECK_CAST(ChimpStr, (ref), CHIMP_VALUE_TYPE_STR)
#define CHIMP_ARRAY(ref)  CHIMP_CHECK_CAST(ChimpArray, (ref), CHIMP_VALUE_TYPE_ARRAY)
#define CHIMP_METHOD(ref) CHIMP_CHECK_CAST(ChimpMethod, (ref), CHIMP_VALUE_TYPE_METHOD)
#define CHIMP_STACK_FRAME(ref) \
    CHIMP_CHECK_CAST(ChimpStackFrame, (ref), CHIMP_VALUE_TYPE_STACK_FRAME)

#define CHIMP_ANY_CLASS(ref) (CHIMP_ANY(ref)->klass)
#define CHIMP_ANY_TYPE(ref) (CHIMP_ANY(ref)->type)

#define CHIMP_EXTERN_CLASS(name) extern struct _ChimpRef *chimp_ ## name ## _class

CHIMP_EXTERN_CLASS(object);
CHIMP_EXTERN_CLASS(class);
CHIMP_EXTERN_CLASS(str);
CHIMP_EXTERN_CLASS(bool);
CHIMP_EXTERN_CLASS(array);
CHIMP_EXTERN_CLASS(method);
CHIMP_EXTERN_CLASS(stack_frame);
CHIMP_EXTERN_CLASS(nil);

extern struct _ChimpRef *chimp_nil;
extern struct _ChimpRef *chimp_true;
extern struct _ChimpRef *chimp_false;

#ifdef __cplusplus
};
#endif

#endif

