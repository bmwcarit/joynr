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

import io.joynr.dispatching.RequestCaller;
import io.joynr.exceptions.JoynrRuntimeException;

/**
 * A data container for provider relevant information. It is used joynr internally to
 * manage multiple information required during the lifetime of the joynr provider.
 *
 * @author christoph.ainhauser
 */

public class ProviderContainer {
    private final RequestCaller requestCaller;
    private final SubscriptionPublisherObservable subscriptionPublisher;
    private final String interfaceName;
    private final Class<?> providedInterface;

    public ProviderContainer(String interfaceName,
                             Class<?> providedInterface,
                             RequestCaller requestCaller,
                             SubscriptionPublisherObservable subscriptionPublisher) {
        this.interfaceName = interfaceName;
        this.providedInterface = providedInterface;
        this.requestCaller = requestCaller;
        this.subscriptionPublisher = subscriptionPublisher;
    }

    public String getInterfaceName() {
        return interfaceName;
    }

    public Class<?> getProvidedInterface() {
        return providedInterface;
    }

    public RequestCaller getRequestCaller() {
        return requestCaller;
    }

    public SubscriptionPublisherObservable getSubscriptionPublisher() {
        return subscriptionPublisher;
    }

    /**
     *
     * @return the reflection proxy used to make the call to the provided method
     */
    public Object getProviderProxy() {
        if (requestCaller == null) {
            throw new JoynrRuntimeException("request caller was not found for " + providedInterface);
        }
        return requestCaller.getProxy();
    }
}
