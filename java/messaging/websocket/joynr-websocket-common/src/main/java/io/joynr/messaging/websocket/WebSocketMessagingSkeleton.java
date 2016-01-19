package io.joynr.messaging.websocket;

import java.io.IOException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingSkeleton;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

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

import com.fasterxml.jackson.databind.ObjectMapper;

import io.joynr.messaging.routing.MessageRouter;
import joynr.JoynrMessage;

/**
 *
 */
public abstract class WebSocketMessagingSkeleton extends MessagingSocket implements IMessagingSkeleton {

    private static final Logger logger = LoggerFactory.getLogger(WebSocketMessagingSkeleton.class);
    private ExecutorService executorThreadPool = Executors.newCachedThreadPool();

    ObjectMapper objectMapper;
    private MessageRouter messageRouter;

    public WebSocketMessagingSkeleton(ObjectMapper objectMapper, MessageRouter messageRouter) {
        this.objectMapper = objectMapper;
        this.messageRouter = messageRouter;
    }

    @Override
    public void onWebSocketText(String json) {
        super.onWebSocketText(json);
        logger.debug("Received TEXT message: " + json);
        try {
            final JoynrMessage message = objectMapper.readValue(json, JoynrMessage.class);
            executorThreadPool.submit(new Runnable() {
                @Override
                public void run() {
                    try {
                        messageRouter.route(message);
                    } catch (IOException e) {
                        logger.error("Error: ", e);
                    }
                }
            });

        } catch (IOException e) {
            logger.error("Failed to parse websocket message", e);
        }
    }

    @Override
    public void transmit(JoynrMessage message, FailureAction failureAction) {
        try {
            messageRouter.route(message);
        } catch (Exception exception) {
            failureAction.execute(exception);
        }
    }

    @Override
    public void transmit(String serializedMessage, FailureAction failureAction) {
        // TODO Auto-generated method stub
    }
}
