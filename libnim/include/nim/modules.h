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

#ifndef _NIM_MODULES_H_INCLUDED_
#define _NIM_MODULES_H_INCLUDED_

#include <nim/any.h>
#include <nim/gc.h>

#ifdef __cplusplus
extern "C" {
#endif

NimRef *
nim_init_io_module (void);

NimRef *
nim_init_assert_module (void);

NimRef *
nim_init_unit_module (void);

NimRef *
nim_init_os_module (void);

NimRef *
nim_init_gc_module (void);

NimRef *
nim_init_net_module (void);

NimRef *
nim_init_http_module (void);

#ifdef __cplusplus
};
#endif

#endif

