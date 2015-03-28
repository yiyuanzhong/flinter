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

#include "flinter/linkage/linkage.h"

#include <assert.h>
#include <errno.h>

#include "flinter/linkage/interface.h"
#include "flinter/linkage/linkage_handler.h"
#include "flinter/linkage/linkage_peer.h"
#include "flinter/linkage/linkage_worker.h"
#include "flinter/logger.h"
#include "flinter/safeio.h"
#include "flinter/utility.h"

namespace flinter {

const int64_t Linkage::kDefaultReceiveTimeout = 0LL;            // Disabled
const int64_t Linkage::kDefaultSendTimeout    = 15000000000LL;  // 15s
const int64_t Linkage::kDefaultIdleTimeout    = 300000000000LL; // 5min

Linkage::Linkage(LinkageHandler *handler,
                 const LinkagePeer &peer,
                 const LinkagePeer &me) : _graceful(false)
                                        , _handler(handler)
                                        , _worker(NULL)
                                        , _peer(new LinkagePeer(peer))
                                        , _me(new LinkagePeer(me))
                                        , _connect(NULL)
                                        , _rlength(0)
                                        , _idle_timeout(kDefaultIdleTimeout)
                                        , _send_timeout(kDefaultSendTimeout)
                                        , _receive_timeout(kDefaultReceiveTimeout)
{
    assert(handler);

    UpdateLastReceived();
    ClearSendJam();
}

Linkage::Linkage(LinkageHandler *handler, const std::string &name)
        : _graceful(false)
        , _handler(handler)
        , _worker(NULL)
        , _peer(new LinkagePeer)
        , _me(new LinkagePeer)
        , _connect(PrepareToConnect(name))
        , _rlength(0)
        , _idle_timeout(kDefaultIdleTimeout)
        , _send_timeout(kDefaultSendTimeout)
        , _receive_timeout(kDefaultReceiveTimeout)
{
    assert(handler);

    UpdateLastReceived();
    ClearSendJam();
}

Linkage::~Linkage()
{
    delete _me;

    safe_close(_peer->fd());
    delete _peer;

    if (_connect) {
        delete _connect->interface;
        delete _connect;
    }
}

Linkage::connect_t *Linkage::PrepareToConnect(const std::string &name)
{
    connect_t *c = new connect_t;
    c->interface = new Interface;
    c->connecting = true;
    c->name = name;
    return c;
}

bool Linkage::DoConnectTcp4(const std::string &host, uint16_t port)
{
    Interface *i = _connect->interface;
    if (!i->ConnectTcp4(host, port, _peer, _me)) {
        CLOG.Warn("Linkage: failed to initiate connection to [%s]: %d: %s",
                  host.c_str(), errno, strerror(errno));

        return false;
    }

    return true;
}

bool Linkage::DoConnectUnix(const std::string &sockname, bool file_based)
{
    Interface *i = _connect->interface;
    if (!i->ConnectUnix(sockname, file_based, _peer)) {
        CLOG.Warn("Linkage: failed to initiate connection to [%s]: %d: %s",
                  sockname.c_str(), errno, strerror(errno));

        return false;
    }

    return true;
}

Linkage *Linkage::ConnectTcp4(LinkageHandler *handler,
                              const std::string &host,
                              uint16_t port)
{
    if (!handler || host.empty() || !port) {
        return NULL;
    }

    Linkage *linkage = new Linkage(handler, host);
    if (!linkage->DoConnectTcp4(host, port)) {
        delete linkage;
        return NULL;
    }

    return linkage;
}

Linkage *Linkage::ConnectUnix(LinkageHandler *handler,
                              const std::string &sockname,
                              bool file_based)
{
    if (!handler || sockname.empty()) {
        return NULL;
    }

    Linkage *linkage = new Linkage(handler, sockname);
    if (!linkage->DoConnectUnix(sockname, file_based)) {
        delete linkage;
        return NULL;
    }

    return linkage;
}

bool Linkage::Attach(LinkageWorker *worker)
{
    if (!worker || !_peer) {
        return false;
    } else if (_worker) {
        return _worker == worker;
    }

    bool connecting = _connect && _connect->connecting;
    if (!DoAttach(worker, _peer->fd(), !connecting, true)) {
        return false;
    }

    if (connecting) {
        if (!worker->SetWannaWrite(this, true)) {
            DoDetach(worker);
            return false;
        }
    }

    _worker = worker;
    return true;
}

bool Linkage::Detach(LinkageWorker *worker)
{
    if (!worker) {
        return false;
    } else if (!_worker || _worker != worker) {
        return true;
    }

    if (!DoDetach(_worker)) {
        return false;
    }

    _worker = NULL;
    return true;
}

int Linkage::OnReceived(const void *buffer, size_t length)
{
    const unsigned char *buf = reinterpret_cast<const unsigned char *>(buffer);
    if (!buffer) {
        return -1;
    } else if (!length) {
        return 1;
    }

    if (_graceful) {
        return 1;
    }

    CLOG.Verbose("Linkage: received [%lu] bytes for fd = %d", length, _peer->fd());
    _rbuffer.insert(_rbuffer.end(), buf, buf + length);
    while (true) {
        if (!_rlength) {
            ssize_t ret = _handler->GetMessageLength(this,
                                                     &_rbuffer[0],
                                                     _rbuffer.size());

            if (ret < 0) {
                CLOG.Verbose("Linkage: failed to get message length for "
                             "fd = %d", _peer->fd());

                return -1;

            } else if (ret == 0) {
                CLOG.Verbose("Linkage: getting message length but instructed "
                             "to shutdown for fd = %d", _peer->fd());

                break;
            }

            _rlength = static_cast<size_t>(ret);
            CLOG.Verbose("Linkage: got message length [%lu] for fd = %d",
                         _rlength, _peer->fd());
        }

        if (_rbuffer.size() < _rlength) {
            break;
        }

        UpdateLastReceived();
        CLOG.Verbose("Linkage: processing message of length [%lu] for fd = %d",
                     _rlength, _peer->fd());

        int next = _handler->OnMessage(this, &_rbuffer[0], _rlength);
        if (next < 0) {
            CLOG.Verbose("Linkage: failed to process message of length [%lu] "
                         "for fd = %d", _rlength, _peer->fd());

            return -1;
        } else if (next == 0) {
            CLOG.Verbose("Linkage: processing message of length [%lu] but "
                         "instructed to shutdown for fd = %d", _rlength, _peer->fd());

            return Shutdown();
        }

        std::vector<unsigned char>::iterator p = _rbuffer.begin();
        std::advance(p, _rlength);
        _rlength = 0;

        _rbuffer.erase(_rbuffer.begin(), p);
        if (_rbuffer.empty()) {
            break;
        }
    }

    return 1;
}

void Linkage::OnError(bool reading_or_writing, int errnum)
{
    _handler->OnError(this, reading_or_writing, errnum);
}

void Linkage::OnDisconnected()
{
    _handler->OnDisconnected(this);
}

bool Linkage::OnConnected()
{
    return _handler->OnConnected(this);
}

int Linkage::OnReadable(LinkageWorker *worker)
{
    unsigned char buffer[8192];
    assert(worker == _worker);
    assert(_peer);

    ssize_t ret = safe_read(_peer->fd(), buffer, sizeof(buffer));
    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 1;
        } else {
            return -1;
        }
    }

    if (ret == 0) {
        return 0;
    }

    return OnReceived(buffer, static_cast<size_t>(ret));
}

