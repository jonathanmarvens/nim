#ifndef _CHIMP_GC_H_INCLUDED_
#define _CHIMP_GC_H_INCLUDED_

#include <chimp/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpGC ChimpGC;
typedef struct _ChimpRef ChimpRef;

union _ChimpValue;
enum _ChimpValueType;

ChimpGC *
chimp_gc_new (void *stack_start);

void
chimp_gc_delete (ChimpGC *gc);

ChimpRef *
chimp_gc_new_object (ChimpGC *gc);

chimp_bool_t
chimp_gc_make_root (ChimpGC *gc, ChimpRef *ref);

union _ChimpValue *
chimp_gc_ref_check_cast (ChimpRef *ref, enum _ChimpValueType type);

enum _ChimpValueType
chimp_gc_ref_type (ChimpRef *ref);

chimp_bool_t
chimp_gc_collect (ChimpGC *gc);

#define CHIMP_REF_TYPE(ref) chimp_gc_ref_type(ref)

#ifdef __cplusplus
};
#endif

#endif

