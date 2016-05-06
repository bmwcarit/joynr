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

import io.joynr.provider.ProviderAnnotations;

import java.util.HashMap;

import io.joynr.runtime.SystemServicesSettings;
import joynr.system.Routing;

import com.google.common.collect.Lists;
import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import joynr.system.DiscoveryAsync;
import joynr.system.DiscoveryProvider;
import joynr.system.DiscoveryProxy;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryQos;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;
import joynr.types.Version;


public class LocalDiscoveryAggregator implements DiscoveryAsync {

    private static final long NO_EXPIRY = Long.MAX_VALUE;
    private HashMap<String, DiscoveryEntry> provisionedDiscoveryEntries = new HashMap<>();
    private DiscoveryProxy discoveryProxy;

    @Inject
    public LocalDiscoveryAggregator(@Named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN) String systemServicesDomain,
                                    @Named(SystemServicesSettings.PROPERTY_CC_DISCOVERY_PROVIDER_PARTICIPANT_ID) String discoveryProviderParticipantId,
                                    @Named(SystemServicesSettings.PROPERTY_CC_ROUTING_PROVIDER_PARTICIPANT_ID) String routingProviderParticipantId) {
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        String defaultPublicKeyId = "";
        provisionedDiscoveryEntries.put(systemServicesDomain + ProviderAnnotations.getInterfaceName(DiscoveryProvider.class),
                                        new DiscoveryEntry(new Version(),
                                                           systemServicesDomain,
                                                           ProviderAnnotations.getInterfaceName(DiscoveryProvider.class),
                                                           discoveryProviderParticipantId,
                                                           providerQos,
                                                           System.currentTimeMillis(),
                                                           NO_EXPIRY,
                                                           defaultPublicKeyId));
        //provision routing provider to prevent lookup via discovery proxy during startup.
        provisionedDiscoveryEntries.put(systemServicesDomain + Routing.INTERFACE_NAME,
                                        new DiscoveryEntry(new Version(),
                                                           systemServicesDomain,
                                                           Routing.INTERFACE_NAME,
                                                           routingProviderParticipantId,
                                                           providerQos,
                                                           System.currentTimeMillis(),
                                                           NO_EXPIRY,
                                                           defaultPublicKeyId));
    }

    public void setDiscoveryProxy(DiscoveryProxy discoveryProxy) {
        this.discoveryProxy = discoveryProxy;
    }


    @Override
    public Future<Void> add(Callback<Void> callback,
                            DiscoveryEntry discoveryEntry) {
            if (discoveryProxy == null) {
                throw new JoynrRuntimeException("LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                        + "local capabilitites directory.");
            }
            return discoveryProxy.add(callback, discoveryEntry);
    }

    @Override
    public Future<DiscoveryEntry[]> lookup(Callback<DiscoveryEntry[]> callback,
                                               String[] domains,
                                               String interfaceName,
                                               DiscoveryQos discoveryQos) {
    	Future<DiscoveryEntry[]> result = null;
    	for (String domain : domains) {
        if (provisionedDiscoveryEntries.containsKey(domain + interfaceName)) {
            DiscoveryEntry discoveryEntry = provisionedDiscoveryEntries.get(domain + interfaceName);
            DiscoveryEntry[] discoveryEntries = new DiscoveryEntry[] {discoveryEntry};
            // prevent varargs from interpreting the array as mulitple arguments
            callback.resolve((Object) discoveryEntries);

            Future<DiscoveryEntry[]> discoveryEntryFuture = new Future<>();
            discoveryEntryFuture.resolve(Lists.newArrayList(discoveryEntry));
            result = discoveryEntryFuture;
        }
        }
    	if (result == null) {
            if (discoveryProxy == null) {
                throw new JoynrRuntimeException("LocalDiscoveryAggregator: discoveryProxy not set. Couldn't reach "
                        + "local capabilitites directory.");
            }
            result = discoveryProxy.lookup(callback, domains, interfaceName, discoveryQos);
        }
    	return result;
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
