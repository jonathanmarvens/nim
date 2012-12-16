#ifndef _CHIMP_MSG_H_INCLUDED_
#define _CHIMP_MSG_H_INCLUDED_

#include <pthread.h>

#include <chimp/any.h>
#include <chimp/gc.h>
#include <chimp/task.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpMsgCell {
    enum {
        CHIMP_MSG_CELL_INT,
        CHIMP_MSG_CELL_STR,
        CHIMP_MSG_CELL_ARRAY,
        CHIMP_MSG_CELL_TASK
    } type;
    union {
        int64_t  int_;
        struct {
            char    *data;
            size_t   size;
        } str;
        struct {
            struct _ChimpMsgCell *items;
            size_t                size;
        } array;
        ChimpTaskInternal    *task;
    };
} ChimpMsgCell;

typedef struct _ChimpMsgInternal {
    size_t                    size;
    ChimpMsgCell             *cell;
    struct _ChimpMsgInternal *next;
} ChimpMsgInternal;

ChimpMsgInternal *
chimp_msg_pack (ChimpRef *array);

ChimpRef *
chimp_msg_unpack (ChimpMsgInternal *internal);

#ifdef __cplusplus
};
#endif

#endif

