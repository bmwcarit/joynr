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
#include <type_traits>

namespace joynr
{

template <class Key, class T>
using mapIterator = typename std::map<Key, T>::const_iterator;

/**
 * Thread safe map
 */
template <class Key, class T>
class ThreadSafeMap
{
public:
    /**
     * @brief ThreadSafeMap
     */
    ThreadSafeMap();
    ~ThreadSafeMap() = default;
    /**
     * @brief insert key-value pair
     * @param key
     * @param value
     */
    void insert(const Key& key, const T& value);
    /**
     * @brief remove element stored under given key
     * @param key
     */
    void remove(const Key& key);
    /**
     * @brief value retrieves copy of value stored under given key
     * @param key
     * @return copy of value
     */
    T value(const Key& key);
    /**
     * @brief take retrieves copy of value stored under given key.
     * After this function returns, given element has been removed from the map
     * @param key
     * @return copy of value
     */
    T take(const Key& key);
    /**
     * @brief contains check if map contains element with given key
     * @param key
     * @return
     */
    bool contains(const Key& key) const;

    /**
     * @brief deleteAll removes all elements from the map.
     * This function releases resources properly in case
     * element stores pointer values.
     */
    void deleteAll()
    {
        deleteAllImpl(std::is_pointer<T>{});
    }
    /**
     * @brief size
     * @return number of elements in the map
     */
    int size();
    /**
     * @brief begin
     * @return iterator pointing to the beginning of the map
     */
    mapIterator<Key, T> begin() const;
    /**
     * @brief end
     * @return iterator pointing to the end of the map
     */
    mapIterator<Key, T> end() const;

private:
    DISALLOW_COPY_AND_ASSIGN(ThreadSafeMap);
    std::map<Key, T> map;
    mutable ReadWriteLock lock;

    void deleteAllImpl(std::false_type)
    {
        WriteLocker locker(lock);
        map.clear();
    }

    void deleteAllImpl(std::true_type)
    {
        WriteLocker locker(lock);
        for (auto mapElement : map) {
            T* value = take(mapElement->first);
            delete value;
        }
    }
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
