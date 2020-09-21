/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.jeeintegration.api;

import joynr.types.ProviderQos;

/**
 * Provide beans implementing this interface in order to customize the settings used by the joynr
 * runtime for the registration of {@link ServiceProvider services} (aka provider).
 * <p>
 * You can create multiple beans implementing this interface. The first one which returns
 * <code>true</code> to {@link #providesFor(Class)} for a given service interface will be used to
 * obtain settings (i.e. {@link ProviderQos} or GBID(s)) in order to perform the service
 * registration. There is no guarantee in which order the factories will be asked, so it's safer to
 * just provide one factory for a given service interface.<br> If no factory is found to provide
 * settings for the service class, then default values are used.
 * <p>
 * This interface offers default methods in order to allow implementations to customize only the
 * settings they are interested in and not always implement the full interface.
 */
public interface ProviderRegistrationSettingsFactory {

    /**
     * This method is called in order to obtain a custom {@link ProviderQos} instance for
     * registering the service with.
     *
     * @return the provider QoS instance which joynr will use for service registration.
     */
    default ProviderQos createProviderQos() {
        return null;
    }

    /**
     * This method is called in order to obtain GBID(s) for registering the service with.
     *
     * @return array of GBIDs (one or more) of the backends in which joynr will register the
     *         service globally.
     */
    default String[] createGbids() {
        return null;
    }

    /**
     * This method is called in order to obtain a custom domain for registering the service with.
     *
     * @return the domain which joynr will use for service registration.
     */
    default String createDomain() {
        return null;
    }

    /**
     * This method is queried on each bean which implements this interface in order to check if it
     * can provide settings for a given service interface.
     *
     * @param serviceInterface
     *            the service interface for which joynr is trying to find registration settings.
     *
     * @return <code>true</code> if the implementing bean is responsible for the settings of this
     *         interface.
     */
    boolean providesFor(Class<?> serviceInterface);
}
