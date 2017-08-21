package io.joynr.messaging;

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

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

import joynr.system.RoutingTypes.Address;

abstract public class AbstractMiddlewareMessagingStubFactory<S extends IMessagingStub, A extends Address> {

    private Map<A, S> stubMap = new HashMap<>();

    protected abstract S createInternal(A address);

    public synchronized IMessagingStub create(A address) {
        if (!stubMap.containsKey(address)) {
            stubMap.put(address, createInternal(address));
        }
        return stubMap.get(address);
    }

    protected Collection<S> getAllMessagingStubs() {
        return stubMap.values();
    }
}
