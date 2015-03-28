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

#include "flinter/thread/semaphore.h"

#include <assert.h>
#include <errno.h>

#include <stdexcept>

#include "config.h"
#if defined(WIN32)
# include <Windows.h>
#elif HAVE_SEMAPHORE_H
#include <semaphore.h>
#else
# error Unsupported: Semaphore
#endif

namespace flinter {

Semaphore::Semaphore(int initial_count) : _context(NULL)
{
    assert(initial_count >= 0);
#ifdef WIN32
    HANDLE *context = new HANDLE;
    *context = CreateSemaphore(NULL, static_cast<LONG>(initial_count), LONG_MAX, NULL);
    if (!*context || *context == INVALID_HANDLE_VALUE) {
#else
    sem_t *context = new sem_t;
    if (sem_init(context, 0, static_cast<unsigned int>(initial_count)) < 0) {
#endif
        delete context;
        throw std::runtime_error("Semaphore::Semaphore()");
    }

    _context = reinterpret_cast<Context *>(context);
}

Semaphore::~Semaphore()
{
#ifdef WIN32
    CloseHandle(*reinterpret_cast<HANDLE *>(_context));
    delete reinterpret_cast<HANDLE *>(_context);
#else
    sem_destroy(reinterpret_cast<sem_t *>(_context));
    delete reinterpret_cast<sem_t *>(_context);
#endif
}

void Semaphore::Acquire()
{
#ifdef WIN32
    if (WaitForSingleObject(*reinterpret_cast<HANDLE *>(_context), INFINITE) == WAIT_OBJECT_0) {
        return;
    }
#else
    do {
        if (sem_wait(reinterpret_cast<sem_t *>(_context)) == 0) {
            return;
        }
    } while (errno == EINTR);
#endif
    throw std::runtime_error("Semaphore::Acquire()");
}

void Semaphore::Release(int count)
{
    assert(count > 0);
    if (!count) {
        return;
    }

#ifdef WIN32
    if (!ReleaseSemaphore(*reinterpret_cast<HANDLE *>(_context),
                         static_cast<LONG>(count), NULL)) {

        throw std::runtime_error("Semaphore::Release()");
    }
#else
    for (int i = 0; i < count; ++i) {
        int ret = sem_post(reinterpret_cast<sem_t *>(_context));
        if (ret < 0) {
            throw std::runtime_error("Semaphore::Release()");
        }
    }
#endif
}

bool Semaphore::TryAcquire()
{
#ifdef WIN32
    DWORD ret = WaitForSingleObject(*reinterpret_cast<HANDLE *>(_context), 0);
    if (ret == WAIT_OBJECT_0) {
        return true;
    } else if (ret == WAIT_TIMEOUT) {
#else
    if (sem_trywait(reinterpret_cast<sem_t *>(_context)) == 0) {
        return true;
    } else if (errno == EAGAIN) {
#endif
        return false;
    } else {
        throw std::runtime_error("Semaphore::TryAcquire()");
    }
}

} // namespace flinter
