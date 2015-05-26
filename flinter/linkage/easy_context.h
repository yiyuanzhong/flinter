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

#ifndef __FLINTER_LINKAGE_EASY_CONTEXT_H__
#define __FLINTER_LINKAGE_EASY_CONTEXT_H__

#include <stdint.h>

#include <flinter/linkage/linkage_peer.h>

namespace flinter {

class Linkage;
class LinkagePeer;
class Object;

class EasyContext {
public:
    typedef uint64_t channel_t;

    EasyContext(channel_t channel, Linkage *linkage);
    ~EasyContext();

    channel_t channel() const
    {
        return _channel;
    }

    const LinkagePeer &me() const
    {
        return _me;
    }

    const LinkagePeer &peer() const
    {
        return _peer;
    }

    Object *context() const
    {
        return _context;
    }

    /// Associate an object to this context which can later be retrieved.
    /// Object associated will be deleted when this EasyContext is destroyed.
    /// @param context life span TAKEN, you can use but don't delete it.
    /// @return pointer of original context.
    /// @warning not thread safe, think before you use.
    Object *set_context(Object *context) const
    {
        Object *old = _context;
        _context = context;
        return old;
    }

private:
    channel_t _channel;
    LinkagePeer _me;
    LinkagePeer _peer;
    mutable Object *_context;

}; // class EasyContext

} // namespace flinter

#endif // __FLINTER_LINKAGE_EASY_CONTEXT_H__
