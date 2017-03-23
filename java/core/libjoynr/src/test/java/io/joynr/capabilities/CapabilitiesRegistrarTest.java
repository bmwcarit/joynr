package io.joynr.capabilities;

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

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.JoynrVersion;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.ProviderDirectory;
import io.joynr.dispatching.RequestCaller;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessLibjoynrMessagingSkeleton;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.provider.AbstractSubscriptionPublisher;
import io.joynr.provider.JoynrInterface;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.ProviderContainer;
import io.joynr.provider.ProviderContainerFactory;
import io.joynr.proxy.Callback;
import joynr.types.DiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

@RunWith(MockitoJUnitRunner.class)
public class CapabilitiesRegistrarTest {

    private static final long ONE_DAY_IN_MS = 1 * 24 * 60 * 60 * 1000;
    private final long expiryDateMs = System.currentTimeMillis() + ONE_DAY_IN_MS;
    CapabilitiesRegistrar registrar;
    @Mock
    private LocalDiscoveryAggregator localDiscoveryAggregator;
    @Mock
    private ProviderDirectory providerDirectory;
    @Mock
    private ProviderContainerFactory providerContainerFactory;

    @Mock
    private MessageRouter messageRouter;

    @Mock
    private Dispatcher dispatcher;

    @Mock
    private TestProvider testProvider;

    @Mock
    private RequestCaller requestCaller;

    @Mock
    private AbstractSubscriptionPublisher subscriptionPublisher;

    @Mock
    private ProviderContainer providerContainer;

    @Mock
    private ParticipantIdStorage participantIdStorage;

    private String domain = "domain";
    private String participantId = "participantId";
    private String publicKeyId = "";
    private ProviderQos providerQos = new ProviderQos();

    @JoynrInterface(provides = TestProvider.class, name = TestProvider.INTERFACE_NAME)
    @JoynrVersion(major = 1337, minor = 42)
    interface TestProvider extends JoynrProvider {
        public static String INTERFACE_NAME = "interfaceName";
    }

    @Before
    public void setUp() {

        registrar = new CapabilitiesRegistrarImpl(localDiscoveryAggregator,
                                                  providerContainerFactory,
                                                  messageRouter,
                                                  providerDirectory,
                                                  participantIdStorage,
                                                  ONE_DAY_IN_MS,
                                                  new InProcessAddress(new InProcessLibjoynrMessagingSkeleton(dispatcher)));
    }

    @SuppressWarnings("unchecked")
    @Test
    public void registerWithCapRegistrar() {
        JoynrVersion currentJoynrVersion = (JoynrVersion) TestProvider.class.getAnnotation(JoynrVersion.class);
        Version testVersion = new Version(currentJoynrVersion.major(), currentJoynrVersion.minor());

        when(providerContainer.getInterfaceName()).thenReturn(TestProvider.INTERFACE_NAME);
        RequestCaller requestCallerMock = mock(RequestCaller.class);
        when(providerContainer.getRequestCaller()).thenReturn(requestCallerMock);
        when(providerContainer.getSubscriptionPublisher()).thenReturn(subscriptionPublisher);
        when(participantIdStorage.getProviderParticipantId(eq(domain), eq(TestProvider.INTERFACE_NAME))).thenReturn(participantId);
        when(providerContainerFactory.create(testProvider)).thenReturn(providerContainer);

        ArgumentCaptor<DiscoveryEntry> discoveryEntryCaptor = ArgumentCaptor.forClass(DiscoveryEntry.class);

        registrar.registerProvider(domain, testProvider, providerQos);
        verify(localDiscoveryAggregator).add(any(Callback.class), discoveryEntryCaptor.capture());
        DiscoveryEntry actual = discoveryEntryCaptor.getValue();
        Assert.assertEquals(actual.getProviderVersion(), testVersion);
        Assert.assertEquals(actual.getDomain(), domain);
        Assert.assertEquals(actual.getInterfaceName(), TestProvider.INTERFACE_NAME);
        Assert.assertEquals(actual.getParticipantId(), participantId);
        Assert.assertEquals(actual.getQos(), providerQos);
        Assert.assertTrue((System.currentTimeMillis() - actual.getLastSeenDateMs()) < 5000);
        Assert.assertTrue((actual.getExpiryDateMs() - expiryDateMs) < 5000);
        Assert.assertEquals(actual.getPublicKeyId(), publicKeyId);

        verify(providerDirectory).add(eq(participantId), eq(providerContainer));
    }

    @SuppressWarnings("unchecked")
    @Test
    public void unregisterProvider() {
        when(providerContainer.getInterfaceName()).thenReturn(TestProvider.INTERFACE_NAME);
        when(participantIdStorage.getProviderParticipantId(eq(domain), eq(TestProvider.INTERFACE_NAME))).thenReturn(participantId);
        registrar.unregisterProvider(domain, testProvider);

        verify(localDiscoveryAggregator).remove(any(Callback.class), eq(participantId));
        verify(providerDirectory).remove(eq(participantId));
    }
}
