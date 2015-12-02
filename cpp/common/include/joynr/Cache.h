/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#ifndef CACHE_H
#define CACHE_H

#include <map>
#include <memory>

namespace joynr
{
/**
 * Class Cache is designed after QCache from Qt framework.
 * It implements some basic features of QCache needed for
 * joynr purposes (if used outside joynr as replacement for QCache
 * consider extending).
 */
template <typename Key, typename Value>
class Cache
{
public:
    /**
     * @brief Cache with default macCost is 100
     */
    Cache() : cacheMap(), cacheCapacity(100)
    {
    }
    /**
     * @brief Cache with initialy customized cacheCapacity
     * @param cacheCapacity
     */
    Cache(uint32_t cacheCapacity) : cacheMap(), cacheCapacity(cacheCapacity)
    {
    }
    /**
     * @brief contains check if Cache contains given key
     * @param key
     * @return
     */
    bool contains(const Key& key)
    {
        return cacheMap.find(key) != cacheMap.end();
    }
    /**
     * @brief object to lookup object in the cache
     * @param key retrieve object stored under given key
     * @return pointer to object, or 0 if no object under given key found
     */
    Value* object(const Key& key)
    {
        if (cacheMap.find(key) == cacheMap.end()) {
            return 0;
        }
        return cacheMap.find(key)->second.get();
    }
    /**
     * @brief setCacheCapacity change cacheCapacity
     * @param cacheCapacity if cacheCapacity is smaller then current Cache size,
     * cache is going to be truncated by removing elements from the beginning
     * (oldest are going to be thrown away)
     */
    void setCacheCapacity(uint32_t cacheCapacity)
    {
        if (cacheCapacity < cacheMap.size()) {
            removeElementsFromTheBeginning(cacheMap.size() - cacheCapacity);
        }
        this->cacheCapacity = cacheCapacity;
    }
    /**
     * @brief insert value under given key. If cache contains value with same key
     * already, previous value is going to be removed, deleted and finaly replaced with new value
     * (this feature comes from std::map<Key, std::unique_ptr<Value>> implicitly)
     * @param key
     * @param value
     */
    void insert(Key key, Value* value)
    {
        if (cacheMap.size() == cacheCapacity) {
            removeElementsFromTheBeginning(1);
            assert(cacheMap.size() == cacheCapacity - 1);
        }
        cacheMap.insert(std::make_pair(key, std::unique_ptr<Value>(value)));
        assert(cacheMap.size() != 0);
        assert(cacheMap.size() <= cacheCapacity);
    }
    /**
     * @brief clear removes and destroys all values
     */
    void clear()
    {
        cacheMap.clear();
    }
    /**
     * @brief size
     * @return number of key-value pairs stored in cache
     */
    int size()
    {
        return cacheMap.size();
    }

private:
    std::map<Key, std::unique_ptr<Value>> cacheMap;
    uint32_t cacheCapacity;

    /**
     * @brief removeElementsFromTheBeginning removes given number of elements from the beginning
     * @param numberOfElements
     */
    void removeElementsFromTheBeginning(int numberOfElements)
    {
        while (numberOfElements != 0) {
            auto firstAndOldestElement = cacheMap.begin();
            std::unique_ptr<Value> olderstValue(std::move(cacheMap.begin()->second));
            cacheMap.erase(firstAndOldestElement);

            Value* value_ptr = olderstValue.release();
            delete value_ptr;
            --numberOfElements;
        }
    }
};

} // namespace joynr
#endif // CACHE_H
