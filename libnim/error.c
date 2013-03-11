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

#include "nim/class.h"
#include "nim/object.h"
#include "nim/str.h"
#include "nim/error.h"

NimRef *nim_error_class = NULL;

static NimRef *
_nim_error_init (NimRef *self, NimRef *args)
{
    NimRef *message = NULL;
    if (!nim_method_parse_args (args, "|o", &message)) {
        return NULL;
    }
    NIM_ERROR(self)->message = message;
    return self;
}

static NimRef *
_nim_error_str (NimRef *self)
{
    NimRef *message = NIM_ERROR(self)->message;
    if (message != NULL) {
        return nim_str_new_format ("<error '%s'>", NIM_STR_DATA(message));
    }
    else {
        return NIM_STR_NEW ("<error>");
    }
}

static void
_nim_error_mark (NimGC *gc, NimRef *self)
{
    NIM_SUPER (self)->mark (gc, self);

    nim_gc_mark_ref (gc, NIM_ERROR(self)->message);
    nim_gc_mark_ref (gc, NIM_ERROR(self)->cause);
    nim_gc_mark_ref (gc, NIM_ERROR(self)->backtrace);
}

nim_bool_t
nim_error_class_bootstrap (void)
{
    nim_error_class =
        nim_class_new (NIM_STR_NEW("error"), NULL, sizeof(NimError));
    if (nim_error_class == NULL) {
        return NIM_FALSE;
    }
    NIM_CLASS(nim_error_class)->str = _nim_error_str;
    NIM_CLASS(nim_error_class)->init = _nim_error_init;
    NIM_CLASS(nim_error_class)->mark = _nim_error_mark;
    nim_gc_make_root (NULL, nim_error_class);
    return NIM_TRUE;
}

NimRef *
nim_error_new (NimRef *message)
{
    return nim_class_new_instance (nim_error_class, message, NULL);
}

NimRef *
nim_error_new_with_format (const char *fmt, ...)
{
    va_list args;
    NimRef *message;

    va_start (args, fmt);
    message = nim_str_new_formatv (fmt, args);
    va_end (args);
    if (message == NULL) {
        return NULL;
    }

    return nim_error_new (message);
}

