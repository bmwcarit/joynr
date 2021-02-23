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
package io.joynr.capabilities;

import io.joynr.proxy.Future;
import joynr.types.ProviderQos;

public interface CapabilitiesRegistrar {
    /**
     * Registers a provider at the capabilities directory for the provided backends to make it available at other cluster controllers and the
     * messaging endpoint directory to dispatch incoming requests.
     *
     * @param domain
     *            Domain of the provided service.
     * @param provider
     *            Provider instance.
     * @param providerQos
     *            Provider quality of service.
     * @param gbids
     *            GBIDs that the provider shall be registered in.
     * @param awaitGlobalRegistration
     *            If true, wait for global registration to complete or timeout, if required.
     * @return registration future
     */
    Future<Void> registerProvider(String domain,
                                  Object provider,
                                  ProviderQos providerQos,
                                  String[] gbids,
                                  boolean awaitGlobalRegistration);

    /**
     * Registers a provider at the capabilities directory for all known backends to make it available at other cluster controllers and the
     * messaging endpoint directory to dispatch incoming requests.
     *
     * @param domain
     *            Domain of the provided service.
     * @param provider
     *            Provider instance.
     * @param providerQos
     *            Provider quality of service.
     * @param awaitGlobalRegistration
     *            If true, wait for global registration to complete or timeout, if required.
     * @return registration future
     * @deprecated Use 
     *             {@link #registerProvider(String, Object, ProviderQos, String[], boolean)} instead.
     */
    @Deprecated
    Future<Void> registerInAllKnownBackends(final String domain,
                                            Object provider,
                                            ProviderQos providerQos,
                                            boolean awaitGlobalRegistration);

    /**
     * Trigger the unregistration of a provider from the joynr communication framework so that it can
     * no longer be used or discovered.
     *
     * This method just triggers the removal of a provider without waiting until it is actually
     * triggered. It returns immediately but it is possible and recommended to get notified if
     * the removal has been triggered via the returned future.
     *
     * <b>Note</b>: This does not guarantee a successful execution of provider's removal from the
     * GlobalCapabilitiesDirectory in case the provider is registered globally (default),
     * i.e. ProviderScope.GLOBAL has been set in ProviderQos when registering the provider.
     * It does not wait for a response from global capabilities directory and does not get informed
     * about errors or success.  The cluster controller will internally repeat the global remove
     * operation until it succeeds or the cluster controller is shut down. The provider will be
     * removed from the local capabilities directory after the global removal. If the provider
     * is running in libjoynr runtime connected to a standalone cluster controller, the cluster
     * controller will still repeat the global remove operation until it succeeds or the
     * cluster controller is shut down.
     * 
     * @param domain
     *            The domain the provider was registered for.
     * @param provider
     *            The provider instance.
     * @return unregistration future
     */
    Future<Void> unregisterProvider(String domain, Object provider);
}
