/* Copyright 2014 yiyuanzhong@gmail.com (Yiyuan Zhong)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(__unix__)

#include "flinter/linkage/interface.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "flinter/linkage/linkage_peer.h"
#include "flinter/linkage/resolver.h"
#include "flinter/logger.h"
#include "flinter/safeio.h"
#include "flinter/utility.h"

namespace flinter {

template <class T>
static bool GetPeerOrClose(int s, LinkagePeer *peer, LinkagePeer *me)
{
    if (peer) {
        T sa;
        socklen_t len = sizeof(sa);
        if (getpeername(s, reinterpret_cast<struct sockaddr *>(&sa), &len) ||
            !peer->Set(&sa, s)) {

            safe_close(s);
            return false;
        }
    }

    if (me) {
        T sa;
        socklen_t len = sizeof(sa);
        if (getsockname(s, reinterpret_cast<struct sockaddr *>(&sa), &len) ||
            !me->Set(&sa, s)) {

            safe_close(s);
            return false;
        }
    }

    return true;
}

template <class T>
bool DoAccept(int s, LinkagePeer *peer, LinkagePeer *me)
{
    T addr;
    struct sockaddr *sa = reinterpret_cast<struct sockaddr *>(&addr);
    socklen_t len = sizeof(addr);
    int fd = safe_accept(s, sa, &len);
    if (fd < 0) {
        return false;
    }

    return GetPeerOrClose<T>(fd, peer, me);
}

static bool Fill(struct sockaddr_in6 *addr, const char *ip, uint16_t port)
{
    memset(addr, 0, sizeof(*addr));
    addr->sin6_family = AF_INET6;
    addr->sin6_addr = in6addr_any;
    addr->sin6_port = htons(port);
    if (ip && *ip) {
        if (!inet_pton(AF_INET6, ip, &addr->sin6_addr)) {
            errno = EINVAL;
            return false;
        }
    }

    return true;
}

static bool Fill(struct sockaddr_in *addr, const char *ip, uint16_t port)
{
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(port);
    if (ip && *ip) {
        if (!inet_pton(AF_INET, ip, &addr->sin_addr)) {
            errno = EINVAL;
            return false;
        }
    }

    return true;
}

Interface::Parameter::Parameter()
        : domain(AF_UNSPEC)
        , type(SOCK_STREAM)
        , protocol(0)
        , tcp_defer_accept(false)
        , tcp_nodelay(false)
        , udp_broadcast(false)
        , udp_multicast(false)
        , socket_interface(NULL)
        , socket_hostname(NULL)
        , socket_bind_port(0)
        , socket_port(0)
        , socket_close_on_exec(false)
        , socket_reuse_address(false)
        , socket_non_blocking(false)
        , socket_keepalive(false)
        , unix_abstract(NULL)
        , unix_pathname(NULL)
        , unix_mode(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
{
    // Intended left blank.
}

Interface::Interface() : _socket(-1), _domain(AF_UNSPEC)
{
    // Intended left blank.
}

Interface::~Interface()
{
    Close();
}

int Interface::Bind(int s, void *sockaddr, size_t addrlen)
{
    return bind(s, reinterpret_cast<struct sockaddr *>(sockaddr),
                   static_cast<socklen_t>(addrlen));
}

int Interface::CreateListenSocket(const Parameter &parameter,
                                  void *sockaddr,
                                  size_t addrlen)
{
    int s = CreateSocket(parameter);
    if (s < 0) {
        return -1;
    }

    if (Bind(s, sockaddr, addrlen)) {
        safe_close(s);
        return -1;
    }

    if (parameter.type == SOCK_STREAM) {
        if (listen(s, kMaximumQueueLength)) {
            safe_close(s);
            return -1;
        }
    }

    return s;
}

int Interface::CreateSocket(const Parameter &parameter)
{
    int s = socket(parameter.domain, parameter.type, parameter.protocol);
    if (s < 0) {
        return -1;
    }

    if (!InitializeSocket(parameter, s)) {
        safe_close(s);
        return -1;
    }

    return s;
}

bool Interface::InitializeSocket(const Parameter &parameter, int s)
{
    if (parameter.socket_reuse_address) {
        if (set_socket_reuse_address(s)) {
            return false;
        }
    }

    if (parameter.socket_non_blocking) {
        if (set_non_blocking_mode(s)) {
            return false;
        }
    }

    if (parameter.socket_close_on_exec) {
        if (set_close_on_exec(s)) {
            return false;
        }
    }

    if (parameter.socket_keepalive) {
        if (set_socket_keepalive(s)) {
            return false;
        }
    }

    if (parameter.tcp_defer_accept) {
        if (set_tcp_defer_accept(s)) {
            return false;
        }
    }

    if (parameter.tcp_nodelay) {
        if (set_tcp_nodelay(s)) {
            return false;
        }
    }

    if (parameter.udp_broadcast) {
        if (set_socket_broadcast(s)) {
            return false;
        }
    }

    return true;
}

int Interface::DoConnectTcp4(const Parameter &parameter,
                             LinkagePeer *peer,
                             LinkagePeer *me)
{
    if (!parameter.socket_hostname  ||
        !*parameter.socket_hostname ||
        !parameter.socket_port      ){

        errno = EINVAL;
        return -1;
    }

    Resolver *const resolver = Resolver::GetInstance();
    struct sockaddr_in addr;

    int s = CreateSocket(parameter);
    if (s < 0) {
        CLOG.Verbose("Interface: failed to initialize socket: %d: %s",
                     errno, strerror(errno));

        return -1;
    }

    if (parameter.socket_interface && *parameter.socket_interface) {
        if (!resolver->Resolve(parameter.socket_interface,
                               parameter.socket_bind_port,
                               &addr)) {

            safe_close(s);
            return -1;
        }

        if (Bind(s, &addr, sizeof(addr))) {
            CLOG.Verbose("Interface: failed to bind socket(%d): %d: %s",
                         s, errno, strerror(errno));

            safe_close(s);
            return -1;
        }
    }

    if (!resolver->Resolve(parameter.socket_hostname,
                           parameter.socket_port,
                           &addr)) {

        safe_close(s);
        return -1;
    }

    int ret = safe_connect(s, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
    if (ret < 0) {
        if (errno != EINPROGRESS) {
            CLOG.Verbose("Interface: failed to connect socket(%d): %d: %s",
                         s, errno, strerror(errno));

            safe_close(s);
            return -1;
        }
    }

    if (!GetPeerOrClose<struct sockaddr_in>(s, NULL, me)) {
        return -1;
    }

    if (peer) {
        if (!peer->Set(&addr, s)) {
            safe_close(s);
            return -1;
        }
    }

    _socket = s;
    _domain = AF_INET;
    return ret == 0 ? 0 : 1;
}

int Interface::DoConnectUnix(const Parameter &parameter,
                             LinkagePeer *peer,
                             LinkagePeer * /*me*/)
{
    bool file_based = true;
    const char *name = NULL;
    if (parameter.unix_pathname && *parameter.unix_pathname) {
        name = parameter.unix_pathname;
    } else if (parameter.unix_abstract && *parameter.unix_abstract) {
        name = parameter.unix_abstract;
        file_based = false;
    }

    struct sockaddr_un aun;
    memset(&aun, 0, sizeof(aun));
    aun.sun_family = AF_UNIX;
    size_t len = strlen(name);
    if (len >= sizeof(aun.sun_path)) {
        errno = EINVAL;
        return -1;
    }

    if (file_based) {
        memcpy(aun.sun_path, name, len);
    } else {
        memcpy(aun.sun_path + 1, name, len);
    }

    len += sizeof(aun) - sizeof(aun.sun_path) + 1;
    int s = CreateSocket(parameter);
    if (s < 0) {
        return -1;
    }

    // UNIX socket will never be in progress.
    if (safe_connect(s, reinterpret_cast<struct sockaddr *>(&aun),
                        static_cast<socklen_t>(len))) {

        safe_close(s);
        return -1;
    }

    if (peer) {
        if (!peer->Set(&aun, s)) {
            return -1;
        }
    }

    _socket = s;
    _domain = AF_UNIX;
    return 0;
}

