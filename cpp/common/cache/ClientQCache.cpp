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

#include <chrono>
#include <cstdint>

namespace joynr
{

using namespace std::chrono;

static const int MAX_CUMMULATIVE_CACHE_COST = 1000;

ClientQCache::ClientQCache() : cache(), mutex()
{
    cache.setMaxCost(MAX_CUMMULATIVE_CACHE_COST);
}

Variant ClientQCache::lookUp(const std::string& attributeId)
{
    QMutexLocker locker(&mutex);
    if (!cache.contains(attributeId)) {
        return Variant::NULL_VARIANT();
    }
    CachedValue<Variant>* entry = cache.object(attributeId);
    return entry->getValue();
}

void ClientQCache::insert(std::string attributeId, Variant value)
{
    QMutexLocker locker(&mutex);
    int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    CachedValue<Variant>* cachedValue = new CachedValue<Variant>(value, now);
    cache.insert(attributeId, cachedValue);
}

int64_t ClientQCache::elapsed(int64_t entryTime)
{
    int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    return now - entryTime;
}

} // namespace joynr
