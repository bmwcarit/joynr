/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.messaging.routing;

import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ScheduledExecutorService;

import javax.inject.Inject;
import javax.inject.Named;
import javax.inject.Singleton;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.accesscontrol.AccessController;
import io.joynr.accesscontrol.HasConsumerPermissionCallback;
import io.joynr.exceptions.JoynrMessageExpiredException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.runtime.ClusterControllerRuntimeModule;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.util.ObjectMapper;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.MutableMessage;
import joynr.Reply;
import joynr.Request;

public class CcMessageRouter extends AbstractMessageRouter {
    private static final Logger logger = LoggerFactory.getLogger(CcMessageRouter.class);
    private AccessController accessController;
    private boolean enableAccessControl;
    private ObjectMapper objectMapper;

    @Inject
    @Singleton
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 8 LINES
    public CcMessageRouter(RoutingTable routingTable,
                           @Named(SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                           @Named(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS) int maxParallelSends,
                           @Named(ConfigurableMessagingSettings.PROPERTY_ROUTING_TABLE_CLEANUP_INTERVAL_MS) long routingTableCleanupIntervalMs,
                           MessagingStubFactory messagingStubFactory,
                           MessagingSkeletonFactory messagingSkeletonFactory,
                           AddressManager addressManager,
                           MulticastReceiverRegistry multicastReceiverRegistry,
                           AccessController accessController,
                           @Named(ClusterControllerRuntimeModule.PROPERTY_ACCESSCONTROL_ENABLE) boolean enableAccessControl,
                           MessageQueue messageQueue,
                           ShutdownNotifier shutdownNotifier,
                           ObjectMapper objectMapper) {
        super(routingTable,
              scheduler,
              maxParallelSends,
              routingTableCleanupIntervalMs,
              messagingStubFactory,
              messagingSkeletonFactory,
              addressManager,
              multicastReceiverRegistry,
              messageQueue,
              shutdownNotifier);

        this.accessController = accessController;
        this.enableAccessControl = enableAccessControl;
        this.objectMapper = objectMapper;
    }

    @Override
    public void routeIn(ImmutableMessage message) {
        route(message);
    }

    @Override
    public void routeOut(ImmutableMessage message) {
        route(message);
    }

    @Override
    protected void route(final ImmutableMessage message) {
        if (enableAccessControl) {
            accessController.hasConsumerPermission(message, new HasConsumerPermissionCallback() {
                @Override
                public void hasConsumerPermission(boolean hasPermission) {
                    if (hasPermission) {
                        try {
                            CcMessageRouter.super.route(message);
                        } catch (JoynrMessageExpiredException e) {
                            logger.warn("Problem processing message. Message {} is dropped: {}",
                                        message.getId(),
                                        e.getMessage());
                            CcMessageRouter.super.finalizeMessageProcessing(message, false);
                        } catch (Exception e) {
                            logger.error("Error processing message. Message {} is dropped: {}",
                                         message.getId(),
                                         e.getMessage());
                            CcMessageRouter.super.finalizeMessageProcessing(message, false);
                        }
                    } else {
                        logger.warn("Dropping message {} from {} to {} because of insufficient access rights",
                                    message.getId(),
                                    message.getSender(),
                                    message.getRecipient());
                        CcMessageRouter.super.finalizeMessageProcessing(message, false);
                    }
                }
            });
        } else {
            super.route(message);
        }
    }

    @Override
    protected ImmutableMessage createReplyMessageWithError(ImmutableMessage requestMessage,
                                                           JoynrRuntimeException error) {
        try {
            String deserializedPayload = new String(requestMessage.getUnencryptedBody(), StandardCharsets.UTF_8);
            final Request request = objectMapper.readValue(deserializedPayload, Request.class);
            String requestReplyId = request.getRequestReplyId();

            MutableMessage replyMessage = new MutableMessage();
            replyMessage.setType(Message.MessageType.VALUE_MESSAGE_TYPE_REPLY);
            if (requestMessage.getEffort() != null) {
                replyMessage.setEffort(requestMessage.getEffort());
            }
            replyMessage.setSender(requestMessage.getRecipient());
            replyMessage.setRecipient(requestMessage.getSender());
            replyMessage.setTtlAbsolute(true);
            replyMessage.setTtlMs(requestMessage.getTtlMs());
            Reply reply = new Reply(requestReplyId, error);
            String serializedPayload = objectMapper.writeValueAsString(reply);
            replyMessage.setPayload(serializedPayload.getBytes(StandardCharsets.UTF_8));
            Map<String, String> customHeaders = new HashMap<>();
            customHeaders.put(Message.CUSTOM_HEADER_REQUEST_REPLY_ID, requestReplyId);
            replyMessage.setCustomHeaders(customHeaders);
            replyMessage.setCompressed(requestMessage.isCompressed());
            return replyMessage.getImmutableMessage();
        } catch (Exception e) {
            logger.error("Failed to prepare ReplyMessageWithError for msgId: {}. from: {} to: {}. Reason: {}",
                         requestMessage.getId(),
                         requestMessage.getSender(),
                         requestMessage.getRecipient(),
                         e.getMessage());
            return null;
        }
    }

}
