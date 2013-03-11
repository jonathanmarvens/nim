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
#include "http_parser.h"

typedef struct _NimHttpParser {
    NimAny base;
    http_parser impl;
    http_parser_settings conf;
    NimRef *request;
    NimRef *header;
    nim_bool_t complete;
} NimHttpParser;

typedef struct _NimHttpRequest {
    NimAny base;
    NimRef *method;
    NimRef *http_version;
    NimRef *url;
    NimRef *headers;
    NimRef *body;
} NimHttpRequest;

static NimRef *nim_http_parser_class = NULL;
static NimRef *nim_http_request_class = NULL;

#define NIM_HTTP_PARSER(ref) \
    NIM_CHECK_CAST(NimHttpParser, (ref), nim_http_parser_class)

#define NIM_HTTP_REQUEST(ref) \
    NIM_CHECK_CAST(NimHttpRequest, (ref), nim_http_request_class)

static int
_nim_http_parser_on_message_begin (http_parser *p)
{
    NimRef *self = p->data;
    NimRef *req;
    req = nim_class_new_instance (nim_http_request_class, NULL);
    if (req == NULL) {
        NIM_BUG ("http.request instantiation failed");
        return 1;
    }
    NIM_HTTP_PARSER(self)->request = req;
    return 0;
}

static int
_nim_http_parser_on_url (http_parser *p, const char *data, size_t len)
{
    NimRef *req = NIM_HTTP_PARSER(p->data)->request;
    NIM_HTTP_REQUEST(req)->url = nim_str_new (data, len);
    if (NIM_HTTP_REQUEST(req)->url == NULL) {
        NIM_BUG ("failed to set request url");
        return 1;
    }
    return 0;
}

static int
_nim_http_parser_on_header_field (
        http_parser *p, const char *data, size_t len)
{
    NIM_HTTP_PARSER(p->data)->header = nim_str_new (data, len);
    if (NIM_HTTP_PARSER(p->data)->header == NULL) {
        NIM_BUG ("failed to allocate header");
        return 1;
    }
    return 0;
}

static int
_nim_http_parser_on_header_value (
        http_parser *p, const char *data, size_t len)
{
    NimRef *req;
    NimRef *values;
    NimRef *headers;
    NimRef *header;
    NimRef *self = p->data;
    NimRef *value = nim_str_new (data, len);
    if (value == NULL) {
        NIM_BUG ("failed to allocate header value");
        return 1;
    }
    
    req = NIM_HTTP_PARSER(self)->request;
    headers = NIM_HTTP_REQUEST(req)->headers;
    header = NIM_HTTP_PARSER(self)->header;
    if (nim_hash_get (headers, header, &values) < 0) {
        NIM_BUG ("failed to get header");
        return 1;
    }
    if (values != nim_nil) {
        if (NIM_ANY_CLASS(values) == nim_array_class) {
            if (!nim_array_push (values, value)) {
                NIM_BUG ("failed to push value onto header array");
                return 1;
            }
        }
        else if (NIM_ANY_CLASS(values) == nim_str_class) {
            values = nim_array_new_var (values, value, NULL);
            if (values == NULL) {
                NIM_BUG ("failed to convert header into array");
                return 1;
            }
            if (!nim_hash_put (headers, header, values)) {
                NIM_BUG ("failed to set header");
                return 1;
            }
        }
        else {
            NIM_BUG ("unknown/unexpected type for header values");
            return 1;
        }
    }
    else {
        if (!nim_hash_put (headers, header, value)) {
            NIM_BUG ("failed to set header");
            return 1;
        }
    }
    NIM_HTTP_PARSER(self)->header = NULL;
    return 0;
}

static int
_nim_http_parser_on_headers_complete (http_parser *p)
{
    NimRef *self = p->data;
    NIM_HTTP_PARSER(self)->header = NULL;
    return 0;
}

static int
_nim_http_parser_on_body (http_parser *p, const char *data, size_t len)
{
    NimRef *self = p->data;
    NimRef *req = NIM_HTTP_PARSER(self)->request;
    if (NIM_HTTP_REQUEST(req)->body == NULL) {
        NIM_HTTP_REQUEST(req)->body = nim_str_new (data, len);
        if (NIM_HTTP_REQUEST(req)->body == NULL) {
            NIM_BUG ("failed to allocate body");
            return 1;
        }
    }
    else {
        if (!nim_str_append_strn (NIM_HTTP_REQUEST(req)->body, data, len)) {
            NIM_BUG ("failed to append body content");
            return 1;
        }
    }
    return 0;
}

