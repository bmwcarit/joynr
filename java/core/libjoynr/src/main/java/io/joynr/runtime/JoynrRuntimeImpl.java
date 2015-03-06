package io.joynr.runtime;

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

import io.joynr.capabilities.CapabilitiesRegistrar;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.capabilities.RegistrationFuture;
import io.joynr.dispatcher.RequestReplyDispatcher;
import io.joynr.dispatcher.RequestReplySender;
import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.provider.JoynrProvider;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderDefaultImpl;
import io.joynr.pubsub.subscription.SubscriptionManager;
import io.joynr.proxy.ProxyInvocationHandlerFactory;
import io.joynr.subtypes.JoynrType;

import java.util.Set;

import joynr.BroadcastSubscriptionRequest;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;

import org.reflections.Reflections;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;

public class JoynrRuntimeImpl implements JoynrRuntime {
    private static final Logger logger = LoggerFactory.getLogger(JoynrRuntimeImpl.class);

    @Inject
    private CapabilitiesRegistrar capabilitiesRegistrar;
    @Inject
    private LocalCapabilitiesDirectory localCapabilitiesDirectory;
    @Inject
    private RequestReplySender messageSender;
    @Inject
    private RequestReplyDispatcher dispatcher;
    @Inject
    private SubscriptionManager subscriptionManager;
    @Inject
    public ObjectMapper objectMapper;
    @Inject
    private ProxyInvocationHandlerFactory proxyInvocationHandlerFactory;

    @Inject
    public JoynrRuntimeImpl(ObjectMapper objectMapper) {
        Reflections reflections = new Reflections("joynr");
        Set<Class<? extends JoynrType>> subClasses = reflections.getSubTypesOf(JoynrType.class);
        objectMapper.registerSubtypes(subClasses.toArray(new Class<?>[subClasses.size()]));

        Class<?>[] messageTypes = new Class[]{ Request.class, Reply.class, SubscriptionRequest.class,
                SubscriptionStop.class, SubscriptionPublication.class, BroadcastSubscriptionRequest.class };
        objectMapper.registerSubtypes(messageTypes);

    }

    @Override
    public <T extends JoynrInterface> ProxyBuilder<T> getProxyBuilder(final String domain, final Class<T> interfaceClass) {

        if (domain == null || domain.isEmpty()) {
            throw new IllegalArgumentException("Cannot create ProxyBuilder: domain was not set");

        }

        if (interfaceClass == null) {
            throw new IllegalArgumentException("Cannot create ProxyBuilder: interfaceClass may not be NULL");
        }

        return new ProxyBuilderDefaultImpl<T>(localCapabilitiesDirectory,
                                              domain,
                                              interfaceClass,
                                              proxyInvocationHandlerFactory);
    }

    @Override
    public <T extends JoynrInterface> RegistrationFuture registerCapability(String domain,
                                                                            JoynrProvider provider,
                                                                            Class<T> providedInterface,
                                                                            String authenticationToken) {
        return capabilitiesRegistrar.registerCapability(domain, provider, providedInterface, authenticationToken);
    }

    @Override
    public <T extends JoynrInterface> void unregisterCapability(String domain,
                                                                JoynrProvider provider,
                                                                Class<T> providedInterface,
                                                                String authenticationToken) {
        capabilitiesRegistrar.unregisterCapability(domain, provider, providedInterface, authenticationToken);

    }

    @Override
    public void shutdown(boolean clear) {
        logger.info("SHUTTING DOWN runtime");
        try {
            capabilitiesRegistrar.shutdown(clear);
        } catch (Exception e) {
            logger.error("error clearing capabiltities while shutting down: {}", e.getMessage());
        }

        try {
            // TODO The channel is deleted but not deregistered from the Channel Url Directory
            dispatcher.shutdown(clear);
        } catch (Exception e) {
            logger.error("error shutting down dispatcher: {}", e.getMessage());
        }

        try {
            messageSender.shutdown();
        } catch (Exception e) {
            logger.error("error shutting down message sender: {}", e.getMessage());
        }
    }
}
