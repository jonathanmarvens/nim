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

#include "chimp/class.h"
#include "chimp/object.h"
#include "chimp/str.h"
#include "chimp/error.h"

ChimpRef *chimp_error_class = NULL;

static ChimpRef *
_chimp_error_init (ChimpRef *self, ChimpRef *args)
{
    ChimpRef *message = NULL;
    if (!chimp_method_parse_args (args, "|o", &message)) {
        return NULL;
    }
    CHIMP_ERROR(self)->message = message;
    return self;
}

static ChimpRef *
_chimp_error_str (ChimpRef *self)
{
    ChimpRef *message = CHIMP_ERROR(self)->message;
    if (message != NULL) {
        return chimp_str_new_format ("<error '%s'>", CHIMP_STR_DATA(message));
    }
    else {
        return CHIMP_STR_NEW ("<error>");
    }
}

static void
_chimp_error_mark (ChimpGC *gc, ChimpRef *self)
{
    CHIMP_SUPER (self)->mark (gc, self);

    chimp_gc_mark_ref (gc, CHIMP_ERROR(self)->message);
    chimp_gc_mark_ref (gc, CHIMP_ERROR(self)->cause);
    chimp_gc_mark_ref (gc, CHIMP_ERROR(self)->backtrace);
}

chimp_bool_t
chimp_error_class_bootstrap (void)
{
    chimp_error_class =
        chimp_class_new (CHIMP_STR_NEW("error"), NULL, sizeof(ChimpError));
    if (chimp_error_class == NULL) {
        return CHIMP_FALSE;
    }
    CHIMP_CLASS(chimp_error_class)->str = _chimp_error_str;
    CHIMP_CLASS(chimp_error_class)->init = _chimp_error_init;
    CHIMP_CLASS(chimp_error_class)->mark = _chimp_error_mark;
    chimp_gc_make_root (NULL, chimp_error_class);
    return CHIMP_TRUE;
}

ChimpRef *
chimp_error_new (ChimpRef *message)
{
    return chimp_class_new_instance (chimp_error_class, message, NULL);
}

ChimpRef *
chimp_error_new_with_format (const char *fmt, ...)
{
    va_list args;
    ChimpRef *message;

    va_start (args, fmt);
    message = chimp_str_new_formatv (fmt, args);
    va_end (args);
    if (message == NULL) {
        return NULL;
    }

    return chimp_error_new (message);
}

