package io.joynr.messaging;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.proxy.Callback;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderFactory;
import joynr.infrastructure.ChannelUrlDirectoryProxy;
import joynr.types.ChannelUrlInformation;

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

public class GlobalChannelUrlDirectoryClient {

    // TODO define a proper max messaging ttl
    private static final long TTL_30_DAYS_IN_MS = 30L * 24L * 60L * 60L * 1000L;
    private ProxyBuilder<ChannelUrlDirectoryProxy> channelUrlProxyBuilder;
    private String domain;
    private ProxyBuilderFactory proxyBuilderFactory;

    public GlobalChannelUrlDirectoryClient(ProxyBuilderFactory proxyBuilderFactory, String domain) {
        this.proxyBuilderFactory = proxyBuilderFactory;
        this.domain = domain;
    }

    private ChannelUrlDirectoryProxy getProxy(long ttl) {
        this.channelUrlProxyBuilder = proxyBuilderFactory.get(domain, ChannelUrlDirectoryProxy.class);
        DiscoveryQos discoveryQos = new DiscoveryQos(DiscoveryScope.GLOBAL_ONLY, DiscoveryQos.NO_MAX_AGE);
        MessagingQos messagingQos = new MessagingQos(ttl);
        return channelUrlProxyBuilder.setDiscoveryQos(discoveryQos).setMessagingQos(messagingQos).build();
    }

    public void registerChannelUrls(Callback<Void> callback,
                                    String channelId,
                                    ChannelUrlInformation channelUrlInformation) {
        getProxy(TTL_30_DAYS_IN_MS).registerChannelUrls(callback, channelId, channelUrlInformation);
    }

    public void unregisterChannelUrls(String channelId) {
        getProxy(TTL_30_DAYS_IN_MS).unregisterChannelUrls(channelId);
    }

    public ChannelUrlInformation getUrlsForChannel(String channelId) {
        return getProxy(TTL_30_DAYS_IN_MS).getUrlsForChannel(channelId);
    }
}
