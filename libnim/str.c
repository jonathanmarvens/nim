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

#include "nim/object.h"
#include "nim/str.h"

NimRef *
nim_str_new (const char *data, size_t size)
{
    char *copy;
    copy = NIM_MALLOC(char, size + 1);
    if (copy == NULL) return NULL;
    memcpy (copy, data, size);
    copy[size] = '\0';
    return nim_str_new_take (copy, size);
}

NimRef *
nim_str_new_take (char *data, size_t size)
{
    NimRef *ref = nim_gc_new_object (NULL);
    if (ref == NULL) {
        return NULL;
    }
    NIM_ANY(ref)->klass = nim_str_class;
    NIM_STR(ref)->data = data;
    NIM_STR(ref)->size = size;
    return ref;
}

NimRef *
nim_str_new_format (const char *fmt, ...)
{
    va_list args;
    NimRef *result;

    va_start (args, fmt);
    result = nim_str_new_formatv (fmt, args);
    va_end (args);

    return result;
}

NimRef *
nim_str_new_formatv (const char *fmt, va_list args)
{
    size_t size;
    va_list args_p;
    
    va_copy (args_p, args);
    size = vsnprintf (NULL, 0, fmt, args);

    if (size >= 0) {
        char *buf = NIM_MALLOC(char, size + 1);
        if (buf == NULL) {
            va_end (args_p);
            return NULL;
        }

        va_end (args);
        va_copy (args, args_p);
        va_end (args_p);

        vsnprintf (buf, size + 1, fmt, args);
        return nim_str_new_take (buf, size);
    }
    else {
        va_end (args_p);
        return NULL;
    }
}

NimRef *
nim_str_new_concat (const char *a, ...)
{
    va_list args;
    const char *arg;
    size_t size = 0;
    char *ptr, *temp;
    size_t len;

    va_start (args, a);
    size = strlen (a);
    while ((arg = va_arg(args, const char *)) != NULL) {
        size += strlen (arg);
    }
    va_end (args);

    temp = ptr = NIM_MALLOC(char, size + 1);
    if (temp == NULL) return NULL;

    va_start (args, a);
    len = strlen (a);
    memcpy (temp, a, len);
    temp += len;
    while ((arg = va_arg(args, const char *)) != NULL) {
        len = strlen (arg);
        memcpy (temp, arg, len);
        temp += len;
    }
    va_end (args);
    *temp = '\0';

    return nim_str_new_take (ptr, size);
}

nim_bool_t
nim_str_append (NimRef *self, NimRef *append_me)
{
    NimRef *append_str = nim_object_str (append_me);
    /* TODO error checking */
    char *data = NIM_REALLOC (char, NIM_STR(self)->data, NIM_STR_SIZE(self) + NIM_STR_SIZE(append_str) + 1);
    if (data == NULL) {
        return NIM_FALSE;
    }
    NIM_STR(self)->data = data;
    memcpy (NIM_STR_DATA(self) + NIM_STR_SIZE(self), NIM_STR_DATA(append_str), NIM_STR_SIZE(append_str));
    NIM_STR(self)->size += NIM_STR_SIZE(append_str);
    NIM_STR(self)->data[NIM_STR(self)->size] = '\0';
    return NIM_TRUE;
}

nim_bool_t
nim_str_append_str (NimRef *self, const char *s)
{
    /* XXX inefficient */
    return nim_str_append (self, nim_str_new (s, strlen(s)));
}

nim_bool_t
nim_str_append_strn (NimRef *self, const char *s, size_t n)
{
    /* XXX inefficient */
    return nim_str_append (self, nim_str_new (s, n));
}

nim_bool_t
nim_str_append_char (NimRef *self, char c)
{
    /* XXX inefficient */
    return nim_str_append (self, nim_str_new (&c, 1));
}

const char *
nim_str_data (NimRef *str)
{
    return NIM_STR_DATA(str);
}

static NimRef *
_nim_str_size (NimRef *self, NimRef *args)
{
    return nim_int_new (NIM_STR_SIZE(self));
}

static NimRef *
_nim_str_index (NimRef *self, NimRef *args)
{
    const char *needle;
    const char *res;
    if (!nim_method_parse_args (args, "s", &needle)) {
        return NULL;
    }
    res = strstr (NIM_STR_DATA(self), needle);
    if (res == NULL) {
        return nim_int_new (-1);
    }
    else {
        return nim_int_new ((size_t)(res - NIM_STR_DATA(self)));
    }
}

