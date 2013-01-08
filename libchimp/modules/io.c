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
#include "chimp/any.h"
#include "chimp/object.h"
#include "chimp/array.h"
#include "chimp/str.h"

typedef struct _ChimpIoFile {
    ChimpAny base;
    FILE *stream;
} ChimpIoFile;

static ChimpRef *chimp_io_file_class = NULL;

#define CHIMP_IO_FILE(ref) \
    CHIMP_CHECK_CAST(ChimpIoFile, (ref), chimp_io_file_class)

static ChimpRef *
_chimp_io_output (ChimpRef *self, ChimpRef *args, char* separator)
{
    size_t i;

    for (i = 0; i < CHIMP_ARRAY_SIZE(args); i++) {
        ChimpRef *str = chimp_object_str (CHIMP_ARRAY_ITEM (args, i));
        printf ("%s%s", CHIMP_STR_DATA(str), separator);
    }

    return chimp_nil;
}

static ChimpRef *
_chimp_io_print (ChimpRef *self, ChimpRef *args)
{
    return _chimp_io_output(self, args, "\n");
}


static ChimpRef *
_chimp_io_write (ChimpRef *self, ChimpRef *args)
{
    return _chimp_io_output(self, args, "");
}


static ChimpRef *
_chimp_io_readline (ChimpRef *self, ChimpRef *args)
{
    char buf[1024];
    size_t len;

    if (fgets (buf, sizeof(buf), stdin) == NULL) {
        return chimp_nil;
    }

    len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') {
        buf[--len] = '\0';
    }
    return chimp_str_new (buf, len);
}

static void
_chimp_io_file_close_internal (ChimpRef *self)
{
    if (CHIMP_IO_FILE(self)->stream != NULL) {
        fclose (CHIMP_IO_FILE(self)->stream);
        CHIMP_IO_FILE(self)->stream = NULL;
    }
}

static ChimpRef *
_chimp_io_file_init (ChimpRef *self, ChimpRef *args)
{
    const char *filename;
    const char *mode;

    if (!chimp_method_parse_args (args, "ss", &filename, &mode)) {
        return NULL;
    }

    CHIMP_IO_FILE(self)->stream = fopen (filename, mode);
    if (CHIMP_IO_FILE(self)->stream == NULL) {
        CHIMP_BUG ("could not open %s\n", filename);
        return NULL;
    }

    return self;
}

static void
_chimp_io_file_dtor (ChimpRef *self)
{
    _chimp_io_file_close_internal (self);
}

static ChimpRef *
_chimp_io_file_close (ChimpRef *self, ChimpRef *args)
{
    _chimp_io_file_close_internal (self);
    return chimp_nil;
}

static ChimpRef *
_chimp_io_file_read (ChimpRef *self, ChimpRef *args)
{
    int64_t size;
    int64_t rsize;
    char *buf;

    if (!chimp_method_parse_args (args, "I", &size)) {
        return NULL;
    }

    if (CHIMP_IO_FILE(self)->stream == NULL) {
        CHIMP_BUG ("attempt to read from closed stream");
        return NULL;
    }

    buf = malloc ((size_t) (size + 1));
    if (buf == NULL) {
        CHIMP_BUG ("out of memory");
        return NULL;
    }

    rsize = fread (buf, 1, (size_t) size, CHIMP_IO_FILE(self)->stream);

    if (rsize <= 0) {
        free (buf);
        return CHIMP_STR_NEW("");
    }
    else {
        if (rsize < size) {
            char *temp = realloc (buf, (size_t) (rsize + 1));
            if (temp == NULL) {
                CHIMP_BUG ("out of memory");
                return NULL;
            }
            buf = temp;
        }
        buf[((size_t)rsize)] = '\0';
        return chimp_str_new_take (buf, (size_t) rsize);
    }
}

static ChimpRef *
_chimp_io_file_write (ChimpRef *self, ChimpRef *args)
{
    ChimpRef *s;
    FILE *stream;

    if (!chimp_method_parse_args (args, "o", &s)) {
        return NULL;
    }

    if (CHIMP_IO_FILE(self)->stream == NULL) {
        CHIMP_BUG ("attempt to write to closed stream");
        return NULL;
    }

    if (!chimp_object_instance_of (s, chimp_str_class)) {
        CHIMP_BUG ("io.file.write(...) accepts a single, string arg");
        return NULL;
    }

    stream = CHIMP_IO_FILE(self)->stream;
    return chimp_int_new (
        fwrite (CHIMP_STR_DATA(s), 1, CHIMP_STR_SIZE(s), stream));
}

static ChimpRef *
_chimp_init_io_file_class (void)
{
    ChimpRef *klass =
        chimp_class_new (CHIMP_STR_NEW("io.file"), NULL, sizeof(ChimpIoFile));
    if (klass == NULL) {
        return NULL;
    }
    CHIMP_CLASS(klass)->init = _chimp_io_file_init;
    CHIMP_CLASS(klass)->dtor = _chimp_io_file_dtor;
    if (!chimp_class_add_native_method (klass, "read", _chimp_io_file_read))
        return NULL;
    if (!chimp_class_add_native_method (klass, "write", _chimp_io_file_write))
        return NULL;
    if (!chimp_class_add_native_method (klass, "close", _chimp_io_file_close))
        return NULL;
    return klass;
}

ChimpRef *
chimp_init_io_module (void)
{
    ChimpRef *io;
    ChimpRef *io_file_class;

    io = chimp_module_new_str ("io", NULL);
    if (io == NULL) {
        return NULL;
    }

    if (!chimp_module_add_method_str (io, "print", _chimp_io_print)) {
        return NULL;
    }

    if (!chimp_module_add_method_str (io, "write", _chimp_io_write)) {
        return NULL;
    }

    if (!chimp_module_add_method_str (io, "readline", _chimp_io_readline)) {
        return NULL;
    }

    io_file_class = _chimp_init_io_file_class ();
    if (io_file_class == NULL)
        return NULL;
    chimp_io_file_class = io_file_class;
    
    if (!chimp_module_add_local_str (io, "file", chimp_io_file_class))
        return NULL;

    return io;
}

