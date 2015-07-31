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
#include "common/cache/ClientMultiQCache.h"

#include <QDateTime>

#include "joynr/CachedValue.h"

namespace joynr
{

const int ClientMultiQCache::MAX_CUMMULATIVE_CACHE_COST = 1000;

ClientMultiQCache::ClientMultiQCache() : cache(), mutex()

{
    cache.setMaxCost(MAX_CUMMULATIVE_CACHE_COST);
}

QList<QVariant> ClientMultiQCache::lookUp(const QString& attributeId, qint64 maxAcceptedAgeInMs)
{
    QMutexLocker locker(&mutex);
    if (!cache.contains(attributeId)) {
        return QList<QVariant>();
    }
    QList<CachedValue<QVariant>>* list = cache.object(attributeId);
    QList<QVariant> result;
    qint64 time;

    for (int i = 0; i < list->size(); i++) {
        time = list->value(i).getTimestamp();
        if (elapsed(time) < maxAcceptedAgeInMs) {
            result.append(list->value(i).getValue());
        }
    }
    return result;
}

void ClientMultiQCache::insert(QString attributeId, QVariant value)
{
    QMutexLocker locker(&mutex);
    CachedValue<QVariant> cachedValue =
            CachedValue<QVariant>(value, QDateTime::currentMSecsSinceEpoch());
    if (cache.contains(attributeId)) {
        cache.object(attributeId)->append(cachedValue);
        return;
    } else {
        QList<CachedValue<QVariant>>* list = new QList<CachedValue<QVariant>>();
        list->append(cachedValue);
        cache.insert(attributeId, list);
    }
}

qint64 ClientMultiQCache::elapsed(qint64 entryTime)
{
    return QDateTime::currentMSecsSinceEpoch() - entryTime;
}

} // namespace joynr
