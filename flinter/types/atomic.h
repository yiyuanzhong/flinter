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

#ifndef __FLINTER_TYPES_ATOMIC_H__
#define __FLINTER_TYPES_ATOMIC_H__

#include <stdint.h>

namespace flinter {

#define __ATOMIC_IMPL(n,b) T n(const T &t) { return b(&_t, t); }
template <class T>
class Atomic {
public:
    Atomic() : _t(T()) {}
    Atomic(const T &t) : _t(t) {}

    const T &Get() const
    {
        return _t;
    }

    void Set(const T &t)
    {
        _t = t;
    }

    __ATOMIC_IMPL(FetchAndAdd , __sync_fetch_and_add );
    __ATOMIC_IMPL(FetchAndSub , __sync_fetch_and_sub );
    __ATOMIC_IMPL(FetchAndOr  , __sync_fetch_and_or  );
    __ATOMIC_IMPL(FetchAndAnd , __sync_fetch_and_and );
    __ATOMIC_IMPL(FetchAndXor , __sync_fetch_and_xor );
    __ATOMIC_IMPL(FetchAndNand, __sync_fetch_and_nand);

    __ATOMIC_IMPL( AddAndFetch, __sync_add_and_fetch );
    __ATOMIC_IMPL( SubAndFetch, __sync_sub_and_fetch );
    __ATOMIC_IMPL(  OrAndFetch, __sync_or_and_fetch  );
    __ATOMIC_IMPL( AndAndFetch, __sync_and_and_fetch );
    __ATOMIC_IMPL( XorAndFetch, __sync_xor_and_fetch );
    __ATOMIC_IMPL(NandAndFetch, __sync_nand_and_fetch);

    __ATOMIC_IMPL(CompareAndSwap, __sync_val_compare_and_swap);

    void Lock()
    {
        while (__sync_lock_test_and_set(&_t, 1)) {}
    }

    void Unlock()
    {
        __sync_lock_release(&_t);
    }

private:
    T _t;

}; // class Atomic

typedef Atomic<int8_t>   atomic8_t;
typedef Atomic<int16_t>  atomic16_t;
typedef Atomic<int32_t>  atomic32_t;
typedef Atomic<int64_t>  atomic64_t;
typedef Atomic<uint8_t>  uatomic8_t;
typedef Atomic<uint16_t> uatomic16_t;
typedef Atomic<uint32_t> uatomic32_t;
typedef Atomic<uint64_t> uatomic64_t;

} // namespace flinter

#endif // __FLINTER_TYPES_ATOMIC_H__
