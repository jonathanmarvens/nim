#ifndef _CHIMP_HASH_H_INCLUDED_
#define _CHIMP_HASH_H_INCLUDED_

#include <chimp/gc.h>
#include <chimp/any.h>
#include <chimp/lwhash.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpHash {
    ChimpAny   base;
    ChimpRef **keys;
    ChimpRef **values;
    size_t     size;
} ChimpHash;

chimp_bool_t
chimp_hash_class_bootstrap (ChimpGC *gc);

ChimpRef *
chimp_hash_new (ChimpGC *gc);

chimp_bool_t
chimp_hash_put (ChimpRef *self, ChimpRef *key, ChimpRef *value);

chimp_bool_t
chimp_hash_put_str (ChimpRef *self, const char *key, ChimpRef *value);

ChimpRef *
chimp_hash_get (ChimpRef *self, ChimpRef *key);

ChimpRef *
chimp_hash_keys (ChimpRef *self);

ChimpRef *
chimp_hash_values (ChimpRef *self);

#define CHIMP_HASH(ref) \
    CHIMP_CHECK_CAST(ChimpHash, (ref), CHIMP_VALUE_TYPE_HASH)

#define CHIMP_HASH_PUT(ref, key, value) \
    chimp_hash_put ((ref), CHIMP_STR_NEW(NULL, (key)), (value))

#define CHIMP_HASH_GET(ref, key) \
    chimp_hash_get ((ref), CHIMP_STR_NEW(NULL, (key)))

#define CHIMP_HASH_SIZE(ref) CHIMP_HASH(ref)->size

CHIMP_EXTERN_CLASS(hash);

#ifdef __cplusplus
};
#endif

#endif


