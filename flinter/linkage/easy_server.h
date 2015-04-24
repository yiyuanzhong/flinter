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
#include <flinter/runnable.h>

namespace flinter {

class Condition;
class EasyHandler;
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

    }; // struct Configure

    /// @param handler life span NOT taken, keep it valid.
    explicit EasyServer(EasyHandler *handler);
    virtual ~EasyServer();

    /// Call before Initialize().
    /// @param interval nanoseconds.
    /// @param timer will be released after executed.
    bool RegisterTimer(int64_t interval, Runnable *timer);

    /// Call before Initialize().
    bool Listen(uint16_t port);

    /// Call before Initialize(), change content directly.
    Configure *configure();

    /// @param slots how many additional I/O threads, typically 1~4.
    /// @param workers how many job threads, 0 to share I/O threads.
    bool Initialize(size_t slots, size_t workers);

    void Disconnect(channel_t channel, bool finish_write = true);

    /// For disconnected incoming connections, message is silently dropped.
    /// For disconnected outgoing connections, new connection is made and sent.
    bool Send(channel_t channel, const void *buffer, size_t length);

    /// Allocate channel for outgoing connection.
    /// Call after Initialize().
    /// @sa Forget()
    bool ConnectTcp4(const std::string &host,
                     uint16_t port,
                     channel_t *channel);

    /// Remove outgoing connection information after disconnected.
    void Forget(channel_t channel);

    /// @param job will be released after executed.
    void QueueOrExecuteJob(Runnable *job);

    /// Thread safe.
    bool Shutdown();

private:
    class ProxyLinkageWorker;
    class ProxyListener;
    class ProxyLinkage;
    class ProxyHandler;
    class JobWorker;

    static bool IsOutgoingChannel(channel_t channel)
    {
        return !!(channel & 0x8000000000000000ull);
    }

    // Locked.
    void ReleaseChannel(channel_t channel);

    // Called by LinkageWorker thread.
    void QueueOrExecuteJob(ProxyLinkage *linkage,
                           const void *buffer,
                           size_t length);

    // Called by JobWorker thread.
    Runnable *GetJob();

    // Called by ProxyListener.
    Linkage *AllocateChannel(const LinkagePeer &peer, const LinkagePeer &me);

    // Called by EasyServer itself.
    channel_t AllocateChannel(bool incoming_or_outgoing);
    bool AttachListeners(LinkageWorker *worker);
    bool DoShutdown(MutexLocker *locker);
    void DoAppendJob(Runnable *job); // No lock.
    void AppendJob(Runnable *job); // Locked.
    void DoDumpJobs();

    LinkageWorker *GetRandomIoWorker() const;

    ProxyLinkage *DoReconnect(channel_t channel,
                              const std::string &host,
                              uint16_t port);

    static const Configure kDefaultConfigure;

    std::list<std::pair<Runnable *, int64_t> > _timers;
    std::list<LinkageWorker *> _io_workers;
    std::list<JobWorker *> _job_workers;
    std::list<Listener *> _listeners;
    std::queue<Runnable *> _jobs;
    ProxyHandler *_proxy_handler;
    EasyHandler *_easy_handler;
    FixedThreadPool *_pool;
    size_t _workers;
    size_t _slots;

    Condition *_incoming;
    Mutex *_mutex;

    typedef std::unordered_map<channel_t, ProxyLinkage *> channel_map_t;
    typedef std::unordered_map<channel_t,
            std::pair<std::string, uint16_t> > outgoing_map_t;

    outgoing_map_t _outgoing_informations;
    channel_map_t _channel_linkages;
    Mutex *_channel_mutex;
    Configure _configure;
    channel_t _channel;

}; // class EasyServer

} // namespace flinter

#endif // __FLINTER_LINKAGE_EASY_SERVER_H__
