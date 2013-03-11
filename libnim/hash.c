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

#include "nim/hash.h"
#include "nim/object.h"

NimRef *nim_hash_class = NULL;

static NimRef *
nim_hash_str (NimRef *self)
{
    size_t size = NIM_HASH_SIZE(self);
    /* '{' + '}' + (': ' x size) + (', ' x (size-1)) + '\0' */
    size_t total_len = 2 + (size * 2) + (size > 0 ? ((size-1) * 2) : 0) + 1;
    NimRef *ref;
    NimRef *strs;
    char *data;
    size_t i, k;


    strs = nim_array_new_with_capacity (size);
    if (strs == NULL) {
        return NULL;
    }

    for (i = 0; i < size; i++) {
        size_t j;
        NimRef *item[2];
        item[0] = NIM_HASH(self)->keys[i];
        item[1] = NIM_HASH(self)->values[i];
        for (j = 0; j < 2; j++) {
            ref = item[j];
            /* XXX what we really want is something like Python's repr() */
            if (NIM_ANY_CLASS(ref) == nim_str_class) {
                /* for surrounding quotes */
                total_len += 2;
            }
            ref = nim_object_str (ref);
            if (ref == NULL) {
                return NULL;
            }
            nim_array_push (strs, ref);
            total_len += NIM_STR_SIZE(ref);
        }
    }

    data = NIM_MALLOC(char, total_len);
    if (data == NULL) {
        return NULL;
    }
    k = 0;
    data[k++] = '{';

    for (i = 0; i < size; i++) {
        size_t j;
        NimRef *item[2];
        item[0] = NIM_HASH(self)->keys[i];
        item[1] = NIM_HASH(self)->values[i];
        for (j = 0; j < 2; j++) {
            ref = NIM_ARRAY_ITEM(strs, (i * 2) + j);
            /* XXX what we really want is something like Python's repr() */
            /* TODO instanceof */
            if (NIM_ANY_CLASS(item[j]) == nim_str_class) {
                data[k++] = '"';
            }
            memcpy (data + k, NIM_STR_DATA(ref), NIM_STR_SIZE(ref));
            k += NIM_STR_SIZE(ref);
            if (NIM_ANY_CLASS(item[j]) == nim_str_class) {
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

    return nim_str_new_take (data, total_len-1);
}

static NimRef *
_nim_hash_put (NimRef *self, NimRef *args)
{
    return NIM_BOOL_REF(nim_hash_put (self, NIM_ARRAY_ITEM(args, 0), NIM_ARRAY_ITEM(args, 1)));
}

static NimRef *
_nim_hash_get (NimRef *self, NimRef *args)
{
    NimRef *value;
    int rc = nim_hash_get (self, NIM_ARRAY_ITEM(args, 0), &value);
    if (rc > 0) {
        return nim_nil;
    }
    else if (rc < 0) {
        return NULL;
    }
    else {
        return value;
    }
}

static NimRef *
_nim_hash_size (NimRef *self, NimRef *args)
{
    return nim_int_new (NIM_HASH_SIZE (self));
}

static NimRef *
_nim_hash_getitem (NimRef *self, NimRef *key)
{
    int rc;    
    NimRef *value;

    rc = nim_hash_get (self, key, &value);
    if (rc < 0) {
        return NULL;
    }
    return value;
}

static void
_nim_hash_dtor (NimRef *self)
{
    NIM_FREE (NIM_HASH(self)->keys);
    NIM_FREE (NIM_HASH(self)->values);
}

static void
_nim_hash_mark (NimGC *gc, NimRef *self)
{
    NIM_SUPER (self)->mark (gc, self);

    size_t i;
    for (i = 0; i < NIM_HASH(self)->size; i++) {
        nim_gc_mark_ref (gc, NIM_HASH(self)->keys[i]);
        nim_gc_mark_ref (gc, NIM_HASH(self)->values[i]);
    }
}

static NimRef *
_nim_hash_items (NimRef *self, NimRef *args)
{
    size_t i;
    NimRef *items = nim_array_new ();
    for (i = 0; i < NIM_HASH(self)->size; i++) {
        NimRef *item =
            nim_array_new_var (
                    NIM_HASH(self)->keys[i],
                    NIM_HASH(self)->values[i], NULL);
        if (item == NULL) {
            return NULL;
        }
        if (!nim_array_push (items, item)) {
            return NULL;
        }
    }
    return items;
}

NimRef *
_nim_hash_nonzero (NimRef *self)
{
    return NIM_BOOL_REF(NIM_HASH_SIZE(self) > 0);
}

nim_bool_t
nim_hash_class_bootstrap (void)
{
    nim_hash_class =
        nim_class_new (NIM_STR_NEW("hash"), nim_object_class, sizeof(NimHash));
    if (nim_hash_class == NULL) {
        return NIM_FALSE;
    }
    NIM_CLASS(nim_hash_class)->str = nim_hash_str;
    NIM_CLASS(nim_hash_class)->dtor = _nim_hash_dtor;
    NIM_CLASS(nim_hash_class)->getitem = _nim_hash_getitem;
    NIM_CLASS(nim_hash_class)->mark = _nim_hash_mark;
    NIM_CLASS(nim_hash_class)->nonzero = _nim_hash_nonzero;
    nim_gc_make_root (NULL, nim_hash_class);
    nim_class_add_native_method (nim_hash_class, "put", _nim_hash_put);
    nim_class_add_native_method (nim_hash_class, "get", _nim_hash_get);
    nim_class_add_native_method (nim_hash_class, "size", _nim_hash_size);
    nim_class_add_native_method (nim_hash_class, "items", _nim_hash_items);
    return NIM_TRUE;
}

NimRef *
nim_hash_new (void)
{
    return nim_class_new_instance (nim_hash_class, NULL);
}

nim_bool_t
nim_hash_put (NimRef *self, NimRef *key, NimRef *value)
{
    NimRef **keys;
    NimRef **values;
    size_t i;

    for (i = 0; i < NIM_HASH_SIZE(self); i++) {
        NimCmpResult r = nim_object_cmp (NIM_HASH(self)->keys[i], key);
        if (r == NIM_CMP_ERROR) {
            return NIM_FALSE;
        }
        else if (r == NIM_CMP_EQ) {
            NIM_HASH(self)->values[i] = value;
            return NIM_TRUE;
        }
    }

    keys = NIM_REALLOC(NimRef *, NIM_HASH(self)->keys, sizeof(*keys) * (NIM_HASH_SIZE(self)+1));
    if (keys == NULL) {
        return NIM_FALSE;
    }
    NIM_HASH(self)->keys = keys;

    values = NIM_REALLOC(NimRef *, NIM_HASH(self)->values, sizeof(*values) * (NIM_HASH_SIZE(self)+1));
    if (values == NULL) {
        return NIM_FALSE;
    }
    NIM_HASH(self)->values = values;

    keys[NIM_HASH_SIZE(self)] = key;
    values[NIM_HASH_SIZE(self)] = value;
    NIM_HASH(self)->size++;

    return NIM_TRUE;
}

nim_bool_t
nim_hash_put_str (NimRef *self, const char *key, NimRef *value)
{
    return nim_hash_put (self, nim_str_new (key, strlen(key)), value);
}

int
nim_hash_get (NimRef *self, NimRef *key, NimRef **value)
{
    size_t i;
    for (i = 0; i < NIM_HASH_SIZE(self); i++) {
        NimCmpResult r = nim_object_cmp (NIM_HASH(self)->keys[i], key);
        if (r == NIM_CMP_ERROR) {
            if (value != NULL) {
                *value = NULL;
            }
            return -1;
        }
        else if (r == NIM_CMP_EQ) {
            if (value != NULL) {
                *value = NIM_HASH(self)->values[i];
            }
            return 0;
        }
    }
    if (value != NULL) {
        *value = nim_nil;
    }
    return 1;
}

NimRef *
nim_hash_keys (NimRef *self)
{
    NIM_BUG ("not implemented");
    return NULL;
}

NimRef *
nim_hash_values (NimRef *self)
{
    NIM_BUG ("not implemented");
    return NULL;
}

