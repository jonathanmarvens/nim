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

#include "nim/array.h"
#include "nim/object.h"
#include "nim/core.h"
#include "nim/class.h"
#include "nim/str.h"

NimRef *nim_array_class = NULL;

static int32_t
_nim_array_real_index (NimRef *self, int32_t pos)
{
    if (pos >= 0) {
        if (pos < NIM_ARRAY_SIZE(self)) {
            return pos;
        }
        else {
            return -1;
        }
    }
    else {
        if ((size_t)(-pos) <= NIM_ARRAY_SIZE(self)) {
            return NIM_ARRAY_SIZE(self) - (size_t)(-pos);
        }
        else {
            return -1;
        }
    }
}

static NimRef *
_nim_array_push (NimRef *self, NimRef *args)
{
    NimRef *arg;
    if (!nim_method_parse_args (args, "o", &arg)) {
        return NULL;
    }
    /* TODO error if args len == 0 */
    if (!nim_array_push (self, arg)) {
        /* XXX error? exception? abort? */
        return NULL;
    }
    return nim_nil;
}

static NimRef *
_nim_array_pop (NimRef *self, NimRef *args)
{
    if (!nim_method_no_args (args)) {
        return NULL;
    }
    return nim_array_pop (self);
}

static NimRef *
_nim_array_map (NimRef *self, NimRef *args)
{
    size_t i;
    NimRef *fn;
    NimRef *result = nim_array_new_with_capacity (NIM_ARRAY_SIZE(self));
    if (!nim_method_parse_args (args, "o", &fn)) {
        return NULL;
    }
    for (i = 0; i < NIM_ARRAY_SIZE(self); i++) {
        NimRef *fn_args;
        NimRef *mapped;
        NimRef *value = NIM_ARRAY_ITEM(self, i);

        fn_args = nim_array_new_var (value, NULL);
        if (fn_args == NULL) {
            return NULL;
        }
        mapped = nim_object_call (fn, fn_args);
        if (mapped == NULL) {
            return NULL;
        }
        if (!nim_array_push (result, mapped)) {
            return NULL;
        }
    }
    return result;
}

static NimRef *
_nim_array_each (NimRef *self, NimRef *args)
{
    size_t i;
    NimRef *fn;
    if (!nim_method_parse_args (args, "o", &fn)) {
        return NULL;
    }
    for (i = 0; i < NIM_ARRAY_SIZE(self); i++) {
        NimRef *fn_args;
        NimRef *value;
        
        value = NIM_ARRAY_ITEM(self, i);
        fn_args = nim_array_new_var (value, NULL);
        if (fn_args == NULL) {
            return NULL;
        }
        if (nim_object_call (fn, fn_args) == NULL) {
            return NULL;
        }
    }
    return nim_nil;
}

static NimRef *
_nim_array_contains (NimRef *self, NimRef *args)
{
    size_t i;
    NimRef *right;
    if (!nim_method_parse_args (args, "o", &right)) {
        return NULL;
    }
    for (i = 0; i < NIM_ARRAY_SIZE(self); i++) {
        NimCmpResult r;
        NimRef *left = NIM_ARRAY_ITEM(self, i);
        
        r = nim_object_cmp (left, right);
        if (r == NIM_CMP_ERROR) {
            return NULL;
        }
        else if (r == NIM_CMP_EQ) {
            return nim_true;
        }
    }
    return nim_false;
}

static NimRef *
_nim_array_any (NimRef *self, NimRef *args)
{

    NimRef *fn;
    size_t i;
    NimRef *item;
    NimRef *fn_args;
    NimRef *result;

    if (!nim_method_parse_args (args, "o", &fn)) {
        return NULL;
    }

    for (i = 0; i < NIM_ARRAY_SIZE(self); i++) {
        item = NIM_ARRAY_ITEM(self, i);

        fn_args = nim_array_new_var (item, NULL);
        if (fn_args == NULL) {
            return NULL;
        }

        result = nim_object_call (fn, fn_args);
        if (result == NULL) {
            return NULL;
        }
        /* TODO nim_object_bool(result) == nim_true? */
        if (result == nim_true) {
            return nim_true;
        }
    }

    return nim_false;
}