int Interface::Connect(const Parameter &parameter,
                       LinkagePeer *peer,
                       LinkagePeer *me)
{
    // For connecting, SOCK_STREAM and SOCK_DGRAM share the same actions.
    if (parameter.type != SOCK_STREAM && parameter.type != SOCK_DGRAM) {
        errno = EINVAL;
        return -1;

    } else if (!Close()) {
        return -1;

    } else if (parameter.domain == AF_INET6) {
        //return DoConnectTcp6(parameter, peer, me); // UDP as well.

    } else if (parameter.domain == AF_INET  ) {
        return DoConnectTcp4(parameter, peer, me); // UDP as well.

    } else if (parameter.domain == AF_UNIX  ) {
        return DoConnectUnix(parameter, peer, me);
    }

    CLOG.Fatal("Interface: BUG: unsupported socket type: <%d:%d:%d>",
               parameter.domain, parameter.type, parameter.protocol);

    return -1;
}

bool Interface::DoListenTcp4(const Parameter &parameter, LinkagePeer *me)
{
    struct ip_mreqn mreqn;
    struct sockaddr_in ain;
    bool multicast = false;

    if (parameter.type == SOCK_DGRAM && parameter.udp_multicast) {
        memset(&mreqn, 0, sizeof(mreqn));
        if (!parameter.socket_interface                                     ||
            !*parameter.socket_interface                                    ||
            inet_aton(parameter.socket_interface, &mreqn.imr_address) != 1  ){

            errno = EINVAL;
            return false;
        }

        if (!Fill(&ain, parameter.socket_hostname, parameter.socket_bind_port)) {
            return false;
        }

        mreqn.imr_multiaddr = ain.sin_addr;
        multicast = true;

    } else if (!Fill(&ain, parameter.socket_interface, parameter.socket_bind_port)) {
        return false;
    }

    int s = CreateListenSocket(parameter, &ain, sizeof(ain));
    if (s < 0) {
        return false;
    }

    if (multicast) {
        if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreqn, sizeof(mreqn))) {
            safe_close(s);
            return false;
        }
    }

    if (!GetPeerOrClose<struct sockaddr_in>(s, NULL, me)) {
        return false;
    }

    _socket = s;
    _domain = AF_INET;

    char buffer[128];
    CLOG.Verbose("Interface: LISTEN<%d> %s4 <%s:%u>", s,
                 parameter.type == SOCK_STREAM ? "TCP" : "UDP",
                 inet_ntop(AF_INET, &ain.sin_addr, buffer, sizeof(buffer)),
                 parameter.socket_bind_port);

    return true;
}

