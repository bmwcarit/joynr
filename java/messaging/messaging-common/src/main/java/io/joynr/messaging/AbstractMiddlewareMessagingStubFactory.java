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
package io.joynr.messaging;

import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.Map;

import joynr.system.RoutingTypes.Address;

abstract public class AbstractMiddlewareMessagingStubFactory<S extends IMessagingStub, A extends Address> {

    private static final int MAX_SIZE = 100000;

    private Map<A, S> stubMap = new LinkedHashMap<>() {
        protected boolean removeEldestEntry(Map.Entry eldest) {
            return size() > MAX_SIZE;
        }
    };

    protected abstract S createInternal(A address);

    public synchronized IMessagingStub create(A address) {
        S stub = stubMap.get(address);
        if (stub == null) {
            stub = createInternal(address);
            stubMap.put(address, stub);
        }
        return stub;
    }

    protected Collection<S> getAllMessagingStubs() {
        return stubMap.values();
    }

    public int getMaxCacheSize() {
        return MAX_SIZE;
    }

    public synchronized int getCacheSize() {
        return stubMap.size();
    }

    public synchronized void clearCache() {
        stubMap.clear();
    }
}
