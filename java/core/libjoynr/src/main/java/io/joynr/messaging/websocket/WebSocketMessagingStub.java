package io.joynr.messaging.websocket;

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

import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.messaging.IMessaging;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.WebSocketAddress;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;

public class WebSocketMessagingStub implements IMessaging {
    private static final Logger logger = LoggerFactory.getLogger(WebSocketMessagingStub.class);

    public WebSocketMessagingStub(WebSocketAddress address) {
        logger.debug("Not yet implemented: create WebSocketMessagingStub with address " + address);
    }

    @Override
    public void transmit(JoynrMessage message) throws IOException {
        logger.error("Not yet implemented:  WebSocketMessagingStub.transmit with message " + message);
        throw new JoynrMessageNotSentException("Not yet implemented:  WebSocketMessagingStub.transmit with message "
                + message);
    }
}
