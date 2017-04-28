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
#ifndef TYPEDCLIENTMULTICACHE_H
#define TYPEDCLIENTMULTICACHE_H

#include "joynr/CachedValue.h"
#include "joynr/Cache.h"

#include <vector>
#include <mutex>
#include <chrono>
#include <cstdint>
#include "joynr/Util.h"

namespace joynr
{

/**
 * Implements a typed cache. Stores SEVERAL objects with a key
 * and a timestamp in milliseconds.
 * The timestamp is generated automatically. Access is thread safe.
 * The cache automatically manages the objects that are inserted and
 * deletes them to make room for new objects, if necessary (more than
 * 1000 entries). This is in memory only, no persistence.
 *
 */

#define MULTICACHE_DEFAULT_MAX_COST 1000

template <class Key, class T>
class TypedClientMultiCache
{
public:
    explicit TypedClientMultiCache<Key, T>(int maxCost = MULTICACHE_DEFAULT_MAX_COST);

    /*
    * Returns the list of values stored for the attributeId filtered by a maximum age. If none
    *exists, it returns an empty
    * std::vector object that can be tested for by using the empty() method.
    * maxAcceptedAge -1 returns all values.
    *
    */
    std::vector<T> lookUp(const Key& key, std::chrono::milliseconds maxAcceptedAge);
    /*
     *  Returns the list of values stored for the attribute not considering their age.
      */
    std::vector<T> lookUpAll(const Key& key) const;
    /*
    * Inserts the key (e.g. attributeId) and object into the cache.  If the attributeId already
    * exists in the cache the value is added to a list (no overwrite).
    * Note, this insert does not perform any validation on the value.
    */
    void insert(const Key& key, T object);
    /*
     * Removes the entry (specified by 'object') associated with key 'attributeID'.
     */
    void remove(const Key& key, T object);
    /*
    * Removes all entries associated with key 'attributeID'.
    */
    void removeAll(const Key& key);
    /*
    * Removes all entries that are older than 'maxAcceptedAgeInMs'. If there are several
    * entries for an attributeID, only the entries which are too old are deleted. Only if all
    * entries for an attributeID are too old will the key become invalid (deleted).
    * Can be used to clear the cache of all entries by setting 'maxAcceptedAgeInMs = 0'.
    */
    void cleanup(std::chrono::milliseconds maxAcceptedAge);

    bool contains(const Key& key);

    T take(const Key& key);

    void setMaxCost(int maxCost);

    int getMaxCost();

    /*
     * Return all entries in the cache as a vector.
     */
    std::vector<Key> getKeys() const;

private:
    /*
     * Returns time since activation in ms (elapsed())
     */
    std::chrono::milliseconds elapsed(TimeStamp entryTime);
    Cache<Key, std::vector<CachedValue<T>>> cache;
    mutable std::mutex mutex;
};

template <class Key, class T>
TypedClientMultiCache<Key, T>::TypedClientMultiCache(int maxCost)
        : cache(), mutex()
{
    cache.setCacheCapacity(maxCost);
}

template <class Key, class T>
std::vector<T> TypedClientMultiCache<Key, T>::lookUp(const Key& key,
                                                     std::chrono::milliseconds maxAcceptedAge)
{
    if (maxAcceptedAge == std::chrono::milliseconds(-1)) {
        return lookUpAll(key);
    }
    std::lock_guard<std::mutex> lock(mutex);
    if (!cache.contains(key)) {
        return std::vector<T>();
    }
    std::vector<CachedValue<T>>* list = cache.object(key);
    std::vector<T> result;

    for (std::size_t i = 0; i < list->size(); i++) {
        auto time = list->at(i).getTimestamp();
        if (elapsed(time) <= maxAcceptedAge) {
            result.push_back(list->at(i).getValue());
        }
    }
    return result;
}

template <class Key, class T>
std::vector<T> TypedClientMultiCache<Key, T>::lookUpAll(const Key& key) const
{
    std::lock_guard<std::mutex> lock(mutex);
    if (!cache.contains(key)) {
        return std::vector<T>();
    }
    std::vector<CachedValue<T>>* list = cache.object(key);
    std::vector<T> result;

    for (std::size_t i = 0; i < list->size(); i++) {
        result.push_back(list->at(i).getValue());
    }
    return result;
}

template <class Key, class T>
void TypedClientMultiCache<Key, T>::insert(const Key& key, T object)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto now = std::chrono::system_clock::now();
    CachedValue<T> cachedValue = CachedValue<T>(object, now);
    if (cache.contains(key)) {
        cache.object(key)->push_back(cachedValue);
        return;
    } else {
        std::vector<CachedValue<T>>* list = new std::vector<CachedValue<T>>();
        list->push_back(cachedValue);
        cache.insert(key, list);
    }
}

