#ifndef _NIM_MODULE_MGR_H_INCLUDED_
#define _NIM_MODULE_MGR_H_INCLUDED_

#include <nim/any.h>

#ifdef __cplusplus
extern "C" {
#endif

nim_bool_t
nim_module_mgr_init (void);

void
nim_module_mgr_shutdown (void);

NimRef *
nim_module_mgr_load (NimRef *name);

NimRef *
nim_module_mgr_compile (NimRef *name, NimRef *filename);

#ifdef __cplusplus
};
#endif

#endif

