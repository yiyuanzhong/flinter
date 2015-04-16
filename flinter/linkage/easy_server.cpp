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
    /* incoming_receive_timeout = */ 0,
    /* incoming_send_timeout    = */ 5000000000LL,
    /* incoming_idle_timeout    = */ 60000000000LL,
    /* outgoing_receive_timeout = */ 0,
    /* outgoing_send_timeout    = */ 5000000000LL,
    /* outgoing_idle_timeout    = */ 60000000000LL,
};

class EasyServer::ProxyLinkageWorker : public LinkageWorker {
public:
    explicit ProxyLinkageWorker(EasyHandler *handler) : _handler(handler) {}
    virtual ~ProxyLinkageWorker() {}

protected:
    virtual bool OnInitialize();
    virtual void OnShutdown();

private:
    EasyHandler *_handler;

}; // class EasyServer::ProxyLinkageWorker

class EasyServer::ProxyListener : public Listener {
public:
    explicit ProxyListener(EasyServer *parent) : _parent(parent) {}
    virtual ~ProxyListener() {}

    virtual LinkageBase *CreateLinkage(const LinkagePeer &peer,
                                       const LinkagePeer &me);

private:
    EasyServer *_parent;

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

    shared_ptr<EasyContext> &context()
    {
        return _context;
    }

private:
    shared_ptr<EasyContext> _context;

}; // class ProxyLinkage

class EasyServer::ProxyHandler : public LinkageHandler {
public:
    virtual ~ProxyHandler() {}
    ProxyHandler(EasyServer *parent, EasyHandler *handler)
            : _handler(handler), _parent(parent) {}

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
    EasyHandler *_handler;
    EasyServer *_parent;

}; // class EasyServer::ProxyHandler

class EasyServer::JobWorker : public Runnable {
public:
    virtual ~JobWorker() {}
    JobWorker(EasyServer *parent, EasyHandler *handler)
            : _handler(handler), _parent(parent) {}

    void Execute(Job *job);
    virtual bool Run();

private:
    EasyHandler *_handler;
    EasyServer *_parent;

}; // class EasyServer::JobWorker

class EasyServer::Job {
public:
    /// @param context copied.
    /// @param buffer copied.
    Job(shared_ptr<EasyContext> &context, const void *buffer, size_t length);

    const EasyContext &context() const
    {
        return *_context;
    }

    const void *buffer() const
    {
        return _message.data();
    }

    size_t length() const
    {
        return _message.length();
    }

private:
    shared_ptr<EasyContext> _context;
    std::string _message;

}; // class EasyServer::Job

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
    return _handler->OnIoThreadInitialize();
}

void EasyServer::ProxyLinkageWorker::OnShutdown()
{
    _handler->OnIoThreadShutdown();
}

EasyServer::Job::Job(shared_ptr<EasyContext> &context,
                     const void *buffer,
                     size_t length)
        : _context(context)
{
    const char *p = reinterpret_cast<const char *>(buffer);
    _message.assign(p, p + length);
}

ssize_t EasyServer::ProxyHandler::GetMessageLength(Linkage *linkage,
                                                   const void *buffer,
                                                   size_t length)
{
    ProxyLinkage *l = static_cast<ProxyLinkage *>(linkage);
    return _handler->GetMessageLength(*l->context(), buffer, length);
}

int EasyServer::ProxyHandler::OnMessage(Linkage *linkage,
                                        const void *buffer,
                                        size_t length)
{
    ProxyLinkage *l = static_cast<ProxyLinkage *>(linkage);
    Job *job = new Job(l->context(), buffer, length);
    _parent->QueueOrExecuteJob(job);
    return 1;
}

void EasyServer::ProxyHandler::OnDisconnected(Linkage *linkage)
{
    ProxyLinkage *l = static_cast<ProxyLinkage *>(linkage);
    _handler->OnDisconnected(*l->context());
    _parent->ReleaseChannel(l->context()->channel());
}

bool EasyServer::ProxyHandler::OnConnected(Linkage *linkage)
{
    ProxyLinkage *l = static_cast<ProxyLinkage *>(linkage);
    return _handler->OnConnected(*l->context());
}

void EasyServer::ProxyHandler::OnError(Linkage *linkage,
                                       bool reading_or_writing,
                                       int errnum)
{
    ProxyLinkage *l = static_cast<ProxyLinkage *>(linkage);
    return _handler->OnError(*l->context(), reading_or_writing, errnum);
}

LinkageBase *EasyServer::ProxyListener::CreateLinkage(const LinkagePeer &peer,
                                                      const LinkagePeer &me)
{
    return _parent->AllocateChannel(peer, me);
}

bool EasyServer::JobWorker::Run()
{
    _handler->OnJobThreadInitialize();

    while (true) {
        Job *job = _parent->GetJob();
        if (!job) {
            break;
        }

        Execute(job);
        delete job;
    }

    _handler->OnJobThreadShutdown();
    return true;
}

void EasyServer::JobWorker::Execute(Job *job)
{
    LOG(VERBOSE) << "JobWorker: executing [" << job << "]";
    int ret = _handler->OnMessage(job->context(), job->buffer(), job->length());
    if (ret < 0) {
        _handler->OnError(job->context(), true, errno);
        _parent->Disconnect(job->context().channel(), false);
    } else if (ret == 0) {
        _parent->Disconnect(job->context().channel(), true);
    }
}

