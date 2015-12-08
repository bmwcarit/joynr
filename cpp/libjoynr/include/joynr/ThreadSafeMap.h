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

#include <QMap>
#include <QReadWriteLock>

namespace joynr
{

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
    bool contains(const Key& key);
    void deleteAll();
    int size();

private:
    DISALLOW_COPY_AND_ASSIGN(ThreadSafeMap);
    QMap<Key, T> map;
    QReadWriteLock lock;
};

template <class Key, class T>
ThreadSafeMap<Key, T>::ThreadSafeMap()
        : map(), lock()
{
}

template <class Key, class T>
void ThreadSafeMap<Key, T>::insert(const Key& key, const T& value)
{
    lock.lockForWrite();
    map.insert(key, value);
    lock.unlock();
}

template <class Key, class T>
void ThreadSafeMap<Key, T>::remove(const Key& key)
{
    lock.lockForWrite();
    map.remove(key);
    lock.unlock();
}

template <class Key, class T>
T ThreadSafeMap<Key, T>::value(const Key& key)
{
    T aValue;
    lock.lockForRead();
    aValue = map.value(key);
    lock.unlock();
    return aValue;
}

template <class Key, class T>
T ThreadSafeMap<Key, T>::take(const Key& key)
{
    T aValue;
    lock.lockForWrite();
    aValue = map.take(key);
    lock.unlock();
    return aValue;
}

template <class Key, class T>
bool ThreadSafeMap<Key, T>::contains(const Key& key)
{
    bool aValue;
    lock.lockForRead();
    aValue = map.contains(key);
    lock.unlock();
    return aValue;
}

template <class Key, class T>
void ThreadSafeMap<Key, T>::deleteAll()
{
    lock.lockForWrite();
    for (const QString& str : map.keys()) {
        T value = map.take(str);
        delete value;
        value = NULL;
    }
    lock.unlock();
}

template <class Key, class T>
int ThreadSafeMap<Key, T>::size()
{
    return map.size();
}

} // namespace joynr

#endif // THREADSAFEMAP_H
