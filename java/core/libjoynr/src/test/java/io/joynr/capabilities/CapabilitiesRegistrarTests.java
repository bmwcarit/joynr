package io.joynr.capabilities;

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

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.ProviderDirectory;
import io.joynr.dispatching.RequestCaller;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessLibjoynrMessagingSkeleton;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.ProviderContainer;
import io.joynr.provider.RequestCallerFactory;
import io.joynr.proxy.Callback;
import joynr.types.CommunicationMiddleware;
import joynr.types.DiscoveryEntry;
import joynr.types.ProviderQos;

@RunWith(MockitoJUnitRunner.class)
public class CapabilitiesRegistrarTests {

    CapabilitiesRegistrar registrar;
    @Mock
    private LocalDiscoveryAggregator localDiscoveryAggregator;
    @Mock
    private RequestCallerFactory requestCallerFactory;
    @Mock
    private ProviderDirectory providerDirectory;

    @Mock
    private MessageRouter messageRouter;

    @Mock
    private Dispatcher dispatcher;

    @Mock
    private JoynrProvider provider;

    @Mock
    private RequestCaller requestCaller;

    @Mock
    private ParticipantIdStorage participantIdStorage;

    private String domain = "domain";
    private String participantId = "participantId";
    private ProviderQos providerQos = new ProviderQos();

    private static String interfaceName = "interfaceName";

    private interface ProvidedInterface extends JoynrInterface {
        @SuppressWarnings("unused")
        // this Field is read with a getField method, so it is indeed needed.
        public static String INTERFACE_NAME = interfaceName;
    }

    interface TestInterface extends JoynrInterface {
        public static String INTERFACE_NAME = interfaceName;
    }

    @Before
    public void setUp() {

        registrar = new CapabilitiesRegistrarImpl(localDiscoveryAggregator,
                                                  requestCallerFactory,
                                                  messageRouter,
                                                  providerDirectory,
                                                  participantIdStorage,
                                                  new InProcessAddress(new InProcessLibjoynrMessagingSkeleton(dispatcher)));
    }

    @Test
    public void registerWithCapRegistrar() {

        when(provider.getProviderQos()).thenReturn(providerQos);
        doReturn(ProvidedInterface.class).when(provider).getProvidedInterface();
        when(provider.getInterfaceName()).thenReturn(TestInterface.INTERFACE_NAME);
        when(participantIdStorage.getProviderParticipantId(eq(domain), eq(ProvidedInterface.class))).thenReturn(participantId);
        when(requestCallerFactory.create(provider)).thenReturn(requestCaller);

        registrar.registerProvider(domain, provider, providerQos);
        verify(localDiscoveryAggregator).add(any(Callback.class),
                                             eq(new DiscoveryEntry(domain,
                                                                   TestInterface.INTERFACE_NAME,
                                                                   participantId,
                                                                   providerQos,
                                                                   new CommunicationMiddleware[]{ CommunicationMiddleware.JOYNR })));

        verify(requestCallerFactory).create(provider);

        verify(providerDirectory).add(participantId, eq(new ProviderContainer(requestCaller)));
    }

    @Test
    public void unregisterProvider() {
        when(provider.getProviderQos()).thenReturn(providerQos);
        doReturn(ProvidedInterface.class).when(provider).getProvidedInterface();
        when(provider.getInterfaceName()).thenReturn(TestInterface.INTERFACE_NAME);
        when(participantIdStorage.getProviderParticipantId(eq(domain), eq(ProvidedInterface.class))).thenReturn(participantId);
        registrar.unregisterProvider(domain, provider);

        verify(localDiscoveryAggregator).remove(any(Callback.class), eq(participantId));
        verify(providerDirectory).remove(eq(participantId));
    }

}
