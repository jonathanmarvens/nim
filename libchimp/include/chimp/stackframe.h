#ifndef _CHIMP_STACKFRAME_H_INCLUDED_
#define _CHIMP_STACKFRAME_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpStackFrame {
    ChimpAny   base;
} ChimpStackFrame;

ChimpRef *
chimp_stack_frame_new (ChimpGC *gc);

chimp_bool_t
chimp_stack_frame_class_bootstrap (ChimpGC *gc);

#define CHIMP_STACK_FRAME(ref) \
    CHIMP_CHECK_CAST(ChimpStackFrame, (ref), CHIMP_VALUE_TYPE_STACK_FRAME)

CHIMP_EXTERN_CLASS(stack_frame);

#ifdef __cplusplus
};
#endif

#endif

