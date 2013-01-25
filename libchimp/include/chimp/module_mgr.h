#ifndef _CHIMP_MODULE_MGR_H_INCLUDED_
#define _CHIMP_MODULE_MGR_H_INCLUDED_

#include <chimp/any.h>

#ifdef __cplusplus
extern "C" {
#endif

chimp_bool_t
chimp_module_mgr_init (void);

void
chimp_module_mgr_shutdown (void);

ChimpRef *
chimp_module_mgr_load (ChimpRef *name);

ChimpRef *
chimp_module_mgr_compile (ChimpRef *name, ChimpRef *filename);

#ifdef __cplusplus
};
#endif

#endif

