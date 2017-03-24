/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

#include <map>
#include <type_traits>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/ReadWriteLock.h"

namespace joynr
{

/**
 * Thread safe map
 */
template <typename Key,
          typename T,
          template <typename K, typename V, typename...> class Map = std::map>
class ThreadSafeMap
{
public:
    using MapIterator = typename Map<Key, T>::const_iterator;
    using mapped_type = T;

    /**
     * @brief ThreadSafeMap
     */
    ThreadSafeMap() : map(), lock()
    {
    }

    ~ThreadSafeMap() = default;

    /**
     * @brief insert key-value pair
     * @param key
     * @param value
     */
    void insert(const Key& key, const T& value)
    {
        WriteLocker locker(lock);
        map.insert(std::make_pair(key, value));
    }

    /**
     * @brief insert key-value pair
     * @param key
     * @param value
     */
    void insert(Key&& key, T&& value)
    {
        WriteLocker locker(lock);
        map.insert(std::make_pair(std::move(key), std::move(value)));
    }

    /**
     * @brief remove element stored under given key
     * @param key
     */
    void remove(const Key& key)
    {
        WriteLocker locker(lock);
        map.erase(map.find(key));
    }

    /**
     * @brief value retrieves copy of value stored under given key
     * @param key
     * @return copy of value. If the requested value is not stored in the map, an instance will
     * be returned, which is created by using the default constructor of T.
     */
    T value(const Key& key) const
    {
        T aValue;
        ReadLocker locker(lock);
        auto found = map.find(key);
        if (found != map.cend()) {
            aValue = found->second;
        }
        return aValue;
    }

    /**
     * @brief take retrieves copy of value stored under given key.
     * After this function returns, given element has been removed from the map
     * @param key
     * @return copy of value
     */
    T take(const Key& key)
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

    template <typename Fun>
    void applyReadFun(Fun&& f) const
    {
        ReadLocker locker(lock);
        f(map);
    }

    /**
     * @brief contains check if map contains element with given key
     * @param key
     * @return
     */
    bool contains(const Key& key) const
    {
        bool aValue;
        ReadLocker locker(lock);
        aValue = map.find(key) != map.end();
        return aValue;
    }

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
    std::size_t size() const
    {
        return map.size();
    }

    /**
     * @brief begin
     * @return iterator pointing to the beginning of the map
     */
    MapIterator begin() const
    {
        return map.begin();
    }

    /**
     * @brief end
     * @return iterator pointing to the end of the map
     */
    MapIterator end() const
    {
        return map.end();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(ThreadSafeMap);
    Map<Key, T> map;
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

} // namespace joynr

#endif // THREADSAFEMAP_H
