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
#ifndef CACHE_H
#define CACHE_H

#include <cassert>
#include <map>
#include <memory>
#include <vector>

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
    Cache() : _cacheMap(), _cacheCapacity(100)
    {
    }
    /**
     * @brief Cache with initialy customized cacheCapacity
     * @param cacheCapacity
     */
    explicit Cache(std::uint32_t cacheCapacity)
            : _cacheMap(), _cacheCapacity(static_cast<std::size_t>(cacheCapacity))
    {
    }
    /**
     * @brief contains check if Cache contains given key
     * @param key
     * @return
     */
    bool contains(const Key& key) const
    {
        return _cacheMap.find(key) != _cacheMap.cend();
    }
    /**
     * @brief object to lookup object in the cache
     * @param key retrieve object stored under given key
     * @return pointer to object, or 0 if no object under given key found
     */
    Value* object(const Key& key) const
    {
        auto elementIterator = _cacheMap.find(key);
        if (elementIterator == _cacheMap.end()) {
            return nullptr;
        }
        return elementIterator->second.get();
    }
    /**
     * @brief setCacheCapacity change cacheCapacity
     * @param cacheCapacity if cacheCapacity is smaller then current Cache size,
     * cache is going to be truncated by removing elements from the beginning
     * (oldest are going to be thrown away)
     */
    void setCacheCapacity(std::uint32_t cacheCapacity)
    {
        std::size_t capacity = static_cast<std::size_t>(cacheCapacity);
        if (capacity < _cacheMap.size()) {
            removeElementsFromTheBeginning(_cacheMap.size() - capacity);
        }
        this->_cacheCapacity = capacity;
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
        if (_cacheMap.size() == _cacheCapacity) {
            removeElementsFromTheBeginning(1);
            assert(_cacheMap.size() == _cacheCapacity - 1);
        }
        std::size_t sizeOld = _cacheMap.size();
        _cacheMap.insert(std::make_pair(key, std::unique_ptr<Value>(value)));
        std::size_t sizeNew = _cacheMap.size();
        assert(sizeOld != sizeNew);
        assert(sizeNew != 0);
        assert(sizeNew <= _cacheCapacity);
        assert(_cacheMap.find(key) != _cacheMap.end());
    }
    /**
     * @brief clear removes and destroys all values
     */
    void clear()
    {
        _cacheMap.clear();
    }
    /**
     * @brief size
     * @return number of key-value pairs stored in cache
     */
    int size()
    {
        return _cacheMap.size();
    }

    /**
     * @brief remove object stored under specified key
     * @param key
     */
    void remove(const Key& key)
    {
        auto elementIterator = _cacheMap.find(key);
        if (elementIterator != _cacheMap.end()) {
            _cacheMap.erase(elementIterator);
        }
    }

    /**
     * @brief keys retrieves collection of all cache keys
     * @return vector of keys
     */
    std::vector<Key> keys() const
    {
        std::vector<Key> keys;
        keys.reserve(_cacheMap.size());
        for (auto&& mapIterator : _cacheMap) {
            keys.push_back(mapIterator.first);
        }

        return keys;
    }

private:
    std::map<Key, std::unique_ptr<Value>> _cacheMap;
    std::size_t _cacheCapacity;

    /**
     * @brief removeElementsFromTheBeginning removes given number of elements from the beginning
     * @param numberOfElements
     */
    void removeElementsFromTheBeginning(int numberOfElements)
    {
        while (numberOfElements != 0) {
            auto firstAndOldestElement = _cacheMap.begin();
            std::unique_ptr<Value> olderstValue(std::move(_cacheMap.begin()->second));
            _cacheMap.erase(firstAndOldestElement);

            Value* value_ptr = olderstValue.release();
            delete value_ptr;
            --numberOfElements;
        }
    }
};

} // namespace joynr
#endif // CACHE_H
