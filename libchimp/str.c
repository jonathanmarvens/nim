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

#include "chimp/object.h"
#include "chimp/str.h"

ChimpRef *
chimp_str_new (const char *data, size_t size)
{
    char *copy;
    copy = CHIMP_MALLOC(char, size + 1);
    if (copy == NULL) return NULL;
    memcpy (copy, data, size);
    copy[size] = '\0';
    return chimp_str_new_take (copy, size);
}

ChimpRef *
chimp_str_new_take (char *data, size_t size)
{
    ChimpRef *ref = chimp_gc_new_object (NULL);
    if (ref == NULL) {
        return NULL;
    }
    CHIMP_ANY(ref)->klass = chimp_str_class;
    CHIMP_STR(ref)->data = data;
    CHIMP_STR(ref)->size = size;
    return ref;
}

ChimpRef *
chimp_str_new_format (const char *fmt, ...)
{
    va_list args;
    ChimpRef *result;

    va_start (args, fmt);
    result = chimp_str_new_formatv (fmt, args);
    va_end (args);

    return result;
}

ChimpRef *
chimp_str_new_formatv (const char *fmt, va_list args)
{
    size_t size;
    va_list args_p;
    
    va_copy (args_p, args);
    size = vsnprintf (NULL, 0, fmt, args);

    if (size >= 0) {
        char *buf = CHIMP_MALLOC(char, size + 1);
        if (buf == NULL) {
            va_end (args_p);
            return NULL;
        }

        va_end (args);
        va_copy (args, args_p);
        va_end (args_p);

        vsnprintf (buf, size + 1, fmt, args);
        return chimp_str_new_take (buf, size);
    }
    else {
        va_end (args_p);
        return NULL;
    }
}

ChimpRef *
chimp_str_new_concat (const char *a, ...)
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

    temp = ptr = CHIMP_MALLOC(char, size + 1);
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

    return chimp_str_new_take (ptr, size);
}

chimp_bool_t
chimp_str_append (ChimpRef *self, ChimpRef *append_me)
{
    ChimpRef *append_str = chimp_object_str (append_me);
    /* TODO error checking */
    char *data = CHIMP_REALLOC (char, CHIMP_STR(self)->data, CHIMP_STR_SIZE(self) + CHIMP_STR_SIZE(append_str) + 1);
    if (data == NULL) {
        return CHIMP_FALSE;
    }
    CHIMP_STR(self)->data = data;
    memcpy (CHIMP_STR_DATA(self) + CHIMP_STR_SIZE(self), CHIMP_STR_DATA(append_str), CHIMP_STR_SIZE(append_str));
    CHIMP_STR(self)->size += CHIMP_STR_SIZE(append_str);
    CHIMP_STR(self)->data[CHIMP_STR(self)->size] = '\0';
    return CHIMP_TRUE;
}

chimp_bool_t
chimp_str_append_str (ChimpRef *self, const char *s)
{
    /* XXX inefficient */
    return chimp_str_append (self, chimp_str_new (s, strlen(s)));
}

chimp_bool_t
chimp_str_append_strn (ChimpRef *self, const char *s, size_t n)
{
    /* XXX inefficient */
    return chimp_str_append (self, chimp_str_new (s, n));
}

chimp_bool_t
chimp_str_append_char (ChimpRef *self, char c)
{
    /* XXX inefficient */
    return chimp_str_append (self, chimp_str_new (&c, 1));
}

const char *
chimp_str_data (ChimpRef *str)
{
    return CHIMP_STR_DATA(str);
}

static ChimpRef *
_chimp_str_size (ChimpRef *self, ChimpRef *args)
{
    return chimp_int_new (CHIMP_STR_SIZE(self));
}

static ChimpRef *
_chimp_str_index (ChimpRef *self, ChimpRef *args)
{
    const char *needle;
    const char *res;
    if (!chimp_method_parse_args (args, "s", &needle)) {
        return NULL;
    }
    res = strstr (CHIMP_STR_DATA(self), needle);
    if (res == NULL) {
        return chimp_int_new (-1);
    }
    else {
        return chimp_int_new ((size_t)(res - CHIMP_STR_DATA(self)));
    }
}

