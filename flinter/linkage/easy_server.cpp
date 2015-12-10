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
#include "flinter/linkage/file_descriptor_io.h"
#include "flinter/linkage/interface.h"
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

namespace flinter {

const EasyServer::Configure EasyServer::kDefaultConfigure = {
    /* incoming_receive_timeout     = */ 5000000000LL,
    /* incoming_connect_timeout     = */ 5000000000LL,
    /* incoming_send_timeout        = */ 5000000000LL,
    /* incoming_idle_timeout        = */ 60000000000LL,
    /* outgoing_receive_timeout     = */ 5000000000LL,
    /* outgoing_connect_timeout     = */ 5000000000LL,
    /* outgoing_send_timeout        = */ 5000000000LL,
    /* outgoing_idle_timeout        = */ 60000000000LL,
    /* maximum_active_connections   = */ 50000,
};

const EasyServer::channel_t EasyServer::kInvalidChannel = 0;
const size_t EasyServer::kMaximumWorkers = 16384;
const size_t EasyServer::kMaximumSlots = 128;

class EasyServer::OutgoingInformation {
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

class EasyServer::ProxyHandler : public LinkageHandler {
public:
    virtual ~ProxyHandler() {}
    ProxyHandler(EasyHandler *easy_handler,
                 Factory<EasyHandler> *easy_factory)
            : _easy_handler(easy_handler)
            , _easy_factory(easy_factory) {}

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

    EasyHandler *easy_handler() const
    {
        return _easy_handler;
    }

    Factory<EasyHandler> *easy_factory() const
    {
        return _easy_factory;
    }

private:
    EasyHandler *const _easy_handler;
    Factory<EasyHandler> *const _easy_factory;

}; // class EasyServer::ProxyHandler

class EasyServer::ProxyLinkage : public Linkage {
public:
    ProxyLinkage(EasyContext *context, AbstractIo *io, ProxyHandler *handler,
                 const LinkagePeer &peer, const LinkagePeer &me)
            : Linkage(io, handler, peer, me), _context(context)
    {
        // Intended left blank.
    }

    virtual ~ProxyLinkage() {}
    const shared_ptr<EasyContext> &context()
    {
        return _context;
    }

private:
    const shared_ptr<EasyContext> _context;

}; // class EasyServer::ProxyLinkage

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
    Job(const shared_ptr<EasyContext> &context,
        const void *buffer, size_t length);

