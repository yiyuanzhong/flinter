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

#include "flinter/linkage/easy_handler.h"

#include "flinter/linkage/linkage_peer.h"
#include "flinter/logger.h"

namespace flinter {

void EasyHandler::OnError(uint64_t channel,
                          bool reading_or_writing,
                          int errnum)
{
    CLOG.Trace("Linkage: ERROR %d when %s for channel = %lu",
               errnum, reading_or_writing ? "reading" : "writing",
               channel);
}

void EasyHandler::OnDisconnected(uint64_t channel)
{
    CLOG.Trace("Linkage: DISCONNECT for channel = %lu", channel);
}

bool EasyHandler::OnConnected(uint64_t channel,
                              const LinkagePeer *peer,
                              const LinkagePeer *me)
{
    if (peer) {
        CLOG.Trace("Linkage: CONNECT channel = %lu [%s:%u]",
                   channel, peer->ip_str().c_str(), peer->port());

    } else {
        CLOG.Trace("Linkage: CONNECT channel = %lu", channel);
    }

    return true;
}

bool EasyHandler::OnJobThreadInitialize()
{
    return true;
}

void EasyHandler::OnJobThreadShutdown()
{
    // Intended left blank.
}

bool EasyHandler::OnIoThreadInitialize()
{
    return true;
}

void EasyHandler::OnIoThreadShutdown()
{
    // Intended left blank.
}

} // namespace flinter
