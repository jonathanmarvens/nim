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

#include "chimp/core.h"
#include "chimp/object.h"
#include "chimp/lwhash.h"

typedef struct _ChimpLWHashItem {
    ChimpRef *key;
    ChimpRef *value;
} ChimpLWHashItem;

struct _ChimpLWHash {
    ChimpLWHashItem *items;
    size_t           size;
};

ChimpLWHash *
chimp_lwhash_new (void)
{
    ChimpLWHash *self = CHIMP_MALLOC(ChimpLWHash, sizeof(*self));
    if (self == NULL) {
        return NULL;
    }
    self->items = NULL;
    self->size = 0;
    return self;
}


void
chimp_lwhash_delete (ChimpLWHash *self)
{
    if (self != NULL) {
        CHIMP_FREE (self->items);
        CHIMP_FREE (self);
    }
}

static chimp_bool_t
chimp_lwhash_find (ChimpLWHash *self, ChimpRef *key, ChimpLWHashItem **result)
{
    size_t i;
    for (i = 0; i < self->size; i++) {
        ChimpLWHashItem *item = self->items + i;
        ChimpCmpResult r = chimp_object_cmp (item->key, key);
        if (r == CHIMP_CMP_ERROR) {
            return CHIMP_FALSE;
        }
        else if (r == CHIMP_CMP_EQ) {
            *result = item;
            return CHIMP_TRUE;
        }
    }
    *result = NULL;
    return CHIMP_TRUE;
}

chimp_bool_t
chimp_lwhash_put (ChimpLWHash *self, ChimpRef *key, ChimpRef *value)
{
    ChimpLWHashItem *item;
    if (!chimp_lwhash_find (self, key, &item)) {
        return CHIMP_FALSE;
    }
    if (item != NULL) {
        item->value = value;
    }
    else {
        ChimpLWHashItem *items;

        items = CHIMP_REALLOC (ChimpLWHashItem, self->items, sizeof (*items) * (self->size + 1));
        if (items == NULL) {
            return CHIMP_FALSE;
        }
        self->items = items;
        items[self->size].key = key;
        items[self->size].value = value;
        self->size++;
    }
    return CHIMP_TRUE;
}

chimp_bool_t
chimp_lwhash_get (ChimpLWHash *self, ChimpRef *key, ChimpRef **value)
{
    ChimpLWHashItem *item;
    if (!chimp_lwhash_find (self, key, &item)) {
        return CHIMP_FALSE;
    }
    if (item != NULL) {
        *value = item->value;
    }
    else {
        *value = NULL;
    }
    return CHIMP_TRUE;
}

void
chimp_lwhash_foreach (ChimpLWHash *self, void (*fn)(ChimpLWHash *, ChimpRef *, ChimpRef *, void *), void *data)
{
    size_t i;
    for (i = 0; i < self->size; i++) {
        fn (self, self->items[i].key, self->items[i].value, data);
    }
}