EasyServer::EasyServer(EasyHandler *handler)
        : _easy_handler(handler)
        , _pool(new FixedThreadPool)
        , _workers(0)
        , _slots(0)
        , _incoming(new Condition)
        , _mutex(new Mutex)
        , _channel_mutex(new Mutex)
        , _configure(kDefaultConfigure)
        , _channel(0)
{
    assert(handler);
    _proxy_handler = new ProxyHandler(this, handler);
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

bool EasyServer::Listen(uint16_t port)
{
    Listener *l = new ProxyListener(this);
    if (!l->ListenTcp4(port, false)) {
        delete l;
        return false;
    }

    MutexLocker locker(_mutex);
    _listeners.push_back(l);
    return true;
}

bool EasyServer::Initialize(size_t slots, size_t workers)
{
    MutexLocker locker(_mutex);
    size_t total = slots + workers;
    if (!_pool->Initialize(total)) {
        LOG(ERROR) << "EasyServer: failed to initialize thread pool.";
        return false;
    }

    for (size_t i = 0; i < slots; ++i) {
        LinkageWorker *worker = new ProxyLinkageWorker(_easy_handler);
        _io_workers.push_back(worker);

        if (!AttachListeners(worker)        ||
            !_pool->AppendJob(worker, true) ){

            LOG(ERROR) << "EasyServer: failed to initialize I/O workers.";
            DoShutdown(&locker);
            return false;
        }
    }

    for (size_t i = 0; i < workers; ++i) {
        JobWorker *worker = new JobWorker(this, _easy_handler);
        _job_workers.push_back(worker);

        if (!_pool->AppendJob(worker, true)) {
            LOG(ERROR) << "EasyServer: failed to append job workers.";
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

    for (std::list<Listener *>::iterator p = _listeners.begin();
         p != _listeners.end(); ++p) {

        delete *p;
    }

    DoDumpJobs();

    _job_workers.clear();
    _io_workers.clear();
    _listeners.clear();
    _workers = 0;
    _slots = 0;

    return true;
}

bool EasyServer::Run()
{
    ProxyLinkageWorker worker(_easy_handler);
    if (!AttachListeners(&worker)) {
        return false;
    }

    return worker.Run();
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

EasyServer::Job *EasyServer::GetJob()
{
    MutexLocker locker(_mutex);
    while (_jobs.empty()) {
        _incoming->Wait(_mutex);
    }

    Job *job = _jobs.front();
    _jobs.pop();
    return job;
}

void EasyServer::AppendJob(Job *job)
{
    MutexLocker locker(_mutex);
    DoAppendJob(job);
}

void EasyServer::DoAppendJob(Job *job)
{
    _jobs.push(job);
    _incoming->WakeOne();
    LOG(VERBOSE) << "EasyServer: appended job [" << job << "]";
}

Linkage *EasyServer::AllocateChannel(const LinkagePeer &peer,
                                     const LinkagePeer &me)
{
    MutexLocker locker(_channel_mutex);
    channel_t channel = AllocateChannel(true);

    ProxyLinkage *linkage = new ProxyLinkage(channel, _proxy_handler, peer, me);
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
}

void EasyServer::QueueOrExecuteJob(Job *job)
{
    MutexLocker locker(_mutex);
    if (_workers) {
        DoAppendJob(job);
        return;
    }

    locker.Unlock();

    JobWorker worker(this, _easy_handler);
    worker.Execute(job);
}

void EasyServer::Forget(channel_t channel)
{
    MutexLocker locker(_channel_mutex);
    _outgoing_informations.erase(channel);
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

        linkage = DoReconnect(channel, q->second.first, q->second.second);
        if (!linkage) {
            return false;
        }

    } else {
        return true;
    }

    // TODO(yiyuanzhong): incorrect, should pass the signal to I/O threads.
    return linkage->Send(buffer, length);
}

EasyServer::ProxyLinkage *EasyServer::DoReconnect(
        channel_t channel,
        const std::string &host,
        uint16_t port)
{
    ProxyLinkage *linkage = ProxyLinkage::ConnectTcp4(
            channel, _proxy_handler, host, port);

    if (!linkage) {
        return NULL;
    }

    // TODO(yiyuanzhong): no I/O workers? Random workers?
    if (!_io_workers.empty()) {
        LinkageWorker *worker = *_io_workers.begin();
        if (!linkage->Attach(worker)) {
            delete linkage;
            return NULL;
        }
    }

    linkage->set_receive_timeout(_configure.incoming_receive_timeout);
    linkage->set_send_timeout(_configure.incoming_send_timeout);
    linkage->set_idle_timeout(_configure.incoming_idle_timeout);

    _channel_linkages.insert(std::make_pair(channel, linkage));
    return linkage;
}

bool EasyServer::ConnectTcp4(const std::string &host,
                             uint16_t port,
                             channel_t *channel)
{
    assert(channel);
    MutexLocker locker(_mutex);
    channel_t c = AllocateChannel(false);

    ProxyLinkage *linkage = DoReconnect(c, host, port);
    if (!linkage) {
        return false;
    }

    _outgoing_informations.insert(std::make_pair(c, std::make_pair(host, port)));
    *channel = c;
    return true;
}

EasyServer::channel_t EasyServer::AllocateChannel(bool incoming_or_outgoing)
{
    uint64_t channel = ++_channel;
    if (!incoming_or_outgoing) {
        channel |= 0x8000000000000000ull;
    }

    return channel;
}

} // namespace flinter
