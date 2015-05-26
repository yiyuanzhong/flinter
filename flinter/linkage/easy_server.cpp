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

#include "flinter/linkage/easy_server.h"

#include <assert.h>

#include "flinter/linkage/easy_context.h"
#include "flinter/linkage/easy_handler.h"
#include "flinter/linkage/easy_tuner.h"
#include "flinter/linkage/linkage.h"
#include "flinter/linkage/linkage_handler.h"
#include "flinter/linkage/linkage_peer.h"
#include "flinter/linkage/linkage_worker.h"
#include "flinter/linkage/listener.h"

#include "flinter/thread/condition.h"
#include "flinter/thread/mutex.h"
#include "flinter/thread/mutex_locker.h"
#include "flinter/thread/fixed_thread_pool.h"

#include "flinter/types/shared_ptr.h"

#include "flinter/logger.h"
#include "flinter/msleep.h"

namespace flinter {

const EasyServer::Configure EasyServer::kDefaultConfigure = {
    /* incoming_receive_timeout     = */ 0,
    /* incoming_send_timeout        = */ 5000000000LL,
    /* incoming_idle_timeout        = */ 60000000000LL,
    /* outgoing_receive_timeout     = */ 0,
    /* outgoing_send_timeout        = */ 5000000000LL,
    /* outgoing_idle_timeout        = */ 60000000000LL,
    /* maximum_active_connections   = */ 50000,
};

const EasyServer::channel_t EasyServer::kInvalidChannel = 0;

static class DummyHandler : public EasyHandler {
public:
    virtual ~DummyHandler() {}
    virtual ssize_t GetMessageLength(const EasyContext &context,
                                     const void *buffer,
                                     size_t length)
    {
        return -1;
    }

    virtual int OnMessage(const EasyContext &context,
                          const void *buffer,
                          size_t length)
    {
        return -1;
    }

    virtual bool OnConnected(const EasyContext &context)
    {
        LOG(FATAL) << "EasyServer: BUG: no global handler configured.";
        return false;
    }

} kStaticDummyHandler; // class DummyHandler

class EasyServer::ProxyLinkageWorker : public LinkageWorker {
public:
    explicit ProxyLinkageWorker(EasyTuner *tuner) : _tuner(tuner) {}
    virtual ~ProxyLinkageWorker() {}

protected:
    virtual bool OnInitialize();
    virtual void OnShutdown();

private:
    EasyTuner *const _tuner;

}; // class EasyServer::ProxyLinkageWorker

class EasyServer::ProxyListener : public Listener {
public:
    explicit ProxyListener(EasyServer *easy_server, ProxyHandler *proxy_handler)
            : _easy_server(easy_server), _proxy_handler(proxy_handler) {}
    virtual ~ProxyListener() {}

    virtual LinkageBase *CreateLinkage(const LinkagePeer &peer,
                                       const LinkagePeer &me);

private:
    EasyServer *const _easy_server;
    ProxyHandler *const _proxy_handler;

}; // class EasyServer::ProxyListener

class EasyServer::ProxyLinkage : public Linkage {
public:
    static ProxyLinkage *ConnectTcp4(channel_t channel,
                                     ProxyHandler *handler,
                                     const std::string &host,
                                     uint16_t port);

    virtual ~ProxyLinkage() {}
    ProxyLinkage(channel_t channel,
                 LinkageHandler *handler,
                 const std::string &name)
            : Linkage(handler, name)
            , _context(new EasyContext(channel, this)) {}

    ProxyLinkage(channel_t channel,
                 LinkageHandler *handler,
                 const LinkagePeer &peer,
                 const LinkagePeer &me)
            : Linkage(handler, peer, me)
            , _context(new EasyContext(channel, this)) {}

    const shared_ptr<EasyContext> &context()
    {
        return _context;
    }

private:
    const shared_ptr<EasyContext> _context;

}; // class EasyServer::ProxyLinkage

class EasyServer::ProxyHandler : public LinkageHandler {
public:
    virtual ~ProxyHandler() {}
    ProxyHandler(EasyServer *easy_server, EasyHandler *easy_handler)
            : _easy_server(easy_server), _easy_handler(easy_handler) {}

