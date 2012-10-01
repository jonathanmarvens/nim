#ifndef _CHIMP_LWHASH_H_INCLUDED_
#define _CHIMP_LWHASH_H_INCLUDED_

#include <chimp/gc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpLWHash ChimpLWHash;

ChimpLWHash *
chimp_lwhash_new (void);

void
chimp_lwhash_delete (ChimpLWHash *self);

chimp_bool_t
chimp_lwhash_put (ChimpLWHash *self, ChimpRef *key, ChimpRef *value);

chimp_bool_t
chimp_lwhash_get (ChimpLWHash *self, ChimpRef *key, ChimpRef **value);

void
chimp_lwhash_foreach (ChimpLWHash *self, void (*fn)(ChimpLWHash *, ChimpRef *, ChimpRef *, void *), void *);

#ifdef __cplusplus
};
#endif

#endif

