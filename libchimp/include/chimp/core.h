#ifndef _CHIMP_CORE_H_INCLUDED_
#define _CHIMP_CORE_H_INCLUDED_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int chimp_bool_t;

#define CHIMP_TRUE ((chimp_bool_t) 1)
#define CHIMP_FALSE ((chimp_bool_t) 0)

chimp_bool_t
chimp_core_startup (void);

void
chimp_core_shutdown (void);

#ifdef __cplusplus
};
#endif

#endif

