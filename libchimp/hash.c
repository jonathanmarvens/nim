/*****************************************************************************
 *                                                                           *
 * Copyright 2012 Thomas Lee                                                 *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License");           *
 * you may not use this file except in compliance with the License.          *
 * You may obtain a copy of the License at                                   *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 * Unless required by applicable law or agreed to in writing, software       *
 * distributed under the License is distributed on an "AS IS" BASIS,         *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 * See the License for the specific language governing permissions and       *
 * limitations under the License.                                            *
 *                                                                           *
 *****************************************************************************/

#include "chimp/hash.h"
#include "chimp/object.h"

ChimpRef *chimp_hash_class = NULL;

static ChimpRef *
chimp_hash_str (ChimpRef *self)
{
    size_t size = CHIMP_HASH_SIZE(self);
    /* '{' + '}' + (': ' x size) + (', ' x (size-1)) + '\0' */
    size_t total_len = 2 + (size * 2) + (size > 0 ? ((size-1) * 2) : 0) + 1;
    ChimpRef *ref;
    ChimpRef *strs;
    char *data;
    size_t i, k;


    strs = chimp_array_new_with_capacity (size);
    if (strs == NULL) {
        return NULL;
    }

    for (i = 0; i < size; i++) {
        size_t j;
        ChimpRef *item[2];
        item[0] = CHIMP_HASH(self)->keys[i];
        item[1] = CHIMP_HASH(self)->values[i];
        for (j = 0; j < 2; j++) {
            ref = item[j];
            /* XXX what we really want is something like Python's repr() */
            if (CHIMP_ANY_CLASS(ref) == chimp_str_class) {
                /* for surrounding quotes */
                total_len += 2;
            }
            ref = chimp_object_str (ref);
            if (ref == NULL) {
                return NULL;
            }
            chimp_array_push (strs, ref);
            total_len += CHIMP_STR_SIZE(ref);
        }
    }

    data = CHIMP_MALLOC(char, total_len);
    if (data == NULL) {
        return NULL;
    }
    k = 0;
    data[k++] = '{';

    for (i = 0; i < size; i++) {
        size_t j;
        ChimpRef *item[2];
        item[0] = CHIMP_HASH(self)->keys[i];
        item[1] = CHIMP_HASH(self)->values[i];
        for (j = 0; j < 2; j++) {
            ref = CHIMP_ARRAY_ITEM(strs, (i * 2) + j);
            /* XXX what we really want is something like Python's repr() */
            /* TODO instanceof */
            if (CHIMP_ANY_CLASS(item[j]) == chimp_str_class) {
                data[k++] = '"';
            }
            memcpy (data + k, CHIMP_STR_DATA(ref), CHIMP_STR_SIZE(ref));
            k += CHIMP_STR_SIZE(ref);
            if (CHIMP_ANY_CLASS(item[j]) == chimp_str_class) {
                data[k++] = '"';
            }
            if (j == 0) {
                data[k++] = ':';
                data[k++] = ' ';
            }
        }
        if (i < (size-1)) {
            data[k++] = ',';
            data[k++] = ' ';
        }
    }

    data[k++] = '}';
    data[k] = '\0';

    return chimp_str_new_take (data, total_len-1);
}

static ChimpRef *
_chimp_hash_put (ChimpRef *self, ChimpRef *args)
{
    return CHIMP_BOOL_REF(chimp_hash_put (self, CHIMP_ARRAY_ITEM(args, 0), CHIMP_ARRAY_ITEM(args, 1)));
}

static ChimpRef *
_chimp_hash_get (ChimpRef *self, ChimpRef *args)
{
    ChimpRef *value;
    int rc = chimp_hash_get (self, CHIMP_ARRAY_ITEM(args, 0), &value);
    if (rc > 0) {
        return chimp_nil;
    }
    else if (rc < 0) {
        return NULL;
    }
    else {
        return value;
    }
}

static ChimpRef *
_chimp_hash_size (ChimpRef *self, ChimpRef *args)
{
    return chimp_int_new (CHIMP_HASH_SIZE (self));
}

static ChimpRef *
_chimp_hash_getitem (ChimpRef *self, ChimpRef *key)
{
    int rc;    
    ChimpRef *value;

    rc = chimp_hash_get (self, key, &value);
    if (rc < 0) {
        return NULL;
    }
    return value;
}

