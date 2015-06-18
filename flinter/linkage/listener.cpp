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

#include "flinter/linkage/listener.h"

#include <errno.h>

#include "flinter/linkage/interface.h"
#include "flinter/linkage/linkage_peer.h"
#include "flinter/linkage/linkage_worker.h"
#include "flinter/logger.h"
#include "flinter/safeio.h"
#include "flinter/utility.h"

namespace flinter {

Listener::Listener() : _listener(new Interface)
{
    // Intended left blank.
}

Listener::~Listener()
{
    delete _listener;
}

bool Listener::Cleanup(int64_t /*now*/)
{
    return true;
}

bool Listener::Attach(LinkageWorker *worker)
{
    if (!worker) {
        return false;
    } else if (_workers.find(worker) != _workers.end()) {
        return true;
    }

    if (!DoAttach(worker, _listener->fd(), true, false)) {
        return false;
    }

    _workers.insert(worker);
    return true;
}

bool Listener::Detach(LinkageWorker *worker)
{
    if (!worker) {
        return false;
    } else if (_workers.find(worker) == _workers.end()) {
        return true;
    }

    if (!DoDetach(worker)) {
        return false;
    }

    _workers.erase(worker);
    return true;
}

int Listener::OnReadable(LinkageWorker *worker)
{
    LinkagePeer me;
    LinkagePeer peer;
    if (!_listener->Accept(&peer, &me)) {
        if (errno == EINTR          ||
            errno == EAGAIN         ||
            errno == EWOULDBLOCK    ||
            errno == ECONNABORTED   ){

            return 1;
        }

        CLOG.Warn("Listener: failed to accept: %d: %s", errno, strerror(errno));
        return -1;
    }

    if (set_non_blocking_mode(peer.fd()) ||
        set_close_on_exec(peer.fd())     ){

        CLOG.Warn("Listener: failed to set fd properties for fd = %d", peer.fd());
        safe_close(peer.fd());
        return 1;
    }

    LinkageBase *linkage = CreateLinkage(peer, me);
    if (!linkage) {
        CLOG.Warn("Listener: failed to create client from fd = %d", peer.fd());
        safe_close(peer.fd());
        return 1;
    }

    if (!linkage->Attach(worker)) {
        CLOG.Error("Listener: failed to attach client for fd = %d", peer.fd());
        delete linkage;
        return -1;
    }

    if (!linkage->OnConnected()) {
        CLOG.Warn("Listener: client failed to initialize for fd = %d", peer.fd());
        linkage->OnDisconnected();
        linkage->Detach(worker);
        delete linkage;
        return 1;
    }

    return 1;
}

bool Listener::ListenTcp6(uint16_t port, bool loopback)
{
    if (!_listener->ListenTcp6(port, loopback)) {
        CLOG.Warn("Listener: failed to listen on TCPv6 %s:%u",
                  loopback ? "loopback" : "any", port);

        return false;
    }

    return true;
}

bool Listener::ListenTcp4(uint16_t port, bool loopback)
{
    if (!_listener->ListenTcp4(port, loopback)) {
        CLOG.Warn("Listener: failed to listen on TCPv4 %s:%u",
                  loopback ? "loopback" : "any", port);

        return false;
    }

    return true;
}

bool Listener::ListenTcp(uint16_t port, bool loopback)
{
    if (!_listener->ListenTcp(port, loopback)) {
        CLOG.Warn("Listener: failed to listen on TCP %s:%u",
                  loopback ? "loopback" : "any", port);

        return false;
    }

    return true;
}

bool Listener::ListenUnix(const std::string &sockname,
                          bool file_based, bool privileged)
{
    if (!_listener->ListenUnix(sockname, file_based, privileged)) {
        CLOG.Warn("Listener: failed to listen on %s%s [%s]",
                  privileged ? "privileged " : "",
                  file_based ? "file" : "namespace",
                  sockname.c_str());

        return false;
    }

    return true;
}

int Listener::Shutdown()
{
    if (!_listener->Close()) {
        return -1;
    }

    return 0;
}

int Listener::OnReceived(const void *buffer, size_t length)
{
    return -1; // The hell?
}

void Listener::OnError(bool reading_or_writing, int errnum)
{
    // Intended left blank.
}

void Listener::OnDisconnected()
{
    // Intended left blank.
}

bool Listener::OnConnected()
{
    return true;
}

int Listener::OnWritable(LinkageWorker *worker)
{
    return -1; // The hell?
}

void Listener::Disconnect(bool /*finish_write*/)
{
    for (std::set<LinkageWorker *>::const_iterator p = _workers.begin();
         p != _workers.end(); ++p) {

        LinkageWorker *worker = *p;
        worker->SetWannaRead(this, false);
    }

    _listener->Close();
}

} // namespace flinter