bool Interface::DoListenTcp6(const Parameter &parameter, LinkagePeer *me)
{
    struct sockaddr_in6 ai6;
    if (!Fill(&ai6, parameter.socket_interface, parameter.socket_bind_port)) {
        return false;
    }

    int s = CreateListenSocket(parameter, &ai6, sizeof(ai6));
    if (s < 0) {
        return false;
    }

    if (!GetPeerOrClose<struct sockaddr_in6>(s, NULL, me)) {
        return false;
    }

    _socket = s;
    _domain = AF_INET6;

    char buffer[128];
    CLOG.Verbose("Interface: LISTEN<%d> %s6 <[%s]:%u>", s,
                 parameter.type == SOCK_STREAM ? "TCP" : "UDP",
                 inet_ntop(AF_INET6, &ai6.sin6_addr, buffer, sizeof(buffer)),
                 parameter.socket_bind_port);

    return true;
}

bool Interface::DoListenUnixSocket(const Parameter &parameter,
                                   LinkagePeer * /*me*/)
{
    struct sockaddr_un aun;
    memset(&aun, 0, sizeof(aun));
    aun.sun_family = AF_UNIX;

    size_t len = strlen(parameter.unix_pathname);
    if (len >= sizeof(aun.sun_path)) {
        errno = EINVAL;
        return false;
    }

    memcpy(aun.sun_path, parameter.unix_pathname, len);
    len += sizeof(aun) - sizeof(aun.sun_path) + 1;

    int s = CreateSocket(parameter);
    if (s < 0) {
        return false;
    }

    if (Bind(s, &aun, len)) {
        safe_close(s);
        return false;
    }

    if (chmod(parameter.unix_pathname, parameter.unix_mode)) {
        safe_close(s);
        unlink(parameter.unix_pathname);
        return false;
    }

    if (listen(s, kMaximumQueueLength)) {
        safe_close(s);
        unlink(parameter.unix_pathname);
        return false;
    }

    _socket = s;
    _domain = AF_UNIX;
    _sockname = parameter.unix_pathname;
    CLOG.Verbose("Interface: LISTEN<%d> PATHNAME%c [%s]", s,
                 parameter.type == SOCK_STREAM ? 's' : 'd',
                 parameter.unix_abstract);

    return true;
}

