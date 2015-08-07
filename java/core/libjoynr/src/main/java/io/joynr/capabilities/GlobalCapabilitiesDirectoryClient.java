package io.joynr.capabilities;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.Callback;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderDefaultImpl;
import io.joynr.proxy.ProxyInvocationHandlerFactory;

import java.util.List;

import joynr.infrastructure.GlobalCapabilitiesDirectoryProxy;
import joynr.types.CapabilityInformation;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

public class GlobalCapabilitiesDirectoryClient {

    // TODO define a proper max messaging ttl
    private static final long TTL_30_DAYS_IN_MS = 30L * 24L * 60L * 60L * 1000L;
    private ProxyBuilder<GlobalCapabilitiesDirectoryProxy> capabilitiesProxyBuilder;
    private String domain;
    private LocalCapabilitiesDirectory capabilitiesDirectory;
    private ProxyInvocationHandlerFactory proxyInvocationHandlerFactory;

    public GlobalCapabilitiesDirectoryClient(String domain,
                                             LocalCapabilitiesDirectory capabilitiesDirectory,
                                             ProxyInvocationHandlerFactory proxyInvocationHandlerFactory) {
        this.domain = domain;
        this.capabilitiesDirectory = capabilitiesDirectory;
        this.proxyInvocationHandlerFactory = proxyInvocationHandlerFactory;
    }

    private GlobalCapabilitiesDirectoryProxy getProxy(long ttl) {
        this.capabilitiesProxyBuilder = new ProxyBuilderDefaultImpl<GlobalCapabilitiesDirectoryProxy>(capabilitiesDirectory,
                                                                                                      domain,
                                                                                                      GlobalCapabilitiesDirectoryProxy.class,
                                                                                                      proxyInvocationHandlerFactory);
        DiscoveryQos discoveryQos = new DiscoveryQos(DiscoveryScope.GLOBAL_ONLY, DiscoveryQos.NO_MAX_AGE);
        MessagingQos messagingQos = new MessagingQos(ttl);
        return capabilitiesProxyBuilder.setDiscoveryQos(discoveryQos).setMessagingQos(messagingQos).build();
    }

    public void add(Callback<Void> callback, CapabilityInformation capabilityInformation) {
        getProxy(TTL_30_DAYS_IN_MS).add(callback, capabilityInformation);
    }

    public void remove(Callback<Void> callback, String participantId) {
        getProxy(TTL_30_DAYS_IN_MS).remove(callback, participantId);

    }

    public void remove(List<String> newArrayList) {
        getProxy(TTL_30_DAYS_IN_MS).remove(newArrayList);
    }

    public void lookup(Callback<CapabilityInformation> callback, String participantId, long timeout) {
        getProxy(timeout).lookup(callback, participantId);
    }

    public void lookup(Callback<List<CapabilityInformation>> callback, String domain, String interfaceName, long timeout) {
        getProxy(timeout).lookup(callback, domain, interfaceName);
    }
}
