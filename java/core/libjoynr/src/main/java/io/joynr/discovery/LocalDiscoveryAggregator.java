package io.joynr.discovery;

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

import java.util.HashMap;

import io.joynr.runtime.SystemServicesSettings;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Lists;
import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import joynr.system.DiscoveryAsync;
import joynr.system.DiscoveryProvider;
import joynr.system.DiscoveryProxy;
import joynr.types.CommunicationMiddleware;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryQos;
import joynr.types.ProviderQos;


public class LocalDiscoveryAggregator implements DiscoveryAsync {

    private static final Logger logger = LoggerFactory.getLogger(LocalDiscoveryAggregator.class);
    private HashMap<String, DiscoveryEntry> provisionedDiscoveryEntries = new HashMap<>();
    private DiscoveryProxy discoveryProxy;

    @Inject
    public LocalDiscoveryAggregator(@Named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN) String systemServicesDomain,
                                    @Named(SystemServicesSettings.PROPERTY_CC_DISCOVERY_PROVIDER_PARTICIPANT_ID) String discoveryProviderParticipantId,
                                    @Named(ConfigurableMessagingSettings.PROPERTY_CC_CONNECTION_TYPE) CommunicationMiddleware clusterControllerConnection) {
        provisionedDiscoveryEntries.put(systemServicesDomain + DiscoveryProvider.INTERFACE_NAME,
                                        new DiscoveryEntry(systemServicesDomain,
                                                           DiscoveryProvider.INTERFACE_NAME,
                                                           discoveryProviderParticipantId,
                                                           new ProviderQos(),
                                                           new CommunicationMiddleware[] {clusterControllerConnection}));
    }

    public void setDiscoveryProxy(DiscoveryProxy discoveryProxy) {
        this.discoveryProxy = discoveryProxy;
    }


    @Override
    public Future<Void> add(Callback<Void> callback,
                            DiscoveryEntry discoveryEntry) {
        if (provisionedDiscoveryEntries.containsKey(discoveryEntry.getDomain() + discoveryEntry.getInterfaceName())) {
            logger.debug("Skipping registration of provisioned provider: " + discoveryEntry);
            Future<Void> future = new Future<>();
            future.resolve();
            callback.resolve();
            return future;
        } else {
            if (discoveryProxy == null) {
                throw new JoynrRuntimeException("LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                        + "local capabilitites directory.");
            }
            return discoveryProxy.add(callback, discoveryEntry);
        }
    }

    @Override
    public Future<DiscoveryEntry[]> lookup(Callback<DiscoveryEntry[]> callback,
                                               String domain,
                                               String interfaceName,
                                               DiscoveryQos discoveryQos) {
        if (provisionedDiscoveryEntries.containsKey(domain + interfaceName)) {
            DiscoveryEntry discoveryEntry = provisionedDiscoveryEntries.get(domain + interfaceName);
            DiscoveryEntry[] result = new DiscoveryEntry[] {discoveryEntry};
            // prevent varargs from interpreting the array as mulitple arguments
            callback.resolve((Object) result);

            Future<DiscoveryEntry[]> discoveryEntryFuture = new Future<>();
            discoveryEntryFuture.resolve(Lists.newArrayList(discoveryEntry));
            return discoveryEntryFuture;
        } else {
            if (discoveryProxy == null) {
                throw new JoynrRuntimeException("LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                        + "local capabilitites directory.");
            }
            return discoveryProxy.lookup(callback, domain, interfaceName, discoveryQos);
        }
    }

    @Override
    public Future<DiscoveryEntry> lookup(Callback<DiscoveryEntry> callback, String participantId) {

        if (discoveryProxy == null) {
            throw new JoynrRuntimeException("LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                    + "local capabilitites directory.");
        }
        return discoveryProxy.lookup(callback, participantId);

    }

    @Override
    public Future<Void> remove(Callback<Void> callback,
                               String participantId) {
        if (discoveryProxy == null) {
            throw new JoynrRuntimeException("LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                    + "local capabilitites directory.");
        }
        return discoveryProxy.remove(callback, participantId);
    }
}
