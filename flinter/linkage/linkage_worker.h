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

#ifndef __FLINTER_LINKAGE_LINKAGE_WORKER_H__
#define __FLINTER_LINKAGE_LINKAGE_WORKER_H__

#include <stddef.h>
#include <stdint.h>

#include <list>

#include <flinter/types/unordered_map.h>
#include <flinter/types/unordered_set.h>
#include <flinter/runnable.h>

struct ev_async;
struct ev_loop;
struct ev_io;
struct ev_timer;

namespace flinter {

class LinkageBase;
class LinkagePeer;
class Mutex;

class LinkageWorker : public Runnable {
public:
    friend class LinkageBase;

    LinkageWorker();
    virtual ~LinkageWorker();

    // Methods below can be called from another thread.

    /// @warning call before Run().
    /// @param interval in nanoseconds.
    bool RegisterTimer(int64_t interval, Runnable *runnable, bool auto_release);

    /// @param command runs in the thread of worker asynchronously,
    ///                auto released when done.
    bool SendCommand(Runnable *command);

    /// Won't block, but Run() will eventually quit.
    virtual bool Shutdown();

    // Methods below must be called from the same thread.

    bool SetWanna(LinkageBase *linkage, bool wanna_read, bool wanna_write);
    bool SetWannaWrite(LinkageBase *linkage, bool wanna);
    bool SetWannaRead(LinkageBase *linkage, bool wanna);

    virtual bool Run();

protected:
    virtual bool OnInitialize();
    virtual void OnShutdown();

    // Only used by Linkages, don't call explicitly.
    bool Attach(LinkageBase *linkage, int fd, bool read_now, bool auto_release);
    bool Detach(LinkageBase *linkage);

private:
    struct timer_t;
    struct client_t;
    class HealthTimer;
    typedef std::unordered_set<struct timer_t *> timers_t;
    typedef std::unordered_map<LinkageBase *, struct client_t *> events_t;

    static void command_cb(struct ev_loop *loop, struct ev_async *w, int revents);
    static void readable_cb(struct ev_loop *loop, struct ev_io *w, int revents);
    static void writable_cb(struct ev_loop *loop, struct ev_io *w, int revents);
    static void timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);

    // Level 1 callbacks.
    void OnReadable(struct client_t *client);
    void OnWritable(struct client_t *client);
    void OnTimer(struct timer_t *timer);
    void OnCommand();

    // Level 2 callbacks.
    void OnError(struct client_t *client, bool reading_or_writing, int errnum);
    void OnDisconnected(struct client_t *client);

    void DoRelease(struct client_t *client, bool erase_container);
    void DoRelease(struct timer_t *timer, bool erase_container);

    struct ev_loop *_loop;
    volatile bool _quit;

    std::list<Runnable *> _commands;
    struct ev_async *_async;
    flinter::Mutex *_mutex;

    // Health checking related.
    bool OnHealthCheck(int64_t now);

    timers_t _timers;
    events_t _events;

    explicit LinkageWorker(const LinkageWorker &);
    LinkageWorker &operator = (const LinkageWorker &);

}; // class LinkageWorker

} // namespace flinter

#endif // __FLINTER_LINKAGE_LINKAGE_WORKER_H__