static ChimpRef *
_chimp_str_trim (ChimpRef *self, ChimpRef *args)
{
    size_t i, j;

    for (i = 0; i < CHIMP_STR_SIZE(self); i++) {
        if (!isspace (CHIMP_STR(self)->data[i])) {
            break;
        }
    }

    if (CHIMP_STR_SIZE(self) > 0) {
        for (j = CHIMP_STR_SIZE(self)-1; j > i; j--) {
            if (!isspace (CHIMP_STR(self)->data[j])) {
                break;
            }
        }
    }
    else {
        j = 0;
    }

    return chimp_str_new (CHIMP_STR_DATA(self) + i, j + 1 - i);
}

static ChimpRef *
_chimp_str_substr (ChimpRef *self, ChimpRef *args)
{
    int64_t i, j;
    ChimpRef *jobj = NULL;

    if (!chimp_method_parse_args (args, "I|o", &i, &jobj)) {
        return NULL;
    }

    if (i < 0) {
        i += CHIMP_STR_SIZE(self);
    }

    if (jobj == NULL) {
        j = CHIMP_STR_SIZE(self);
    }
    else {
        j = CHIMP_INT(jobj)->value;
        if (j < 0) {
            j += CHIMP_STR_SIZE(self);
        }
    }

    return chimp_str_new (CHIMP_STR_DATA(self) + i, j - i);
}

static ChimpRef *
_chimp_str_split (ChimpRef *self, ChimpRef *args)
{
    const char *begin;
    const char *end;
    const char *sep;
    size_t seplen;
    ChimpRef *arr;
    const size_t len = CHIMP_STR_SIZE(self);
    size_t n = 0;

    if (!chimp_method_parse_args (args, "s", &sep)) {
        return NULL;
    }

    /* XXX could pull this directly from the str object */
    seplen = strlen (sep);

    arr = chimp_array_new ();
    if (arr == NULL) {
        return NULL;
    }

    begin = CHIMP_STR_DATA(self);
    while (n < len) {
        ChimpRef *ref;
        end = strstr (begin + n, sep);
        if (end == NULL) {
            end = CHIMP_STR_DATA(self) + CHIMP_STR_SIZE(self);
        }
        ref = chimp_str_new (begin + n, (size_t)(end - begin - n));
        if (ref == NULL) {
            return NULL;
        }
        if (!chimp_array_push (arr, ref)) {
            return NULL;
        }
        n += CHIMP_STR_SIZE(ref) + seplen;
    }
    return arr;
}

static ChimpRef *
_chimp_str_upper (ChimpRef *self, ChimpRef *args)
{
    size_t i;
    char *data;
    size_t len = CHIMP_STR_SIZE(self);
    ChimpRef *str = chimp_str_new (CHIMP_STR_DATA(self), len);

    data = CHIMP_STR_DATA(str);

    for (i = 0; i < len; i++) {
        if (data[i] >= 'a' && data[i] <= 'z') {
            data[i] -= (char) 0x20;
        }
    }

    return str;
}

static ChimpRef *
_chimp_str_lower (ChimpRef *self, ChimpRef *args)
{
    size_t i;
    char *data;
    size_t len = CHIMP_STR_SIZE(self);
    ChimpRef *str = chimp_str_new (CHIMP_STR_DATA(self), len);

    data = CHIMP_STR_DATA(str);

    for (i = 0; i < len; i++) {
        if (data[i] >= 'A' && data[i] <= 'Z') {
            data[i] += (char) 0x20;
        }
    }

    return str;
}

int
chimp_str_class_init_2 (void)
{
    if (!chimp_class_add_native_method (chimp_str_class, "size", _chimp_str_size)) {
        return CHIMP_FALSE;
    }

    if (!chimp_class_add_native_method (chimp_str_class, "trim", _chimp_str_trim)) {
        return CHIMP_FALSE;
    }

    if (!chimp_class_add_native_method (chimp_str_class, "index", _chimp_str_index)) {
        return CHIMP_FALSE;
    }

    if (!chimp_class_add_native_method (chimp_str_class, "substr", _chimp_str_substr)) {
        return CHIMP_FALSE;
    }

    if (!chimp_class_add_native_method (chimp_str_class, "split", _chimp_str_split)) {
        return CHIMP_FALSE;
    }

    if (!chimp_class_add_native_method (chimp_str_class, "upper", _chimp_str_upper)) {
        return CHIMP_FALSE;
    }

    if (!chimp_class_add_native_method (chimp_str_class, "lower", _chimp_str_lower)) {
        return CHIMP_FALSE;
    }

    return CHIMP_TRUE;
}