static void
_chimp_hash_dtor (ChimpRef *self)
{
    CHIMP_FREE (CHIMP_HASH(self)->keys);
    CHIMP_FREE (CHIMP_HASH(self)->values);
}

static void
_chimp_hash_mark (ChimpGC *gc, ChimpRef *self)
{
    CHIMP_SUPER (self)->mark (gc, self);

    size_t i;
    for (i = 0; i < CHIMP_HASH(self)->size; i++) {
        chimp_gc_mark_ref (gc, CHIMP_HASH(self)->keys[i]);
        chimp_gc_mark_ref (gc, CHIMP_HASH(self)->values[i]);
    }
}

chimp_bool_t
chimp_hash_class_bootstrap (void)
{
    chimp_hash_class =
        chimp_class_new (CHIMP_STR_NEW("hash"), chimp_object_class, sizeof(ChimpHash));
    if (chimp_hash_class == NULL) {
        return CHIMP_FALSE;
    }
    CHIMP_CLASS(chimp_hash_class)->str = chimp_hash_str;
    CHIMP_CLASS(chimp_hash_class)->dtor = _chimp_hash_dtor;
    CHIMP_CLASS(chimp_hash_class)->getitem = _chimp_hash_getitem;
    CHIMP_CLASS(chimp_hash_class)->mark = _chimp_hash_mark;
    chimp_gc_make_root (NULL, chimp_hash_class);
    chimp_class_add_native_method (chimp_hash_class, "put", _chimp_hash_put);
    chimp_class_add_native_method (chimp_hash_class, "get", _chimp_hash_get);
    chimp_class_add_native_method (chimp_hash_class, "size", _chimp_hash_size);
    return CHIMP_TRUE;
}

ChimpRef *
chimp_hash_new (void)
{
    return chimp_class_new_instance (chimp_hash_class, NULL);
}

chimp_bool_t
chimp_hash_put (ChimpRef *self, ChimpRef *key, ChimpRef *value)
{
    ChimpRef **keys;
    ChimpRef **values;
    size_t i;

    for (i = 0; i < CHIMP_HASH_SIZE(self); i++) {
        ChimpCmpResult r = chimp_object_cmp (CHIMP_HASH(self)->keys[i], key);
        if (r == CHIMP_CMP_ERROR) {
            return CHIMP_FALSE;
        }
        else if (r == CHIMP_CMP_EQ) {
            CHIMP_HASH(self)->values[i] = value;
            return CHIMP_TRUE;
        }
    }

    keys = CHIMP_REALLOC(ChimpRef *, CHIMP_HASH(self)->keys, sizeof(*keys) * (CHIMP_HASH_SIZE(self)+1));
    if (keys == NULL) {
        return CHIMP_FALSE;
    }
    CHIMP_HASH(self)->keys = keys;

    values = CHIMP_REALLOC(ChimpRef *, CHIMP_HASH(self)->values, sizeof(*values) * (CHIMP_HASH_SIZE(self)+1));
    if (values == NULL) {
        return CHIMP_FALSE;
    }
    CHIMP_HASH(self)->values = values;

    keys[CHIMP_HASH_SIZE(self)] = key;
    values[CHIMP_HASH_SIZE(self)] = value;
    CHIMP_HASH(self)->size++;

    return CHIMP_TRUE;
}

chimp_bool_t
chimp_hash_put_str (ChimpRef *self, const char *key, ChimpRef *value)
{
    return chimp_hash_put (self, chimp_str_new (key, strlen(key)), value);
}

int
chimp_hash_get (ChimpRef *self, ChimpRef *key, ChimpRef **value)
{
    size_t i;
    for (i = 0; i < CHIMP_HASH_SIZE(self); i++) {
        ChimpCmpResult r = chimp_object_cmp (CHIMP_HASH(self)->keys[i], key);
        if (r == CHIMP_CMP_ERROR) {
            if (value != NULL) {
                *value = NULL;
            }
            return -1;
        }
        else if (r == CHIMP_CMP_EQ) {
            if (value != NULL) {
                *value = CHIMP_HASH(self)->values[i];
            }
            return 0;
        }
    }
    if (value != NULL) {
        *value = chimp_nil;
    }
    return 1;
}

ChimpRef *
chimp_hash_keys (ChimpRef *self)
{
    CHIMP_BUG ("not implemented");
    return NULL;
}

ChimpRef *
chimp_hash_values (ChimpRef *self)
{
    CHIMP_BUG ("not implemented");
    return NULL;
}

