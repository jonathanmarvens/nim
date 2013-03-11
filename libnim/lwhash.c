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

#include "nim/core.h"
#include "nim/object.h"
#include "nim/lwhash.h"

typedef struct _NimLWHashItem {
    NimRef *key;
    NimRef *value;
} NimLWHashItem;

struct _NimLWHash {
    NimLWHashItem *items;
    size_t           size;
};

NimLWHash *
nim_lwhash_new (void)
{
    NimLWHash *self = NIM_MALLOC(NimLWHash, sizeof(*self));
    if (self == NULL) {
        return NULL;
    }
    self->items = NULL;
    self->size = 0;
    return self;
}


void
nim_lwhash_delete (NimLWHash *self)
{
    if (self != NULL) {
        NIM_FREE (self->items);
        NIM_FREE (self);
    }
}

static nim_bool_t
nim_lwhash_find (NimLWHash *self, NimRef *key, NimLWHashItem **result)
{
    size_t i;
    for (i = 0; i < self->size; i++) {
        NimLWHashItem *item = self->items + i;
        NimCmpResult r = nim_object_cmp (item->key, key);
        if (r == NIM_CMP_ERROR) {
            return NIM_FALSE;
        }
        else if (r == NIM_CMP_EQ) {
            *result = item;
            return NIM_TRUE;
        }
    }
    *result = NULL;
    return NIM_TRUE;
}

nim_bool_t
nim_lwhash_put (NimLWHash *self, NimRef *key, NimRef *value)
{
    NimLWHashItem *item;
    if (!nim_lwhash_find (self, key, &item)) {
        return NIM_FALSE;
    }
    if (item != NULL) {
        item->value = value;
    }
    else {
        NimLWHashItem *items;

        items = NIM_REALLOC (NimLWHashItem, self->items, sizeof (*items) * (self->size + 1));
        if (items == NULL) {
            return NIM_FALSE;
        }
        self->items = items;
        items[self->size].key = key;
        items[self->size].value = value;
        self->size++;
    }
    return NIM_TRUE;
}

nim_bool_t
nim_lwhash_get (NimLWHash *self, NimRef *key, NimRef **value)
{
    NimLWHashItem *item;
    if (!nim_lwhash_find (self, key, &item)) {
        return NIM_FALSE;
    }
    if (item != NULL) {
        *value = item->value;
    }
    else {
        *value = NULL;
    }
    return NIM_TRUE;
}

void
nim_lwhash_foreach (NimLWHash *self, void (*fn)(NimLWHash *, NimRef *, NimRef *, void *), void *data)
{
    size_t i;
    for (i = 0; i < self->size; i++) {
        fn (self, self->items[i].key, self->items[i].value, data);
    }
}

