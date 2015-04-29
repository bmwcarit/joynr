package io.joynr.capabilities;

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

import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.Mockito.doReturn;
import io.joynr.dispatcher.RequestCaller;
import io.joynr.dispatcher.RequestReplyDispatcher;
import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.RequestCallerFactory;
import io.joynr.pubsub.publication.PublicationManager;
import joynr.types.ProviderQos;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class CapabilitiesRegistrarTests {

    CapabilitiesRegistrar registrar;
    @Mock
    private LocalCapabilitiesDirectory localCapabilitiesDirectory;
    @Mock
    private RequestCallerFactory requestCallerFactory;
    @Mock
    private RequestReplyDispatcher dispatcher;
    @Mock
    private JoynrProvider provider;

    @Mock
    private RequestCaller requestCaller;

    @Mock
    private PublicationManager publicationManager;

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

        registrar = new CapabilitiesRegistrarImpl(localCapabilitiesDirectory,
                                                  requestCallerFactory,
                                                  dispatcher,
                                                  publicationManager,
                                                  participantIdStorage);
    }

    @Test
    public void registerWithCapRegistrar() {

        when(provider.getProviderQos()).thenReturn(providerQos);
        doReturn(ProvidedInterface.class).when(provider).getProvidedInterface();
        when(participantIdStorage.getProviderParticipantId(eq(domain), eq(ProvidedInterface.class), anyString())).thenReturn(participantId);
        when(requestCallerFactory.create(provider)).thenReturn(requestCaller);

        registrar.registerCapability(domain, provider, "registerWithCapRegistrar");
        verify(localCapabilitiesDirectory).add(eq(new CapabilityEntryImpl(domain,
                                                                          TestInterface.INTERFACE_NAME,
                                                                          providerQos,
                                                                          participantId,
                                                                          System.currentTimeMillis())));
        verify(requestCallerFactory).create(provider);

        verify(dispatcher).addRequestCaller(participantId, requestCaller);
    }

    @Test
    public void unregisterCapability() {
        when(provider.getProviderQos()).thenReturn(providerQos);
        when(participantIdStorage.getProviderParticipantId(eq(domain), eq(ProvidedInterface.class), anyString())).thenReturn(participantId);
        registrar.unregisterCapability(domain, provider, "unregisterWithRegistrar");

        verify(localCapabilitiesDirectory).remove(eq(new CapabilityEntryImpl(domain,
                                                                             TestInterface.INTERFACE_NAME,
                                                                             providerQos,
                                                                             participantId,
                                                                             System.currentTimeMillis())));
        verify(dispatcher).removeRequestCaller(eq(participantId));
    }

}