static NimRef *
_nim_str_trim (NimRef *self, NimRef *args)
{
    size_t i, j;

    for (i = 0; i < NIM_STR_SIZE(self); i++) {
        if (!isspace (NIM_STR(self)->data[i])) {
            break;
        }
    }

    if (NIM_STR_SIZE(self) > 0) {
        for (j = NIM_STR_SIZE(self)-1; j > i; j--) {
            if (!isspace (NIM_STR(self)->data[j])) {
                break;
            }
        }
    }
    else {
        j = 0;
    }

    return nim_str_new (NIM_STR_DATA(self) + i, j + 1 - i);
}

static NimRef *
_nim_str_substr (NimRef *self, NimRef *args)
{
    int64_t i, j;
    NimRef *jobj = NULL;

    if (!nim_method_parse_args (args, "I|o", &i, &jobj)) {
        return NULL;
    }

    if (i < 0) {
        i += NIM_STR_SIZE(self);
    }

    if (jobj == NULL) {
        j = NIM_STR_SIZE(self);
    }
    else {
        j = NIM_INT(jobj)->value;
        if (j < 0) {
            j += NIM_STR_SIZE(self);
        }
    }

    return nim_str_new (NIM_STR_DATA(self) + i, j - i);
}

static NimRef *
_nim_str_split (NimRef *self, NimRef *args)
{
    const char *begin;
    const char *end;
    const char *sep;
    size_t seplen;
    NimRef *arr;
    const size_t len = NIM_STR_SIZE(self);
    size_t n = 0;

    if (!nim_method_parse_args (args, "s", &sep)) {
        return NULL;
    }

    /* XXX could pull this directly from the str object */
    seplen = strlen (sep);

    arr = nim_array_new ();
    if (arr == NULL) {
        return NULL;
    }

    begin = NIM_STR_DATA(self);
    while (n < len) {
        NimRef *ref;
        end = strstr (begin + n, sep);
        if (end == NULL) {
            end = NIM_STR_DATA(self) + NIM_STR_SIZE(self);
        }
        ref = nim_str_new (begin + n, (size_t)(end - begin - n));
        if (ref == NULL) {
            return NULL;
        }
        if (!nim_array_push (arr, ref)) {
            return NULL;
        }
        n += NIM_STR_SIZE(ref) + seplen;
    }
    return arr;
}

static NimRef *
_nim_str_upper (NimRef *self, NimRef *args)
{
    size_t i;
    char *data;
    size_t len = NIM_STR_SIZE(self);
    NimRef *str = nim_str_new (NIM_STR_DATA(self), len);

    data = NIM_STR_DATA(str);

    for (i = 0; i < len; i++) {
        if (data[i] >= 'a' && data[i] <= 'z') {
            data[i] -= (char) 0x20;
        }
    }

    return str;
}

static NimRef *
_nim_str_lower (NimRef *self, NimRef *args)
{
    size_t i;
    char *data;
    size_t len = NIM_STR_SIZE(self);
    NimRef *str = nim_str_new (NIM_STR_DATA(self), len);

    data = NIM_STR_DATA(str);

    for (i = 0; i < len; i++) {
        if (data[i] >= 'A' && data[i] <= 'Z') {
            data[i] += (char) 0x20;
        }
    }

    return str;
}

int
nim_str_class_init_2 (void)
{
    if (!nim_class_add_native_method (nim_str_class, "size", _nim_str_size)) {
        return NIM_FALSE;
    }

    if (!nim_class_add_native_method (nim_str_class, "trim", _nim_str_trim)) {
        return NIM_FALSE;
    }

    if (!nim_class_add_native_method (nim_str_class, "index", _nim_str_index)) {
        return NIM_FALSE;
    }

    if (!nim_class_add_native_method (nim_str_class, "substr", _nim_str_substr)) {
        return NIM_FALSE;
    }

    if (!nim_class_add_native_method (nim_str_class, "split", _nim_str_split)) {
        return NIM_FALSE;
    }

    if (!nim_class_add_native_method (nim_str_class, "upper", _nim_str_upper)) {
        return NIM_FALSE;
    }

    if (!nim_class_add_native_method (nim_str_class, "lower", _nim_str_lower)) {
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