static int
_nim_http_parser_on_message_complete (http_parser *p)
{
    const char *method_str = http_method_str (p->method);
    NimRef *req = NIM_HTTP_PARSER(p->data)->request;
    NIM_HTTP_REQUEST(req)->method =
        nim_str_new (method_str, strlen (method_str));
    if (NIM_HTTP_REQUEST(req)->method == NULL) {
        NIM_BUG ("failed to allocate method");
        return 1;
    }
    NIM_HTTP_REQUEST(req)->http_version = nim_array_new_var (
            nim_int_new (p->http_major),
            nim_int_new (p->http_minor),
            NULL
        );
    if (NIM_HTTP_REQUEST(req)->http_version == NULL) {
        NIM_BUG ("failed to allocate version");
        return 1;
    }
    NIM_HTTP_PARSER(p->data)->complete = NIM_TRUE;
    return 0;
}

static NimRef *
_nim_http_parser_init (NimRef *self, NimRef *args)
{
    http_parser_settings *conf;
    http_parser_init (&NIM_HTTP_PARSER(self)->impl, HTTP_REQUEST);
    NIM_HTTP_PARSER(self)->impl.data = self;
    conf = &NIM_HTTP_PARSER(self)->conf;
    memset (conf, 0, sizeof(*conf));
    conf->on_message_begin = _nim_http_parser_on_message_begin;
    conf->on_url = _nim_http_parser_on_url;
    conf->on_header_field = _nim_http_parser_on_header_field;
    conf->on_header_value = _nim_http_parser_on_header_value;
    conf->on_headers_complete = _nim_http_parser_on_headers_complete;
    conf->on_body = _nim_http_parser_on_body;
    conf->on_message_complete = _nim_http_parser_on_message_complete;
    return self;
}

static void
_nim_http_parser_dtor (NimRef *self)
{
}

static void
_nim_http_parser_mark (NimGC *gc, NimRef *self)
{
    nim_gc_mark_ref (gc, NIM_HTTP_PARSER(self)->request);
    nim_gc_mark_ref (gc, NIM_HTTP_PARSER(self)->header);
}

static NimRef *
_nim_http_parser_parse_request (NimRef *self, NimRef *args)
{
    NimRef *size;
    NimRef *recv_args;
    NimRef *data;
    NimRef *req;
    http_parser *p = &NIM_HTTP_PARSER(self)->impl;
    http_parser_settings *conf = &NIM_HTTP_PARSER(self)->conf;
    NimRef *sck;
    
    if (!nim_method_parse_args (args, "o", &sck)) {
        return NULL;
    }
    
    size = nim_int_new (8192);
    if (size == NULL) {
        return NULL;
    }
    recv_args = nim_array_new_var (size, NULL);
    if (recv_args == NULL) {
        return NULL;
    }
    while (!NIM_HTTP_PARSER(self)->complete) {
        size_t size;
        data = nim_object_call_method (sck, "recv", recv_args);
        if (data == NULL) {
            return NULL;
        }
        else if (NIM_STR_SIZE(data) == 0) {
            http_parser_execute (p, conf, "", 0);
            if (HTTP_PARSER_ERRNO(p) != HPE_OK) {
                req = NULL;
                NIM_BUG (http_errno_description (HTTP_PARSER_ERRNO(p)));
                goto done;
            }
            break;
        }
        size = NIM_STR_SIZE(data);
        http_parser_execute (p, conf, NIM_STR_DATA(data), size); 
        if (HTTP_PARSER_ERRNO(p) != HPE_OK) {
            req = NULL;
            NIM_BUG (http_errno_description (HTTP_PARSER_ERRNO(p)));
            goto done;
        }
    }
    req = NIM_HTTP_PARSER(self)->request;
done:
    NIM_HTTP_PARSER(self)->request = NULL;
    NIM_HTTP_PARSER(self)->header = NULL;
    NIM_HTTP_PARSER(self)->complete = NIM_FALSE;
    return (req == NULL) ? nim_nil : req;
}

