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
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "nim/any.h"
#include "nim/object.h"
#include "nim/array.h"
#include "nim/str.h"

#define NIM_MODULE_INT_CONSTANT(mod, name, value) \
    do { \
        NimRef *temp = nim_int_new (value); \
        if (temp == NULL) { \
            return NULL; \
        } \
        if (!nim_module_add_local_str ((mod), (name), temp)) { \
            return NULL; \
        } \
    } while (0)

#define NIM_MODULE_ADD_CLASS(mod, name, klass) \
    do { \
        if ((klass) == NULL) { \
            return NULL; \
        } \
        \
        if (!nim_module_add_local_str ((mod), (name), (klass))) { \
            return NULL; \
        } \
    } while (0)

typedef struct _NimNetSocket {
    NimAny base;
    int fd;
} NimNetSocket;

static NimRef *nim_net_socket_class = NULL;

#define NIM_NET_SOCKET(ref) \
    NIM_CHECK_CAST(NimNetSocket, (ref), nim_net_socket_class)

static NimRef *
_nim_socket_init (NimRef *self, NimRef *args)
{
    int64_t fd;
    int64_t family, type, protocol = 0;
    
    if (NIM_ARRAY_SIZE(args) == 3) {
        if (!nim_method_parse_args (args, "III", &family, &type, &protocol)) {
            return NULL;
        }
        NIM_NET_SOCKET(self)->fd =
            socket ((int)family, (int)type, (int)protocol);
        if (NIM_NET_SOCKET(self)->fd < 0) {
            NIM_BUG ("socket() failed");
            return NULL;
        }
        return self;
    }
    else if (NIM_ARRAY_SIZE(args) == 1) {
        if (!nim_method_parse_args (args, "I", &fd)) {
            return NULL;
        }
        NIM_NET_SOCKET(self)->fd = fd;
        return self;
    }
    else {
        return NULL;
    }
}

