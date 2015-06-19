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
#include "joynr/ClientQCache.h"

#include <QDateTime>

namespace joynr
{

static const int MAX_CUMMULATIVE_CACHE_COST = 1000;

ClientQCache::ClientQCache() : cache(), mutex()
{
    cache.setMaxCost(MAX_CUMMULATIVE_CACHE_COST);
}

QVariant ClientQCache::lookUp(const QString& attributeId)
{
    QMutexLocker locker(&mutex);
    if (!cache.contains(attributeId)) {
        return QVariant();
    }
    CachedValue<QVariant>* entry = cache.object(attributeId);
    return entry->getValue();
}

void ClientQCache::insert(QString attributeId, QVariant value)
{
    QMutexLocker locker(&mutex);
    CachedValue<QVariant>* cachedValue =
            new CachedValue<QVariant>(value, QDateTime::currentMSecsSinceEpoch());
    cache.insert(attributeId, cachedValue);
}

qint64 ClientQCache::elapsed(qint64 entryTime)
{
    return QDateTime::currentMSecsSinceEpoch() - entryTime;
}

} // namespace joynr