static NimRef *
_nim_init_http_parser_class (void)
{
    NimRef *klass = nim_class_new (
            NIM_STR_NEW("http.parser"), NULL, sizeof(NimHttpParser));
    if (klass == NULL) {
        return NULL;
    }
    NIM_CLASS(klass)->init = _nim_http_parser_init;
    NIM_CLASS(klass)->dtor = _nim_http_parser_dtor;
    NIM_CLASS(klass)->mark = _nim_http_parser_mark;
    if (!nim_class_add_native_method (
            klass, "parse_request", _nim_http_parser_parse_request)) {
        return NULL;
    }
    return klass;
}

static NimRef *
_nim_http_request_init (NimRef *self, NimRef *args)
{
    NIM_HTTP_REQUEST(self)->headers = nim_hash_new ();
    NIM_HTTP_REQUEST(self)->body = nim_str_new ("", 0);
    return self;
}

static void
_nim_http_request_dtor (NimRef *self)
{
}

static void
_nim_http_request_mark (NimGC *gc, NimRef *self)
{
    nim_gc_mark_ref (gc, NIM_HTTP_REQUEST(self)->method);
    nim_gc_mark_ref (gc, NIM_HTTP_REQUEST(self)->http_version);
    nim_gc_mark_ref (gc, NIM_HTTP_REQUEST(self)->url);
    nim_gc_mark_ref (gc, NIM_HTTP_REQUEST(self)->headers);
    nim_gc_mark_ref (gc, NIM_HTTP_REQUEST(self)->body);
}

static NimRef *
_nim_http_request_getattr (NimRef *self, NimRef *attr)
{
    if (strcmp (NIM_STR_DATA(attr), "method") == 0) {
        return NIM_HTTP_REQUEST(self)->method;
    }
    else if (strcmp (NIM_STR_DATA(attr), "url") == 0) {
        return NIM_HTTP_REQUEST(self)->url;
    }
    else if (strcmp (NIM_STR_DATA(attr), "headers") == 0) {
        return NIM_HTTP_REQUEST(self)->headers;
    }
    else if (strcmp (NIM_STR_DATA(attr), "body") == 0) {
        return NIM_HTTP_REQUEST(self)->body;
    }
    else if (strcmp (NIM_STR_DATA(attr), "version") == 0) {
        return NIM_HTTP_REQUEST(self)->http_version;
    }
    else {
        NIM_BUG (
            "unknown attribute %s on http.request", NIM_STR_DATA(attr));
        return NULL;
    }
}

static NimRef *
_nim_http_request_str (NimRef *self)
{
  NimRef *method = NIM_HTTP_REQUEST(self)->method;
  NimRef *url = NIM_HTTP_REQUEST(self)->url;
  NimRef *headers = NIM_HTTP_REQUEST(self)->headers;
  NimRef *body = NIM_HTTP_REQUEST(self)->body;
  return nim_str_new_format (
      "http.request(method=%s, url=\"%s\", headers=%s, body=\"%s\")",
      method == NULL ? "<null>" : NIM_STR_DATA(method),
      url == NULL ? "<null>" : NIM_STR_DATA(url),
      headers == NULL ? "<null>" : NIM_STR_DATA(nim_object_str (headers)),
      body == NULL ? "<null>" : NIM_STR_DATA(body));
}

static NimRef *
_nim_init_http_request_class (void)
{
    NimRef *klass = nim_class_new (
            NIM_STR_NEW("http.request"), NULL, sizeof(NimHttpRequest));
    if (klass == NULL) {
        return NULL;
    }
    NIM_CLASS(klass)->init = _nim_http_request_init;
    NIM_CLASS(klass)->dtor = _nim_http_request_dtor;
    NIM_CLASS(klass)->mark = _nim_http_request_mark;
    NIM_CLASS(klass)->getattr = _nim_http_request_getattr;
    NIM_CLASS(klass)->str = _nim_http_request_str;
    return klass;
}

NimRef *
nim_init_http_module (void)
{
    NimRef *http;
    NimRef *http_parser_class;
    NimRef *http_request_class;

    http = nim_module_new_str ("http", NULL);
    if (http == NULL) {
        return NULL;
    }

    http_parser_class = _nim_init_http_parser_class ();
    if (!nim_module_add_local_str (http, "parser", http_parser_class)) {
        return NULL;
    }
    nim_http_parser_class = http_parser_class;

    http_request_class = _nim_init_http_request_class ();
    if (!nim_module_add_local_str (http, "request", http_request_class)) {
        return NULL;
    }
    nim_http_request_class = http_request_class;

    return http;
}

