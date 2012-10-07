#ifndef _CHIMP_VM_H_INCLUDED_
#define _CHIMP_VM_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpVM ChimpVM;

ChimpVM *
chimp_vm_new (void);

void
chimp_vm_delete (ChimpVM *vm);

ChimpRef *
chimp_vm_eval (ChimpVM *vm, ChimpRef *code);

#ifdef __cplusplus
};
#endif

#endif

