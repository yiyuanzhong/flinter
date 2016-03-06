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

#ifdef __unix__

#include "flinter/linkage/interface.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <netinet/in.h>
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
static bool GetPeerOrClose(int s,
                           const T &addr,
                           LinkagePeer *peer,
                           LinkagePeer *me)
{
    if (set_non_blocking_mode(s) || set_close_on_exec(s)) {
        safe_close(s);
        return false;
    }

    if (peer) {
        if (!peer->Set(&addr, s)) {
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

Interface::Interface() : _file_based(true), _socket(-1), _domain(AF_UNSPEC), _client(true)
{
    // Intended left blank.
}

Interface::~Interface()
{
    Close();
}

int Interface::CreateSocket(int domain, int type, int protocol)
{
    int s = socket(domain, type, protocol);
    if (s < 0) {
        return -1;
    }

    if (set_non_blocking_mode(s) || set_close_on_exec(s)) {
        safe_close(s);
        return -1;
    }

    return s;
}

int Interface::BindIPv6(uint16_t port, bool loopback)
{
    int s = CreateSocket(AF_INET6, SOCK_STREAM, 0);
    if (s < 0) {
        return -1;
    }

    struct sockaddr_in6 addr6;
    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons(port);
    if (loopback) {
        memcpy(&addr6.sin6_addr, &in6addr_loopback, sizeof(addr6.sin6_addr));
    } else {
        memcpy(&addr6.sin6_addr, &in6addr_any, sizeof(addr6.sin6_addr));
    }

    if (set_socket_reuse_address(s) ||
        bind(s, reinterpret_cast<struct sockaddr *>(&addr6), sizeof(addr6))) {

        safe_close(s);
        return -1;
    }

    return s;
}

int Interface::BindIPv4(uint16_t port, bool loopback)
{
    int s = CreateSocket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (loopback) {
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    } else {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if (set_socket_reuse_address(s) ||
        bind(s, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr))) {

        safe_close(s);
        return -1;
    }

    return s;
}

bool Interface::ListenTcp6(uint16_t port, bool loopback)
{
    return DoListenTcp(port, loopback, AF_INET6);
}

bool Interface::ListenTcp4(uint16_t port, bool loopback)
{
    return DoListenTcp(port, loopback, AF_INET);
}

bool Interface::ListenTcp(uint16_t port, bool loopback)
{
    return DoListenTcp(port, loopback, AF_UNSPEC);
}

bool Interface::DoListenTcp(uint16_t port, bool loopback, int domain)
{
    if (_socket >= 0) {
        return true;
    }

    int s = -1;

    if (domain != AF_INET) {
        s = BindIPv6(port, loopback);
        if (s >= 0) {
            domain = AF_INET6;
        }
    }

    if (s < 0 && domain != AF_INET6) {
        s = BindIPv4(port, loopback);
        if (s >= 0) {
            domain = AF_INET;
        }
    }

    if (s < 0) {
        return false;
    }

    if (safe_listen(s, kMaximumQueueLength)) {
        safe_close(s);
        return false;
    }

    _socket = s;
    _file_based = false;
    _sockname.clear();
    _client = false;
    _domain = domain;

    CLOG.Trace("Interface: listening on %s:%u...", loopback ? "loopback" : "any", port);
    return true;
}

bool Interface::ListenUnix(const std::string &sockname, bool file_based, bool privileged)
{
    if (sockname.empty()) {
        return false;
    } else if (_socket >= 0) {
        return true;
    }

    struct sockaddr_un addr;
    ssize_t size = FillAddress(&addr, sockname, file_based);
    if (size < 0) {
        errno = EINVAL;
        return false;
    }

    int s = CreateSocket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) {
        return false;
    }

    if (bind(s, reinterpret_cast<struct sockaddr *>(&addr),
                static_cast<socklen_t>(size))) {

        if (file_based && errno == EADDRINUSE) {
            // TODO(yiyuanzhong): check if the socket is actually dead.
        }

        safe_close(s);
        return false;
    }

    if (file_based) {
        mode_t mode = privileged ? (S_IRUSR | S_IWUSR)
                                 : (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

        if (chmod(sockname.c_str(), mode)) {
            safe_close(s);
            unlink(sockname.c_str());
            return false;
        }
    }

    if (safe_listen(s, kMaximumQueueLength)) {
        safe_close(s);
        if (file_based) {
            unlink(sockname.c_str());
        }
        return false;
    }

    _socket = s;
    _file_based = file_based;
    _sockname = sockname;
    _client = false;
    _domain = AF_UNIX;

    CLOG.Trace("Interface: listening on %s%s [%s]...",
               privileged ? "privileged " : "",
               file_based ? "file" : "namespace",
               sockname.c_str());

    return true;
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

    if (_file_based) {
        // Try but not necessarily succeed.
        unlink(_sockname.c_str());
    }

    int socket = _socket;
    _file_based = true;
    _client = true;
    _socket = -1;

    if (safe_close(socket)) {
        return false;
    }

    return true;
}

bool Interface::Accept(LinkagePeer *peer, LinkagePeer *me)
{
    assert(_socket >= 0);
    assert(!_client);
    assert(peer);

    if (_socket < 0 || _client || !peer) {
        return false;
    }

    if (_domain == AF_UNIX) {
        struct sockaddr_un addr;
        socklen_t len = sizeof(addr);
        int fd = safe_accept(_socket, reinterpret_cast<struct sockaddr *>(&addr), &len);
        if (fd < 0) {
            return false;
        }

        if (addr.sun_family != AF_UNIX) { // Something has gone terribly wrong.
            safe_close(fd);
            return false;
        }

        return GetPeerOrClose(fd, addr, peer, me);

    } else if (_domain == AF_INET6) {
        struct sockaddr_in6 addr6;
        socklen_t len = sizeof(addr6);
        int fd = safe_accept(_socket, reinterpret_cast<struct sockaddr *>(&addr6), &len);
        if (fd < 0) {
            return false;
        }

        if (addr6.sin6_family != AF_INET6) { // Something has gone terribly wrong.
            safe_close(fd);
            return false;
        }

        return GetPeerOrClose(fd, addr6, peer, me);

    } else if (_domain == AF_INET) {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        int fd = safe_accept(_socket, reinterpret_cast<struct sockaddr *>(&addr), &len);
        if (fd < 0) {
            return false;
        }

        if (addr.sin_family != AF_INET) { // Something has gone terribly wrong.
            safe_close(fd);
            return false;
        }

        return GetPeerOrClose(fd, addr, peer, me);
    }

    return false;
}

bool Interface::Accepted(int fd)
{
    if (_socket >= 0) {
        errno = EINVAL;
        return false;
    }

    if (set_non_blocking_mode(fd) || set_close_on_exec(fd)) {
        return false;
    }

    _file_based = false;
    _socket = fd;
    return true;
}

int Interface::ConnectUnix(const std::string &sockname,
                           bool file_based,
                           LinkagePeer *peer,
                           LinkagePeer *me)
{
    assert(sockname.length());
    assert(_socket < 0);
    assert(_client);

    if (_socket >= 0) {
        errno = EINVAL;
        return -1;
    }

    struct sockaddr_un addr;
    ssize_t size = FillAddress(&addr, sockname, file_based);
    if (size < 0) {
        errno = EINVAL;
        return -1;
    }

    int s = CreateSocket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) {
        return -1;
    }

    // UNIX socket will never be in progress.
    if (safe_connect(s, (struct sockaddr *)&addr, static_cast<socklen_t>(size))) {
        safe_close(s);
        return -1;
    }

    if (!GetPeerOrClose(s, addr, peer, me)) {
        if (file_based) {
            unlink(sockname.c_str());
        }

        return -1;
    }

    _file_based = file_based;
    _socket = s;
    return 0;
}

int Interface::ConnectTcp4(const std::string &hostname,
                           uint16_t port,
                           LinkagePeer *peer,
                           LinkagePeer *me)
{
    assert(hostname.length());
    assert(_client);

    if (_socket >= 0) {
        errno = EINVAL;
        return -1;
    }

    std::string ip;
    if (!Resolver::GetInstance()->Resolve(hostname, &ip)) {
        errno = ENXIO;
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_aton(ip.c_str(), &addr.sin_addr) == 0) {
        errno = ENXIO;
        return -1;
    }

    int s = CreateSocket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        return -1;
    }

    int ret = safe_connect(s, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
    if (ret < 0) {
        if (errno != EINPROGRESS) {
            safe_close(s);
            return -1;
        }
    }

    if (!GetPeerOrClose(s, addr, peer, me)) {
        return -1;
    }

    _socket = s;
    _file_based = false;
    return ret == 0 ? 0 : 1;
}

bool Interface::WaitUntilConnected(int milliseconds)
{
    if (_socket < 0) {
        return false;
    }

    return safe_wait_until_connected(_socket, milliseconds) == 0;
}

bool Interface::TestIfConnected()
{
    if (_socket < 0) {
        errno = EBADF;
        return false;
    }

    return safe_test_if_connected(_socket) == 0;
}

ssize_t Interface::FillAddress(struct sockaddr_un *addr,
                               const std::string &sockname,
                               bool file_based)
{
    assert(addr);
    assert(sockname.length());

    size_t size = 0;
    memset(addr, 0, sizeof(*addr));
    addr->sun_family = AF_UNIX;

    if (file_based) {
        int ret = snprintf(addr->sun_path, sizeof(addr->sun_path),
                           "%s", sockname.c_str());

        if (ret < 0 || static_cast<size_t>(ret) >= sizeof(addr->sun_path)) {
            return -1;
        }

        size = SUN_LEN(addr);

    } else {
        size = sockname.length();
        if (size > sizeof(addr->sun_path) - 1) {
            return -1;
        }

        memcpy(addr->sun_path + 1, sockname.data(), size);
        size += sizeof(*addr) - sizeof(addr->sun_path) + 1;
    }

    return static_cast<socklen_t>(size);
}

} // namespace flinter

#endif // __unix__
