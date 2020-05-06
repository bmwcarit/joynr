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
package io.joynr.messaging.channel;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.http.HttpMessageSender;
import joynr.ImmutableMessage;
import joynr.system.RoutingTypes.ChannelAddress;

public class ChannelMessagingStub implements IMessagingStub {
    private static final Logger logger = LoggerFactory.getLogger(ChannelMessagingStub.class);

    private ChannelAddress address;
    private HttpMessageSender httpMessageSender;

    public ChannelMessagingStub(ChannelAddress address, HttpMessageSender httpMessageSender) {
        this.address = address;
        this.httpMessageSender = httpMessageSender;
    }

    @Override
    public void transmit(ImmutableMessage message, SuccessAction successAction, FailureAction failureAction) {
        logger.debug(">>> OUTGOING >>> {}", message);
        httpMessageSender.sendMessage(address, message.getSerializedMessage(), successAction, failureAction);
    }
}
