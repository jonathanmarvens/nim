#ifndef _CHIMP_MODULES_H_INCLUDED_
#define _CHIMP_MODULES_H_INCLUDED_

#include <chimp/any.h>
#include <chimp/gc.h>

#ifdef __cplusplus
extern "C" {
#endif

ChimpRef *
chimp_init_io_module (void);

ChimpRef *
chimp_init_assert_module (void);

ChimpRef *
chimp_init_os_module (void);

ChimpRef *
chimp_init_gc_module (void);

#ifdef __cplusplus
};
#endif

#endif