static NimRef *
_nim_array_filter (NimRef *self, NimRef *args)
{
    size_t i;
    NimRef *result;
    NimRef *fn;

    if (!nim_method_parse_args (args, "o", &fn)) {
        return NULL;
    }

    result = nim_array_new ();
    for (i = 0; i < NIM_ARRAY_SIZE(self); i++) {
        NimRef *value = NIM_ARRAY_ITEM(self, i);
        NimRef *fn_args;
        NimRef *r;
        
        fn_args = nim_array_new_var (value, NULL);
        if (fn_args == NULL) {
            return NULL;
        }
        r = nim_object_call (fn, fn_args);
        if (r == NULL) {
            return NULL;
        }
        if (r == nim_true) {
            if (!nim_array_push (result, value)) {
                return NULL;
            }
        }
    }
    return result;
}

static NimRef *
nim_array_str (NimRef *self)
{
    size_t size = NIM_ARRAY_SIZE(self);
    /* '[' + ']' + (', ' x (size-1)) + '\0' */
    size_t total_len = 2 + (size > 0 ? ((size-1) * 2) : 0) + 1;
    NimRef *ref;
    NimRef *item_strs;
    char *data;
    size_t i, j;

    item_strs = nim_array_new ();
    if (item_strs == NULL) {
        return NULL;
    }

    for (i = 0; i < size; i++) {
        ref = NIM_ARRAY_ITEM(self, i);
        /* XXX what we really want is something like Python's repr() */
        /* TODO instanceof */
        if (NIM_ANY_CLASS(ref) == nim_str_class) {
            /* for surrounding quotes */
            total_len += 2;
        }
        ref = nim_object_str (ref);
        if (ref == NULL) {
            return NULL;
        }
        nim_array_push (item_strs, ref);
        total_len += NIM_STR_SIZE(ref);
    }

    data = NIM_MALLOC(char, total_len);
    if (data == NULL) {
        return NULL;
    }
    j = 0;
    data[j++] = '[';

    for (i = 0; i < size; i++) {
        ref = NIM_ARRAY_ITEM(item_strs, i);
        /* XXX what we really want is something like Python's repr() */
        if (NIM_ANY_CLASS(NIM_ARRAY_ITEM(self, i)) == nim_str_class) {
            data[j++] = '"';
        }
        memcpy (data + j, NIM_STR_DATA(ref), NIM_STR_SIZE(ref));
        j += NIM_STR_SIZE(ref);
        if (NIM_ANY_CLASS(NIM_ARRAY_ITEM(self, i)) == nim_str_class) {
            data[j++] = '"';
        }
        if (i < (size-1)) {
            memcpy (data + j, ", ", 2);
            j += 2;
        }
    }

    data[j++] = ']';
    data[j] = '\0';

    return nim_str_new_take (data, total_len-1);
}

static NimRef *
_nim_array_insert (NimRef *self, NimRef *args)
{
    int32_t pos;
    NimRef *value;

    if (!nim_method_parse_args (args, "io", &pos, &value)) {
        return NULL;
    }

    if (!nim_array_insert (self, pos, value)) {
        return NULL;
    }
    return nim_nil;
}

static NimRef *
_nim_array_size (NimRef *self, NimRef *args)
{
    if (!nim_method_no_args (args)) {
        return NULL;
    }

    return nim_int_new (NIM_ARRAY_SIZE(self));
}

static NimRef *
_nim_array_join (NimRef *self, NimRef *args)
{
    size_t i;
    size_t len;
    size_t seplen;
    const char *sep;
    NimRef *strs;
    char *result;
    char *ptr;

    if (!nim_method_parse_args (args, "s", &sep)) {
        return NULL;
    }
    seplen = strlen (sep);

    strs = nim_array_new_with_capacity (NIM_ARRAY_SIZE(self));
    if (strs == NULL) {
        return NULL;
    }

    len = strlen (sep) * (NIM_ARRAY_SIZE(self) - 1);
    for (i = 0; i < NIM_ARRAY_SIZE(self); i++) {
        NimRef *strval = nim_object_str (NIM_ARRAY_ITEM(self, i));
        if (strval == NULL) {
            return NULL;
        }
        if (!nim_array_push (strs, strval)) {
            return NULL;
        }
        len += NIM_STR_SIZE(strval);
    }

    result = malloc (len + 1);
    if (result == NULL) {
        return NULL;
    }
    ptr = result;

    for (i = 0; i < NIM_ARRAY_SIZE(self); i++) {
        NimRef *s = NIM_ARRAY_ITEM(strs, i);
        memcpy (ptr, NIM_STR_DATA(s), NIM_STR_SIZE(s));
        ptr += NIM_STR_SIZE(s);
        if (i < NIM_ARRAY_SIZE(self)-1) {
            memcpy (ptr, sep, seplen);
            ptr += seplen;
        }
    }
    result[len] = '\0';

    return nim_str_new_take (result, len);
}

