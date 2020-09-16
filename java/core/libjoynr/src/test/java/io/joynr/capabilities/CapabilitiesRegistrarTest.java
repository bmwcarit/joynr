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
package io.joynr.capabilities;

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
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
import io.joynr.proxy.CallbackWithModeledError;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryError;
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
    private JoynrVersion currentJoynrVersion;
    private Version testVersion;
    private RequestCaller requestCallerMock;
    private ArgumentCaptor<DiscoveryEntry> discoveryEntryCaptor;

    @JoynrInterface(provider = TestProvider.class, provides = TestProvider.class, name = TestProvider.INTERFACE_NAME)
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
        currentJoynrVersion = (JoynrVersion) TestProvider.class.getAnnotation(JoynrVersion.class);
        testVersion = new Version(currentJoynrVersion.major(), currentJoynrVersion.minor());

        when(providerContainer.getInterfaceName()).thenReturn(TestProvider.INTERFACE_NAME);
        requestCallerMock = mock(RequestCaller.class);
        when(providerContainer.getRequestCaller()).thenReturn(requestCallerMock);
        when(providerContainer.getSubscriptionPublisher()).thenReturn(subscriptionPublisher);
        when(participantIdStorage.getProviderParticipantId(eq(domain),
                                                           eq(TestProvider.INTERFACE_NAME),
                                                           anyInt())).thenReturn(participantId);
        when(providerContainerFactory.create(testProvider)).thenReturn(providerContainer);

        discoveryEntryCaptor = ArgumentCaptor.forClass(DiscoveryEntry.class);
    }

    private void verifyRegisterProviderResults(boolean awaitGlobalRegistration, String[] gbids) {
        verify(localDiscoveryAggregator).add(any(CallbackWithModeledError.class),
                                             discoveryEntryCaptor.capture(),
                                             eq(awaitGlobalRegistration),
                                             eq(gbids));
        DiscoveryEntry actual = discoveryEntryCaptor.getValue();
        verifyDiscoveryEntry(actual);
        verify(providerDirectory).add(eq(participantId), eq(providerContainer));
    }

    @Deprecated
    private void verifyRegisterInallKnownBackendsResults(boolean awaitGlobalRegistration) {
        verify(localDiscoveryAggregator).addToAll(any(CallbackWithModeledError.class),
                                                  discoveryEntryCaptor.capture(),
                                                  eq(awaitGlobalRegistration));
        DiscoveryEntry actual = discoveryEntryCaptor.getValue();
        verifyDiscoveryEntry(actual);
        verify(providerDirectory).add(eq(participantId), eq(providerContainer));
    }

    private void verifyDiscoveryEntry(DiscoveryEntry discoveryEntry) {
        Assert.assertEquals(discoveryEntry.getProviderVersion(), testVersion);
        Assert.assertEquals(discoveryEntry.getDomain(), domain);
        Assert.assertEquals(discoveryEntry.getInterfaceName(), TestProvider.INTERFACE_NAME);
        Assert.assertEquals(discoveryEntry.getParticipantId(), participantId);
        Assert.assertEquals(discoveryEntry.getQos(), providerQos);
        Assert.assertTrue((System.currentTimeMillis() - discoveryEntry.getLastSeenDateMs()) < 5000);
        Assert.assertTrue((discoveryEntry.getExpiryDateMs() - expiryDateMs) < 5000);
        Assert.assertEquals(discoveryEntry.getPublicKeyId(), publicKeyId);
    }

    @SuppressWarnings("unchecked")
    @Test
    public void registerWithCapRegistrarWithoutAwaitGlobalRegistration() {
        boolean awaitGlobalRegistration = false;
        registrar.registerProvider(domain, testProvider, providerQos, new String[]{}, awaitGlobalRegistration);
        verifyRegisterProviderResults(awaitGlobalRegistration, new String[]{});
    }

    @SuppressWarnings("unchecked")
    @Test
    public void registerWithCapRegistrarWithAwaitGlobalRegistration() {
        boolean awaitGlobalRegistration = true;
        registrar.registerProvider(domain, testProvider, providerQos, new String[]{}, awaitGlobalRegistration);
        verifyRegisterProviderResults(awaitGlobalRegistration, new String[]{});
    }

    @Test
    public void registerWithCapRegistrarWithAwaitGlobalRegistrationAndGbids() {
        boolean awaitGlobalRegistration = true;
        String[] gbids = new String[]{ "testgbid1" };
        registrar.registerProvider(domain, testProvider, providerQos, gbids, awaitGlobalRegistration);
        verifyRegisterProviderResults(awaitGlobalRegistration, gbids);
    }

    @Test
    @Deprecated
    public void testRegisterInAllKnownBackendsWithoutAwaitGlobalRegistration() {
        boolean awaitGlobalRegistration = false;
        registrar.registerInAllKnownBackends(domain, testProvider, providerQos, awaitGlobalRegistration);
        verifyRegisterInallKnownBackendsResults(awaitGlobalRegistration);
    }

    @Test
    @Deprecated
    public void testRegisterInAllKnownBackendsWithAwaitGlobalRegistration() {
        boolean awaitGlobalRegistration = true;
        registrar.registerInAllKnownBackends(domain, testProvider, providerQos, awaitGlobalRegistration);
        verifyRegisterInallKnownBackendsResults(awaitGlobalRegistration);
    }

    @SuppressWarnings("unchecked")
    @Test
    public void unregisterProvider() {
        registrar.unregisterProvider(domain, testProvider);

        verify(localDiscoveryAggregator).remove(any(Callback.class), eq(participantId));
        verify(providerDirectory).remove(eq(participantId));
    }
}
