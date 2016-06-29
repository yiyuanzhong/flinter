#include <stdlib.h>
#include <string.h>

#include <flinter/linkage/easy_context.h>
#include <flinter/linkage/easy_handler.h>
#include <flinter/linkage/easy_server.h>

#include <flinter/thread/keyed_thread_pool.h>

#include <flinter/timeout_pool.h>
#include <flinter/logger.h>
#include <flinter/msleep.h>
#include <flinter/signals.h>

/*
 * This file demonstrates own connection mode: each worker thread will has its
 * own connection to the backend. Every response is routed back to the worker
 * where the request is initiated so that worker can use separate context
 * without mutex protection.
 */

const int N = 10;

class Request;

// Fake context for each worker, no mutex.
size_t g_context[N];

// TimeoutPool, key is request id, value is original request.
flinter::TimeoutPool<int, Request *> *g_timeout[N];

class Request {
public:
    explicit Request(int reqid, int i) : _reqid(reqid), _i(i), _done(false) {}
    ~Request()
    {
        if (_done) {
            CLOG.Info("Request [%d] [%d] DONE", _reqid, _i);
        } else {
            CLOG.Warn("Request [%d] [%d] TIMEOUT", _reqid, _i);
        }
    }
    void Done()
    {
        _done = true;
    }

private:
    int _reqid;
    int _i;
    bool _done;

}; // class Request

class Job : public flinter::Runnable {
public:
    Job(const flinter::EasyContext &context,
        const void *buffer, size_t length,
        size_t id) : _id(id)
    {
        // Maybe parse the buffer (uses I/O thread CPU), or copy the buffer.
        // Here I demonstrate parse the buffer (fake).
        _reqid = atoi(reinterpret_cast<const char *>(buffer));
        _channel = context.channel();
        _length = length;
    }

    virtual ~Job() {}
    virtual bool Run()
    {
        // This runs in worker threads.

        // I can access my own context without lock.
        size_t my_context = g_context[_id];

        // Now to deal with TimeoutPool.
        LOG(DEBUG) << "FIND " << _reqid << " FROM " << _id;
        Request *request = g_timeout[_id]->Erase(_reqid);
        if (!request) {
            LOG(WARN) << "Job: unknown request id: " << _reqid;
            return true;
        }

        LOG(INFO) << "Handler: I'm (" << my_context << ") OnMessage("
                  << _channel << ", " << _length << ")";

        request->Done();
        delete request;
        return true;
    }

    size_t length() const
    {
        return _length;
    }

private:
    uint64_t _channel;
    size_t _length;
    int _reqid;
    size_t _id;

}; // class Job

class Handler : public flinter::EasyHandler {
public:
    Handler(flinter::KeyedThreadPool *pool, size_t id) : _pool(pool), _id(id) {}
    virtual ~Handler() {}
    virtual ssize_t GetMessageLength(const flinter::EasyContext &context,
                                     const void *buffer,
                                     size_t length)
    {
        // This method is in the I/O thread.
        LOG(TRACE) << "Handler: GetMessageLength(" << context.channel() << ")";

        size_t s = 1;
        for (const char *p = reinterpret_cast<const char *>(buffer);
             s <= length; ++p, ++s) {

            if (*p == '\n') {
                return static_cast<ssize_t>(s);
            } else if (*p == '\r') {
                if (s == length) {
                    return 0;
                }

                if (*(p + 1) == '\n') {
                    return static_cast<ssize_t>(s + 1);
                }

                return -1;
            }
        }

        return 0;
    }

    virtual int OnMessage(const flinter::EasyContext &context,
                          const void *buffer, size_t length)
    {
        // Here we're in the I/O thread, dispatch the request to thread pool.
        Job *job = new Job(context, buffer, length, _id);
        if (!_pool->AppendJobWithKey(_id, job, true)) {
            // Something wrong.
            return -1;
        }

        return 1;
    }

    size_t id() const
    {
        return _id;
    }

private:
    flinter::KeyedThreadPool *const _pool;
    const size_t _id;

}; // class Handler

int main()
{
    flinter::Logger::SetFilter(flinter::Logger::kLevelDebug);
    signals_ignore(SIGPIPE);

    flinter::EasyServer s;
    flinter::KeyedThreadPool p;

    // For KeyedThreadPool with size N, you can always use 0..N-1 as key to
    // uniquely dispatch requests.
    if (!p.Initialize(N)) {
        return EXIT_FAILURE;
    }

    // Don't start any workers since we use own pool.
    if (!s.Initialize(3, 0)) {
        return EXIT_FAILURE;
    }

    uint64_t channels[N];
    Handler *handlers[N];
    for (size_t i = 0; i < N; ++i) {
        g_context[i] = i + 10000;
        g_timeout[i] = new flinter::TimeoutPool<int, Request *>(3000000000LL); // 3s timeout.

        // Each connection will remember which worker to be routed back.
        handlers[i] = new Handler(&p, i);
        channels[i] = s.ConnectTcp4("127.0.0.1", 5566, handlers[i]);
        if (!channels[i]) {
            // Should release resources.
            return EXIT_FAILURE;
        }
    }

    // Now do your business.
    int reqid = 1000;
    for (int i = 0; i < 120; ++i) {
        LOG(WARN) << "ROUND " << i;

        for (int j = 0; j < N; ++j) {
            int request = rand();
            ++reqid;
            LOG(DEBUG) << "INSERT " << reqid << " INTO " << j;
            g_timeout[j]->Insert(reqid, new Request(reqid, request));
            g_timeout[j]->Check();

            std::ostringstream o;
            o << reqid << "\r\n";
            const std::string &str = o.str();
            s.Send(channels[j], str.data(), str.length());
        }

        msleep(1000);
    }

    LOG(WARN) << "QUITING";
    msleep(5000);
    s.Shutdown();
    for (int i = 0; i < N; ++i) {
        delete handlers[i];
        delete g_timeout[i];
    }

    return EXIT_SUCCESS;
}
