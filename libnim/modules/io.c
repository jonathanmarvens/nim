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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "nim/any.h"
#include "nim/object.h"
#include "nim/array.h"
#include "nim/str.h"

typedef struct _NimIoFile {
    NimAny base;
    FILE *stream;
} NimIoFile;

static NimRef *nim_io_file_class = NULL;

#define NIM_IO_FILE(ref) \
    NIM_CHECK_CAST(NimIoFile, (ref), nim_io_file_class)

static NimRef *
_nim_io_output (NimRef *self, NimRef *args, char* separator)
{
    size_t i;

    for (i = 0; i < NIM_ARRAY_SIZE(args); i++) {
        NimRef *str = nim_object_str (NIM_ARRAY_ITEM (args, i));
        printf ("%s%s", NIM_STR_DATA(str), separator);
    }

    return nim_nil;
}

static NimRef *
_nim_io_print (NimRef *self, NimRef *args)
{
    return _nim_io_output(self, args, "\n");
}


static NimRef *
_nim_io_write (NimRef *self, NimRef *args)
{
    return _nim_io_output(self, args, "");
}


static NimRef *
_nim_io_readline (NimRef *self, NimRef *args)
{
    char buf[1024];
    size_t len;

    if (fgets (buf, sizeof(buf), stdin) == NULL) {
        return nim_nil;
    }

    len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') {
        buf[--len] = '\0';
    }
    return nim_str_new (buf, len);
}

static void
_nim_io_file_close_internal (NimRef *self)
{
    if (NIM_IO_FILE(self)->stream != NULL) {
        fclose (NIM_IO_FILE(self)->stream);
        NIM_IO_FILE(self)->stream = NULL;
    }
}

static NimRef *
_nim_io_file_init (NimRef *self, NimRef *args)
{
    const char *filename;
    const char *mode;

    if (!nim_method_parse_args (args, "ss", &filename, &mode)) {
        return NULL;
    }

    NIM_IO_FILE(self)->stream = fopen (filename, mode);
    if (NIM_IO_FILE(self)->stream == NULL) {
        NIM_BUG ("could not open %s\n", filename);
        return NULL;
    }

    return self;
}

static void
_nim_io_file_dtor (NimRef *self)
{
    _nim_io_file_close_internal (self);
}

static NimRef *
_nim_io_file_close (NimRef *self, NimRef *args)
{
    _nim_io_file_close_internal (self);
    return nim_nil;
}

static NimRef *
_nim_io_file_read (NimRef *self, NimRef *args)
{
    int64_t size;
    int64_t rsize;
    char *buf;

    if (NIM_ARRAY_SIZE(args) > 0) {
        if (!nim_method_parse_args (args, "I", &size)) {
            return NULL;
        }
    }
    else {
        struct stat buf;
        int fd = fileno (NIM_IO_FILE(self)->stream);
        if (fd < 0) {
            NIM_BUG ("fileno (...) failed");
            return NULL;
        }
        if (fstat (fd, &buf) != 0) {
            NIM_BUG ("fstat (...) failed");
            return NULL;
        }
        size = buf.st_size;
    }

    if (NIM_IO_FILE(self)->stream == NULL) {
        NIM_BUG ("attempt to read from closed stream");
        return NULL;
    }

    buf = malloc ((size_t) (size + 1));
    if (buf == NULL) {
        NIM_BUG ("out of memory");
        return NULL;
    }

    rsize = fread (buf, 1, (size_t) size, NIM_IO_FILE(self)->stream);

    if (rsize <= 0) {
        free (buf);
        return NIM_STR_NEW("");
    }
    else {
        if (rsize < size) {
            char *temp = realloc (buf, (size_t) (rsize + 1));
            if (temp == NULL) {
                NIM_BUG ("out of memory");
                return NULL;
            }
            buf = temp;
        }
        buf[((size_t)rsize)] = '\0';
        return nim_str_new_take (buf, (size_t) rsize);
    }
}

static NimRef *
_nim_io_file_write (NimRef *self, NimRef *args)
{
    NimRef *s;
    FILE *stream;

    if (!nim_method_parse_args (args, "o", &s)) {
        return NULL;
    }

    if (NIM_IO_FILE(self)->stream == NULL) {
        NIM_BUG ("attempt to write to closed stream");
        return NULL;
    }

    if (!nim_object_instance_of (s, nim_str_class)) {
        NIM_BUG ("io.file.write(...) accepts a single, string arg");
        return NULL;
    }

    stream = NIM_IO_FILE(self)->stream;
    return nim_int_new (
        fwrite (NIM_STR_DATA(s), 1, NIM_STR_SIZE(s), stream));
}

static NimRef *
_nim_init_io_file_class (void)
{
    NimRef *klass =
        nim_class_new (NIM_STR_NEW("io.file"), NULL, sizeof(NimIoFile));
    if (klass == NULL) {
        return NULL;
    }
    NIM_CLASS(klass)->init = _nim_io_file_init;
    NIM_CLASS(klass)->dtor = _nim_io_file_dtor;
    if (!nim_class_add_native_method (klass, "read", _nim_io_file_read))
        return NULL;
    if (!nim_class_add_native_method (klass, "write", _nim_io_file_write))
        return NULL;
    if (!nim_class_add_native_method (klass, "close", _nim_io_file_close))
        return NULL;
    return klass;
}

NimRef *
nim_init_io_module (void)
{
    NimRef *io;
    NimRef *io_file_class;

    io = nim_module_new_str ("io", NULL);
    if (io == NULL) {
        return NULL;
    }

    if (!nim_module_add_method_str (io, "print", _nim_io_print)) {
        return NULL;
    }

    if (!nim_module_add_method_str (io, "write", _nim_io_write)) {
        return NULL;
    }

    if (!nim_module_add_method_str (io, "readline", _nim_io_readline)) {
        return NULL;
    }

    io_file_class = _nim_init_io_file_class ();
    if (io_file_class == NULL)
        return NULL;
    nim_io_file_class = io_file_class;
    
    if (!nim_module_add_local_str (io, "file", nim_io_file_class))
        return NULL;

    return io;
}

