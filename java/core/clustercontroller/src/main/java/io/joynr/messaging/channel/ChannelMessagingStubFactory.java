package io.joynr.messaging.channel;

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
import java.util.Set;
import com.google.inject.Inject;

import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.JoynrMessageSerializer;
import io.joynr.messaging.http.HttpGlobalAddressFactory;
import io.joynr.messaging.http.HttpMessageSender;
import io.joynr.messaging.routing.GlobalAddressFactory;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;

/**
 *  Message stub factory for joynr channel addresses
 */
public class ChannelMessagingStubFactory extends
        AbstractMiddlewareMessagingStubFactory<ChannelMessagingStub, ChannelAddress> {
    private HttpMessageSender httpMessageSender;
    private ChannelMessageSerializerFactory channelMessageSerializerFactory;
    private HttpGlobalAddressFactory channelAddressFactory;

    @Inject
    public ChannelMessagingStubFactory(ChannelMessageSerializerFactory channelMessageSerializerFactory,
                                       HttpMessageSender messageSender,
                                       Set<GlobalAddressFactory<? extends Address>> addressFactories) {
        this.channelMessageSerializerFactory = channelMessageSerializerFactory;
        this.httpMessageSender = messageSender;

        for (GlobalAddressFactory<? extends Address> addressFactory : addressFactories) {
            if (addressFactory instanceof HttpGlobalAddressFactory) {
                this.channelAddressFactory = (HttpGlobalAddressFactory) addressFactory;
            }
        }
        if (channelAddressFactory == null) {
            throw new IllegalStateException("A http global address factiory must be registered if using channel messaging via HTTP");
        }
    }

    @Override
    protected ChannelMessagingStub createInternal(final ChannelAddress address) {
        final JoynrMessageSerializer messageSerializer = channelMessageSerializerFactory.create(address);
        ChannelAddress replyToAddress;
        replyToAddress = channelAddressFactory.create();

        ChannelMessagingStub messagingStub = new ChannelMessagingStub(address,
                                                                      replyToAddress,
                                                                      messageSerializer,
                                                                      httpMessageSender);
        return messagingStub;
    }

    @Override
    public void shutdown() {
        httpMessageSender.shutdown();
    }
}
