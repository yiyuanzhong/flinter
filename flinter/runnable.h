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

#ifndef __FLINTER_RUNNABLE_H__
#define __FLINTER_RUNNABLE_H__

namespace flinter {

class Runnable {
public:
    /// Destructor.
    virtual ~Runnable() {}

    /// Run this runnable, must be implemented.
    virtual bool Run() = 0;

    /// Shutdown this runnable.
    virtual bool Shutdown()
    {
        return false;
    }

}; // class Runnable

} // namespace flinter

#endif // __FLINTER_RUNNABLE_H__
