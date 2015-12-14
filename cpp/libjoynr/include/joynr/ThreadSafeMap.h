/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
#ifndef THREADSAFEMAP_H
#define THREADSAFEMAP_H
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ReadWriteLock.h"

#include <map>

namespace joynr
{

template <class Key, class T>
using mapIterator = typename std::map<Key, T>::const_iterator;

/**
 * Thread-safe map. It has been used at the moment to store shared_ptr as values.
 * NOTICE: DO NOT STORE RAW POINTERS IN THIS MAP!!!
 */
template <class Key, class T>
class ThreadSafeMap
{
public:
    ThreadSafeMap();
    virtual ~ThreadSafeMap()
    {
    }
    void insert(const Key& key, const T& value);
    void remove(const Key& key);
    T value(const Key& key);
    T take(const Key& key);
    bool contains(const Key& key) const;
    void deleteAll();
    int size();
    mapIterator<Key, T> begin() const;
    mapIterator<Key, T> end() const;

private:
    DISALLOW_COPY_AND_ASSIGN(ThreadSafeMap);
    std::map<Key, T> map;
    mutable ReadWriteLock lock;
};

template <class Key, class T>
ThreadSafeMap<Key, T>::ThreadSafeMap()
        : map(), lock()
{
}

template <class Key, class T>
void ThreadSafeMap<Key, T>::insert(const Key& key, const T& value)
{
    WriteLocker locker(lock);
    map.insert(std::make_pair(key, value));
}

template <class Key, class T>
void ThreadSafeMap<Key, T>::remove(const Key& key)
{
    WriteLocker locker(lock);
    map.erase(map.find(key));
}

template <class Key, class T>
T ThreadSafeMap<Key, T>::value(const Key& key)
{
    T aValue;
    ReadLocker locker(lock);
    aValue = map.find(key)->second;
    return aValue;
}

template <class Key, class T>
T ThreadSafeMap<Key, T>::take(const Key& key)
{
    T aValue;
    WriteLocker locker(lock);
    auto mapElement = map.find(key);
    if (mapElement != map.end()) {
        aValue = mapElement->second;
        map.erase(mapElement);
    }
    return aValue;
}

template <class Key, class T>
bool ThreadSafeMap<Key, T>::contains(const Key& key) const
{
    bool aValue;
    ReadLocker locker(lock);
    aValue = map.find(key) != map.end();
    return aValue;
}

template <class Key, class T>
void ThreadSafeMap<Key, T>::deleteAll()
{
    WriteLocker locker(lock);
    map.clear();
}

template <class Key, class T>
int ThreadSafeMap<Key, T>::size()
{
    return map.size();
}

template <class Key, class T>
mapIterator<Key, T> ThreadSafeMap<Key, T>::begin() const
{
    return map.begin();
}

template <class Key, class T>
mapIterator<Key, T> ThreadSafeMap<Key, T>::end() const
{
    return map.end();
}
} // namespace joynr

#endif // THREADSAFEMAP_H
