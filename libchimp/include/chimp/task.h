#ifndef _CHIMP_TASK_H_INCLUDED_
#define _CHIMP_TASK_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpTask ChimpTask;

ChimpTask *
chimp_task_new (ChimpRef *callable);

ChimpTask *
chimp_task_new_main (void);

void
chimp_task_delete (ChimpTask *task);

ChimpTask *
chimp_task_current (void);

ChimpGC *
chimp_task_get_gc (ChimpTask *task);

#define CHIMP_CURRENT_TASK chimp_task_current ()
#define CHIMP_CURRENT_GC chimp_task_get_gc (chimp_task_current())

#ifdef __cplusplus
};
#endif

#endif

