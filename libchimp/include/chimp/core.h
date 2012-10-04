#ifndef _CHIMP_CORE_H_INCLUDED_
#define _CHIMP_CORE_H_INCLUDED_

#include <stdio.h>
#include <stdarg.h>
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
chimp_core_startup (void *stack_start);

void
chimp_core_shutdown (void);

void
chimp_bug (const char *filename, int lineno, const char *format, ...);

#define CHIMP_MALLOC(type, size) ((type *) malloc (size))
#define CHIMP_REALLOC(type, ptr, size) ((type *) realloc ((ptr), (size)))
#define CHIMP_FREE(ptr) \
    do { \
        if ((ptr) != NULL) { free (ptr); } \
    } while (0)

#define CHIMP_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            chimp_bug (__FILE__, __LINE__, "assertion failed: \"%s\"", #cond); \
        } \
    } while (0)

#ifdef __cplusplus
};
#endif

#endif

