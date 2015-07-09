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

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include <flinter/factory.h>
#include <flinter/utility.h>

namespace flinter {

template <class K, class T>
class ObjectMap {
public:
    /// @param object_factory life span taken, don't use it any more.
    explicit ObjectMap(Factory<T> *object_factory);
    virtual ~ObjectMap();

    /// Might rewind GetNext().
    void SetAll(const std::set<K> &keys);

    /// Might rewind GetNext().
    void Erase(const K &key);

    /// Might rewind GetNext().
    void Clear();

    /// Create or get object.
    /// Might rewind GetNext().
    T *Add(const K &key);

    /// Get object specified by key.
    T *Get(const K &key) const;

    /// Get a random object.
    /// This method is of low efficiency, avoid it.
    T *GetRandom() const;

    /// Get objects in order.
    T *GetNext();

private:
    typename std::map<K, T *>::const_iterator _ptr;
    std::map<K, T *> _map;
    Factory<T> *_factory;

}; // class ObjectMap

template <class K, class T>
inline ObjectMap<K, T>::ObjectMap(Factory<T> *object_factory)
        : _ptr(_map.end()), _factory(object_factory)
{
    // Intended left blank.
}

template <class K, class T>
inline ObjectMap<K, T>::~ObjectMap()
{
    Clear();
}

template <class K, class T>
inline void ObjectMap<K, T>::Clear()
{
    for (typename std::map<K, T *>::iterator
         p = _map.begin(); p != _map.end(); ++p) {

        delete p->second;
    }

    _map.clear();
    _ptr = _map.end();
}

template <class K, class T>
inline T *ObjectMap<K, T>::Add(const K &key)
{
    typename std::map<K, T *>::iterator p = _map.find(key);
    if (p != _map.end()) {
        return p->second;
    }

    T *object = _factory->Create();
    _map.insert(std::make_pair(key, object));
    _ptr = _map.end();
    return object;
}

template <class K, class T>
inline void ObjectMap<K, T>::Erase(const K &key)
{
    typename std::map<K, T *>::iterator p = _map.find(key);
    if (p != _map.end()) {
        delete p->second;
        _map.erase(p);
        _ptr = _map.end();
    }
}

template <class K, class T>
inline T *ObjectMap<K, T>::Get(const K &key) const
{
    typename std::map<K, T *>::const_iterator p = _map.find(key);
    if (p == _map.end()) {
        return NULL;
    }

    return p->second;
}

template <class K, class T>
inline T *ObjectMap<K, T>::GetNext()
{
    if (_map.empty()) {
        return NULL;
    }

    if (_ptr != _map.end()) {
        ++_ptr;
    }

    if (_ptr == _map.end()) {
        _ptr = _map.begin();
    }

    return _ptr->second;
}

template <class K, class T>
inline T *ObjectMap<K, T>::GetRandom() const
{
    typename std::map<K, T *>::const_iterator p = _map.begin();
    if (p == _map.end()) {
        return NULL;
    }

    int index = ranged_rand(_map.size());
    if (index) {
        std::advance(p, index);
    }

    return p->second;
}

template <class K, class T>
inline void ObjectMap<K, T>::SetAll(const std::set<K> &keys)
{
    for (typename std::set<K>::const_iterator
         p = keys.begin(); p != keys.end(); ++p) {

        if (_map.find(*p) == _map.end()) {
            Add(*p);
        }
    }

    std::vector<K> gone;
    for (typename std::map<K, T *>::const_iterator
         p = _map.begin(); p != _map.end(); ++p) {

        if (keys.find(p->first) == keys.end()) {
            gone.push_back(p->first);
        }
    }

    for (typename std::vector<K>::const_iterator
         p = gone.begin(); p != gone.end(); ++p) {

        Erase(*p);
    }
}

} // namespace flinter

#endif // __FLINTER_OBJECT_MAP_H__
