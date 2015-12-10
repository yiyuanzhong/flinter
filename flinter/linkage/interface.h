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

#ifndef  __FLINTER_LINKAGE_INTERFACE_H__
#define  __FLINTER_LINKAGE_INTERFACE_H__

#if defined(__unix__) || defined(__MACH__)

#include <arpa/inet.h>
#include <sys/un.h>
#include <stdint.h>

#include <string>

namespace flinter {

class LinkagePeer;

class Interface {
public:
    Interface();
    ~Interface();

    /**
     * Create, bind and listen for a unix domain socket.
     *
     * @param sockname is the name without leading zero.
     * @param file_based sockets will have a file on disk, removed automatically if
     *        possible, unless chroot() is called, permission denied or the program
     *        crashed.
     * @param privileged umask 600 or 0666 for file based, ignored if not file based.
     *
     */
    bool ListenUnix(const std::string &sockname, bool file_based = true, bool privileged = false);

    /// IPv6 any interface can accept IPv4 connections, but loopback interface can't.
    /// @param port port in host order.
    /// @param loopback listen on local loopback interface.
    bool ListenTcp6(uint16_t port, bool loopback);

    /// @param port port in host order.
    /// @param loopback listen on local loopback interface.
    bool ListenTcp4(uint16_t port, bool loopback);

    /// IPv6 any interface can accept IPv4 connections, but loopback interface can't.
    /// @param port port in host order.
    /// @param loopback listen on local loopback interface.
    bool ListenTcp(uint16_t port, bool loopback);

    /// @param peer can NOT be NULL.
    /// @param me can be NULL.
    bool Accept(LinkagePeer *peer, LinkagePeer *me = NULL);

    /// If you accept from some other places.
    bool Accepted(int fd);

    /// <0 failed, 0 connected, >0 in progress.
    int ConnectUnix(const std::string &sockname,
                    bool file_based = true,
                    LinkagePeer *peer = NULL,
                    LinkagePeer *me = NULL);

    /// <0 failed, 0 connected, >0 in progress.
    /// @param hostname target hostname.
    /// @param port port in host order.
    /// @param peer save peer information if not NULL.
    int ConnectTcp4(const std::string &hostname,
                    uint16_t port,
                    LinkagePeer *peer = NULL,
                    LinkagePeer *me = NULL);

    /// @param milliseconds to wait, <0 for infinity.
    /// @warning only call to this method if connect() gives "in progress".
    bool WaitUntilConnected(int milliseconds);

    /// @warning only call to this method if connect() gives "in progress" and the fd is
    ///          writable for the first time.
    bool TestIfConnected();

    /// Shutdown (but keep fd) all underlying sockets.
    bool Shutdown();

    /// Close all underlying sockets.
    bool Close();

    int domain() const
    {
        return _domain;
    }

    bool unix_socket() const
    {
        return _domain == AF_UNIX;
    }

    int fd() const
    {
        return _socket;
    }

    static const int kMaximumQueueLength = 256;

private:
    static int CreateSocket(int domain, int type, int protocol);
    static int BindIPv6(uint16_t port, bool loopback);
    static int BindIPv4(uint16_t port, bool loopback);

    static ssize_t FillAddress(struct sockaddr_un *addr,
                               const std::string &sockname,
                               bool file_based);

    bool DoListenTcp(uint16_t port, bool loopback, int domain);

    std::string _sockname;
    bool _file_based;

    int _socket;
    int _domain;
    bool _client;

}; // class Interface

} // namespace flinter

#endif // defined(__unix__) || defined(__MACH__)

#endif //__FLINTER_LINKAGE_INTERFACE_H__