static NimRef *
_nim_array_getitem (NimRef *self, NimRef *key)
{
    if (NIM_ANY_CLASS(key) != nim_int_class) {
        NIM_BUG ("bad argument type for array.__getattr__");
        return NULL;
    }

    return NIM_ARRAY_ITEM(self, (size_t)NIM_INT(key)->value);
}

static void
_nim_array_dtor (NimRef *self)
{
    NIM_FREE (NIM_ARRAY(self)->items);
}

static void
_nim_array_mark (NimGC *gc, NimRef *self)
{
    NIM_SUPER (self)->mark (gc, self);

    size_t i;
    for (i = 0; i < NIM_ARRAY(self)->size; i++) {
        nim_gc_mark_ref (gc, NIM_ARRAY(self)->items[i]);
    }
}

static NimCmpResult
_nim_array_cmp(NimRef *left, NimRef *right)
{
    size_t lsize = NIM_ARRAY_SIZE(left);
    size_t rsize = NIM_ARRAY_SIZE(right);

    if (lsize != rsize)
    {
        return NIM_CMP_NOT_IMPL;
    }
    else
    {
        int i;
        for (i = 0; i < lsize; i++) {
            NimRef *lval = NIM_ARRAY_ITEM(left, i);
            NimRef *rval = NIM_ARRAY_ITEM(right, i);

            NimCmpResult r = nim_object_cmp (lval, rval);
            if (r != NIM_CMP_EQ)
            {
                return r;
            }
        }
        return NIM_CMP_EQ;
    }
}

static NimRef *
_nim_array_add (NimRef *self, NimRef *other)
{
    if (NIM_ANY_CLASS(self) == NIM_ANY_CLASS(other)) {
        size_t i;
        size_t required_capacity =
            NIM_ARRAY_SIZE(self) + NIM_ARRAY_SIZE(other);
        NimRef *new = nim_array_new_with_capacity (required_capacity);
        if (new == NULL) {
            NIM_BUG ("could not allocate array");
            return NULL;
        }
        for (i = 0; i < NIM_ARRAY_SIZE(self); i++) {
            NIM_ARRAY(new)->items[i] = NIM_ARRAY_ITEM(self, i);
        }
        for (; i < required_capacity; i++) {
            NIM_ARRAY(new)->items[i] =
                NIM_ARRAY_ITEM(other, i - NIM_ARRAY_SIZE(self));
        }
        NIM_ARRAY(new)->size = required_capacity;
        return new;
    }
    else {
        NIM_BUG ("cannot add array and %s type",
                NIM_CLASS_NAME (NIM_ANY_CLASS(other)));
        return NULL;
    }
}

static void
_nim_array_remove_at_internal (NimRef *self, size_t index)
{
    if (NIM_ARRAY_SIZE(self) > index) {
        memmove (
            NIM_ARRAY(self)->items + index,
            NIM_ARRAY(self)->items + index + 1,
            sizeof(NimRef*) * (NIM_ARRAY_SIZE(self) - index - 1)
        );
    }
    NIM_ARRAY(self)->size--;
}

static NimRef *
_nim_array_remove (NimRef *self, NimRef *args)
{
    NimRef *other;
    size_t i;
    if (!nim_method_parse_args (args, "o", &other)) {
        return NULL;
    }
    for (i = 0; i < NIM_ARRAY_SIZE(self); i++) {
        int rc = nim_object_cmp (NIM_ARRAY_ITEM(self, i), other);
        if (rc == NIM_CMP_EQ) {
            _nim_array_remove_at_internal (self, i);
            return nim_true;
        }
        else if (rc == NIM_CMP_ERROR) {
            return NULL;
        }
    }
    return nim_false;
}

static NimRef *
_nim_array_remove_at (NimRef *self, NimRef *args)
{
    NimRef *removed;
    int64_t pos;
    if (!nim_method_parse_args (args, "I", &pos)) {
        return NULL;
    }
    if (pos < 0) {
        pos = NIM_ARRAY_SIZE(self) + pos;
    }
    if (pos < 0 || pos >= NIM_ARRAY_SIZE(self)) {
        NIM_BUG ("invalid array index: %zu", (intmax_t) pos);
        return NULL;
    }
    removed = NIM_ARRAY_ITEM(self, (size_t) pos);
    _nim_array_remove_at_internal (self, (size_t) pos);
    return removed;
}