static NimRef *
_nim_socket_bind (NimRef *self, NimRef *args)
{
    struct sockaddr_in addr;
    int64_t port;
    int fd;

    if (!nim_method_parse_args (args, "I", &port)) {
        return NULL;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons ((int)port);
    memset (addr.sin_zero, 0, sizeof(addr.sin_zero));

    fd = NIM_NET_SOCKET(self)->fd;

    if (bind (fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        NIM_BUG ("bind() failed");
        return NULL;
    }

    return nim_nil;
}

static NimRef *
_nim_socket_listen (NimRef *self, NimRef *args)
{
    int64_t backlog = SOMAXCONN;
    int fd = NIM_NET_SOCKET(self)->fd;

    if (!nim_method_parse_args (args, "|I", &backlog)) {
        return NULL;
    }

    if (listen (fd, backlog) < 0) {
        NIM_BUG ("listen() failed");
        return NULL;
    }
    return nim_nil;
}

static NimRef *
_nim_socket_setsockopt (NimRef *self, NimRef *args)
{
    int32_t level, optname, optval;

    if (!nim_method_parse_args (args, "iii", &level, &optname, &optval)) {
        return NULL;
    }

    if (setsockopt (NIM_NET_SOCKET(self)->fd, level, optname, &optval, sizeof(optval)) != 0) {
        return nim_false;
    }

    return nim_true;
}

static NimRef *
_nim_socket_accept (NimRef *self, NimRef *args)
{
    int fd = NIM_NET_SOCKET(self)->fd;
    struct sockaddr_in addr;
    uint32_t addrlen = sizeof(addr);
    int cfd;
    NimRef *fd_obj;

    cfd = accept (fd, (struct sockaddr *)&addr, &addrlen);
    if (cfd < 0) {
        NIM_BUG ("accept() failed");
        return NULL;
    }

    fd_obj = nim_int_new (cfd);
    if (fd_obj == NULL) {
        return NULL;
    }
    return nim_class_new_instance (
            nim_net_socket_class, fd_obj, NULL);
}

static NimRef *
_nim_socket_close (NimRef *self, NimRef *args)
{
    if (NIM_NET_SOCKET(self)->fd > 0) {
        close (NIM_NET_SOCKET(self)->fd);
        NIM_NET_SOCKET(self)->fd = -1;
    }
    return nim_nil;
}

static NimRef *
_nim_socket_shutdown (NimRef *self, NimRef *args)
{
    int32_t how = SHUT_RDWR;

    if (!nim_method_parse_args (args, "i", &how)) {
        return NULL;
    }

    if (shutdown (NIM_NET_SOCKET (self)->fd, how) != 0) {
        return nim_false;
    }
    return nim_true;
}

static NimRef *
_nim_socket_send (NimRef *self, NimRef *args)
{
    NimRef *data;
    int rc;

    if (!nim_method_parse_args (args, "o", &data)) {
        return NULL;
    }

    rc = send (NIM_NET_SOCKET (self)->fd,
                NIM_STR_DATA(data), NIM_STR_SIZE(data), 0);

    return nim_int_new (rc);
}

static NimRef *
_nim_socket_recv (NimRef *self, NimRef *args)
{
    int32_t size;
    char *buf;
    int n;

    if (!nim_method_parse_args (args, "i", &size)) {
        return NULL;
    }

    buf = malloc (size + 1);
    if (buf == NULL) {
        NIM_BUG ("Failed to allocate recv buffer");
        return NULL;
    }

    n = recv (NIM_NET_SOCKET (self)->fd, buf, size, 0);
    /* XXX can't distinguish between an error and an EOF atm */
    if (n <= 0) {
        return NIM_STR_NEW ("");
    }

    if (n < size) {
        char *temp = realloc (buf, n + 1);
        if (temp == NULL) {
            NIM_BUG ("Failed to shrink recv buffer");
            return NULL;
        }
        buf = temp;
    }
    buf[n] = '\0';

    return nim_str_new_take (buf, n);
}

static NimRef *
_nim_socket_getattr (NimRef *self, NimRef *attr)
{
    if (strcmp ("fd", NIM_STR_DATA(attr)) == 0) {
        return nim_int_new (NIM_NET_SOCKET (self)->fd);
    }
    else {
        NimRef *super = NIM_CLASS_SUPER(NIM_ANY_CLASS(self));
        return NIM_CLASS(super)->getattr (self, attr);
    }
}

static void
_nim_socket_dtor (NimRef *self)
{
#if 0
    _nim_socket_close (self, NULL);
#endif
}

static nim_bool_t
_nim_socket_class_bootstrap (void)
{
    if (nim_net_socket_class == NULL) {
        NimRef *net_socket_class;
        nim_net_socket_class = nim_class_new (
            NIM_STR_NEW("net.socket"), NULL, sizeof(NimNetSocket));
        if (nim_net_socket_class == NULL) {
            return NIM_FALSE;
        }

        /* protect net.socket from the GC */
        net_socket_class = nim_net_socket_class;

        NIM_CLASS(net_socket_class)->init = _nim_socket_init;
        NIM_CLASS(net_socket_class)->dtor = _nim_socket_dtor;
        NIM_CLASS(net_socket_class)->getattr = _nim_socket_getattr;

        if (!nim_class_add_native_method (
                net_socket_class, "close", _nim_socket_close)) {
            return NIM_FALSE;
        }

        if (!nim_class_add_native_method (
                net_socket_class, "bind", _nim_socket_bind)) {
            return NIM_FALSE;
        }

        if (!nim_class_add_native_method (
                net_socket_class, "setsockopt", _nim_socket_setsockopt)) {
            return NIM_FALSE;
        }

        if (!nim_class_add_native_method (
                net_socket_class, "listen", _nim_socket_listen)) {
            return NIM_FALSE;
        }

        if (!nim_class_add_native_method (
                net_socket_class, "accept", _nim_socket_accept)) {
            return NIM_FALSE;
        }

        if (!nim_class_add_native_method (
                net_socket_class, "recv", _nim_socket_recv)) {
            return NIM_FALSE;
        }

        if (!nim_class_add_native_method (
                net_socket_class, "send", _nim_socket_send)) {
            return NIM_FALSE;
        }

        if (!nim_class_add_native_method (
                net_socket_class, "shutdown", _nim_socket_shutdown)) {
            return NIM_FALSE;
        }
    }
    return NIM_TRUE;
}

NimRef *
nim_init_net_module (void)
{
    NimRef *net;
    NimRef *net_socket;

    net = nim_module_new_str ("net", NULL);
    if (net == NULL) {
        return NULL;
    }

    NIM_MODULE_INT_CONSTANT(net, "AF_INET", AF_INET);
    NIM_MODULE_INT_CONSTANT(net, "SOCK_DGRAM", SOCK_DGRAM);
    NIM_MODULE_INT_CONSTANT(net, "SOCK_STREAM", SOCK_STREAM);
    NIM_MODULE_INT_CONSTANT(net, "SOL_SOCKET", SOL_SOCKET);
    NIM_MODULE_INT_CONSTANT(net, "SO_REUSEADDR", SO_REUSEADDR);
    NIM_MODULE_INT_CONSTANT(net, "SHUT_RD", SHUT_RD);
    NIM_MODULE_INT_CONSTANT(net, "SHUT_WR", SHUT_WR);
    /* XXX adding this seems to trigger a GC bug */
    NIM_MODULE_INT_CONSTANT(net, "SHUT_RDWR", SHUT_RDWR);

    if (!_nim_socket_class_bootstrap ()) {
        return NULL;
    }

    /* protect net.socket from the garbage collector */
    net_socket = nim_net_socket_class;

    NIM_MODULE_ADD_CLASS(net, "socket", net_socket);

    return net;
}

