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

#ifndef __FLINTER_LINKAGE_EASY_SERVER_H__
#define __FLINTER_LINKAGE_EASY_SERVER_H__

#include <stddef.h>
#include <stdint.h>

#include <list>
#include <queue>
#include <string>

#include <flinter/types/unordered_map.h>
#include <flinter/factory.h>
#include <flinter/runnable.h>

namespace flinter {

class Condition;
class EasyContext;
class EasyHandler;
class EasyTuner;
class FixedThreadPool;
class Linkage;
class LinkagePeer;
class LinkageWorker;
class Listener;
class Mutex;
class MutexLocker;

class EasyServer {
public:
    typedef uint64_t channel_t;

    struct Configure {
        int64_t incoming_receive_timeout;
        int64_t incoming_send_timeout;
        int64_t incoming_idle_timeout;

        int64_t outgoing_receive_timeout;
        int64_t outgoing_send_timeout;
        int64_t outgoing_idle_timeout;

         size_t maximum_incoming_connections;

    }; // struct Configure

    EasyServer();
    virtual ~EasyServer();

    /// Call before Initialize().
    /// @param interval nanoseconds.
    /// @param timer will be released after executed.
    bool RegisterTimer(int64_t interval, Runnable *timer);

    /// Call before Initialize().
    /// @param easy_handler life span NOT taken, keep it valid.
    bool Listen(uint16_t port, EasyHandler *easy_handler);

    /// Call before Initialize().
    /// @param easy_factory life span NOT taken, keep it valid.
    bool Listen(uint16_t port, Factory<EasyHandler> *easy_factory);

    /// Call before Initialize(), change content directly.
    Configure *configure()
    {
        return &_configure;
    }

    /// @param slots how many additional I/O threads, typically 1~4.
    /// @param workers how many job threads, 0 to share I/O threads.
    /// @param easy_tuner life span NOT taken, keep it valid.
    bool Initialize(size_t slots,
                    size_t workers,
                    EasyTuner *easy_tuner = NULL);

    void Disconnect(channel_t channel, bool finish_write = true);
    void Disconnect(const EasyContext &context, bool finish_write = true);

    /// For disconnected incoming connections, message is silently dropped.
    /// For disconnected outgoing connections, new connection is made and sent.
    bool Send(channel_t channel, const void *buffer, size_t length);
    bool Send(const EasyContext &context, const void *buffer, size_t length);

    /// Allocate channel for outgoing connection.
    /// Call after Initialize().
    /// @sa Forget()
    channel_t ConnectTcp4(const std::string &host,
                          uint16_t port,
                          EasyHandler *easy_handler);

    /// Allocate channel for outgoing connection.
    /// Call after Initialize().
    /// @sa Forget()
    channel_t ConnectTcp4(const std::string &host,
                          uint16_t port,
                          Factory<EasyHandler> *easy_factory);

    /// Remove outgoing connection information after disconnected.
    void Forget(channel_t channel);
    void Forget(const EasyContext &context);

    /// @param job will be released after executed.
    void QueueOrExecuteJob(Runnable *job);

    /// Thread safe.
    bool Shutdown();

    static bool IsOutgoingChannel(channel_t channel)
    {
        return !!(channel & 0x8000000000000000ull);
    }

    static bool IsInvalidChannel(channel_t channel)
    {
        return channel == 0;
    }

    static const channel_t kInvalidChannel;

private:
    class ProxyLinkageWorker;
    class ProxyListener;
    class ProxyLinkage;
    class ProxyHandler;
    class JobWorker;

    class OutgoingInformation {
    public:
        OutgoingInformation(ProxyHandler *proxy_handler,
                            const std::string &host,
                            uint16_t port) : _proxy_handler(proxy_handler)
                                           , _host(host), _port(port) {}

        ProxyHandler *proxy_handler() const { return _proxy_handler; }
        const std::string &host() const     { return _host;          }
        uint16_t port() const               { return _port;          }

    private:
        ProxyHandler *_proxy_handler;
        std::string _host;
        uint16_t _port;

    }; // class OutgoingInformation

    // Locked.
    void ReleaseChannel(channel_t channel);

    // Called by JobWorker thread.
    Runnable *GetJob();

    // Called by ProxyListener.
    Linkage *AllocateChannel(const LinkagePeer &peer,
                             const LinkagePeer &me,
                             ProxyHandler *proxy_handler);

    // Called by EasyServer itself.
    channel_t AllocateChannel(bool incoming_or_outgoing);
    bool AttachListeners(LinkageWorker *worker);
    bool DoShutdown(MutexLocker *locker);
    void DoAppendJob(Runnable *job); // No lock.
    void AppendJob(Runnable *job); // Locked.
    void DoDumpJobs();

    // Locked.
    bool DoListen(uint16_t port,
                  EasyHandler *easy_handler,
                  Factory<EasyHandler> *easy_factory);

    // Locked.
    channel_t DoConnectTcp4(const std::string &host,
                            uint16_t port,
                            EasyHandler *easy_handler,
                            Factory<EasyHandler> *easy_factory);

    LinkageWorker *GetRandomIoWorker() const;

    ProxyLinkage *DoReconnect(channel_t channel,
                              const OutgoingInformation &info);

    static const Configure kDefaultConfigure;

    std::list<std::pair<Runnable *, int64_t> > _timers;
    std::list<ProxyHandler *> _proxy_handlers;
    std::list<LinkageWorker *> _io_workers;
    std::list<JobWorker *> _job_workers;
    std::list<Listener *> _listeners;
    std::queue<Runnable *> _jobs;
    size_t _workers;
    size_t _slots;

    FixedThreadPool *const _pool;
    Condition *const _incoming;
    Mutex *const _mutex;

    typedef std::unordered_map<channel_t, OutgoingInformation> outgoing_map_t;
    typedef std::unordered_map<channel_t, ProxyLinkage *> channel_map_t;

    outgoing_map_t _outgoing_informations;
    channel_map_t _channel_linkages;
    Configure _configure;
    channel_t _channel;

    // Connection management.
    size_t _incoming_connections;
    size_t _outgoing_connections;

}; // class EasyServer

} // namespace flinter

#endif // __FLINTER_LINKAGE_EASY_SERVER_H__
