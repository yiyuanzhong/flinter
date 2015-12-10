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

#ifndef __FLINTER_LINKAGE_ROUTE_HANDLER_H__
#define __FLINTER_LINKAGE_ROUTE_HANDLER_H__

#include <stdint.h>

#include <string>

namespace flinter {

template <class task_t, class seq_t = uint64_t, class channel_t = uint64_t>
class RouteHandler {
public:
    virtual ~RouteHandler() {}

    virtual bool Create(const std::string &host,
                        uint16_t port,
                        channel_t *channel) = 0;

    virtual bool Destroy(const channel_t &channel) = 0;

    virtual void OnTimeout(const seq_t &seq, task_t *task);

}; // class RouteHandler

template <class T, class S, class C>
inline void RouteHandler<T, S, C>::OnTimeout(const S & /*seq*/, T * /*task*/)
{
    // Intended left blank.
}

} // namespace flinter

#endif // __FLINTER_LINKAGE_ROUTE_HANDLER_H__
