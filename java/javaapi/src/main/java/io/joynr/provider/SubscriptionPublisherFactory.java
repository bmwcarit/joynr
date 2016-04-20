package io.joynr.provider;

/*
 * #%L
 * %%
 * Copyright (C) 2016 BMW Car IT GmbH
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

import static io.joynr.provider.ProviderAnnotations.getInterfaceName;
import io.joynr.exceptions.JoynrRuntimeException;

public class SubscriptionPublisherFactory {
    /*
     * Suppressing warnings allows to case a provider to SubscriptionPublisherInjection without template
     * parameter. It is guaranteed by the generated joynr providers that the cast works as expected
     */
    @SuppressWarnings({ "unchecked", "rawtypes" })
    public AbstractSubscriptionPublisher create(final Object provider) throws JoynrRuntimeException {
        try {
            String subscriptionPublisherClassName = "joynr." + getInterfaceName(provider).replace("/", ".")
                    + SubscriptionPublisher.class.getSimpleName();

            Class<?> subscriptionPublisherImplClass = Class.forName(subscriptionPublisherClassName + "Impl");
            AbstractSubscriptionPublisher subscriptionPublisherImpl = (AbstractSubscriptionPublisher) subscriptionPublisherImplClass.newInstance();
            try {
                Class<?> subscriptionPublisherInjectionClass = Class.forName(subscriptionPublisherClassName
                        + "Injection");
                if (subscriptionPublisherInjectionClass.isInstance(provider)) {
                    ((SubscriptionPublisherInjection) provider).setSubscriptionPublisher(subscriptionPublisherImpl);
                }
            } catch (ClassNotFoundException exception) {
                //if subscriptionPublisherInjectionClassname could not be found, we assume that the provider does not require a SubscriptionPublisher
            }

            return subscriptionPublisherImpl;
        } catch (Exception e) {
            throw new JoynrRuntimeException("ProviderContainerFactory could not create subscription publisher for "
                    + provider.getClass().getSimpleName() + " due to the following error: " + e.getMessage());
        }
    }
}