bool Interface::DoListenUnixNamespace(const Parameter &parameter,
                                      LinkagePeer * /*me*/)
{
    struct sockaddr_un aun;
    memset(&aun, 0, sizeof(aun));
    aun.sun_family = AF_UNIX;

    size_t len = strlen(parameter.unix_abstract);
    if (len >= sizeof(aun.sun_path)) {
        errno = EINVAL;
        return false;
    }

    memcpy(aun.sun_path + 1, parameter.unix_pathname, len);
    len += sizeof(aun) - sizeof(aun.sun_path) + 1;

    int s = CreateListenSocket(parameter, &aun, len);
    if (s < 0) {
        return false;
    }

    _socket = s;
    _domain = AF_UNIX;
    CLOG.Verbose("Interface: LISTEN<%d> ABSTRACT%c [%s]", s,
                 parameter.type == SOCK_STREAM ? 's' : 'd',
                 parameter.unix_abstract);

    return true;
}

bool Interface::DoListenUnix(const Parameter &parameter, LinkagePeer *me)
{
    if (parameter.unix_pathname && *parameter.unix_pathname) {
        return DoListenUnixSocket(parameter, me);

    } else if (parameter.unix_abstract && *parameter.unix_abstract) {
        return DoListenUnixNamespace(parameter, me);
    }

    CLOG.Fatal("Interface: BUG: neither pathname nor abstract namespace "
                               "specified for UNIX sockets.");

    errno = EINVAL;
    return false;
}

bool Interface::Listen(const Parameter &parameter, LinkagePeer *me)
{
    if (parameter.type != SOCK_STREAM && parameter.type != SOCK_DGRAM) {
        errno = EINVAL;
        return false;

    } else if (!Close()) {
        return false;

    } else if (parameter.domain == AF_INET6) {
        return DoListenTcp6(parameter, me); // UDP as well.

    } else if (parameter.domain == AF_INET ) {
        return DoListenTcp4(parameter, me); // UDP as well.

    } else if (parameter.domain == AF_UNIX ) {
        return DoListenUnix(parameter, me);
    }

    CLOG.Fatal("Interface: BUG: unsupported socket type: <%d:%d:%d>",
               parameter.domain, parameter.type, parameter.protocol);

    errno = EINVAL;
    return false;
}

bool Interface::ListenTcp6(uint16_t port, bool loopback)
{
    Parameter p;
    p.domain = AF_INET6;
    p.type = SOCK_STREAM;
    p.socket_bind_port = port;
    p.socket_non_blocking = true;
    p.socket_reuse_address = true;
    p.socket_close_on_exec = true;
    p.socket_interface = loopback ? "::1" : "::";
    return Listen(p);
}

bool Interface::ListenTcp4(uint16_t port, bool loopback)
{
    Parameter p;
    p.domain = AF_INET;
    p.type = SOCK_STREAM;
    p.socket_bind_port = port;
    p.socket_non_blocking = true;
    p.socket_reuse_address = true;
    p.socket_close_on_exec = true;
    p.socket_interface = loopback ? "127.0.0.1" : "0.0.0.0";
    return Listen(p);
}

bool Interface::ListenTcp(uint16_t port, bool loopback)
{
    return ListenTcp6(port, loopback) || ListenTcp4(port, loopback);
}

bool Interface::ListenUnix(const std::string &sockname, bool file_based, bool privileged)
{
    static const mode_t kPriviledged = S_IRUSR | S_IWUSR;
    static const mode_t kNormal = S_IRUSR | S_IWUSR
                                | S_IRGRP | S_IWGRP
                                | S_IROTH | S_IWOTH;

    Parameter p;
    p.domain = AF_UNIX;
    p.type = SOCK_STREAM;
    p.socket_non_blocking = true;
    p.socket_close_on_exec = true;
    if (file_based) {
        p.unix_pathname = sockname.c_str();
        p.unix_mode = privileged ? kPriviledged : kNormal;

    } else {
        p.unix_abstract = sockname.c_str();
    }

    return Listen(p);
}

