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

#ifndef _NIM_VM_H_INCLUDED_
#define _NIM_VM_H_INCLUDED_

#include <nim/gc.h>
#include <nim/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NimVM NimVM;

NimVM *
nim_vm_new (void);

void
nim_vm_delete (NimVM *vm);

/*
NimRef *
nim_vm_eval (NimVM *vm, NimRef *code, NimRef *locals);
*/

NimRef *
nim_vm_invoke (NimVM *vm, NimRef *method, NimRef *args);

void
nim_vm_panic (NimVM *vm, NimRef *value);

#ifdef __cplusplus
};
#endif

#endif

