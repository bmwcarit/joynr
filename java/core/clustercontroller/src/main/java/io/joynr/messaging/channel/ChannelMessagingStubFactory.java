package io.joynr.messaging.channel;

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
import com.google.inject.Inject;

import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.http.HttpMessageSender;
import joynr.system.RoutingTypes.ChannelAddress;

/**
 *  Message stub factory for joynr channel addresses
 */
public class ChannelMessagingStubFactory extends
        AbstractMiddlewareMessagingStubFactory<ChannelMessagingStub, ChannelAddress> {
    private HttpMessageSender httpMessageSender;

    @Inject
    public ChannelMessagingStubFactory(HttpMessageSender messageSender) {
        this.httpMessageSender = messageSender;
    }

    @Override
    protected ChannelMessagingStub createInternal(final ChannelAddress address) {
        ChannelMessagingStub messagingStub = new ChannelMessagingStub(address, httpMessageSender);
        return messagingStub;
    }
}
