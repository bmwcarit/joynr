package io.joynr.proxy;

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

import io.joynr.dispatching.RequestReplyDispatcher;
import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.messaging.MessagingQos;

import java.lang.reflect.Method;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

import joynr.MethodMetaInformation;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.JsonMappingException;
import com.google.inject.Inject;

/**
 * This class creates a connector to access a service over JoynRPC using Java dynamic proxies.
 */
public class JoynrMessagingConnectorFactory {

    private static final Logger logger = LoggerFactory.getLogger(JoynrMessagingConnectorFactory.class);

    // use for caching because creation of MethodMetaInformation is expensive
    private static final ConcurrentMap<Method, MethodMetaInformation> metaInformationMap = new ConcurrentHashMap<Method, MethodMetaInformation>();

    private RequestReplyManager requestReplyManager;
    private RequestReplyDispatcher dispatcher;
    private SubscriptionManager subscriptionManager;

    @Inject
    public JoynrMessagingConnectorFactory(RequestReplyManager requestReplyManager,
                                          RequestReplyDispatcher dispatcher,
                                          SubscriptionManager subscriptionManager) {
        this.requestReplyManager = requestReplyManager;
        this.dispatcher = dispatcher;
        this.subscriptionManager = subscriptionManager;
    }

    /**
     * Creates a connector (java reflection dynamic proxy object) to execute remote procedure calls. Internally uses
     * JoynMessaging to transmit calls.
     * 
     * @param fromParticipantId
     *            Participant Id of the created stub.
     * @param toParticipantId
     *            Participant of the Provider/Receiver.
     * @param qosSettings
     *            MessagingQos settings
     * @return connector to execute remote procedure calls
     */
    public JoynrMessagingConnectorInvocationHandler create(final String fromParticipantId,
                                                           final String toParticipantId,
                                                           final MessagingQos qosSettings) {

        return new JoynrMessagingConnectorInvocationHandler(toParticipantId,
                                                            fromParticipantId,
                                                            qosSettings,
                                                            requestReplyManager,
                                                            dispatcher,
                                                            subscriptionManager);
    }

    public static MethodMetaInformation ensureMethodMetaInformationPresent(Method method) throws JsonMappingException {
        if (metaInformationMap.containsKey(method)) {
            return metaInformationMap.get(method);
        }

        MethodMetaInformation metaInformation = new MethodMetaInformation(method);
        MethodMetaInformation existingMetaInformation = metaInformationMap.putIfAbsent(method, metaInformation);
        if (existingMetaInformation != null) {
            // we only use putIfAbsent instead of .put, because putIfAbsent is threadsafe
            logger.debug("There was already a metaInformation object for that method in the map.");
        }
        return metaInformation;
    }
}
