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

import io.joynr.arbitration.Arbitrator;
import io.joynr.arbitration.ArbitratorFactory;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.dispatcher.RequestReplyDispatcher;
import io.joynr.dispatcher.RequestReplySender;
import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.exceptions.JoynrArbitrationException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.MessagingQos;
import io.joynr.pubsub.subscription.SubscriptionManager;

import java.util.UUID;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ProxyBuilderDefaultImpl<T extends JoynrInterface> implements ProxyBuilder<T> {
    private static final Logger logger = LoggerFactory.getLogger(ProxyBuilderDefaultImpl.class);

    private DiscoveryQos discoveryQos;
    // private ProxyQos proxyQos;
    private MessagingQos messagingQos;
    private Arbitrator arbitrator;
    private LocalCapabilitiesDirectory localCapabilitiesDirectory;
    private String domain;
    private RequestReplySender messageSender;
    private RequestReplyDispatcher dispatcher;
    private String proxyParticipantId;
    private boolean buildCalled;
    Class<T> myClass;
    private final String interfaceName;
    private DiscoveryAgent discoveryAgent;

    private SubscriptionManager subscriptionManager;

    public ProxyBuilderDefaultImpl(LocalCapabilitiesDirectory capabilitiesDirectory,
                                   String domain,
                                   Class<T> interfaceClass,
                                   RequestReplySender messageSender,
                                   RequestReplyDispatcher dispatcher,
                                   SubscriptionManager subscriptionManager) {
        this.subscriptionManager = subscriptionManager;
        try {
            interfaceName = (String) interfaceClass.getField("INTERFACE_NAME").get(String.class);
        } catch (Exception e) {
            logger.error("INTERFACE_NAME needs to be set in the interface class {}", interfaceClass);
            throw new IllegalStateException(e);
        }

        myClass = interfaceClass;
        this.proxyParticipantId = UUID.randomUUID().toString();

        this.localCapabilitiesDirectory = capabilitiesDirectory;
        this.domain = domain;
        this.messageSender = messageSender;
        this.dispatcher = dispatcher;
        this.discoveryAgent = new DiscoveryAgent();
        discoveryQos = new DiscoveryQos();
        arbitrator = ArbitratorFactory.create(domain, interfaceName, discoveryQos, localCapabilitiesDirectory);
        messagingQos = new MessagingQos();
        buildCalled = false;

    }

    // public ProxyBuilder(String domain,
    // Class<T> interfaceClass,
    // RequestReplySender messageSender,
    // RequestReplyDispatcher dispatcher) {
    // this(null, domain, interfaceClass, messageSender, dispatcher);
    // }

    /*
     * (non-Javadoc)
     * 
     * @see io.joynr.proxy.ProxyBuilder#getParticipantId()
     */
    @Override
    public String getParticipantId() {
        return proxyParticipantId;
    }

    /*
     * (non-Javadoc)
     * 
     * @see io.joynr.proxy.ProxyBuilder#setParticipantId(java.lang.String)
     */
    @Override
    public void setParticipantId(String participantId) {
        this.proxyParticipantId = participantId;
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * io.joynr.proxy.ProxyBuilder#setDiscoveryQos(io.joynr.arbitration.DiscoveryQos
     * )
     */
    @Override
    public ProxyBuilder<T> setDiscoveryQos(final DiscoveryQos discoveryQos) throws JoynrArbitrationException {
        this.discoveryQos = discoveryQos;
        // TODO which interfaceName should be used here?
        arbitrator = ArbitratorFactory.create(domain, interfaceName, discoveryQos, localCapabilitiesDirectory);
        arbitrator.startArbitration();

        return this;
    }

    // Proxy Qos not yet supported
    // /**
    // * @param proxyQos
    // * @return
    // */
    // public ProxyBuilder<T> setProxyQos(final ProxyQos proxyQos) {
    // this.proxyQos = proxyQos;
    // return this;
    // }

    /*
     * (non-Javadoc)
     * 
     * @see
     * io.joynr.proxy.ProxyBuilder#setMessagingQos(io.joynr.messaging.MessagingQos)
     */
    @Override
    public ProxyBuilder<T> setMessagingQos(final MessagingQos messagingQos) {
        this.messagingQos = messagingQos;
        return this;
    }

    /*
     * (non-Javadoc)
     * 
     * @see io.joynr.proxy.ProxyBuilder#build()
     */
    @Override
    public T build() throws JoynrArbitrationException, JoynrIllegalStateException {
        ProxyInvocationHandler proxyInvocationHandler = createProxyInvocationHandler();

        return ProxyFactory.createProxy(myClass, messagingQos, proxyInvocationHandler);
    }

    @Override
    public void build(final ProxyCreatedCallback<T> callback) {
        try {
            T proxy = build();
            callback.onProxyCreated(proxy);
        } catch (JoynrIllegalStateException e) {
            logger.error(e.toString());
            callback.onProxyCreationError(e.toString());
        }
    }

    // Method called by both synchronous and asynchronous build() to create a ProxyInvocationHandler 
    private ProxyInvocationHandler createProxyInvocationHandler() throws JoynrIllegalStateException {
        if (buildCalled) {
            throw new JoynrIllegalStateException("Proxy builder was already used to build a proxy. Please create a new proxy builder for each proxy.");
        }
        buildCalled = true;

        ProxyInvocationHandler ret = new ProxyInvocationHandler(domain,
                                                                interfaceName,
                                                                proxyParticipantId,
                                                                discoveryQos,
                                                                messagingQos,
                                                                messageSender,
                                                                dispatcher,
                                                                subscriptionManager);

        // This order is necessary because the Arbitrator might return early
        // But if the listener is set after the ProxyInvocationHandler the 
        // Arbitrator cannot return early
        discoveryAgent.setProxyInvocationHandler(ret);
        arbitrator.setArbitrationListener(this.discoveryAgent);

        return ret;
    }
}
