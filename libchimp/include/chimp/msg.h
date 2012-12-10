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
    struct _ChimpMsgInternal *impl;
} ChimpMsg;

typedef struct _ChimpMsgCell {
    enum {
        CHIMP_MSG_CELL_INT,
        CHIMP_MSG_CELL_STR
    } type;
    union {
        int64_t  int_;
        struct {
            char    *data;
            size_t   size;
        } str;
    };
} ChimpMsgCell;

typedef struct _ChimpMsgInternal {
    size_t                    size;
    ChimpMsgCell             *cells;
    size_t                    num_cells;
    struct _ChimpMsgInternal *next;
} ChimpMsgInternal;

chimp_bool_t
chimp_msg_class_bootstrap (void);

ChimpRef *
chimp_msg_new (ChimpRef *data);

ChimpRef *
chimp_msg_unpack (ChimpMsgInternal *internal);

#define CHIMP_MSG(ref) \
    CHIMP_CHECK_CAST(ChimpMsg, (ref), CHIMP_VALUE_TYPE_MSG)

CHIMP_EXTERN_CLASS(msg);

#ifdef __cplusplus
};
#endif

#endif

