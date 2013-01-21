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
#include "http_parser.h"

typedef struct _ChimpHttpParser {
    ChimpAny base;
    http_parser impl;
    http_parser_settings conf;
    ChimpRef *request;
    ChimpRef *header;
    chimp_bool_t complete;
} ChimpHttpParser;

typedef struct _ChimpHttpRequest {
    ChimpAny base;
    ChimpRef *method;
    ChimpRef *http_version;
    ChimpRef *url;
    ChimpRef *headers;
    ChimpRef *body;
} ChimpHttpRequest;

static ChimpRef *chimp_http_parser_class = NULL;
static ChimpRef *chimp_http_request_class = NULL;

#define CHIMP_HTTP_PARSER(ref) \
    CHIMP_CHECK_CAST(ChimpHttpParser, (ref), chimp_http_parser_class)

#define CHIMP_HTTP_REQUEST(ref) \
    CHIMP_CHECK_CAST(ChimpHttpRequest, (ref), chimp_http_request_class)

static int
_chimp_http_parser_on_message_begin (http_parser *p)
{
    ChimpRef *self = p->data;
    ChimpRef *req;
    req = chimp_class_new_instance (chimp_http_request_class, NULL);
    if (req == NULL) {
        CHIMP_BUG ("http.request instantiation failed");
        return 1;
    }
    CHIMP_HTTP_PARSER(self)->request = req;
    return 0;
}

static int
_chimp_http_parser_on_url (http_parser *p, const char *data, size_t len)
{
    ChimpRef *req = CHIMP_HTTP_PARSER(p->data)->request;
    CHIMP_HTTP_REQUEST(req)->url = chimp_str_new (data, len);
    if (CHIMP_HTTP_REQUEST(req)->url == NULL) {
        CHIMP_BUG ("failed to set request url");
        return 1;
    }
    return 0;
}

static int
_chimp_http_parser_on_header_field (
        http_parser *p, const char *data, size_t len)
{
    CHIMP_HTTP_PARSER(p->data)->header = chimp_str_new (data, len);
    if (CHIMP_HTTP_PARSER(p->data)->header == NULL) {
        CHIMP_BUG ("failed to allocate header");
        return 1;
    }
    return 0;
}

static int
_chimp_http_parser_on_header_value (
        http_parser *p, const char *data, size_t len)
{
    ChimpRef *req;
    ChimpRef *values;
    ChimpRef *headers;
    ChimpRef *header;
    ChimpRef *self = p->data;
    ChimpRef *value = chimp_str_new (data, len);
    if (value == NULL) {
        CHIMP_BUG ("failed to allocate header value");
        return 1;
    }
    
    req = CHIMP_HTTP_PARSER(self)->request;
    headers = CHIMP_HTTP_REQUEST(req)->headers;
    header = CHIMP_HTTP_PARSER(self)->header;
    if (chimp_hash_get (headers, header, &values) < 0) {
        CHIMP_BUG ("failed to get header");
        return 1;
    }
    if (values != chimp_nil) {
        if (CHIMP_ANY_CLASS(values) == chimp_array_class) {
            if (!chimp_array_push (values, value)) {
                CHIMP_BUG ("failed to push value onto header array");
                return 1;
            }
        }
        else if (CHIMP_ANY_CLASS(values) == chimp_str_class) {
            values = chimp_array_new_var (values, value, NULL);
            if (values == NULL) {
                CHIMP_BUG ("failed to convert header into array");
                return 1;
            }
            if (!chimp_hash_put (headers, header, values)) {
                CHIMP_BUG ("failed to set header");
                return 1;
            }
        }
        else {
            CHIMP_BUG ("unknown/unexpected type for header values");
            return 1;
        }
    }
    else {
        if (!chimp_hash_put (headers, header, value)) {
            CHIMP_BUG ("failed to set header");
            return 1;
        }
    }
    CHIMP_HTTP_PARSER(self)->header = NULL;
    return 0;
}

static int
_chimp_http_parser_on_headers_complete (http_parser *p)
{
    ChimpRef *self = p->data;
    CHIMP_HTTP_PARSER(self)->header = NULL;
    return 0;
}

static int
_chimp_http_parser_on_body (http_parser *p, const char *data, size_t len)
{
    ChimpRef *self = p->data;
    ChimpRef *req = CHIMP_HTTP_PARSER(self)->request;
    if (CHIMP_HTTP_REQUEST(req)->body == NULL) {
        CHIMP_HTTP_REQUEST(req)->body = chimp_str_new (data, len);
        if (CHIMP_HTTP_REQUEST(req)->body == NULL) {
            CHIMP_BUG ("failed to allocate body");
            return 1;
        }
    }
    else {
        if (!chimp_str_append_strn (CHIMP_HTTP_REQUEST(req)->body, data, len)) {
            CHIMP_BUG ("failed to append body content");
            return 1;
        }
    }
    return 0;
}