bool Linkage::Send(const void *buffer, size_t length)
{
    if (!buffer || !_worker) {
        return false;
    } else if (!length) {
        return true;
    }

    CLOG.Verbose("Linkage: sending [%lu] bytes for fd = %d", length, _peer->fd());

    if ((_connect && _connect->connecting) || GetSendingBufferSize()) {
        CLOG.Verbose("Linkage: queued [%lu] bytes for fd = %d", length, _peer->fd());
        AppendSendingBuffer(buffer, length);
        return true;
    }

    assert(_peer);
    ssize_t ret = safe_write(_peer->fd(), buffer, length);
    if (ret < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            return false;
        }

        CLOG.Verbose("Linkage: queued [%lu] bytes for fd = %d", length, _peer->fd());
        AppendSendingBuffer(buffer, length);
        _worker->SetWannaWrite(this, true);
        UpdateSendJam();

    } else if (static_cast<size_t>(ret) < length) {
        CLOG.Verbose("Linkage: sent [%lu] bytes, queued [%lu] bytes for fd = %d",
                     ret, length - ret, _peer->fd());

        const unsigned char *buf = reinterpret_cast<const unsigned char *>(buffer);
        AppendSendingBuffer(buf + ret, length - ret);
        _worker->SetWannaWrite(this, true);
        UpdateSendJam();

    } else {
        CLOG.Verbose("Linkage: sent [%lu] bytes for fd = %d", length, _peer->fd());
        ClearSendJam();
    }

    return true;
}

