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

