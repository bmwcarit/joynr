package io.joynr.provider;

/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

import io.joynr.dispatching.RequestCallerFactory;
import io.joynr.exceptions.JoynrRuntimeException;

import java.util.HashMap;
import java.util.Map;

import com.google.inject.Inject;

public class ProviderContainerFactory {
    private final Map<Object, ProviderContainer> providerContainers;
    private final SubscriptionPublisherFactory subscriptionPublisherFactory;
    private final RequestCallerFactory requestCallerFactory;

    @Inject
    public ProviderContainerFactory(SubscriptionPublisherFactory subscriptionPublisherFactory,
                                    RequestCallerFactory requestCallerFactory) {
        this.subscriptionPublisherFactory = subscriptionPublisherFactory;
        this.requestCallerFactory = requestCallerFactory;
        providerContainers = new HashMap<>();
    }

    public ProviderContainer create(final Object provider) {
        if (providerContainers.get(provider) == null) {
            providerContainers.put(provider, createInternal(provider));
        }
        return providerContainers.get(provider);
    }

    private ProviderContainer createInternal(final Object provider) throws JoynrRuntimeException {
        return new ProviderContainer(ProviderAnnotations.getInterfaceName(provider),
                                     ProviderAnnotations.getProvidedInterface(provider),
                                     requestCallerFactory.create(provider),
                                     subscriptionPublisherFactory.create(provider));
    }

    public void removeProviderContainer(JoynrProvider provider) {
        providerContainers.remove(provider);
    }
}
