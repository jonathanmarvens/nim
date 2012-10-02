#ifndef _CHIMP_STACKFRAME_H_INCLUDED_
#define _CHIMP_STACKFRAME_H_INCLUDED_

#include <chimp/gc.h>

#ifdef __cplusplus
extern "C" {
#endif

ChimpRef *
chimp_stack_frame_new (ChimpGC *gc);

chimp_bool_t
chimp_stack_frame_class_bootstrap (ChimpGC *gc);

#ifdef __cplusplus
};
#endif

#endif

