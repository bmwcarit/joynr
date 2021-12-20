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
package io.joynr.provider;

import java.util.concurrent.ConcurrentHashMap;

import com.google.inject.Inject;

import io.joynr.dispatching.RequestCallerFactory;
import io.joynr.exceptions.JoynrRuntimeException;

public class ProviderContainerFactory {
    private final ConcurrentHashMap<Object, ProviderContainer> providerContainers;
    private final SubscriptionPublisherFactory subscriptionPublisherFactory;
    private final RequestCallerFactory requestCallerFactory;

    @Inject
    public ProviderContainerFactory(SubscriptionPublisherFactory subscriptionPublisherFactory,
                                    RequestCallerFactory requestCallerFactory) {
        this.subscriptionPublisherFactory = subscriptionPublisherFactory;
        this.requestCallerFactory = requestCallerFactory;
        providerContainers = new ConcurrentHashMap<>();
    }

    public ProviderContainer create(final Object provider) {
        ProviderContainer container = providerContainers.computeIfAbsent(provider, k -> createInternal(k));
        return container;
    }

    private ProviderContainer createInternal(final Object provider) throws JoynrRuntimeException {
        return new ProviderContainer(ProviderAnnotations.getInterfaceName(provider),
                                     ProviderAnnotations.getProvidedInterface(provider),
                                     ProviderAnnotations.getMajorVersion(provider),
                                     requestCallerFactory.create(provider),
                                     subscriptionPublisherFactory.create(provider));
    }

    public void removeProviderContainer(JoynrProvider provider) {
        providerContainers.remove(provider);
    }
}
