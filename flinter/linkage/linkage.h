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

#ifndef __FLINTER_LINKAGE_LINKAGE_H__
#define __FLINTER_LINKAGE_LINKAGE_H__

#include "flinter/linkage/linkage_base.h"

#include <sys/types.h>
#include <stdint.h>

#include <string>
#include <vector>

namespace flinter {

class Interface;
class LinkageHandler;

class Linkage : public LinkageBase {
public:
    static Linkage *ConnectTcp4(LinkageHandler *handler,
                                const std::string &host,
                                uint16_t port);

    static Linkage *ConnectUnix(LinkageHandler *handler,
                                const std::string &sockname,
                                bool file_based);

    // Accepted.
    // @param handler life span NOT taken.
    Linkage(LinkageHandler *handler,
            const LinkagePeer &peer,
            const LinkagePeer &me);

    virtual ~Linkage();

    virtual int OnReceived(const void *buffer, size_t length);
    virtual void OnError(bool reading_or_writing, int errnum);
    virtual void OnDisconnected();
    virtual bool OnConnected();

    virtual void Disconnect(bool finish_write = true);
    virtual bool Attach(LinkageWorker *worker);
    virtual bool Detach(LinkageWorker *worker);
    virtual bool Cleanup(int64_t now);

    // @param handler life span NOT taken.
    void SetHandler(LinkageHandler *handler);

    const LinkagePeer *peer() const
    {
        return _peer;
    }

    const LinkagePeer *me() const
    {
        return _me;
    }

    /// @return true sent or queued.
    virtual bool Send(const void *buffer, size_t length);

    void set_receive_timeout(int64_t timeout) { _receive_timeout = timeout; }
    void set_send_timeout   (int64_t timeout) { _send_timeout    = timeout; }
    void set_idle_timeout   (int64_t timeout) { _idle_timeout    = timeout; }

    static const int64_t kDefaultReceiveTimeout;
    static const int64_t kDefaultSendTimeout;
    static const int64_t kDefaultIdleTimeout;

protected:
    // Outbound connection.
    // @param handler life span NOT taken.
    Linkage(LinkageHandler *handler, const std::string &name);

    bool DoCheckConnected();
    bool DoConnectTcp4(const std::string &host, uint16_t port);
    bool DoConnectUnix(const std::string &sockname, bool file_based);

    // Don't call methods below explicitly.
    virtual int OnReadable(LinkageWorker *worker);
    virtual int OnWritable(LinkageWorker *worker);
    virtual int Shutdown();

    /// @param sent if some bytes are just sent.
    /// @param jammed if the message is incomplete.
    void UpdateLastSent(bool sent, bool jammed);

    void AppendSendingBuffer(const void *buffer, size_t length);
    size_t PickSendingBuffer(void *buffer, size_t length);
    void ConsumeSendingBuffer(size_t length);
    size_t GetSendingBufferSize() const;

    LinkageWorker *worker() const
    {
        return _worker;
    }

    LinkagePeer *mutable_peer()
    {
        return _peer;
    }

    LinkagePeer *mutable_me()
    {
        return _me;
    }

    bool _graceful;

private:
    typedef struct {
        Interface *interface;
        std::string name;
        bool connecting;
    } connect_t;

    static connect_t *PrepareToConnect(const std::string &name);

    /// Incoming message is incomplete.
    void UpdateReceiveJam(bool jammed);

    /// Some bytes are just received.
    void UpdateLastReceived();

    // Not a very good data structure, use it for now.
    std::vector<unsigned char> _rbuffer;

    // Only used when kernel send buffer is full.
    std::vector<unsigned char> _wbuffer;

    LinkageHandler *_handler;
    LinkageWorker *_worker;
    connect_t *_connect;
    LinkagePeer *_peer;
    LinkagePeer *_me;

    int64_t _last_received;
    int64_t _receive_jam;
    int64_t _last_sent;
    int64_t _send_jam;

    size_t _rlength;
    int64_t _idle_timeout;
    int64_t _send_timeout;
    int64_t _receive_timeout;

    explicit Linkage(const Linkage &);
    Linkage &operator = (const Linkage &);

}; // class Linkage

} // namespace flinter

#endif // __FLINTER_LINKAGE_LINKAGE_H__