static int
_chimp_http_parser_on_message_complete (http_parser *p)
{
    const char *method_str = http_method_str (p->method);
    ChimpRef *req = CHIMP_HTTP_PARSER(p->data)->request;
    CHIMP_HTTP_REQUEST(req)->method =
        chimp_str_new (method_str, strlen (method_str));
    if (CHIMP_HTTP_REQUEST(req)->method == NULL) {
        CHIMP_BUG ("failed to allocate method");
        return 1;
    }
    CHIMP_HTTP_REQUEST(req)->http_version = chimp_array_new_var (
            chimp_int_new (p->http_major),
            chimp_int_new (p->http_minor),
            NULL
        );
    if (CHIMP_HTTP_REQUEST(req)->http_version == NULL) {
        CHIMP_BUG ("failed to allocate version");
        return 1;
    }
    CHIMP_HTTP_PARSER(p->data)->complete = CHIMP_TRUE;
    return 0;
}

static ChimpRef *
_chimp_http_parser_init (ChimpRef *self, ChimpRef *args)
{
    http_parser_settings *conf;
    http_parser_init (&CHIMP_HTTP_PARSER(self)->impl, HTTP_REQUEST);
    CHIMP_HTTP_PARSER(self)->impl.data = self;
    conf = &CHIMP_HTTP_PARSER(self)->conf;
    memset (conf, 0, sizeof(*conf));
    conf->on_message_begin = _chimp_http_parser_on_message_begin;
    conf->on_url = _chimp_http_parser_on_url;
    conf->on_header_field = _chimp_http_parser_on_header_field;
    conf->on_header_value = _chimp_http_parser_on_header_value;
    conf->on_headers_complete = _chimp_http_parser_on_headers_complete;
    conf->on_body = _chimp_http_parser_on_body;
    conf->on_message_complete = _chimp_http_parser_on_message_complete;
    return self;
}

static void
_chimp_http_parser_dtor (ChimpRef *self)
{
}

static void
_chimp_http_parser_mark (ChimpGC *gc, ChimpRef *self)
{
    chimp_gc_mark_ref (gc, CHIMP_HTTP_PARSER(self)->request);
    chimp_gc_mark_ref (gc, CHIMP_HTTP_PARSER(self)->header);
}

static ChimpRef *
_chimp_http_parser_parse_request (ChimpRef *self, ChimpRef *args)
{
    ChimpRef *size;
    ChimpRef *recv_args;
    ChimpRef *data;
    ChimpRef *req;
    http_parser *p = &CHIMP_HTTP_PARSER(self)->impl;
    http_parser_settings *conf = &CHIMP_HTTP_PARSER(self)->conf;
    ChimpRef *sck;
    
    if (!chimp_method_parse_args (args, "o", &sck)) {
        return NULL;
    }
    
    size = chimp_int_new (8192);
    if (size == NULL) {
        return NULL;
    }
    recv_args = chimp_array_new_var (size, NULL);
    if (recv_args == NULL) {
        return NULL;
    }
    while (!CHIMP_HTTP_PARSER(self)->complete) {
        size_t size;
        data = chimp_object_call_method (sck, "recv", recv_args);
        if (data == NULL) {
            return NULL;
        }
        else if (CHIMP_STR_SIZE(data) == 0) {
            http_parser_execute (p, conf, "", 0);
            if (HTTP_PARSER_ERRNO(p) != HPE_OK) {
                req = NULL;
                CHIMP_BUG (http_errno_description (HTTP_PARSER_ERRNO(p)));
                goto done;
            }
            break;
        }
        size = CHIMP_STR_SIZE(data);
        http_parser_execute (p, conf, CHIMP_STR_DATA(data), size); 
        if (HTTP_PARSER_ERRNO(p) != HPE_OK) {
            req = NULL;
            CHIMP_BUG (http_errno_description (HTTP_PARSER_ERRNO(p)));
            goto done;
        }
    }
    req = CHIMP_HTTP_PARSER(self)->request;
done:
    CHIMP_HTTP_PARSER(self)->request = NULL;
    CHIMP_HTTP_PARSER(self)->header = NULL;
    CHIMP_HTTP_PARSER(self)->complete = CHIMP_FALSE;
    return (req == NULL) ? chimp_nil : req;
}

