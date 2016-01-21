package io.joynr.messaging.serialize;

import java.util.Map;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.messaging.JoynrMessageSerializer;
import joynr.system.RoutingTypes.Address;

public class MessageSerializerFactory {

    public static final String MIDDLEWARE_MESSAGE_SERIALIZER_FACTORIES = "middleware_message_serializer_factories";
    @SuppressWarnings("rawtypes")
    private Map<Class<? extends Address>, AbstractMiddlewareMessageSerializerFactory> middlewareMessageSerializerFactories;

    @Inject
    @SuppressWarnings("rawtypes")
    public MessageSerializerFactory(@Named(MIDDLEWARE_MESSAGE_SERIALIZER_FACTORIES) Map<Class<? extends Address>, AbstractMiddlewareMessageSerializerFactory> middlewareMessageSerializerFactories) {
        this.middlewareMessageSerializerFactories = middlewareMessageSerializerFactories;
    }

    @SuppressWarnings({ "unchecked", "rawtypes" })
    public JoynrMessageSerializer create(Address address) {
        AbstractMiddlewareMessageSerializerFactory messageSerializerFactory = middlewareMessageSerializerFactories.get(address.getClass());
        if (messageSerializerFactory == null) {
            throw new JoynrMessageNotSentException("Failed to find serializer. Address type not supported: "
                    + address.getClass().getCanonicalName());
        }
        return messageSerializerFactory.create(address);
    }

    @SuppressWarnings("rawtypes")
    public void register(Class<? extends Address> address,
                         AbstractMiddlewareMessageSerializerFactory middlewareMessageSerializerFactory) {
        middlewareMessageSerializerFactories.put(address, middlewareMessageSerializerFactory);
    }
}
