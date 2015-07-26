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

#ifndef __FLINTER_OBJECT_MAP_H__
#define __FLINTER_OBJECT_MAP_H__

#include <assert.h>

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include <flinter/thread/mutex.h>
#include <flinter/thread/mutex_locker.h>
#include <flinter/utility.h>

namespace flinter {

template <class K, class T>
class ObjectMap {
public:
    virtual ~ObjectMap();

    /// Might rewind GetNext().
    void SetAll(const std::set<K> &keys);
    void EraseAll();

    /// Might rewind GetNext().
    void Erase(const K &key);

    /// Might rewind GetNext().
    /// Subclasses must call this in their deconstructors.
    void Clear();

    /// Create or get object.
    /// Increases reference count.
    /// Might rewind GetNext().
    /// @warning might return NULL.
    T *Add(const K &key);

    /// Get object specified by key.
    /// Increases reference count.
    /// @warning might return NULL.
    T *Get(const K &key);

    /// Get a random object.
    /// Increases reference count.
    /// @warning might return NULL.
    /// @warning low efficiency.
    T *GetRandom();

    /// Get objects in order.
    /// Increases reference count.
    /// @warning might return NULL.
    T *GetNext();

    /// Decreases reference count.
    /// Objects no longer in the map with 0 reference count will be released.
    void Release(const K &key);

    size_t size() const;

protected:
    virtual void Destroy(T *object) = 0;
    virtual T *Create(const K &key) = 0;
    ObjectMap();

private:
    void DoErase(const K &key);
    T *DoAdd(const K &key);

    mutable Mutex _mutex;
    typedef std::pair<T *, size_t> V;
    typename std::map<K, V>::iterator _ptr;
    std::map<K, V> _drop;
    std::map<K, V> _map;

}; // class ObjectMap

template <class K, class T>
inline ObjectMap<K, T>::ObjectMap()
        : _ptr(_map.end())
{
    // Intended left blank.
}

template <class K, class T>
inline ObjectMap<K, T>::~ObjectMap()
{
    assert(_map.empty());
}

template <class K, class T>
inline size_t ObjectMap<K, T>::size() const
{
    MutexLocker locker(&_mutex);
    return _map.size() + _drop.size();
}

template <class K, class T>
inline void ObjectMap<K, T>::Clear()
{
    MutexLocker locker(&_mutex);
    for (typename std::map<K, V>::iterator
         p = _drop.begin(); p != _drop.end(); ++p) {

        Destroy(p->second.first);
    }

    for (typename std::map<K, V>::iterator
         p = _map.begin(); p != _map.end(); ++p) {

        Destroy(p->second.first);
    }

    _map.clear();
    _drop.clear();
    _ptr = _map.end();
}

template <class K, class T>
inline T *ObjectMap<K, T>::Add(const K &key)
{
    MutexLocker locker(&_mutex);
    return DoAdd(key);
}

template <class K, class T>
inline T *ObjectMap<K, T>::DoAdd(const K &key)
{
    typename std::map<K, V>::iterator p = _map.find(key);
    if (p != _map.end()) {
        ++p->second.second;
        return p->second.first;
    }

    T *object = Create(key);
    if (!object) {
        return NULL;
    }

    _map.insert(std::make_pair(key, V(object, 1)));
    _ptr = _map.end();
    return object;
}

template <class K, class T>
inline void ObjectMap<K, T>::Erase(const K &key)
{
    MutexLocker locker(&_mutex);
    DoErase(key);
}

template <class K, class T>
inline void ObjectMap<K, T>::DoErase(const K &key)
{
    typename std::map<K, V>::iterator p = _map.find(key);
    if (p == _map.end()) {
        return;
    }

    if (!p->second.second) {
        Destroy(p->second.first);
    } else {
        _drop.insert(*p);
    }

    _map.erase(p);
    _ptr = _map.end();
}

template <class K, class T>
inline void ObjectMap<K, T>::EraseAll()
{
    MutexLocker locker(&_mutex);
    for (typename std::map<K, V>::iterator p = _map.begin();
         p != _map.end(); ++p) {

        if (!p->second.second) {
            Destroy(p->second.first);
        } else {
            _drop.insert(*p);
        }
    }

    _map.clear();
    _ptr = _map.end();
}

template <class K, class T>
inline void ObjectMap<K, T>::Release(const K &key)
{
    MutexLocker locker(&_mutex);
    typename std::map<K, V>::iterator p = _map.find(key);
    if (p != _map.end()) {
        --p->second.second;
        return;
    }

    p = _drop.find(key);
    if (p != _drop.end()) {
        --p->second.second;
        if (!p->second.second) {
            Destroy(p->second.first);
            _drop.erase(p);
        }
    }
}

template <class K, class T>
inline T *ObjectMap<K, T>::Get(const K &key)
{
    MutexLocker locker(&_mutex);
    typename std::map<K, V>::iterator p = _map.find(key);
    if (p == _map.end()) {
        return NULL;
    }

    ++p->second.second;
    return p->second.first;
}

template <class K, class T>
inline T *ObjectMap<K, T>::GetNext()
{
    MutexLocker locker(&_mutex);
    if (_map.empty()) {
        return NULL;
    }

    if (_ptr != _map.end()) {
        ++_ptr;
    }

    if (_ptr == _map.end()) {
        _ptr = _map.begin();
    }

    ++_ptr->second.second;
    return _ptr->second.first;
}

template <class K, class T>
inline T *ObjectMap<K, T>::GetRandom()
{
    MutexLocker locker(&_mutex);
    typename std::map<K, T *>::iterator p = _map.begin();
    if (p == _map.end()) {
        return NULL;
    }

    int index = ranged_rand(_map.size());
    if (index) {
        std::advance(p, index);
    }

    ++p->second.second;
    return p->second.first;
}

template <class K, class T>
inline void ObjectMap<K, T>::SetAll(const std::set<K> &keys)
{
    MutexLocker locker(&_mutex);
    for (typename std::set<K>::const_iterator
         p = keys.begin(); p != keys.end(); ++p) {

        if (_map.find(*p) == _map.end()) {
            DoAdd(*p);
        }
    }

    std::vector<K> gone;
    for (typename std::map<K, V>::iterator
         p = _map.begin(); p != _map.end(); ++p) {

        if (keys.find(p->first) == keys.end()) {
            gone.push_back(p->first);
        }
    }

    for (typename std::vector<K>::const_iterator
         p = gone.begin(); p != gone.end(); ++p) {

        DoErase(*p);
    }
}

} // namespace flinter

#endif // __FLINTER_OBJECT_MAP_H__