    virtual ssize_t GetMessageLength(Linkage *linkage,
                                     const void *buffer,
                                     size_t length);

    virtual int OnMessage(Linkage *linkage,
                          const void *buffer,
                          size_t length);

    virtual void OnDisconnected(Linkage *linkage);
    virtual bool OnConnected(Linkage *linkage);
    virtual void OnError(Linkage *linkage,
                         bool reading_or_writing,
                         int errnum);

private:
    EasyServer *const _easy_server;
    EasyHandler *const _easy_handler;

}; // class EasyServer::ProxyHandler

class EasyServer::JobWorker : public Runnable {
public:
    JobWorker(EasyServer *easy_server, EasyTuner *easy_tuner)
            : _easy_tuner(easy_tuner), _easy_server(easy_server) {}

    virtual ~JobWorker() {}
    virtual bool Run();
    class Job;

private:
    EasyTuner *const _easy_tuner;
    EasyServer *const _easy_server;

}; // class EasyServer::JobWorker

class EasyServer::JobWorker::Job : public Runnable {
public:
    /// @param context COWed.
    /// @param buffer copied.
    Job(EasyServer *server,
        EasyHandler *handler,
        const shared_ptr<EasyContext> &context,
        const void *buffer, size_t length);

    virtual ~Job() {}
    virtual bool Run();

private:
    const shared_ptr<EasyContext> _context;
    EasyHandler *const _easy_handler;
    const std::string _message;
    EasyServer *const _server;

}; // class EasyServer::JobWorker::Job

EasyServer::ProxyLinkage *EasyServer::ProxyLinkage::ConnectTcp4(
        channel_t channel,
        ProxyHandler *handler,
        const std::string &host,
        uint16_t port)
{
    ProxyLinkage *linkage = new ProxyLinkage(channel, handler, host);
    if (!linkage->DoConnectTcp4(host, port)) {
        delete linkage;
        return NULL;
    }

    return linkage;
}

bool EasyServer::ProxyLinkageWorker::OnInitialize()
{
    if (!_tuner) {
        return true;
    }

    return _tuner->OnIoThreadInitialize();
}

void EasyServer::ProxyLinkageWorker::OnShutdown()
{
    if (!_tuner) {
        return;
    }

    _tuner->OnIoThreadShutdown();
}

EasyServer::JobWorker::Job::Job(EasyServer *server,
                                EasyHandler *easy_handler,
                                const shared_ptr<EasyContext> &context,
                                const void *buffer,
                                size_t length)
        : _context(context)
        , _easy_handler(easy_handler)
        , _message(reinterpret_cast<const char *>(buffer),
                   reinterpret_cast<const char *>(buffer) + length)
        , _server(server)
{
    // Intended left blank.
}

ssize_t EasyServer::ProxyHandler::GetMessageLength(Linkage *linkage,
                                                   const void *buffer,
                                                   size_t length)
{
    ProxyLinkage *l = static_cast<ProxyLinkage *>(linkage);
    return _easy_handler->GetMessageLength(*l->context(), buffer, length);
}

int EasyServer::ProxyHandler::OnMessage(Linkage *linkage,
                                        const void *buffer,
                                        size_t length)
{
    ProxyLinkage *l = static_cast<ProxyLinkage *>(linkage);
    JobWorker::Job *job = new JobWorker::Job(_easy_server, _easy_handler,
                                             l->context(), buffer, length);

    _easy_server->QueueOrExecuteJob(job);
    return 1;
}

void EasyServer::ProxyHandler::OnDisconnected(Linkage *linkage)
{
    ProxyLinkage *l = static_cast<ProxyLinkage *>(linkage);
    _easy_handler->OnDisconnected(*l->context());
    _easy_server->ReleaseChannel(l->context()->channel());
}

bool EasyServer::ProxyHandler::OnConnected(Linkage *linkage)
{
    ProxyLinkage *l = static_cast<ProxyLinkage *>(linkage);
    return _easy_handler->OnConnected(*l->context());
}

void EasyServer::ProxyHandler::OnError(Linkage *linkage,
                                       bool reading_or_writing,
                                       int errnum)
{
    ProxyLinkage *l = static_cast<ProxyLinkage *>(linkage);
    return _easy_handler->OnError(*l->context(), reading_or_writing, errnum);
}

LinkageBase *EasyServer::ProxyListener::CreateLinkage(const LinkagePeer &peer,
                                                      const LinkagePeer &me)
{
    return _easy_server->AllocateChannel(peer, me, _proxy_handler);
}

bool EasyServer::JobWorker::Run()
{
    if (_easy_tuner) {
        if (!_easy_tuner->OnJobThreadInitialize()) {
            LOG(ERROR) << "EasyServer: failed to initialize job worker.";

            // TODO(yiyuanzhong): but what can I do?
            return false;
        }
    }

    while (true) {
        Runnable *job = _easy_server->GetJob();
        if (!job) {
            break;
        }

        job->Run();
        delete job;
    }

    if (_easy_tuner) {
        _easy_tuner->OnJobThreadShutdown();
    }

    return true;
}

bool EasyServer::JobWorker::Job::Run()
{
    LOG(VERBOSE) << "JobWorker: executing [" << this << "]";
    int ret = _easy_handler->OnMessage(*_context, _message.data(), _message.length());
    if (ret < 0) {
        _easy_handler->OnError(*_context, true, errno);
        _server->Disconnect(_context->channel(), false);

    } else if (ret == 0) {
        _server->Disconnect(_context->channel(), true);
    }

    return true;
}

EasyServer::EasyServer(EasyHandler *easy_handler, EasyTuner *easy_tuner)
        : _proxy_handler(NULL)
        , _workers(0)
        , _slots(0)
        , _easy_handler(easy_handler ? easy_handler : &kStaticDummyHandler)
        , _easy_tuner(easy_tuner)
        , _pool(new FixedThreadPool)
        , _incoming(new Condition)
        , _mutex(new Mutex)
        , _channel_mutex(new Mutex)
        , _configure(kDefaultConfigure)
        , _channel(0)
        , _incoming_connections(0)
        , _outgoing_connections(0)
{
    assert(_easy_handler);
    _proxy_handler = new ProxyHandler(this, _easy_handler);
}

EasyServer::~EasyServer()
{
    Shutdown();

    delete _proxy_handler;
    delete _pool;

    delete _incoming;
    delete _channel_mutex;
    delete _mutex;
}

bool EasyServer::Listen(uint16_t port, EasyHandler *easy_handler)
{
    MutexLocker locker(_mutex);
    if (!easy_handler && _easy_handler == &kStaticDummyHandler) {
        LOG(FATAL) << "EasyServer: BUG: no global handler configured.";
        return false;
    }

    ProxyHandler *proxy_handler = NULL;
    if (easy_handler) {
        proxy_handler = new ProxyHandler(this, easy_handler);
    }

    Listener *listener = new ProxyListener(this, proxy_handler ? proxy_handler
                                                               : _proxy_handler);

    if (!listener->ListenTcp4(port, false)) {
        delete proxy_handler;
        delete listener;
        return false;
    }

    if (proxy_handler) {
        _proxy_handlers.push_back(proxy_handler);
    }

    _listeners.push_back(listener);
    return true;
}

bool EasyServer::RegisterTimer(int64_t interval, Runnable *timer)
{
    if (interval <= 0 || !timer) {
        return false;
    }

    MutexLocker locker(_mutex);
    _timers.push_back(std::make_pair(timer, interval));
    return true;
}

bool EasyServer::Initialize(size_t slots, size_t workers)
{
    if (!slots) {
        return false;
    }

    MutexLocker locker(_mutex);
    size_t total = slots + workers;
    if (!_pool->Initialize(total)) {
        LOG(ERROR) << "EasyServer: failed to initialize thread pool.";
        return false;
    }

    for (size_t i = 0; i < slots; ++i) {
        LinkageWorker *worker = new ProxyLinkageWorker(_easy_tuner);
        _io_workers.push_back(worker);

        if (!AttachListeners(worker)        ||
            !_pool->AppendJob(worker, true) ){

            LOG(ERROR) << "EasyServer: failed to initialize I/O workers.";
            DoShutdown(&locker);
            return false;
        }
    }

    for (size_t i = 0; i < workers; ++i) {
        JobWorker *worker = new JobWorker(this, _easy_tuner);
        _job_workers.push_back(worker);

        if (!_pool->AppendJob(worker, true)) {
            LOG(ERROR) << "EasyServer: failed to append job workers.";
            DoShutdown(&locker);
            return false;
        }
    }

    for (std::list<std::pair<Runnable *, int64_t> >::iterator
         p = _timers.begin(); p != _timers.end(); ++p) {

        LinkageWorker *worker = GetRandomIoWorker();
        if (!worker->RegisterTimer(p->second, p->first, true)) {
            LOG(ERROR) << "EasyServer: failed to initialize timers.";
            DoShutdown(&locker);
            return false;
        }
    }

    _workers = workers;
    _slots = slots;
    return true;
}

bool EasyServer::Shutdown()
{
    MutexLocker locker(_mutex);
    return DoShutdown(&locker);
}

void EasyServer::DoDumpJobs()
{
    while (!_jobs.empty()) {
        delete _jobs.front();
        _jobs.pop();
    }
}

bool EasyServer::DoShutdown(MutexLocker *locker)
{
    DoDumpJobs();
    for (size_t i = 0; i < _workers; ++i) {
        DoAppendJob(NULL);
    }

    for (std::list<LinkageWorker *>::const_iterator p = _io_workers.begin();
         p != _io_workers.end(); ++p) {

        (*p)->Shutdown();
    }

    // Job workers and I/O workers are released as well.
    locker->Unlock();
    _pool->Shutdown();
    locker->Relock();

    for (std::list<ProxyHandler *>::iterator p = _proxy_handlers.begin();
         p != _proxy_handlers.end(); ++p) {

        delete *p;
    }

    for (std::list<Listener *>::iterator p = _listeners.begin();
         p != _listeners.end(); ++p) {

        delete *p;
    }

    DoDumpJobs();

    _proxy_handlers.clear();
    _job_workers.clear();
    _io_workers.clear();
    _listeners.clear();
    _timers.clear();
    _workers = 0;
    _slots = 0;

    return true;
}

bool EasyServer::AttachListeners(LinkageWorker *worker)
{
    for (std::list<Listener *>::const_iterator p = _listeners.begin();
         p != _listeners.end(); ++p) {

        Listener *l = *p;
        if (!l->Attach(worker)) {
            return false;
        }
    }

    return true;
}

Runnable *EasyServer::GetJob()
{
    MutexLocker locker(_mutex);
    while (_jobs.empty()) {
        _incoming->Wait(_mutex);
    }

    Runnable *job = _jobs.front();
    _jobs.pop();
    return job;
}

void EasyServer::AppendJob(Runnable *job)
{
    MutexLocker locker(_mutex);
    DoAppendJob(job);
}

void EasyServer::DoAppendJob(Runnable *job)
{
    _jobs.push(job);
    _incoming->WakeOne();
    LOG(VERBOSE) << "EasyServer: appended job [" << job << "]";
}

Linkage *EasyServer::AllocateChannel(const LinkagePeer &peer,
                                     const LinkagePeer &me,
                                     ProxyHandler *proxy_handler)
{
    MutexLocker locker(_channel_mutex);
    if (_incoming_connections >= _configure.maximum_incoming_connections) {
        LOG(VERBOSE) << "EasyServer: incoming connections over limit: "
                     << _configure.maximum_incoming_connections;

        return NULL;
    }

    channel_t channel = AllocateChannel(true);
    ++_incoming_connections;

    ProxyLinkage *linkage = new ProxyLinkage(channel, proxy_handler, peer, me);
    linkage->set_receive_timeout(_configure.incoming_receive_timeout);
    linkage->set_send_timeout(_configure.incoming_send_timeout);
    linkage->set_idle_timeout(_configure.incoming_idle_timeout);

    _channel_linkages.insert(std::make_pair(channel, linkage));
    LOG(VERBOSE) << "EasyServer: creating linkage: " << peer.ip_str();
    return linkage;
}

void EasyServer::ReleaseChannel(channel_t channel)
{
    MutexLocker locker(_channel_mutex);
    _channel_linkages.erase(channel);
    if (IsOutgoingChannel(channel)) {
        --_outgoing_connections;
    } else {
        --_incoming_connections;
    }
}

void EasyServer::QueueOrExecuteJob(Runnable *job)
{
    MutexLocker locker(_mutex);
    if (_workers) {
        DoAppendJob(job);
        return;
    }

    locker.Unlock();
    job->Run();
    delete job;
}

void EasyServer::Forget(channel_t channel)
{
    MutexLocker locker(_channel_mutex);
    _outgoing_informations.erase(channel);
}

void EasyServer::Forget(const EasyContext &context)
{
    Forget(context.channel());
}

void EasyServer::Disconnect(channel_t channel, bool finish_write)
{
    MutexLocker locker(_mutex);
    channel_map_t::iterator p = _channel_linkages.find(channel);
    if (p == _channel_linkages.end()) {
        return;
    }

    ProxyLinkage *linkage = p->second;
    linkage->Disconnect(finish_write);
}

void EasyServer::Disconnect(const EasyContext &context, bool finish_write)
{
    Disconnect(context.channel(), finish_write);
}

bool EasyServer::Send(channel_t channel, const void *buffer, size_t length)
{
    MutexLocker locker(_mutex);
    ProxyLinkage *linkage = NULL;
    channel_map_t::iterator p = _channel_linkages.find(channel);

    if (p != _channel_linkages.end()) {
        linkage = p->second;

    } else if (IsOutgoingChannel(channel)) {
        outgoing_map_t::const_iterator q = _outgoing_informations.find(channel);
        if (q == _outgoing_informations.end()) {
            return true;
        }

        linkage = DoReconnect(channel, q->second);
        if (!linkage) {
            return false;
        }

    } else {
        return true;
    }

    // TODO(yiyuanzhong): incorrect, should pass the signal to I/O threads.
    return linkage->Send(buffer, length);
}

bool EasyServer::Send(const EasyContext &context, const void *buffer, size_t length)
{
    return Send(context.channel(), buffer, length);
}

LinkageWorker *EasyServer::GetRandomIoWorker() const
{
    std::list<LinkageWorker *>::const_iterator p = _io_workers.begin();
    int r = rand() % _io_workers.size();
    if (r) {
        std::advance(p, r);
    }

    return *p;
}

EasyServer::ProxyLinkage *EasyServer::DoReconnect(
        channel_t channel,
        const OutgoingInformation &info)
{
    ProxyLinkage *linkage = ProxyLinkage::ConnectTcp4(
            channel, info.proxy_handler(), info.host(), info.port());

    if (!linkage) {
        return NULL;
    }

    LinkageWorker *worker = GetRandomIoWorker();
    if (!linkage->Attach(worker)) {
        delete linkage;
        return NULL;
    }

    linkage->set_receive_timeout(_configure.incoming_receive_timeout);
    linkage->set_send_timeout(_configure.incoming_send_timeout);
    linkage->set_idle_timeout(_configure.incoming_idle_timeout);

    _channel_linkages.insert(std::make_pair(channel, linkage));
    ++_outgoing_connections;
    return linkage;
}

EasyServer::channel_t EasyServer::ConnectTcp4(const std::string &host,
                                              uint16_t port,
                                              EasyHandler *easy_handler)
{
    assert(channel);
    MutexLocker locker(_mutex);
    if (!easy_handler && _easy_handler == &kStaticDummyHandler) {
        LOG(FATAL) << "EasyServer: BUG: no global handler configured.";
        return kInvalidChannel;
    }

    ProxyHandler *proxy_handler = NULL;
    if (easy_handler) {
        proxy_handler = new ProxyHandler(this, easy_handler);
    }

    OutgoingInformation info(proxy_handler ? proxy_handler : _proxy_handler,
                             host, port);

    channel_t c = AllocateChannel(false);
    ProxyLinkage *linkage = DoReconnect(c, info);
    if (!linkage) {
        delete proxy_handler;
        return kInvalidChannel;
    }

    if (proxy_handler) {
        _proxy_handlers.push_back(proxy_handler);
    }

    _outgoing_informations.insert(std::make_pair(c, info));
    return c;
}

EasyServer::channel_t EasyServer::AllocateChannel(bool incoming_or_outgoing)
{
    channel_t channel = ++_channel;
    if (!incoming_or_outgoing) {
        channel |= 0x8000000000000000ull;
    }

    return channel;
}

} // namespace flinter
