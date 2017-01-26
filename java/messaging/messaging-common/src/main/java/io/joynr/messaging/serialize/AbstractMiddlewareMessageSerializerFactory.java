package io.joynr.messaging.serialize;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import java.util.HashMap;
import java.util.Map;

import io.joynr.messaging.JoynrMessageSerializer;
import joynr.system.RoutingTypes.Address;

abstract public class AbstractMiddlewareMessageSerializerFactory<A extends Address> {

    private Map<A, JoynrMessageSerializer> serializerMap = new HashMap<>();

    protected abstract JoynrMessageSerializer createInternal(A address);

    public JoynrMessageSerializer create(A address) {
        if (!serializerMap.containsKey(address)) {
            serializerMap.put(address, createInternal(address));
        }
        return serializerMap.get(address);
    }
}
