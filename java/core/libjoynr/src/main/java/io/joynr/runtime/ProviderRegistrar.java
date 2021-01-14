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
package io.joynr.runtime;

import java.util.Arrays;

import io.joynr.capabilities.CapabilitiesRegistrar;
import io.joynr.provider.JoynrProvider;
import io.joynr.proxy.Future;
import joynr.types.ProviderQos;

/**
 * Class handling provider registration. An object of this Class is retrieved by calling getProviderRegistrar()
 * in the JoynrRuntime. The following properties are mandatory and have to be supplied for this call:
 *
 *     Domain:
 *         The domain the provider should be registered for. Has to be identical at the client to be able to find
 *         the provider.
 *
 *     Provider:
 *         Instance of the provider implementation. It is assumed that the provided implementations offers
 *         the following annotations in its (inherited) class definition: {@link io.joynr.provider.JoynrInterface}
 *         and {@link io.joynr.JoynrVersion}.
 *
 * Afterwards, the following properties can be configured via a builder pattern, before calling register() or
 * registerInAllBackends():
 *
 *     ProviderQos:
 *         The provider's quality of service settings. Per default local and global scope
 *         registration is selected.
 *
 *     Gbids:
 *         If registration to local and global scope is requested, the provider is registered per
 *         default to all GBIDs configured in the cluster controller.
 *         The 'Gbids' parameter can be provided to override the GBIDs selection in the cluster
 *         controller.
 *         The global capabilities directory identified by the first selected GBID performs the
 *         registration.
 *
 *     AwaitGlobalRegistration:
 *         If true, wait for global registration to complete or timeout, if required.
 *         The default value is false.
 *
 */
public class ProviderRegistrar {

    private CapabilitiesRegistrar capabilitiesRegistrar;
    private String domain;
    private Object provider;
    private ProviderQos providerQos;
    private String[] gbids;
    private boolean awaitGlobalRegistration;

    /**
     * Do not instantiate ProviderRegistrar objects yourself. Instead use the getProviderRegistrar() method
     * of the JoynrRuntime to have a properly instantiated object.
     *
     * @param capabilitiesRegistrar
     *            The CapabilitiesRegistrar to be used by this ProviderRegistrar.
     * @param domain
     *            The domain the provider should be registered for. Has to be identical at the client to be able to find
     *            the provider.
     * @param provider
     *            Instance of the provider implementation. It is assumed that the provided implementations offers
     *            the following annotations in its (inherited) class definition: {@link io.joynr.provider.JoynrInterface}
     *            and {@link io.joynr.JoynrVersion}.
     */
    ProviderRegistrar(CapabilitiesRegistrar capabilitiesRegistrar, String domain, JoynrProvider provider) {
        this.capabilitiesRegistrar = capabilitiesRegistrar;
        this.domain = domain;
        this.provider = provider;
        providerQos = new ProviderQos();

        gbids = new String[0];
        awaitGlobalRegistration = false;
    }

    /**
     * @param providerQos
     *            The provider's quality of service settings.
     * @return the ProviderRegistrar instance.
     */
    public ProviderRegistrar withProviderQos(ProviderQos providerQos) {
        this.providerQos = providerQos;
        return this;
    }

    /**
     * @param gbids
     *            Subset of GBIDs configured in the cluster controller for custom global
     *            registration.
     *            If the 'gbids' parameter is empty, the GBIDs configured in the cluster controller
     *            are used (reset to default behavior).
     * @return the ProviderRegistrar instance.
     */
    public ProviderRegistrar withGbids(String[] gbids) {
        if (gbids == null) {
            throw new IllegalArgumentException("Provided gbid array must not be null!");
        }
        for (String gbid : gbids) {
            if (gbid == null || gbid.equals("")) {
                throw new IllegalArgumentException("Provided gbid value(s) must not be null or empty!");
            }
        }
        this.gbids = gbids.clone();
        return this;
    }

    /**
     * Enable awaitGlobalRegistration to wait for global registration result.
     * @return the ProviderRegistrar instance.
     */
    public ProviderRegistrar awaitGlobalRegistration() {
        this.awaitGlobalRegistration = true;
        return this;
    }

    /**
     * Register the provider with the configured settings.
     * @return Returns a Future which can be used to check the registration status.
     */
    public Future<Void> register() {
        return capabilitiesRegistrar.registerProvider(domain, provider, providerQos, gbids, awaitGlobalRegistration);
    }

    /**
     * Register the provider in all known backends. Configured GBIDs will be ignored.
     * @return Returns a Future which can be used to check the registration status.
     * 
     * @deprecated Use {@link #register()} instead.
     */
    @Deprecated
    public Future<Void> registerInAllBackends() {
        return capabilitiesRegistrar.registerInAllKnownBackends(domain, provider, providerQos, awaitGlobalRegistration);
    }

    public String toString() {
        StringBuffer buffer = new StringBuffer();
        buffer.append("ProviderRegistrar:[ domain:\"" + domain + "\", provider:\"" + provider.toString()
                + "\", providerQos:\"" + providerQos.toString() + "\", gbids: [" + Arrays.toString(gbids)
                + "], awaitGlobalRegistration: \"" + awaitGlobalRegistration + "\" ]");
        return buffer.toString();
    }
}
