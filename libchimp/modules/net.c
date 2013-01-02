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

#include "chimp/any.h"
#include "chimp/object.h"
#include "chimp/array.h"
#include "chimp/str.h"

typedef struct _ChimpNetSocket {
    ChimpAny base;
    int fd;
} ChimpNetSocket;

static ChimpRef *chimp_net_socket_class = NULL;

#define CHIMP_NET_SOCKET(ref) \
    CHIMP_CHECK_CAST(ChimpNetSocket, (ref), chimp_net_socket_class)

static ChimpRef *
_chimp_socket_init (ChimpRef *self, ChimpRef *args)
{
    int64_t fd;
    int64_t family, type, protocol = 0;
    
    if (CHIMP_ARRAY_SIZE(args) == 3) {
        if (!chimp_method_parse_args (args, "III", &family, &type, &protocol)) {
            return NULL;
        }
        CHIMP_NET_SOCKET(self)->fd =
            socket ((int)family, (int)type, (int)protocol);
        if (CHIMP_NET_SOCKET(self)->fd < 0) {
            CHIMP_BUG ("socket() failed");
            return NULL;
        }
        return self;
    }
    else if (CHIMP_ARRAY_SIZE(args) == 1) {
        if (!chimp_method_parse_args (args, "I", &fd)) {
            return NULL;
        }
        CHIMP_NET_SOCKET(self)->fd = fd;
        return self;
    }
    else {
        return NULL;
    }
}

static ChimpRef *
_chimp_socket_bind (ChimpRef *self, ChimpRef *args)
{
    struct sockaddr_in addr;
    int64_t port;
    int fd;

    if (!chimp_method_parse_args (args, "I", &port)) {
        return NULL;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons ((int)port);
    memset (addr.sin_zero, 0, sizeof(addr.sin_zero));

    fd = CHIMP_NET_SOCKET(self)->fd;

    if (bind (fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        CHIMP_BUG ("bind() failed");
        return NULL;
    }

    return chimp_nil;
}

static ChimpRef *
_chimp_socket_listen (ChimpRef *self, ChimpRef *args)
{
    int64_t backlog = SOMAXCONN;
    int fd = CHIMP_NET_SOCKET(self)->fd;

    if (!chimp_method_parse_args (args, "|I", &backlog)) {
        return NULL;
    }

    if (listen (fd, backlog) < 0) {
        CHIMP_BUG ("listen() failed");
        return NULL;
    }
    return chimp_nil;
}

static ChimpRef *
_chimp_socket_accept (ChimpRef *self, ChimpRef *args)
{
    int fd = CHIMP_NET_SOCKET(self)->fd;
    struct sockaddr_in addr;
    uint32_t addrlen = sizeof(addr);
    int cfd;
    ChimpRef *fd_obj;

    cfd = accept (fd, (struct sockaddr *)&addr, &addrlen);
    if (cfd < 0) {
        CHIMP_BUG ("accept() failed");
        return NULL;
    }

    fd_obj = chimp_int_new (cfd);
    if (fd_obj == NULL) {
        return NULL;
    }
    return chimp_class_new_instance (
            chimp_net_socket_class, fd_obj, NULL);
}

static ChimpRef *
_chimp_socket_close (ChimpRef *self, ChimpRef *args)
{
    if (CHIMP_NET_SOCKET(self)->fd >= 0) {
        close (CHIMP_NET_SOCKET(self)->fd);
    }
    return chimp_nil;
}

static void
_chimp_socket_dtor (ChimpRef *self)
{
    _chimp_socket_close (self, NULL);
}

static chimp_bool_t
_chimp_socket_class_bootstrap (void)
{
    if (chimp_net_socket_class == NULL) {
        chimp_net_socket_class = chimp_class_new (
            CHIMP_STR_NEW("net.socket"), NULL, sizeof(ChimpNetSocket));
        if (chimp_net_socket_class == NULL) {
            return CHIMP_FALSE;
        }
        CHIMP_CLASS(chimp_net_socket_class)->init = _chimp_socket_init;
        CHIMP_CLASS(chimp_net_socket_class)->dtor = _chimp_socket_dtor;

        if (!chimp_class_add_native_method (
                chimp_net_socket_class, "close", _chimp_socket_close)) {
            return CHIMP_FALSE;
        }

        if (!chimp_class_add_native_method (
                chimp_net_socket_class, "bind", _chimp_socket_bind)) {
            return CHIMP_FALSE;
        }

        if (!chimp_class_add_native_method (
                chimp_net_socket_class, "listen", _chimp_socket_listen)) {
            return CHIMP_FALSE;
        }

        if (!chimp_class_add_native_method (
                chimp_net_socket_class, "accept", _chimp_socket_accept)) {
            return CHIMP_FALSE;
        }
    }
    return CHIMP_TRUE;
}

ChimpRef *
chimp_init_net_module (void)
{
    ChimpRef *net;
    ChimpRef *temp;

    net = chimp_module_new_str ("net", NULL);
    if (net == NULL) {
        return NULL;
    }

    temp = chimp_int_new (AF_INET);
    if (temp == NULL) {
        return NULL;
    }
    if (!chimp_module_add_local_str (net, "AF_INET", temp)) {
        return NULL;
    }

    temp = chimp_int_new (SOCK_DGRAM);
    if (temp == NULL) {
        return NULL;
    }
    if (!chimp_module_add_local_str (net, "SOCK_DGRAM", temp)) {
        return NULL;
    }

    temp = chimp_int_new (SOCK_STREAM);
    if (temp == NULL) {
        return NULL;
    }
    if (!chimp_module_add_local_str (net, "SOCK_STREAM", temp)) {
        return NULL;
    }

    if (!_chimp_socket_class_bootstrap ()) {
        return NULL;
    }

    if (!chimp_module_add_local_str (net, "socket", chimp_net_socket_class)) {
        return NULL;
    }

    return net;
}

