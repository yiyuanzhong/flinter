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

#ifndef __FLINTER_SINGLETON_H__
#define __FLINTER_SINGLETON_H__

#include <memory>

namespace flinter {

/**
 * Every singleton should derive from this base class.
 *
 * @warning Although it seems straight forward, this class does NOT work on Windows.
 *          When I say NOT, I mean not ALL the circumstances, mostly due to DLL hells.
 *          Windows isolates stacks and heaps between any two modules, like exe and dlls,
 *          so that even dlls are loaded into the same process context, they use their own
 *          heaps, thus global variants and TLSs are all different among.
 *
 *          Look at the std::auto_ptr<> below, let's simply assume that it's basically an
 *          interlocked global (static) variants, two dlls (or exe and dlls) see different
 *          values for the "same" object, and they'll instantiate different objects if
 *          called from different modules. Within the same module, yes, it still works.
 *
 *          So please make sure that you only use one class of Singleton within the same module.
 */
template <class T>
class Singleton {
public:
    /// The only important method.
    static T *GetInstance()
    {
        static std::auto_ptr<T> singleton(new T);
        return singleton.get();
    }

}; // class Singleton

} // namespace flinter

#define DECLARE_SINGLETON(klass) \
public: \
    friend class ::flinter::Singleton<klass>; \
    friend class std::auto_ptr<klass>; \
private: \
    explicit klass(const klass &); \
    klass &operator = (const klass &);

#endif // __FLINTER_SINGLETON_H__
