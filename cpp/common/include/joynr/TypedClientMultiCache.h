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
#ifndef TYPEDCLIENTMULTICACHE_H
#define TYPEDCLIENTMULTICACHE_H

#include "joynr/CachedValue.h"

#include <QCache>
#include <QList>
#include <QMutex>
#include <QDateTime>

namespace joynr {

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
class TypedClientMultiCache {
    public:
        TypedClientMultiCache<Key, T>(int maxCost = MULTICACHE_DEFAULT_MAX_COST);


        /*
        * Returns the list of values stored for the attributeId filtered by a maximum age. If none exists, it returns an empty
        * QList object that can be tested for by using the isEmpty() method.
        * maxAcceptedAgeInMs -1 returns all values.
        *
        */
        QList< T > lookUp(const Key &key, qint64 maxAcceptedAgeInMs);
        /*
         *  Returns the list of values stored for the attribute not considering their age.
          */
        QList< T > lookUpAll(const Key &key);
        /*
        * Inserts the key (e.g. attributeId) and object into the cache.  If the attributeId already
        * exists in the cache the value is added to a list (no overwrite).
        * Note, this insert does not perform any validation on the value.
        */
        void insert(const Key &key, T object);
        /*
         * Removes the entry (specified by 'object') associated with key 'attributeID'.
         */
        void remove(const Key &key, T object);
        /*
        * Removes all entries associated with key 'attributeID'.
        */
        void removeAll(const Key &key);
        /*
        * Removes all entries that are older than 'maxAcceptedAgeInMs'. If there are several
        * entries for an attributeID, only the entries which are too old are deleted. Only if all
        * entries for an attributeID are too old will the key become invalid (deleted).
        * Can be used to clear the cache of all entries by setting 'maxAcceptedAgeInMs = 0'.
        */
        void cleanup(qint64 maxAcceptedAgeInMs);

        bool contains(const Key &key);

        T take(const Key &key);

        void setMaxCost(int maxCost);

        int getMaxCost();
    private:
        /*
         * Returns time since activation in ms (elapsed())
         */
        qint64 elapsed(qint64 entryTime);
        QCache<Key, QList<CachedValue<T> > > cache;
        QMutex mutex;
};


template <class Key, class T>
TypedClientMultiCache<Key, T>::TypedClientMultiCache(int maxCost) :
    cache(),
    mutex()
{
    cache.setMaxCost(maxCost);
}

template <class Key, class T>
QList< T > TypedClientMultiCache<Key, T>::lookUp(const Key &key, qint64 maxAcceptedAgeInMs){
    if (maxAcceptedAgeInMs==-1) {
        return lookUpAll(key);
    }
    QMutexLocker locker(&mutex);
    if(!cache.contains(key)){
        return QList<T>();
    }
    QList<CachedValue<T> >* list = cache.object(key);
    QList<T> result;
    qint64 time;

    for(int i=0;i<list->size();i++){
        time = list->value(i).getTimestamp();
        if(elapsed(time) <= maxAcceptedAgeInMs){
            result.append(list->value(i).getValue());
        }
    }
    return result;
}

template <class Key, class T>
QList< T > TypedClientMultiCache<Key, T>::lookUpAll(const Key &key){
    QMutexLocker locker(&mutex);
    if(!cache.contains(key)){
        return QList<T>();
    }
    QList<CachedValue<T> >* list = cache.object(key);
    QList<T> result;

    for(int i=0;i<list->size();i++){
            result.append(list->value(i).getValue());
    }
    return result;
}

template <class Key, class T>
void TypedClientMultiCache<Key, T>::insert(const Key &key, T object) {
    QMutexLocker locker(&mutex);
    CachedValue<T> cachedValue = CachedValue<T>(object, QDateTime::currentMSecsSinceEpoch());
    if(cache.contains(key)){
        cache.object(key)->append(cachedValue);
        return;
    }else{
         QList<CachedValue<T> >* list = new QList<CachedValue<T> >();
         list->append(cachedValue);
         cache.insert(key, list);
    }
}

template <class Key, class T>
void TypedClientMultiCache<Key, T>::remove(const Key &key, T object){
    QMutexLocker locker(&mutex);
    if(!cache.contains(key)){
        return;
    }
    QList<CachedValue<T> >* list = cache.object(key);
    CachedValue<T> cachedValue = CachedValue<T>(object, QDateTime::currentMSecsSinceEpoch());
    if(!list->contains(cachedValue)){
        return;
    }
    list->removeOne(cachedValue);
    if (list->isEmpty()){
        cache.remove(key);
    }
}

template <class Key, class T>
void TypedClientMultiCache<Key, T>::removeAll(const Key &key){
    QMutexLocker locker(&mutex);
    if(!cache.contains(key)){
        return;
    }
    cache.remove(key);
}

template <class Key, class T>
void TypedClientMultiCache<Key, T>::cleanup(qint64 maxAcceptedAgeInMs){
    QMutexLocker locker(&mutex);
    QList<Key> keyset = cache.keys();
    QList<CachedValue<T> >* entries;
    CachedValue<T> avalue;
    QList<int> attributesToBeRemoved;
    QList<Key> listsToBeRemoved;

    for(int i = 0;i< keyset.size();i++){
        entries = cache.object(keyset.value(i));
        attributesToBeRemoved.clear();
        for(int e=0;e<entries->size();e++){
            if(elapsed(entries->value(e).getTimestamp()) >= maxAcceptedAgeInMs){
                attributesToBeRemoved.push_back(e);
            }
        }
        for(int u = 0;u< attributesToBeRemoved.size();u++){
            // size of list shrinks as an entry is removed
            cache.object(keyset.value(i))->removeAt(attributesToBeRemoved.value(u)-u);
        }
        if(cache.object(keyset.value(i))->isEmpty()){
            listsToBeRemoved.append(keyset.value(i));
        }
    }
    for(int v=0; v<listsToBeRemoved.size();v++){
        cache.remove(listsToBeRemoved.value(v));
    }
}

template <class Key, class T>
qint64 TypedClientMultiCache<Key, T>::elapsed(qint64 entryTime){
    return QDateTime::currentMSecsSinceEpoch() - entryTime;
}

template <class Key, class T>
bool TypedClientMultiCache<Key, T>::contains(const Key &key){
    if(!cache.contains(key)){
        return false;
    } else {
        return !cache.object(key)->isEmpty();
    }
}

template <class Key, class T>
T TypedClientMultiCache<Key, T>::take(const Key &key){
    if (cache.contains(key)){
        QList<CachedValue<T> >* list = cache.object(key);
        CachedValue<T> cachedValue = list->first();
        list->removeOne(cachedValue);
        if(list->isEmpty()){
            cache.remove(key);
        }
        return cachedValue.getValue();
    } else {
        return T();
    }
}

template <class Key, class T>
void TypedClientMultiCache<Key, T>::setMaxCost(int maxCost){
    cache.setMaxCost(maxCost);
}

template <class Key, class T>
int TypedClientMultiCache<Key, T>::getMaxCost(){
    return cache.maxCost();
}


} // namespace joynr
#endif // TYPEDCLIENTMULTICACHE_H
