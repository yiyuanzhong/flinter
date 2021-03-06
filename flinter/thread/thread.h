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

#ifndef FLINTER_THREAD_THREAD_H
#define FLINTER_THREAD_THREAD_H

#include <assert.h>

#include <stdexcept>

#include <flinter/thread/condition.h>
#include <flinter/common.h>

namespace flinter {

class AbstractThreadPool;
class Runnable;

namespace internal {

class ThreadContext;
class ThreadJob;

/// Thread is used internally by ThreadPool.
class Thread {
public:
    /// Constructor.
    Thread(AbstractThreadPool *pool, void *parameter);

    /// Destructor.
    ~Thread();

    /// Request terminating.
    void Terminate();

    /// Wait for thread to quit.
    void Join();

    /// Send a signal to the thread, only for Unix.
    bool Kill(int signum);

    /// Don't call methods below, used internally.
    void ThreadDaemon();
    void *parameter()
    {
        return _parameter;
    }

private:
    NON_COPYABLE(Thread);       ///< Don't copy me.

    ThreadJob *RequestJob();    ///< Ask for job.

    void *_parameter;           ///< ThreadPool parameter.
    ThreadContext *_context;    ///< Internally used.
    AbstractThreadPool *_pool;  ///< ThreadPool instance.

}; // class Thread

} // namespace internal
} // namespace flinter

#endif // FLINTER_THREAD_THREAD_H
