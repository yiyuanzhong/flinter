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

#ifndef __FLINTER_LINKAGE_EASY_HANDLER_H__
#define __FLINTER_LINKAGE_EASY_HANDLER_H__

#include <sys/types.h>
#include <stdint.h>

namespace flinter {

class LinkagePeer;

class EasyHandler {
public:
    virtual ~EasyHandler() {};

    /// Called within I/O threads.
    ///
    /// Return the packet size even if it's incomplete as long as you can tell.
    /// Very handy when you write the message length in the header.
    ///
    /// @return >0 message length.
    /// @return  0 message length is yet determined, keep receiving.
    /// @return <0 message is invalid.
    virtual ssize_t GetMessageLength(uint64_t channel,
                                     const void *buffer,
                                     size_t length) = 0;

    /// Called within worker threads, in case there's none, within I/O threads.
    ///
    /// @return >0 keep coming.
    /// @return  0 hang up gracefully.
    /// @return <0 error occurred, hang up immediately.
    virtual int OnMessage(uint64_t channel,
                          const void *buffer,
                          size_t length) = 0;

    /// Called within I/O threads.
    virtual void OnDisconnected(uint64_t channel);

    /// Called within I/O threads.
    virtual bool OnConnected(uint64_t channel,
                             const LinkagePeer *peer,
                             const LinkagePeer *me);

    /// Called within I/O threads.
    virtual void OnError(uint64_t channel,
                         bool reading_or_writing,
                         int errnum);

    /// Called within job threads right after they start.
    virtual bool OnJobThreadInitialize();

    /// Called within job threads right before they terminate.
    virtual void OnJobThreadShutdown();

    /// Called within I/O threads right after they start.
    virtual bool OnIoThreadInitialize();

    /// Called within I/O threads right before they terminate.
    virtual void OnIoThreadShutdown();

}; // class EasyHandler

} // namespace flinter

#endif // __FLINTER_LINKAGE_EASY_HANDLER_H__
