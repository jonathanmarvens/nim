#ifndef _CHIMP_ARRAY_H_INCLUDED_
#define _CHIMP_ARRAY_H_INCLUDED_

#include <chimp/gc.h>

#ifdef __cplusplus
extern "C" {
#endif

ChimpRef *
chimp_array_new (ChimpGC *gc);

chimp_bool_t
chimp_array_push (ChimpRef *self, ChimpRef *value);

ChimpRef *
chimp_array_pop (ChimpRef *self);

#define CHIMP_ARRAY_ITEMS(ref) (CHIMP_ARRAY(ref)->items)
#define CHIMP_ARRAY_SIZE(ref) (CHIMP_ARRAY(ref)->size)

#ifdef __cplusplus
};
#endif

#endif

