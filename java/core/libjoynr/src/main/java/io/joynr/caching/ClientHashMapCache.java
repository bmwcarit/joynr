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
package io.joynr.caching;

import java.util.NoSuchElementException;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.function.Predicate;

import com.google.inject.Inject;

import io.joynr.qos.QualityOfService;
import io.joynr.qos.TimeComparisonCompatibility;
import io.joynr.qos.compatibility.QoSCacheEntryTimeToLiveCompatibility;
import io.joynr.qos.compatibility.QoSDataFreshnessCompatibility;

public class ClientHashMapCache implements ClientCache {

    private ConcurrentMap<String, CachedValue> cache = new ConcurrentHashMap<String, CachedValue>();
    private QualityOfService qos = new QualityOfService();
    private QoSDataFreshnessCompatibility dataFreshnessCompatibility = new QoSDataFreshnessCompatibility(qos);

    @Inject
    public ClientHashMapCache() {
    }

    public void setQoS(QualityOfService qos) {
        this.qos = (qos != null) ? new QualityOfService(qos) : new QualityOfService();
        dataFreshnessCompatibility = new QoSDataFreshnessCompatibility(qos);
    }

    @Override
    public boolean isCacheValueValid(String attributeId) {
        if (!cache.containsKey(attributeId)) {
            return false;
        }
        return dataFreshnessCompatibility.test(cache.get(attributeId));
    }

    @Override
    public Object lookUp(String attributeId) throws NoSuchElementException {
        if (!cache.containsKey(attributeId)) {
            throw new NoSuchElementException("The attribute id " + attributeId + " could not be found.");
        }

        CachedValue value = cache.get(attributeId);
        return value.getValue();
    }

    @Override
    public void insert(String attributeId, Object value) {
        Long timestamp = System.currentTimeMillis();
        CachedValue cachedValue = new CachedValue(value, timestamp);
        cache.put(attributeId, cachedValue);

    }

    public void cleanUp() {
        Predicate<TimeComparisonCompatibility> qoSCacheEntryTimeToLive = new QoSCacheEntryTimeToLiveCompatibility(qos);

        cache.forEach((String k, CachedValue v) -> {
            // remove if expired
            if (!qoSCacheEntryTimeToLive.test(v)) {
                cache.remove(k);
            }
        });
    }
}
