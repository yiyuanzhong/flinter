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

#ifndef __FLINTER_LINKAGE_ABSTRACT_IO_H__
#define __FLINTER_LINKAGE_ABSTRACT_IO_H__

#include <stddef.h>

namespace flinter {

class AbstractIo {
public:
    enum Status {
        kStatusOk           =  0, // Action completed successfully.
        kStatusBug          = -1, // Action completed with failures.
        kStatusError        = -2, // Action completed with failures.
        kStatusJammed       = -3, // Action completed with failures.
        kStatusClosed       = -4, // Action completed with failures.
        kStatusWannaRead    = -5, // Action incomplete, should call again.
        kStatusWannaWrite   = -6, // Action incomplete, should call again.
    }; // enum Status

    enum Action {
        kActionNone     = 0,
        kActionRead     = 1,
        kActionWrite    = 2,
        kActionAccept   = 3,
        kActionConnect  = 4,
        kActionShutdown = 5,
    }; // enum Action

    virtual ~AbstractIo() {}

    virtual bool Initialize(Action *action,      // Invoke immediately
                            Action *next_action, // Invoke when event happens
                            bool *wanna_read,
                            bool *wanna_write) = 0;

    virtual Status Write(const void *buffer, size_t length, size_t *retlen) = 0;
    virtual Status Read(void *buffer, size_t length, size_t *retlen) = 0;
    virtual Status Shutdown() = 0;
    virtual Status Connect() = 0;
    virtual Status Accept() = 0;

}; // class AbstractIo

} // namespace flinter

#endif // __FLINTER_LINKAGE_ABSTRACT_IO_H__
