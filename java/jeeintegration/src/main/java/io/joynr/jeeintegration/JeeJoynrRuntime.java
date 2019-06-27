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
package io.joynr.jeeintegration;

import io.joynr.provider.JoynrProvider;
import io.joynr.proxy.Future;
import io.joynr.runtime.JoynrRuntime;
import joynr.types.ProviderQos;

public interface JeeJoynrRuntime extends JoynrRuntime {

    /**
     * Registers a provider in the joynr framework for the provided GBIDs (for internal use by JEE integration)
     *
     * @param domain
     *            The domain the provider should be registered for. Has to be identical at the client to be able to find
     *            the provider.
     * @param provider
     *            Instance of the provider implementation (has to extend a generated ...AbstractProvider).
     *            It is assumed that the provided implementations offers the following annotations in its
     *            (inherited) class definition: {@link io.joynr.provider.JoynrInterface} and {@link io.joynr.JoynrVersion}.
     * @param providerQos
     *            The provider's quality of service settings.
     * @param gbids
     *            The GBIDs in which the provider shall be registered. This parameter may be provided as String
     *            array with zero elements, in which case the provider is registered in the default backend.
     * @param awaitGlobalRegistration
     *            If true, wait for global registration to complete or timeout in case of problems.
     * @param interfaceClass
     *            The interface class of the provider.
     * @return Returns a Future which can be used to check the registration status.
     */
    Future<Void> registerProvider(String domain,
                                  JoynrProvider provider,
                                  ProviderQos providerQos,
                                  String[] gbids,
                                  boolean awaitGlobalRegistration,
                                  final Class<?> interfaceClass);

}
