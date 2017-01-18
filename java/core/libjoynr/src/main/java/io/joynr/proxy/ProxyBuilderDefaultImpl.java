package io.joynr.proxy;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import java.util.Set;
import java.util.UUID;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.arbitration.ArbitrationCallback;
import io.joynr.arbitration.ArbitrationResult;
import io.joynr.arbitration.Arbitrator;
import io.joynr.arbitration.ArbitratorFactory;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.util.VersionUtil;
import joynr.system.DiscoveryAsync;
import joynr.system.RoutingTypes.Address;
import joynr.types.Version;

public class ProxyBuilderDefaultImpl<T> implements ProxyBuilder<T> {
    private static final Logger logger = LoggerFactory.getLogger(ProxyBuilderDefaultImpl.class);

    private DiscoveryQos discoveryQos;
    MessagingQos messagingQos;
    private Arbitrator arbitrator;
    private DiscoveryAsync localDiscoveryAggregator;
    private Set<String> domains;
    private String proxyParticipantId;
    private boolean buildCalled;
    Class<T> myClass;
    private final String interfaceName;
    private Version interfaceVersion;
    private ProxyInvocationHandlerFactory proxyInvocationHandlerFactory;

    private MessageRouter messageRouter;
    private Address libjoynrMessagingAddress;
    private long maxMessagingTtl;

    private T proxy;

    ProxyBuilderDefaultImpl(DiscoveryAsync localDiscoveryAggregator,
                            Set<String> domains,
                            Class<T> interfaceClass,
                            ProxyInvocationHandlerFactory proxyInvocationHandlerFactory,
                            MessageRouter messageRouter,
                            long maxMessagingTtl,
                            Address libjoynrMessagingAddress) {
        this.proxyInvocationHandlerFactory = proxyInvocationHandlerFactory;
        this.messageRouter = messageRouter;
        this.maxMessagingTtl = maxMessagingTtl;
        this.libjoynrMessagingAddress = libjoynrMessagingAddress;
        try {
            interfaceName = (String) interfaceClass.getField("INTERFACE_NAME").get(String.class);
        } catch (Exception e) {
            logger.error("INTERFACE_NAME needs to be set in the interface class {}", interfaceClass);
            throw new IllegalStateException(e);
        }
        interfaceVersion = VersionUtil.getVersionFromAnnotation(interfaceClass);

        myClass = interfaceClass;
        this.proxyParticipantId = UUID.randomUUID().toString();

        this.localDiscoveryAggregator = localDiscoveryAggregator;
        this.domains = domains;
        discoveryQos = new DiscoveryQos();
        arbitrator = ArbitratorFactory.create(domains,
                                              interfaceName,
                                              interfaceVersion,
                                              discoveryQos,
                                              localDiscoveryAggregator);
        messagingQos = new MessagingQos();
        buildCalled = false;

    }

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
    public ProxyBuilder<T> setDiscoveryQos(final DiscoveryQos discoveryQos) throws DiscoveryException {
        this.discoveryQos = discoveryQos;
        // TODO which interfaceName should be used here?
        arbitrator = ArbitratorFactory.create(domains,
                                              interfaceName,
                                              interfaceVersion,
                                              discoveryQos,
                                              localDiscoveryAggregator);

        return this;
    }

    /*
     * (non-Javadoc)
     *
     * @see
     * io.joynr.proxy.ProxyBuilder#setMessagingQos(io.joynr.messaging.MessagingQos)
     */
    @Override
    public ProxyBuilder<T> setMessagingQos(final MessagingQos messagingQos) {
        if (messagingQos.getRoundTripTtl_ms() > maxMessagingTtl) {
            logger.warn("Error in MessageQos. domains: {} interface: {} Max allowed ttl: {}. Passed ttl: {}",
                        new Object[]{ domains, interfaceName, maxMessagingTtl, messagingQos.getRoundTripTtl_ms() });
            messagingQos.setTtl_ms(maxMessagingTtl);
        }

        this.messagingQos = messagingQos;
        return this;
    }

    /*
     * (non-Javadoc)
     *
     * @see io.joynr.proxy.ProxyBuilder#build()
     */
    @Override
    public T build() {
        return build(new ProxyCreatedCallback<T>() {

            @Override
            public void onProxyCreationFinished(T result) {
                logger.trace("proxy created: interface: {} domains: {}", interfaceName, domains);
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                logger.error("error creating proxy: interface: {} domains: {}, error", new Object[]{ interfaceName,
                        domains, error.getMessage() });
            }
        });
    }

    @Override
    public T build(final ProxyCreatedCallback<T> callback) {
        try {
            ProxyInvocationHandler proxyInvocationHandler = createProxyInvocationHandler(callback);
            arbitrator.startArbitration();
            proxy = ProxyFactory.createProxy(myClass, messagingQos, proxyInvocationHandler);
            return proxy;
        } catch (JoynrRuntimeException e) {
            logger.debug("error building proxy", e);
            callback.onProxyCreationError(e);
            throw e;
        }
    }

    // Method called by both synchronous and asynchronous build() to create a ProxyInvocationHandler
    private ProxyInvocationHandler createProxyInvocationHandler(final ProxyCreatedCallback<T> callback) {
        if (buildCalled) {
            throw new JoynrIllegalStateException("Proxy builder was already used to build a proxy. Please create a new proxy builder for each proxy.");
        }
        buildCalled = true;

        final ProxyInvocationHandler proxyInvocationHandler = proxyInvocationHandlerFactory.create(domains,
                                                                                                   interfaceName,
                                                                                                   proxyParticipantId,
                                                                                                   discoveryQos,
                                                                                                   messagingQos);

        // This order is necessary because the Arbitrator might return early
        // But if the listener is set after the ProxyInvocationHandler the
        // Arbitrator cannot return early
        arbitrator.setArbitrationListener(new ArbitrationCallback() {
            @Override
            public void onSuccess(ArbitrationResult arbitrationResult) {
                proxyInvocationHandler.createConnector(arbitrationResult);
                messageRouter.addNextHop(getParticipantId(), libjoynrMessagingAddress);
                callback.onProxyCreationFinished(proxy);
            }

            @Override
            public void onError(Throwable throwable) {
                JoynrRuntimeException reason;
                if (throwable instanceof JoynrRuntimeException) {
                    reason = (JoynrRuntimeException) throwable;
                } else {
                    reason = new JoynrRuntimeException(throwable);
                }
                proxyInvocationHandler.abort(reason);
                callback.onProxyCreationError(reason);
            }
        });

        return proxyInvocationHandler;
    }
}