static NimRef *
_nim_array_slice(NimRef *self, NimRef *args)
{
    int64_t start;
    int64_t end;
    if (!nim_method_parse_args (args, "II", &start, &end)) {
        NIM_BUG("invalid parameters, expecting two numeric values");
        return NULL;
    }
    // XXX - Better parameter validation?
    // XXX - Negative positioning for slicing

    NimRef *result = nim_array_new_with_capacity (end - start);
    int64_t i;
    for (i = start; i < end; i++) {
      NimRef *item = NIM_ARRAY_ITEM(self, (size_t) i);
      nim_array_push(result, item);
    }
    return result;
}

static NimRef *
_nim_array_shift (NimRef *self, NimRef *args)
{
    if (!nim_method_no_args (args)) {
        return NULL;
    }

    return nim_array_shift (self);
}
static NimRef *
_nim_array_unshift (NimRef *self, NimRef *args)
{
    NimRef *arg;

    if (!nim_method_parse_args (args, "o", &arg)) {
        return NULL;
    }

    if (!nim_array_unshift (self, arg)) {
        return NULL;
    }

    return nim_nil;
}

static NimRef *
_nim_array_nonzero (NimRef *self)
{
    return NIM_BOOL_REF(NIM_ARRAY_SIZE(self) > 0);
}

nim_bool_t
nim_array_class_bootstrap (void)
{
    nim_array_class = nim_class_new (
            NIM_STR_NEW("array"), nim_object_class, sizeof(NimArray));
    if (nim_array_class == NULL) {
        return NIM_FALSE;
    }
    NIM_CLASS(nim_array_class)->str = nim_array_str;
    NIM_CLASS(nim_array_class)->dtor = _nim_array_dtor;
    NIM_CLASS(nim_array_class)->getitem = _nim_array_getitem;
    NIM_CLASS(nim_array_class)->mark = _nim_array_mark;
    NIM_CLASS(nim_array_class)->cmp = _nim_array_cmp;
    NIM_CLASS(nim_array_class)->add = _nim_array_add;
    NIM_CLASS(nim_array_class)->nonzero = _nim_array_nonzero;
    nim_gc_make_root (NULL, nim_array_class);
    nim_class_add_native_method (nim_array_class, "push", _nim_array_push);
    nim_class_add_native_method (nim_array_class, "pop", _nim_array_pop);
    nim_class_add_native_method (nim_array_class, "map", _nim_array_map);
    nim_class_add_native_method (nim_array_class, "shift", _nim_array_shift);
    nim_class_add_native_method (nim_array_class, "unshift", _nim_array_unshift);
    nim_class_add_native_method (nim_array_class, "filter", _nim_array_filter);
    nim_class_add_native_method (nim_array_class, "each", _nim_array_each);
    nim_class_add_native_method (nim_array_class, "contains", _nim_array_contains);
    nim_class_add_native_method (nim_array_class, "any", _nim_array_any);
    nim_class_add_native_method (nim_array_class, "size", _nim_array_size);
    nim_class_add_native_method (nim_array_class, "join", _nim_array_join);
    nim_class_add_native_method (nim_array_class, "remove", _nim_array_remove);
    nim_class_add_native_method (nim_array_class, "remove_at", _nim_array_remove_at);
    nim_class_add_native_method (nim_array_class, "slice", _nim_array_slice);
    nim_class_add_native_method (nim_array_class, "insert", _nim_array_insert);
    return NIM_TRUE;
}

NimRef *
nim_array_new (void)
{
    return nim_array_new_with_capacity (10);
}

NimRef *
nim_array_new_with_capacity (size_t capacity)
{
    NimRef *ref = nim_gc_new_object (NULL);
    if (ref == NULL) {
        return NULL;
    }
    NIM_ANY(ref)->klass = nim_array_class;
    NIM_ARRAY(ref)->items = NIM_MALLOC(NimRef *, capacity * sizeof(NimRef *));
    if (NIM_ARRAY(ref)->items == NULL) {
        return NULL;
    }
    NIM_ARRAY(ref)->size = 0;
    NIM_ARRAY(ref)->capacity = capacity;
    return ref;
}

NimRef *
nim_array_new_var (NimRef *a, ...)
{
    va_list args;
    NimRef *arg;
    NimRef *ref = nim_array_new ();
    if (ref == NULL) {
        return NULL;
    }

    va_start (args, a);
    if (!nim_array_push (ref, a)) {
        return NULL;
    }
    while ((arg = va_arg (args, NimRef *)) != NULL) {
        nim_array_push (ref, arg);
    }
    va_end (args);

    return ref;
}