    virtual ~Job() {}
    virtual bool Run();

private:
    const shared_ptr<EasyContext> _context;
    const std::string _message;

}; // class EasyServer::JobWorker::Job

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

EasyServer::JobWorker::Job::Job(const shared_ptr<EasyContext> &context,
                                const void *buffer,
                                size_t length)
        : _context(context)
        , _message(reinterpret_cast<const char *>(buffer),
                   reinterpret_cast<const char *>(buffer) + length)
{
    // Intended left blank.
}

ssize_t EasyServer::ProxyHandler::GetMessageLength(Linkage *linkage,
                                                   const void *buffer,
                                                   size_t length)
{
    ProxyLinkage *l = static_cast<ProxyLinkage *>(linkage);
    EasyHandler *h = l->context()->easy_handler();
    return h->GetMessageLength(*l->context(), buffer, length);
}

int EasyServer::ProxyHandler::OnMessage(Linkage *linkage,
                                        const void *buffer,
                                        size_t length)
{
    ProxyLinkage *l = static_cast<ProxyLinkage *>(linkage);
    EasyServer *s = l->context()->easy_server();

    JobWorker::Job *job = new JobWorker::Job(l->context(), buffer, length);
    s->QueueOrExecuteJob(job);
    return 1;
}

void EasyServer::ProxyHandler::OnDisconnected(Linkage *linkage)
{
    ProxyLinkage *l = static_cast<ProxyLinkage *>(linkage);
    EasyHandler *h = l->context()->easy_handler();
    EasyServer *s = l->context()->easy_server();

    h->OnDisconnected(*l->context());
    s->ReleaseChannel(l->context()->channel());
}

bool EasyServer::ProxyHandler::OnConnected(Linkage *linkage)
{
    ProxyLinkage *l = static_cast<ProxyLinkage *>(linkage);
    EasyHandler *h = l->context()->easy_handler();
    return h->OnConnected(*l->context());
}

void EasyServer::ProxyHandler::OnError(Linkage *linkage,
                                       bool reading_or_writing,
                                       int errnum)
{
    ProxyLinkage *l = static_cast<ProxyLinkage *>(linkage);
    EasyHandler *h = l->context()->easy_handler();
    return h->OnError(*l->context(), reading_or_writing, errnum);
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
    EasyHandler *h = _context->easy_handler();
    EasyServer *s = _context->easy_server();

    int ret = h->OnMessage(*_context, _message.data(), _message.length());
    if (ret < 0) {
        h->OnError(*_context, true, errno);
        s->Disconnect(_context->channel(), false);

    } else if (ret == 0) {
        s->Disconnect(_context->channel(), true);
    }

    return true;
}

EasyServer::EasyServer()
        : _workers(0)
        , _slots(0)
        , _pool(new FixedThreadPool)
        , _incoming(new Condition)
        , _mutex(new Mutex)
        , _configure(kDefaultConfigure)
        , _channel(0)
        , _incoming_connections(0)
        , _outgoing_connections(0)
{
    // Intended left blank.
}

EasyServer::~EasyServer()
{
    Shutdown();

    delete _pool;
    delete _mutex;
    delete _incoming;
}

bool EasyServer::DoListen(uint16_t port,
                          EasyHandler *easy_handler,
                          Factory<EasyHandler> *easy_factory)
{
    MutexLocker locker(_mutex);
    ProxyHandler *proxy_handler = new ProxyHandler(easy_handler,
                                                   easy_factory);

    Listener *listener = new ProxyListener(this, proxy_handler);

    if (!listener->ListenTcp4(port, false)) {
        delete proxy_handler;
        delete listener;
        return false;
    }

    _listen_proxy_handlers.push_back(proxy_handler);
    _listeners.push_back(listener);
    return true;
}

bool EasyServer::Listen(uint16_t port, EasyHandler *easy_handler)
{
    if (!easy_handler) {
        LOG(FATAL) << "EasyServer: BUG: no handler configured.";
        return false;
    }

    return DoListen(port, easy_handler, NULL);
}

bool EasyServer::Listen(uint16_t port, Factory<EasyHandler> *easy_factory)
{
    if (!easy_factory) {
        LOG(FATAL) << "EasyServer: BUG: no handler factory configured.";
        return false;
    }

    return DoListen(port, NULL, easy_factory);
}

bool EasyServer::RegisterTimer(int64_t interval, Runnable *timer)
{
    if (interval <= 0 || !timer) {
        return false;
    }

    MutexLocker locker(_mutex);
    if (_io_workers.empty()) {
        _timers.push_back(std::make_pair(timer, interval));
        return true;
    }

    LinkageWorker *worker = GetRandomIoWorker();
    if (!worker->RegisterTimer(interval, timer, true)) {
        LOG(ERROR) << "EasyServer: failed to register timer.";
        return false;
    }

    return true;
}

bool EasyServer::Initialize(size_t slots,
                            size_t workers,
                            EasyTuner *easy_tuner)
{
    if (slots  == 0               ||
        slots   > kMaximumSlots   ||
        workers > kMaximumWorkers ){

        return false;
    }

    MutexLocker locker(_mutex);
    size_t total = slots + workers;
    if (!_pool->Initialize(total)) {
        LOG(ERROR) << "EasyServer: failed to initialize thread pool.";
        return false;
    }

    for (size_t i = 0; i < slots; ++i) {
        LinkageWorker *worker = new ProxyLinkageWorker(easy_tuner);
        _io_workers.push_back(worker);

        if (!AttachListeners(worker)        ||
            !_pool->AppendJob(worker, true) ){

            LOG(ERROR) << "EasyServer: failed to initialize I/O workers.";
            DoShutdown(&locker);
            return false;
        }
    }

    for (size_t i = 0; i < workers; ++i) {
        JobWorker *worker = new JobWorker(this, easy_tuner);
        _job_workers.push_back(worker);

        if (!_pool->AppendJob(worker, true)) {
            LOG(ERROR) << "EasyServer: failed to append job workers.";
            DoShutdown(&locker);
            return false;
        }
    }

    for (std::list<std::pair<Runnable *, int64_t> >::iterator
         p = _timers.begin(); p != _timers.end();) {

        LinkageWorker *worker = GetRandomIoWorker();
        if (!worker->RegisterTimer(p->second, p->first, true)) {
            LOG(ERROR) << "EasyServer: failed to initialize timers.";
            DoShutdown(&locker);
            return false;
        }

        p = _timers.erase(p);
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

    for (std::list<ProxyHandler *>::iterator p = _listen_proxy_handlers.begin();
         p != _listen_proxy_handlers.end(); ++p) {

        delete *p;
    }

    for (connect_map_t::iterator p = _connect_proxy_handlers.begin();
         p != _connect_proxy_handlers.end(); ++p) {

        delete p->second;
    }

    for (std::list<Listener *>::iterator p = _listeners.begin();
         p != _listeners.end(); ++p) {

        delete *p;
    }

    for (outgoing_map_t::iterator p = _outgoing_informations.begin();
         p != _outgoing_informations.end(); ++p) {

        delete p->second;
    }

    for (std::list<std::pair<Runnable *, int64_t> >::iterator
         p = _timers.begin(); p != _timers.end(); ++p) {

        delete p->first;
    }

    DoDumpJobs();

    _connect_proxy_handlers.clear();
    _listen_proxy_handlers.clear();
    _outgoing_informations.clear();
    _job_workers.clear();
    _io_workers.clear();
    _listeners.clear();
    _timers.clear();
    _workers = 0;
    _slots = 0;

    // Preserve _channel.
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
    MutexLocker locker(_mutex);
    if (_incoming_connections >= _configure.maximum_incoming_connections) {
        LOG(VERBOSE) << "EasyServer: incoming connections over limit: "
                     << _configure.maximum_incoming_connections;

        return NULL;
    }

    channel_t channel = AllocateChannel(true);
    ++_incoming_connections;

    EasyHandler *h = proxy_handler->easy_handler();
    Factory<EasyHandler> *f = proxy_handler->easy_factory();
    if (f) {
        h = f->Create();
    }

    Interface *interface = new Interface;
    if (!interface->Accepted(peer.fd())) {
        delete interface;
        return NULL;
    }

    EasyContext *context = new EasyContext(this, h, f ? true : false,
                                           channel, peer, me);

    FileDescriptorIo *io = new FileDescriptorIo(interface, false);
    ProxyLinkage *linkage = new ProxyLinkage(context, io, proxy_handler, peer, me);
    linkage->set_receive_timeout(_configure.incoming_receive_timeout);
    linkage->set_connect_timeout(_configure.incoming_connect_timeout);
    linkage->set_send_timeout(_configure.incoming_send_timeout);
    linkage->set_idle_timeout(_configure.incoming_idle_timeout);

    _channel_linkages.insert(std::make_pair(channel, linkage));
    LOG(VERBOSE) << "EasyServer: creating linkage: " << peer.ip_str();
    return linkage;
}

void EasyServer::ReleaseChannel(channel_t channel)
{
    MutexLocker locker(_mutex);
    _channel_linkages.erase(channel);
    if (!IsOutgoingChannel(channel)) {
        --_incoming_connections;
        return;
    }

    --_outgoing_connections;
    if (_outgoing_informations.find(channel) != _outgoing_informations.end()) {
        // Still needed.
        return;
    }

    LOG(VERBOSE) << "EasyServer: outgoing ProxyHandler released: " << channel;
    connect_map_t::iterator p = _connect_proxy_handlers.find(channel);
    assert(p != _connect_proxy_handlers.end());
    delete p->second;
    _connect_proxy_handlers.erase(p);
}

void EasyServer::QueueOrExecuteJob(Runnable *job)
{
    if (!job) {
        return;
    }

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
    MutexLocker locker(_mutex);
    outgoing_map_t::iterator p = _outgoing_informations.find(channel);
    if (p != _outgoing_informations.end()) {
        delete p->second;
        _outgoing_informations.erase(p);
    }

    if (_channel_linkages.find(channel) != _channel_linkages.end()) {
        // Still needed.
        return;
    }

    connect_map_t::iterator q = _connect_proxy_handlers.find(channel);
    if (q == _connect_proxy_handlers.end()) {
        return;
    }

    delete q->second;
    _connect_proxy_handlers.erase(q);
    LOG(VERBOSE) << "EasyServer: outgoing ProxyHandler released: " << channel;
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

    // TODO(yiyuanzhong): incorrect, should pass the signal to I/O threads.
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
    int r = rand() % static_cast<int>(_io_workers.size());
    if (r) {
        std::advance(p, r);
    }

    return *p;
}

EasyServer::ProxyLinkage *EasyServer::DoReconnect(
        channel_t channel,
        const OutgoingInformation *info)
{
    EasyHandler *h = info->proxy_handler()->easy_handler();
    Factory<EasyHandler> *f = info->proxy_handler()->easy_factory();
    if (f) {
        h = f->Create();
    }

    LinkagePeer me;
    LinkagePeer peer;
    Interface *interface = new Interface;
    if (!interface->ConnectTcp4(info->host(), info->port(), &peer, &me)) {
        delete interface;
        return NULL;
    }

    EasyContext *context = new EasyContext(this, h, f ? true : false,
                                           channel, peer, me);

    FileDescriptorIo *io = new FileDescriptorIo(interface, true);
    ProxyLinkage *linkage = new ProxyLinkage(context, io, info->proxy_handler(),
                                             peer, me);

    LinkageWorker *worker = GetRandomIoWorker();
    if (!linkage->Attach(worker)) {
        delete linkage;
        return NULL;
    }

    linkage->set_receive_timeout(_configure.outgoing_receive_timeout);
    linkage->set_connect_timeout(_configure.outgoing_receive_timeout);
    linkage->set_send_timeout(_configure.outgoing_send_timeout);
    linkage->set_idle_timeout(_configure.outgoing_idle_timeout);

    _channel_linkages.insert(std::make_pair(channel, linkage));
    ++_outgoing_connections;
    return linkage;
}

EasyServer::channel_t EasyServer::DoConnectTcp4(
        const std::string &host,
        uint16_t port,
        EasyHandler *easy_handler,
        Factory<EasyHandler> *easy_factory)
{
    MutexLocker locker(_mutex);
    ProxyHandler *proxy_handler = new ProxyHandler(easy_handler,
                                                   easy_factory);

    OutgoingInformation *info =
            new OutgoingInformation(proxy_handler, host, port);

    channel_t c = AllocateChannel(false);
    if (!_io_workers.empty()) {
        ProxyLinkage *linkage = DoReconnect(c, info);
        if (!linkage) {
            delete info;
            delete proxy_handler;
            return kInvalidChannel;
        }
    }

    _connect_proxy_handlers.insert(std::make_pair(c, proxy_handler));
    _outgoing_informations.insert(std::make_pair(c, info));
    return c;
}

EasyServer::channel_t EasyServer::ConnectTcp4(
        const std::string &host,
        uint16_t port,
        EasyHandler *easy_handler)
{
    if (!easy_handler) {
        LOG(FATAL) << "EasyServer: BUG: no handler configured.";
        return kInvalidChannel;
    }

    return DoConnectTcp4(host, port, easy_handler, NULL);
}

EasyServer::channel_t EasyServer::ConnectTcp4(
        const std::string &host,
        uint16_t port,
        Factory<EasyHandler> *easy_factory)
{
    if (!easy_factory) {
        LOG(FATAL) << "EasyServer: BUG: no handler factory configured.";
        return kInvalidChannel;
    }

    return DoConnectTcp4(host, port, NULL, easy_factory);
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
