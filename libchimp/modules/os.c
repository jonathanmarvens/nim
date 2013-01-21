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

#include "chimp/any.h"
#include "chimp/object.h"
#include "chimp/array.h"
#include "chimp/str.h"

static ChimpRef *
_chimp_os_getenv (ChimpRef *self, ChimpRef *args)
{
    const char *value = getenv(CHIMP_STR_DATA(CHIMP_ARRAY_ITEM(args, 0)));
    if (value == NULL) {
        return chimp_nil;
    }
    else {
        return chimp_str_new (value, strlen(value));
    }
}

static ChimpRef *
_chimp_os_sleep (ChimpRef *ref, ChimpRef *args)
{
    ChimpRef *duration = CHIMP_ARRAY_ITEM(args, 0);
    if (duration == NULL) {
        sleep (0);
    }
    else {
        sleep ((time_t)CHIMP_INT(duration)->value);
    }
    return chimp_nil;
}

static ChimpRef *
_chimp_os_basename (ChimpRef *self, ChimpRef *args)
{
    ChimpRef *path = CHIMP_ARRAY_ITEM(args, 0);
    size_t len = CHIMP_STR_SIZE(path);
    const char *s = CHIMP_STR_DATA(path) + len;

    if (len > 0) len--;
    while (len > 0) {
        if (*s == '/') {
            s++;
            break;
        }
        s--;
        len--;
    }

    return chimp_str_new (s, CHIMP_STR_SIZE(path) - len);
}

static ChimpRef *
_chimp_os_dirname (ChimpRef *self, ChimpRef *args)
{
    ChimpRef *path = CHIMP_ARRAY_ITEM(args, 0);
    size_t i;
    const char *begin = CHIMP_STR_DATA(path);
    const char *end = begin + CHIMP_STR_SIZE(path);

    for (i = 0; i < CHIMP_STR_SIZE(path); i++) {
        end--;
        if (*end == '/') {
            return chimp_str_new (begin, (size_t) (end - begin));
        }
    }
    return CHIMP_STR_NEW (".");
}

static ChimpRef *
_chimp_os_glob (ChimpRef *self, ChimpRef *args)
{
    const char *pattern;
    glob_t res;
    ChimpRef *arr;
    size_t i;

    if (!chimp_method_parse_args (args, "s", &pattern)) {
        return NULL;
    }

    if (glob (pattern, 0, NULL, &res) != 0) {
        CHIMP_BUG ("glob() failed");
        return NULL;
    }

    arr = chimp_array_new_with_capacity (res.gl_pathc);
    if (arr == NULL) {
        globfree (&res);
        return NULL;
    }

    for (i = 0; i < res.gl_pathc; i++) {
        CHIMP_ARRAY(arr)->items[i] =
            chimp_str_new (res.gl_pathv[i], strlen (res.gl_pathv[i]));
        if (CHIMP_ARRAY(arr)->items[i] == NULL) {
            return NULL;
        }
    }
    CHIMP_ARRAY(arr)->size = res.gl_pathc;

    globfree (&res);

    return arr;
}

static ChimpRef *
_chimp_os_setuid (ChimpRef *self, ChimpRef *args)
{
    int64_t uid;

    if (!chimp_method_parse_args (args, "I", &uid)) {
        return NULL;
    }

    if (setuid ((uid_t) uid) != 0) {
        return chimp_false;
    }
    else {
        return chimp_true;
    }
}

static ChimpRef *
_chimp_os_setgid (ChimpRef *self, ChimpRef *args)
{
    int64_t gid;

    if (!chimp_method_parse_args (args, "I", &gid)) {
        return NULL;
    }

    if (setgid ((gid_t) gid) != 0) {
        return chimp_false;
    }
    else {
        return chimp_true;
    }
}

ChimpRef *
chimp_init_os_module (void)
{
    ChimpRef *os;

    os = chimp_module_new_str ("os", NULL);
    if (os == NULL) {
        return NULL;
    }

    if (!chimp_module_add_method_str (os, "getenv", _chimp_os_getenv)) {
        return NULL;
    }

    if (!chimp_module_add_method_str (os, "sleep", _chimp_os_sleep)) {
        return NULL;
    }

    if (!chimp_module_add_method_str (os, "basename", _chimp_os_basename)) {
        return NULL;
    }

    if (!chimp_module_add_method_str (os, "dirname", _chimp_os_dirname)) {
        return NULL;
    }

    if (!chimp_module_add_method_str (os, "glob", _chimp_os_glob)) {
        return NULL;
    }

    if (!chimp_module_add_method_str (os, "setuid", _chimp_os_setuid)) {
        return NULL;
    }

    if (!chimp_module_add_method_str (os, "setgid", _chimp_os_setgid)) {
        return NULL;
    }

    return os;
}

