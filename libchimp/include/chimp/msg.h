#ifndef _CHIMP_MSG_H_INCLUDED_
#define _CHIMP_MSG_H_INCLUDED_

#include <pthread.h>

#include <chimp/any.h>
#include <chimp/gc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpMsg {
    ChimpAny any;
    ChimpRef *data;
} ChimpMsg;

chimp_bool_t
chimp_msg_class_bootstrap (void);

ChimpRef *
chimp_msg_new (ChimpRef *data);

ChimpRef *
chimp_msg_dup (ChimpRef *self);

#define CHIMP_MSG(ref) \
    CHIMP_CHECK_CAST(ChimpMsg, (ref), CHIMP_VALUE_TYPE_MSG)

CHIMP_EXTERN_CLASS(msg);

#ifdef __cplusplus
};
#endif

#endif

