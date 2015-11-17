package io.joynr.dispatching.subscription;

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
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;
import io.joynr.pubsub.subscription.BroadcastSubscriptionListener;

import java.io.IOException;

import javax.annotation.CheckForNull;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;

public interface SubscriptionManager {

    void registerAttributeSubscription(String fromParticipantId,
                                       String toParticipantId,
                                       AttributeSubscribeInvocation subscriptionRequest)
                                                                                        throws JoynrSendBufferFullException,
                                                                                        JoynrMessageNotSentException,
                                                                                        JsonGenerationException,
                                                                                        JsonMappingException,
                                                                                        IOException;

    void registerBroadcastSubscription(String fromParticipantId,
                                       String toParticipantId,
                                       BroadcastSubscribeInvocation subscriptionRequest)
                                                                                        throws JoynrSendBufferFullException,
                                                                                        JoynrMessageNotSentException,
                                                                                        JsonGenerationException,
                                                                                        JsonMappingException,
                                                                                        IOException;

    void unregisterSubscription(String fromParticipantId,
                                String toParticipantId,
                                String subscriptionId,
                                MessagingQos qosSettings) throws JoynrSendBufferFullException,
                                                         JoynrMessageNotSentException, JsonGenerationException,
                                                         JsonMappingException, IOException;

    void touchSubscriptionState(final String subscriptionId);

    Class<?> getAttributeType(String subscriptionId);

    Class<?>[] getBroadcastOutParameterTypes(String subscriptionId);

    boolean isBroadcast(String subscriptionId);

    BroadcastSubscriptionListener getBroadcastSubscriptionListener(String subscriptionId);

    @CheckForNull
    <T> AttributeSubscriptionListener<T> getSubscriptionListener(String subscriptionId);

    void handleBroadcastPublication(String subscriptionId, Object[] broadcastValues);

    <T> void handleAttributePublication(String subscriptionId, T attributeValue);

    <T> void handleAttributePublicationError(String subscriptionId, JoynrRuntimeException error);

}