int Linkage::OnWritable(LinkageWorker *worker)
{
    static const size_t kMaximumBytesPerRun = 131072;
    unsigned char buffer[16384];
    size_t count = 0;

    assert(worker == _worker);
    if (_connect && _connect->connecting) {
        if (!_connect->interface->TestIfConnected()) {
            CLOG.Warn("Linkage: connecting to [%s] failed: %d: %s",
                      _connect->name.c_str(), errno, strerror(errno));

            return -1;
        }

        CLOG.Verbose("Linkage: connected to [%s].", _connect->name.c_str());
        worker->SetWannaRead(this, true);
        _connect->connecting = false;
        if (!OnConnected()) {
            return -1;
        }
    }

    while (true) {
        size_t length = PickSendingBuffer(buffer, sizeof(buffer));
        if (!length) {
            break;
        }

        assert(_peer);
        ssize_t ret = safe_write(_peer->fd(), buffer, length);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                worker->SetWannaWrite(this, true);
                MaybeUpdateSendJam();
                return 1;
            }

            return errno == EPIPE ? 0 : -1;
        }

        if (static_cast<size_t>(ret) < length) {
            UpdateSendJam();
            worker->SetWannaWrite(this, true);
            ConsumeSendingBuffer(static_cast<size_t>(ret));
            CLOG.Verbose("Linkage: dequeued [%lu] bytes and sent [%lu] bytes for fd = %d",
                         length, static_cast<size_t>(ret), _peer->fd());

            return 1;

        } else {
            ClearSendJam();
            ConsumeSendingBuffer(length);
            CLOG.Verbose("Linkage: dequeued [%lu] bytes and sent [%lu] bytes for fd = %d",
                         length, length, _peer->fd());

            // Don't overrun the link.
            count += length;
            if (count >= kMaximumBytesPerRun && GetSendingBufferSize()) {
                worker->SetWannaWrite(this, true);
                return 1;
            }
        }
    }

    worker->SetWannaWrite(this, false);
    ClearSendJam();

    if (_graceful && !GetSendingBufferSize()) {
        return Shutdown();
    }

    return 1;
}

void Linkage::UpdateLastReceived()
{
    _last_received = get_monotonic_timestamp();
}

void Linkage::MaybeUpdateSendJam()
{
    if (!_send_jam) {
        _send_jam = get_monotonic_timestamp();
    }
}

void Linkage::UpdateSendJam()
{
    _send_jam = get_monotonic_timestamp();
}

void Linkage::ClearSendJam()
{
    _send_jam = 0;
}

void Linkage::AppendSendingBuffer(const void *buffer, size_t length)
{
    assert(buffer);
    assert(length);
    const unsigned char *p = reinterpret_cast<const unsigned char *>(buffer);
    _wbuffer.insert(_wbuffer.end(), p, p + length);
}

size_t Linkage::PickSendingBuffer(void *buffer, size_t length)
{
    assert(buffer);
    assert(length);
    if (_wbuffer.empty()) {
        return 0;
    }

    size_t len = length < _wbuffer.size() ? length : _wbuffer.size();
    unsigned char *b = reinterpret_cast<unsigned char *>(buffer);
    std::vector<unsigned char>::iterator p = _wbuffer.begin();
    std::advance(p, len);
    std::copy(_wbuffer.begin(), p, b);
    return len;
}

void Linkage::ConsumeSendingBuffer(size_t length)
{
    assert(length <= _wbuffer.size());
    std::vector<unsigned char>::iterator p = _wbuffer.begin();
    std::advance(p, length);
    _wbuffer.erase(_wbuffer.begin(), p);
}

size_t Linkage::GetSendingBufferSize() const
{
    return _wbuffer.size();
}

int Linkage::Shutdown()
{
    if (shutdown(_peer->fd(), SHUT_RDWR)) {
        return -1;
    }

    return 0;
}

bool Linkage::Cleanup(int64_t now)
{
    int64_t passed_r = now - _last_received;
    int64_t passed_w = _send_jam ? now - _send_jam : 0;
    int64_t idle = !passed_w ? passed_r : (passed_r < passed_w ? passed_r : passed_w);

    if ((_send_timeout && passed_w && passed_w >= _send_timeout)  ||
        (_receive_timeout && passed_r >= _receive_timeout)        ||
        (_idle_timeout && idle >= _idle_timeout)                  ){

        return false;
    }

    return true;
}

void Linkage::SetHandler(LinkageHandler *handler)
{
    assert(handler);
    _handler = handler;
}

void Linkage::Disconnect(bool finish_write)
{
    if (!finish_write) {
        size_t size = GetSendingBufferSize();
        if (size) {
            ConsumeSendingBuffer(size);
        }
    }

    _graceful = true;
    if (!GetSendingBufferSize()) {
        Shutdown();
        if (_worker) {
            _worker->SetWanna(this, true, false);
        }

    } else if (_worker) {
        _worker->SetWanna(this, true, true);
    }
}

} // namespace flinter