bool Interface::Shutdown()
{
    if (_socket < 0) {
        return true;
    }

    // There're race conditions since socket handling is in kernel space,
    // if peer hungup is done by kernel, even if user space haven't call
    // shutdown() before, it will fail with ENOTCONN, so it's normal.
    if (shutdown(_socket, SHUT_RDWR)) {
        if (errno != ENOTCONN) {
            CLOG.Warn("Interface: failed to shutdown(%d): %d: %s",
                      _socket, errno, strerror(errno));

            return false;
        }
    }

    return true;
}

bool Interface::Close()
{
    if (_socket < 0) {
        return true;
    }

    if (!_sockname.empty()) {
        // Try but not necessarily succeed.
        unlink(_sockname.c_str());
        _sockname.clear();
    }

    int socket = _socket;
    _socket = -1;

    if (safe_close(socket)) {
        return false;
    }

    _domain = AF_UNSPEC;
    return true;
}

bool Interface::Accept(LinkagePeer *peer, LinkagePeer *me)
{
    Parameter p;
    p.socket_close_on_exec = true;
    p.socket_non_blocking = true;
    return Accept(p, peer, me);
}

bool Interface::Accept(const Parameter &parameter,
                       LinkagePeer *peer,
                       LinkagePeer *me)
{
    assert(_socket >= 0);
    assert(peer);

    bool ret = false;
    if (_socket < 0 || !peer) {
        return false;

    } else if (_domain == AF_INET6) {
        ret = DoAccept<struct sockaddr_in6>(_socket, peer, me);

    } else if (_domain == AF_INET) {
        ret = DoAccept<struct sockaddr_in >(_socket, peer, me);

    } else if (_domain == AF_UNIX) {
        ret = DoAccept<struct sockaddr_un >(_socket, peer, me);
    }

    if (!ret) {
        return false;
    }

    if (!InitializeSocket(parameter, peer->fd())) {
        safe_close(peer->fd());
        return false;
    }

    return true;
}

bool Interface::Accepted(int fd)
{
    Parameter p;
    p.socket_non_blocking = true;
    p.socket_close_on_exec = true;
    return Accepted(p, fd);
}

bool Interface::Accepted(const Parameter &parameter, int fd)
{
    if (!Close() || !InitializeSocket(parameter, fd)) {
        return false;
    }

    _domain = parameter.domain;
    _socket = fd;
    return true;
}

int Interface::ConnectUnix(const std::string &sockname,
                           bool file_based,
                           LinkagePeer *peer,
                           LinkagePeer *me)
{
    Parameter p;
    p.domain = AF_UNIX;
    p.type = SOCK_STREAM;
    p.socket_non_blocking = true;
    p.socket_close_on_exec = true;

    if (file_based) {
        p.unix_pathname = sockname.c_str();
    } else {
        p.unix_abstract = sockname.c_str();
    }

    return Connect(p, peer, me);
}

int Interface::ConnectTcp4(const std::string &hostname,
                           uint16_t port,
                           LinkagePeer *peer,
                           LinkagePeer *me)
{
    Parameter p;
    p.domain = AF_INET;
    p.type = SOCK_STREAM;
    p.socket_port = port;
    p.socket_non_blocking = true;
    p.socket_close_on_exec = true;
    p.socket_hostname = hostname.c_str();
    return Connect(p, peer, me);
}

bool Interface::WaitUntilConnected(int64_t timeout)
{
    if (_socket < 0) {
        return false;
    }

    int milliseconds = -1;
    if (timeout >= 0) {
        milliseconds = static_cast<int>(timeout / 1000000LL);
    }

    int ret = safe_wait_until_connected(_socket, milliseconds);
    if (ret) {
        CLOG.Verbose("Interface: failed to wait until connected(%d): %d: %s",
                     _socket, errno, strerror(errno));

        return false;
    }

    return true;
}

bool Interface::TestIfConnected()
{
    if (_socket < 0) {
        errno = EBADF;
        return false;
    }

    int ret = safe_test_if_connected(_socket);
    if (ret) {
        CLOG.Verbose("Interface: failed to test if connected(%d): %d: %s",
                     _socket, errno, strerror(errno));

        return false;
    }

    return true;
}

} // namespace flinter

#endif // defined(__unix__)
