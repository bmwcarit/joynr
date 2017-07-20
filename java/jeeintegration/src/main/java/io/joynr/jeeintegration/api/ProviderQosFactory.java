/**
 *
 */
package io.joynr.jeeintegration.api;

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

import joynr.types.ProviderQos;

/**
 * Provide beans implementing this interface in order to customise the {@link ProviderQos} instance used to register
 * {@link ServiceProvider services}.
 * <p>
 * You can provide multiple beans implementing this interface. The first one which returns <code>true</code> to
 * {@link #providesFor(Class)} for a given service interface will be used to obtain a {@link ProviderQos} instance via
 * {@link #create()} in order to perform the service registration. There is no guarantee in which order the factories
 * will be asked, so it's safer to just provide one factory for a given service interface type.<br>
 * If no factory is found to provide the QoS information, then the default values are used.
 */
public interface ProviderQosFactory {

    /**
     * This method is called in order to obtain a custom {@link ProviderQos} instance for registering the service with.
     *
     * @return see method description.
     */
    ProviderQos create();

    /**
     * This method is queried on each bean which implements this interface in order to see if it can {@link #create()
     * provide} an instance of {@link ProviderQos} for a given service interface.
     *
     * @param serviceInterface
     *            the service interface for which a provider QoS instance is required for registration.
     *
     * @return the provider QoS instance which is provided to the runtime for service registration.
     */
    boolean providesFor(Class<?> serviceInterface);

}
