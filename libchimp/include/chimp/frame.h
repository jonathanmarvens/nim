#ifndef _CHIMP_FRAME_H_INCLUDED_
#define _CHIMP_FRAME_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpFrame {
    ChimpAny   base;
    ChimpRef  *method;
    ChimpRef  *locals;
} ChimpFrame;

chimp_bool_t
chimp_frame_class_bootstrap (void);

ChimpRef *
chimp_frame_new (ChimpRef *method);

#define CHIMP_FRAME(ref) \
    CHIMP_CHECK_CAST(ChimpFrame, (ref), CHIMP_VALUE_TYPE_FRAME)

#define CHIMP_FRAME_CODE(ref) \
    CHIMP_METHOD(CHIMP_FRAME(ref)->method)->bytecode.code

CHIMP_EXTERN_CLASS(frame);

#ifdef __cplusplus
};
#endif

#endif