template <class Key, class T>
void TypedClientMultiCache<Key, T>::remove(const Key& key, T object)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (!cache.contains(key)) {
        return;
    }
    std::vector<CachedValue<T>>* list = cache.object(key);
    auto now = std::chrono::system_clock::now();
    CachedValue<T> cachedValue = CachedValue<T>(object, now);
    auto position = std::find(list->cbegin(), list->cend(), cachedValue);
    if (position == list->cend()) {
        return;
    }
    list->erase(position);
    if (list->empty()) {
        cache.remove(key);
    }
}

template <class Key, class T>
void TypedClientMultiCache<Key, T>::removeAll(const Key& key)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (!cache.contains(key)) {
        return;
    }
    cache.remove(key);
}

template <class Key, class T>
void TypedClientMultiCache<Key, T>::cleanup(std::chrono::milliseconds maxAcceptedAge)
{
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<Key> keyset = cache.keys();
    std::vector<CachedValue<T>>* entries;
    std::vector<int> attributesToBeRemoved;
    std::vector<Key> listsToBeRemoved;

    for (std::size_t i = 0; i < keyset.size(); i++) {
        entries = cache.object(keyset[i]);
        attributesToBeRemoved.clear();
        for (std::size_t e = 0; e < entries->size(); e++) {
            if (elapsed(entries->at(e).getTimestamp()) >= maxAcceptedAge) {
                attributesToBeRemoved.push_back(e);
            }
        }
        for (std::size_t u = 0; u < attributesToBeRemoved.size(); u++) {
            // size of list shrinks as an entry is removed
            auto begin = cache.object(keyset[i])->begin();
            cache.object(keyset[i])->erase(begin + (attributesToBeRemoved[u] - u));
        }
        if (cache.object(keyset[i])->empty()) {
            listsToBeRemoved.push_back(keyset[i]);
        }
    }
    for (std::size_t v = 0; v < listsToBeRemoved.size(); v++) {
        cache.remove(listsToBeRemoved[v]);
    }
}

template <class Key, class T>
std::chrono::milliseconds TypedClientMultiCache<Key, T>::elapsed(TimeStamp entryTime)
{
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - entryTime);
}

template <class Key, class T>
bool TypedClientMultiCache<Key, T>::contains(const Key& key)
{
    if (!cache.contains(key)) {
        return false;
    } else {
        return !cache.object(key)->empty();
    }
}

template <class Key, class T>
T TypedClientMultiCache<Key, T>::take(const Key& key)
{
    if (cache.contains(key)) {
        std::vector<CachedValue<T>>* list = cache.object(key);
        auto cachedValue = list->begin();
        auto returnCopy = *cachedValue;
        list->erase(cachedValue);
        if (list->empty()) {
            cache.remove(key);
        }
        return returnCopy.getValue();
    } else {
        return T();
    }
}

template <class Key, class T>
void TypedClientMultiCache<Key, T>::setMaxCost(int maxCost)
{
    cache.setMaxCost(maxCost);
}

template <class Key, class T>
int TypedClientMultiCache<Key, T>::getMaxCost()
{
    return cache.maxCost();
}

template <class Key, class T>
std::vector<Key> TypedClientMultiCache<Key, T>::getKeys() const
{
    return cache.keys();
}

} // namespace joynr
#endif // TYPEDCLIENTMULTICACHE_H