static ChimpRef *
_chimp_init_http_parser_class (void)
{
    ChimpRef *klass = chimp_class_new (
            CHIMP_STR_NEW("http.parser"), NULL, sizeof(ChimpHttpParser));
    if (klass == NULL) {
        return NULL;
    }
    CHIMP_CLASS(klass)->init = _chimp_http_parser_init;
    CHIMP_CLASS(klass)->dtor = _chimp_http_parser_dtor;
    CHIMP_CLASS(klass)->mark = _chimp_http_parser_mark;
    if (!chimp_class_add_native_method (
            klass, "parse_request", _chimp_http_parser_parse_request)) {
        return NULL;
    }
    return klass;
}

static ChimpRef *
_chimp_http_request_init (ChimpRef *self, ChimpRef *args)
{
    CHIMP_HTTP_REQUEST(self)->headers = chimp_hash_new ();
    CHIMP_HTTP_REQUEST(self)->body = chimp_str_new ("", 0);
    return self;
}

static void
_chimp_http_request_dtor (ChimpRef *self)
{
}

static void
_chimp_http_request_mark (ChimpGC *gc, ChimpRef *self)
{
    chimp_gc_mark_ref (gc, CHIMP_HTTP_REQUEST(self)->method);
    chimp_gc_mark_ref (gc, CHIMP_HTTP_REQUEST(self)->http_version);
    chimp_gc_mark_ref (gc, CHIMP_HTTP_REQUEST(self)->url);
    chimp_gc_mark_ref (gc, CHIMP_HTTP_REQUEST(self)->headers);
    chimp_gc_mark_ref (gc, CHIMP_HTTP_REQUEST(self)->body);
}

static ChimpRef *
_chimp_http_request_getattr (ChimpRef *self, ChimpRef *attr)
{
    if (strcmp (CHIMP_STR_DATA(attr), "method") == 0) {
        return CHIMP_HTTP_REQUEST(self)->method;
    }
    else if (strcmp (CHIMP_STR_DATA(attr), "url") == 0) {
        return CHIMP_HTTP_REQUEST(self)->url;
    }
    else if (strcmp (CHIMP_STR_DATA(attr), "headers") == 0) {
        return CHIMP_HTTP_REQUEST(self)->headers;
    }
    else if (strcmp (CHIMP_STR_DATA(attr), "body") == 0) {
        return CHIMP_HTTP_REQUEST(self)->body;
    }
    else if (strcmp (CHIMP_STR_DATA(attr), "version") == 0) {
        return CHIMP_HTTP_REQUEST(self)->http_version;
    }
    else {
        CHIMP_BUG (
            "unknown attribute %s on http.request", CHIMP_STR_DATA(attr));
        return NULL;
    }
}

static ChimpRef *
_chimp_http_request_str (ChimpRef *self)
{
  ChimpRef *method = CHIMP_HTTP_REQUEST(self)->method;
  ChimpRef *url = CHIMP_HTTP_REQUEST(self)->url;
  ChimpRef *headers = CHIMP_HTTP_REQUEST(self)->headers;
  ChimpRef *body = CHIMP_HTTP_REQUEST(self)->body;
  return chimp_str_new_format (
      "http.request(method=%s, url=\"%s\", headers=%s, body=\"%s\")",
      method == NULL ? "<null>" : CHIMP_STR_DATA(method),
      url == NULL ? "<null>" : CHIMP_STR_DATA(url),
      headers == NULL ? "<null>" : CHIMP_STR_DATA(chimp_object_str (headers)),
      body == NULL ? "<null>" : CHIMP_STR_DATA(body));
}

static ChimpRef *
_chimp_init_http_request_class (void)
{
    ChimpRef *klass = chimp_class_new (
            CHIMP_STR_NEW("http.request"), NULL, sizeof(ChimpHttpRequest));
    if (klass == NULL) {
        return NULL;
    }
    CHIMP_CLASS(klass)->init = _chimp_http_request_init;
    CHIMP_CLASS(klass)->dtor = _chimp_http_request_dtor;
    CHIMP_CLASS(klass)->mark = _chimp_http_request_mark;
    CHIMP_CLASS(klass)->getattr = _chimp_http_request_getattr;
    CHIMP_CLASS(klass)->str = _chimp_http_request_str;
    return klass;
}

ChimpRef *
chimp_init_http_module (void)
{
    ChimpRef *http;
    ChimpRef *http_parser_class;
    ChimpRef *http_request_class;

    http = chimp_module_new_str ("http", NULL);
    if (http == NULL) {
        return NULL;
    }

    http_parser_class = _chimp_init_http_parser_class ();
    if (!chimp_module_add_local_str (http, "parser", http_parser_class)) {
        return NULL;
    }
    chimp_http_parser_class = http_parser_class;

    http_request_class = _chimp_init_http_request_class ();
    if (!chimp_module_add_local_str (http, "request", http_request_class)) {
        return NULL;
    }
    chimp_http_request_class = http_request_class;

    return http;
}

