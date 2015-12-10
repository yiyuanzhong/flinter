/* Copyright 2015 yiyuanzhong@gmail.com (Yiyuan Zhong)
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

#ifndef __FLINTER_LINKAGE_ASYNC_ROUTER_H__
#define __FLINTER_LINKAGE_ASYNC_ROUTER_H__

#include <assert.h>
#include <stdint.h>

#include <string>

#include <flinter/linkage/route_handler.h>
#include <flinter/object_map.h>
#include <flinter/timeout_pool.h>

namespace flinter {

template <class task_t, class seq_t = uint64_t, class channel_t = uint64_t>
class AsyncRouter {
public:
    /// @param route_handler life span NOT taken.
    /// @param default_timeout 5s if <= 0.
    explicit AsyncRouter(RouteHandler<task_t, seq_t, channel_t> *route_handler,
                         int64_t default_timeout = 0);

    ~AsyncRouter();

    void Add(const std::string &host, uint16_t port, int64_t expire = 0);
    void Del(const std::string &host, uint16_t port);

    /// @param seq to identify this call in a multiplex connection.
    /// @param task can be NULL, can be retrieved in Put(). If timed out, it's
    ///             auto released.
    /// @param channel return outgoing channel.
    /// @param timeout use default if <= 0.
    bool Get(const seq_t &seq,
             task_t *task,
             channel_t *channel,
             int64_t timeout = 0);

    /// @param seq to identify this call in a multiplex connection.
    /// @param rc 0 if succeeded.
    /// @param task if not null, return pointer set in Get(), caller to release.
    bool Put(const seq_t &seq, int rc, task_t **task);

    void Shutdown();
    void Cleanup();

    /// @warning not thread safe.
    void Clear();

private:
    struct server_t {
        bool operator < (const server_t &o) const
        {
            int c = host.compare(o.host);
            return c < 0 || (c == 0 && port < o.port);
        }

        std::string host;
        uint16_t port;

    }; // struct server_t

    struct context_t {
        int64_t expire;
        size_t pendings;
        server_t server;
        channel_t channel;
        int64_t last_active;

    }; // struct context_t

    class Map : public ObjectMap<server_t, context_t> {
    public:
        explicit Map(AsyncRouter *router) : _router(router) {}
        virtual ~Map() {}

        virtual context_t *Create(const server_t &key)
        {
            return _router->CreateConnection(key);
        }

        virtual void Destroy(context_t *value)
        {
            _router->DestroyConnection(value);
        }

    private:
        AsyncRouter *const _router;

    }; // class Map

    class Track {
    public:
        Track(AsyncRouter *router,
              const seq_t &seq,
              task_t *task,
              context_t *context) : _seq(seq)
                                  , _task(task)
                                  , _context(context)
                                  , _router(router)
                                  , _done(false) {}

        ~Track()
        {
            if (!_done) {
                _router->OnTimeout(this);
            }
        }

        void Done()
        {
            _done = true;
        }

        const seq_t _seq;
        task_t *const _task;
        context_t *const _context;
        AsyncRouter *const _router;

    private:
        bool _done;

    }; // class Track

    // Called by Map.
    context_t *CreateConnection(const server_t &key);
    void DestroyConnection(context_t *context);

    // Called by Track.
    void OnTimeout(Track *track);

    RouteHandler<task_t, seq_t, channel_t> *const _route_handler;
    TimeoutPool<seq_t, Track *> _timeout;
    Map *const _map;
    bool _clearing;

}; // class AsyncRouter

template <class T, class S, class C>
inline AsyncRouter<T, S, C>::AsyncRouter(RouteHandler<T, S, C> *route_handler,
                                         int64_t default_timeout)
        : _route_handler(route_handler)
        , _timeout(default_timeout > 0 ? default_timeout : 5000000000LL)
        , _map(new Map(this))
        , _clearing(false)
{
    // Intended left blank.
}

template <class T, class S, class C>
inline AsyncRouter<T, S, C>::~AsyncRouter()
{
    Clear();
    delete _map;
}

template <class T, class S, class C>
inline bool AsyncRouter<T, S, C>::Get(const S &seq,
                                      T *task,
                                      C *channel,
                                      int64_t timeout)
{
    context_t *c = _map->GetNext();
    if (!c) {
        return false;
    }

    _timeout.Insert(seq, new Track(this, seq, task, c), timeout);
    *channel = c->channel;
    return true;
}

template <class T, class S, class C>
inline bool AsyncRouter<T, S, C>::Put(const S &seq, int rc, T **task)
{
    Track *track = _timeout.Erase(seq);
    if (!track) {
        return false;
    }

    if (rc) {
        // TODO(yiyuanzhong): report call result.
    }

    if (task) {
        *task = track->_task;
    }

    _map->Release(track->_context->server, track->_context);
    track->Done();

    // Don't delete task, give it back.
    delete track;
    return true;
}

template <class T, class S, class C>
inline void AsyncRouter<T, S, C>::Clear()
{
    _clearing = true;
    _timeout.Clear();
    _map->Clear();
    _clearing = false;
}

template <class T, class S, class C>
inline void AsyncRouter<T, S, C>::Shutdown()
{
    _timeout.Clear();
    _map->EraseAll(true);
}

template <class T, class S, class C>
inline void AsyncRouter<T, S, C>::Cleanup()
{
    _timeout.Check();
}

template <class T, class S, class C>
inline void AsyncRouter<T, S, C>::OnTimeout(Track *track)
{
    // Don't lock!
    if (_clearing) {
        return;
    }

    _route_handler->OnTimeout(track->_seq, track->_task);
    _map->Release(track->_context->server, track->_context);
    delete track->_task;
}

template <class T, class S, class C>
inline void AsyncRouter<T, S, C>::Add(const std::string &host,
                                      uint16_t port,
                                      int64_t expire)
{
    server_t s;
    s.host = host;
    s.port = port;

    context_t *c = _map->Add(s);
    if (!c) {
        return;
    }

    c->expire = expire; // The callback didn't fill it in.
    _map->Release(s, c);
}

template <class T, class S, class C>
inline void AsyncRouter<T, S, C>::Del(const std::string &host, uint16_t port)
{
    server_t s;
    s.host = host;
    s.port = port;
    _map->Erase(s);
}

template <class T, class S, class channel_t>
inline typename AsyncRouter<T, S, channel_t>::context_t *
AsyncRouter<T, S, channel_t>::CreateConnection(const server_t &key)
{
    // Don't lock!
    channel_t channel;
    if (!_route_handler->Create(key.host, key.port, &channel)) {
        return NULL;
    }

    context_t *c = new context_t;
    c->channel = channel;
    c->server = key;
    c->expire = 0; // Not yet known, will be filled in outside callback.
    return c;
}

template <class T, class S, class C>
inline void AsyncRouter<T, S, C>::DestroyConnection(context_t *value)
{
    // Don't lock!
    _route_handler->Destroy(value->channel);
    delete value;
}

} // namespace flinter

#endif // __FLINTER_LINKAGE_ASYNC_ROUTER_H__
