#ifndef _CHIMP_STR_H_INCLUDED_
#define _CHIMP_STR_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/object.h>

#ifdef __cplusplus
extern "C" {
#endif

ChimpRef *
chimp_str_new (ChimpGC *gc, const char *data, size_t size);

#define CHIMP_STR_DATA(ref) (CHIMP_STR(ref)->data)
#define CHIMP_STR_SIZE(ref) (CHIMP_STR(ref)->size)

#define CHIMP_STR_NEW(gc, data) chimp_str_new ((gc), (data), sizeof(data)-1)

#ifdef __cplusplus
};
#endif

#endif

