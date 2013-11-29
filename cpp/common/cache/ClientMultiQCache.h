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
#ifndef CLIENTMULTIQCACHE_H
#define CLIENTMULTIQCACHE_H

#include "common/cache/IClientMultiCache.h"

#include <QCache>
#include <QMutex>
#include <QString>
#include <QVariant>
#include <QList>

namespace joynr {

/**
 * Implements IClientMultiCache. Stores SEVERAL objects with a key
 * and a timestamp in milliseconds (insert does not overwrite).
 * The timestamp is generated automatically. Access is thread safe.
 * The cache automatically manages the objects that are inserted and
 * deletes them to make room for new objects, if necessary (more than
 * 1000 entries). This is in memory only, no persistence.
 *
 */
class ClientMultiQCache : public IClientMultiCache{

public:
    virtual ~ClientMultiQCache(){}

    ClientMultiQCache();
    /*
     * Returns the list of values stored for the attributeId. If none exists, it returns an empty
     * QList object that can be tested for by using the isEmpty() method.
     */
    virtual QList<QVariant> lookUp(const QString& attributeId, qint64 maxAcceptedAgeInMs);

    /*
     * Inserts the key (attributeId) and value into the cache.  If the attributeId already
     * exists in the cache the value is added to a list (no overwrite).
     * Note, this insert does not perform any validation on the value.
     */
    virtual void insert(QString attributeId, QVariant value);

private:
    /*
     * Returns time since activation in ms (elapsed())
     */
    qint64 elapsed(qint64 entryTime);
    QCache<QString, QList<CachedValue<QVariant> > > cache;
    static const int MAX_CUMMULATIVE_CACHE_COST;
    QMutex mutex;
};


} // namespace joynr
#endif // CLIENTMULTIQCACHE_H
