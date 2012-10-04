#ifndef _CHIMP_ARRAY_H_INCLUDED_
#define _CHIMP_ARRAY_H_INCLUDED_

#include <chimp/gc.h>

#ifdef __cplusplus
extern "C" {
#endif

chimp_bool_t
chimp_array_class_bootstrap (ChimpGC *gc);

ChimpRef *
chimp_array_new (ChimpGC *gc);

chimp_bool_t
chimp_array_push (ChimpRef *self, ChimpRef *value);

ChimpRef *
chimp_array_pop (ChimpRef *self);

ChimpRef *
chimp_array_get (ChimpRef *self, int32_t pos);

ChimpRef *
chimp_array_first (ChimpRef *self);

ChimpRef *
chimp_array_last (ChimpRef *self);

#define CHIMP_ARRAY_ITEMS(ref) (CHIMP_ARRAY(ref)->items)
#define CHIMP_ARRAY_SIZE(ref) (CHIMP_ARRAY(ref)->size)

#define CHIMP_ARRAY_FIRST(ref) chimp_array_get(ref, 0)
#define CHIMP_ARRAY_LAST(ref)  chimp_array_get(ref, -1)
#define CHIMP_ARRAY_ITEM(ref, i) chimp_array_get((ref), (i))

#ifdef __cplusplus
};
#endif

#endif

