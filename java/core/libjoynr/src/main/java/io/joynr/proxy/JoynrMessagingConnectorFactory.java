package io.joynr.proxy;

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

import java.lang.reflect.Method;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.JsonMappingException;
import com.google.inject.Inject;

import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import joynr.MethodMetaInformation;
import joynr.types.DiscoveryEntryWithMetaInfo;

/**
 * This class creates a connector to access a service over JoynRPC using Java dynamic proxies.
 */
public class JoynrMessagingConnectorFactory {

    private static final Logger logger = LoggerFactory.getLogger(JoynrMessagingConnectorFactory.class);

    // use for caching because creation of MethodMetaInformation is expensive
    private static final ConcurrentMap<Method, MethodMetaInformation> metaInformationMap = new ConcurrentHashMap<Method, MethodMetaInformation>();

    private RequestReplyManager requestReplyManager;
    private SubscriptionManager subscriptionManager;

    private ReplyCallerDirectory replyCallerDirectory;

    @Inject
    public JoynrMessagingConnectorFactory(RequestReplyManager requestReplyManager,
                                          ReplyCallerDirectory replyCallerDirectory,
                                          SubscriptionManager subscriptionManager) {
        this.requestReplyManager = requestReplyManager;
        this.replyCallerDirectory = replyCallerDirectory;
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
                                                           final Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries,
                                                           final MessagingQos qosSettings) {

        return new JoynrMessagingConnectorInvocationHandler(toDiscoveryEntries,
                                                            fromParticipantId,
                                                            qosSettings,
                                                            requestReplyManager,
                                                            replyCallerDirectory,
                                                            subscriptionManager);
    }

    public static MethodMetaInformation ensureMethodMetaInformationPresent(Method method) {
        if (metaInformationMap.containsKey(method)) {
            return metaInformationMap.get(method);
        }

        MethodMetaInformation metaInformation;
        try {
            metaInformation = new MethodMetaInformation(method);
        } catch (JsonMappingException e) {
            throw new JoynrRuntimeException(e);
        }
        MethodMetaInformation existingMetaInformation = metaInformationMap.putIfAbsent(method, metaInformation);
        if (existingMetaInformation != null) {
            // we only use putIfAbsent instead of .put, because putIfAbsent is threadsafe
            logger.debug("There was already a metaInformation object for that method in the map.");
        }
        return metaInformation;
    }
}
