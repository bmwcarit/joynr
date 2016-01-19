package io.joynr.messaging.routing;

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
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.IMessaging;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessMessagingStub;
import joynr.system.RoutingTypes.Address;

import java.util.Map;

public class MessagingStubFactory {

    @SuppressWarnings("rawtypes")
    private Map<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory> messagingStubFactories;

    @Inject
    @SuppressWarnings("rawtypes")
    public MessagingStubFactory(Map<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory> messagingStubFactories) {
        this.messagingStubFactories = messagingStubFactories;
        messagingStubFactories.put(InProcessAddress.class,
                                   new AbstractMiddlewareMessagingStubFactory<InProcessAddress>() {
                                       @Override
                                       protected IMessaging createInternal(InProcessAddress address) {
                                           return new InProcessMessagingStub((address).getSkeleton());
                                       }

                                       @Override
                                       public void shutdown() {
                                           //nothing to do
                                       }
                                   });
    }

    @SuppressWarnings({ "unchecked", "rawtypes" })
    public IMessaging create(Address address) {
        AbstractMiddlewareMessagingStubFactory messagingStubFactory = messagingStubFactories.get(address.getClass());
        if (messagingStubFactory == null) {
            throw new JoynrMessageNotSentException("Failed to send Request: Address type not supported");
        }
        return messagingStubFactory.create(address);
    }

    @SuppressWarnings("rawtypes")
    public void shutdown() {
        for (AbstractMiddlewareMessagingStubFactory messagingStubFactory : messagingStubFactories.values()) {
            messagingStubFactory.shutdown();
        }
    }
}
