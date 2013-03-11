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

#include <stdio.h>
#include <unistd.h>
#include <glob.h>

#include "nim/any.h"
#include "nim/object.h"
#include "nim/array.h"
#include "nim/str.h"

static NimRef *
_nim_os_getenv (NimRef *self, NimRef *args)
{
    const char *value = getenv(NIM_STR_DATA(NIM_ARRAY_ITEM(args, 0)));
    if (value == NULL) {
        return nim_nil;
    }
    else {
        return nim_str_new (value, strlen(value));
    }
}

static NimRef *
_nim_os_sleep (NimRef *ref, NimRef *args)
{
    NimRef *duration = NIM_ARRAY_ITEM(args, 0);
    if (duration == NULL) {
        sleep (0);
    }
    else {
        sleep ((time_t)NIM_INT(duration)->value);
    }
    return nim_nil;
}

static NimRef *
_nim_os_basename (NimRef *self, NimRef *args)
{
    NimRef *path = NIM_ARRAY_ITEM(args, 0);
    size_t len = NIM_STR_SIZE(path);
    const char *s = NIM_STR_DATA(path) + len;

    if (len > 0) len--;
    while (len > 0) {
        if (*s == '/') {
            s++;
            break;
        }
        s--;
        len--;
    }

    return nim_str_new (s, NIM_STR_SIZE(path) - len - 1);
}

static NimRef *
_nim_os_dirname (NimRef *self, NimRef *args)
{
    NimRef *path = NIM_ARRAY_ITEM(args, 0);
    size_t i;
    const char *begin = NIM_STR_DATA(path);
    const char *end = begin + NIM_STR_SIZE(path);

    for (i = 0; i < NIM_STR_SIZE(path); i++) {
        end--;
        if (*end == '/') {
            return nim_str_new (begin, (size_t) (end - begin));
        }
    }
    return NIM_STR_NEW (".");
}

static NimRef *
_nim_os_glob (NimRef *self, NimRef *args)
{
    const char *pattern;
    glob_t res;
    NimRef *arr;
    size_t i;

    if (!nim_method_parse_args (args, "s", &pattern)) {
        return NULL;
    }

    if (glob (pattern, 0, NULL, &res) != 0) {
        NIM_BUG ("glob() failed");
        return NULL;
    }

    arr = nim_array_new_with_capacity (res.gl_pathc);
    if (arr == NULL) {
        globfree (&res);
        return NULL;
    }

    for (i = 0; i < res.gl_pathc; i++) {
        NIM_ARRAY(arr)->items[i] =
            nim_str_new (res.gl_pathv[i], strlen (res.gl_pathv[i]));
        if (NIM_ARRAY(arr)->items[i] == NULL) {
            return NULL;
        }
    }
    NIM_ARRAY(arr)->size = res.gl_pathc;

    globfree (&res);

    return arr;
}

static NimRef *
_nim_os_setuid (NimRef *self, NimRef *args)
{
    int64_t uid;

    if (!nim_method_parse_args (args, "I", &uid)) {
        return NULL;
    }

    if (setuid ((uid_t) uid) != 0) {
        return nim_false;
    }
    else {
        return nim_true;
    }
}

static NimRef *
_nim_os_setgid (NimRef *self, NimRef *args)
{
    int64_t gid;

    if (!nim_method_parse_args (args, "I", &gid)) {
        return NULL;
    }

    if (setgid ((gid_t) gid) != 0) {
        return nim_false;
    }
    else {
        return nim_true;
    }
}

NimRef *
nim_init_os_module (void)
{
    NimRef *os;

    os = nim_module_new_str ("os", NULL);
    if (os == NULL) {
        return NULL;
    }

    if (!nim_module_add_method_str (os, "getenv", _nim_os_getenv)) {
        return NULL;
    }

    if (!nim_module_add_method_str (os, "sleep", _nim_os_sleep)) {
        return NULL;
    }

    if (!nim_module_add_method_str (os, "basename", _nim_os_basename)) {
        return NULL;
    }

    if (!nim_module_add_method_str (os, "dirname", _nim_os_dirname)) {
        return NULL;
    }

    if (!nim_module_add_method_str (os, "glob", _nim_os_glob)) {
        return NULL;
    }

    if (!nim_module_add_method_str (os, "setuid", _nim_os_setuid)) {
        return NULL;
    }

    if (!nim_module_add_method_str (os, "setgid", _nim_os_setgid)) {
        return NULL;
    }

    return os;
}

