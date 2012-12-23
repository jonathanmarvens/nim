/*****************************************************************************
 *                                                                           *
 * Copyright 2012 Thomas Lee                                                 *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License");           *
 * you may not use this file except in compliance with the License.          *
 * You may obtain a copy of the License at                                   *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 * Unless required by applicable law or agreed to in writing, software       *
 * distributed under the License is distributed on an "AS IS" BASIS,         *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 * See the License for the specific language governing permissions and       *
 * limitations under the License.                                            *
 *                                                                           *
 *****************************************************************************/

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

/*
ChimpRef *
chimp_vm_eval (ChimpVM *vm, ChimpRef *code, ChimpRef *locals);
*/

ChimpRef *
chimp_vm_invoke (ChimpVM *vm, ChimpRef *method, ChimpRef *args);

void
chimp_vm_panic (ChimpVM *vm, ChimpRef *value);

#ifdef __cplusplus
};
#endif

#endif