#if 0
static nim_bool_t
nim_array_ensure_capacity (NimRef *self, size_t capacity)
{
    if (NIM_ARRAY(self)->capacity < capacity) {
        NimRef **items;
        items = NIM_REALLOC(
            NimRef *, arr->items, sizeof(*arr->items) * new_capacity);
        if (items == NULL) {
            return NIM_FALSE;
        }
        arr->items = items;
        arr->capacity = capacity;
    }
    return NIM_TRUE;
}
#endif

static nim_bool_t
nim_array_grow (NimRef *self)
{
    NimRef **items;
    NimArray *arr = NIM_ARRAY(self);
    if (arr->size >= arr->capacity) {
        size_t new_capacity =
            (arr->capacity == 0 ? 10 : (size_t)(arr->capacity * 1.8));
        items = NIM_REALLOC(
            NimRef *, arr->items, sizeof(*arr->items) * new_capacity);
        if (items == NULL) {
            NIM_BUG ("failed to grow internal array buffer");
            return NIM_FALSE;
        }
        arr->items = items;
        arr->capacity = new_capacity;
    }
    return NIM_TRUE;
}

nim_bool_t
nim_array_insert (NimRef *self, int32_t pos, NimRef *value)
{
    NimRef **items;
    if (pos < 0 || pos > NIM_ARRAY_SIZE(self)) {
        pos = _nim_array_real_index (self, pos);
        if (pos < 0) {
            NIM_BUG ("invalid index into array: %d", pos);
            return NIM_FALSE;
        }
    }
    if (!nim_array_grow (self)) {
        return NIM_FALSE;
    }
    items = NIM_ARRAY(self)->items;
    if (NIM_ARRAY_SIZE(self) - pos > 0) {
        memmove (
            items + pos + 1,
            items + pos,
            sizeof(*items) * (NIM_ARRAY_SIZE(self) - pos)
        );
    }
    NIM_ARRAY(self)->size++;
    items[pos] = value;
    return NIM_TRUE;
}

nim_bool_t
nim_array_unshift (NimRef *self, NimRef *value)
{
    size_t i;
    NimArray *arr = NIM_ARRAY(self);
    if (!nim_array_grow (self)) {
        return NIM_FALSE;
    }
    if (arr->size > 0) {
        for (i = arr->size; i > 0; i--) {
            arr->items[i] = arr->items[i-1];
        }
    }
    arr->items[0] = value;
    arr->size++;
    return NIM_TRUE;
}

NimRef *
nim_array_shift (NimRef *self)
{
    if (NIM_ARRAY_SIZE(self) > 0) {
        NimRef **items = NIM_ARRAY(self)->items;
        NimRef *first = NIM_ARRAY_ITEM(self, 0);
        memmove (
            items, items + 1, sizeof(*items) * (NIM_ARRAY_SIZE(self) -1));
        NIM_ARRAY(self)->size--;
        return first;
    }
    else {
        return NULL;
    }
}

nim_bool_t
nim_array_push (NimRef *self, NimRef *value)
{
    NimArray *arr = NIM_ARRAY(self);
    if (!nim_array_grow (self)) {
        return NIM_FALSE;
    }
    arr->items[arr->size++] = value;
    return NIM_TRUE;
}

NimRef *
nim_array_pop (NimRef *self)
{
    NimArray *arr = NIM_ARRAY(self);
    if (arr->size > 0) {
        return arr->items[--arr->size];
    }
    else {
        return nim_nil;
    }
}

NimRef *
nim_array_get (NimRef *self, int32_t pos)
{
    pos = _nim_array_real_index (self, pos);
    if (pos < 0) {
        return nim_nil;
    }
    return NIM_ARRAY(self)->items[pos];
}

int32_t
nim_array_find (NimRef *self, NimRef *value)
{
    size_t i;
    for (i = 0; i < NIM_ARRAY_SIZE(self); i++) {
        NimCmpResult r = nim_object_cmp (NIM_ARRAY_ITEM(self, i), value);
        if (r == NIM_CMP_ERROR) {
            return -1;
        }
        else if (r == NIM_CMP_EQ) {
            return (int32_t)i;
        }
    }
    return -1;
}

NimRef *
nim_array_first (NimRef *self)
{
    if (NIM_ARRAY_SIZE(self) > 0) {
        return NIM_ARRAY_ITEM(self, 0);
    }
    else {
        return nim_nil;
    }
}

NimRef *
nim_array_last (NimRef *self)
{
    if (NIM_ARRAY_SIZE(self) > 0) {
        return NIM_ARRAY_ITEM(self, NIM_ARRAY_SIZE(self)-1);
    }
    else {
        return nim_nil;
    }
}

