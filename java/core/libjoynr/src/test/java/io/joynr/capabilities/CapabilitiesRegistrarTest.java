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

import io.joynr.JoynrVersion;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.ProviderDirectory;
import io.joynr.dispatching.RequestCaller;
import io.joynr.dispatching.rpc.RequestInterpreter;
import io.joynr.exceptions.JoynrRuntimeException;
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
import io.joynr.proxy.Future;
import joynr.system.RoutingTypes.Address;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryError;
import joynr.types.ProviderQos;
import joynr.types.Version;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.lenient;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

@RunWith(MockitoJUnitRunner.class)
public class CapabilitiesRegistrarTest {

    private static final long ONE_DAY_IN_MS = 24 * 60 * 60 * 1000;
    private final long expiryDateMs = System.currentTimeMillis() + ONE_DAY_IN_MS;
    private CapabilitiesRegistrar registrar;
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
    private Address dispatcherAddress;

    @Mock
    private TestProvider testProvider;

    @Mock
    private AbstractSubscriptionPublisher subscriptionPublisher;

    @Mock
    private ProviderContainer providerContainer;

    @Mock
    private ParticipantIdStorage participantIdStorage;
    @Mock
    private RequestInterpreter requestInterpreter;

    private final String domain = "domain";
    private final String participantId = "participantId";
    private final ProviderQos providerQos = new ProviderQos();
    private Version testVersion;
    private ArgumentCaptor<DiscoveryEntry> discoveryEntryCaptor;

    @JoynrInterface(provider = TestProvider.class, provides = TestProvider.class, name = TestProvider.INTERFACE_NAME)
    @JoynrVersion(major = 1337, minor = 42)
    interface TestProvider extends JoynrProvider {
        String INTERFACE_NAME = "interfaceName";
    }

    @SuppressWarnings("unchecked")
    @Before
    public void setUp() {
        dispatcherAddress = new InProcessAddress(new InProcessLibjoynrMessagingSkeleton(dispatcher));
        registrar = new CapabilitiesRegistrarImpl(localDiscoveryAggregator,
                                                  providerContainerFactory,
                                                  messageRouter,
                                                  providerDirectory,
                                                  participantIdStorage,
                                                  ONE_DAY_IN_MS,
                                                  dispatcherAddress,
                                                  requestInterpreter);
        final JoynrVersion currentJoynrVersion = TestProvider.class.getAnnotation(JoynrVersion.class);
        testVersion = new Version(currentJoynrVersion.major(), currentJoynrVersion.minor());

        when(providerContainer.getInterfaceName()).thenReturn(TestProvider.INTERFACE_NAME);
        final RequestCaller requestCallerMock = mock(RequestCaller.class);
        lenient().when(providerContainer.getRequestCaller()).thenReturn(requestCallerMock);
        lenient().when(providerContainer.getSubscriptionPublisher()).thenReturn(subscriptionPublisher);
        when(participantIdStorage.getProviderParticipantId(eq(domain),
                                                           eq(TestProvider.INTERFACE_NAME),
                                                           anyInt())).thenReturn(participantId);
        when(providerContainerFactory.create(testProvider)).thenReturn(providerContainer);

        discoveryEntryCaptor = ArgumentCaptor.forClass(DiscoveryEntry.class);

        // Trigger onSuccess by default to make sure that the onSuccess callback is invoked (for coverage)
        doAnswer((Answer<Future<Void>>) invocation -> {
            final Object[] args = invocation.getArguments();
            @SuppressWarnings("unchecked")
            final CallbackWithModeledError<Void, DiscoveryError> callback = (CallbackWithModeledError<Void, DiscoveryError>) args[0];
            callback.onSuccess(null);
            return null;
        }).when(localDiscoveryAggregator)
          .add(any(CallbackWithModeledError.class), any(DiscoveryEntry.class), any(Boolean.class), any(String[].class));
    }

    @SuppressWarnings("unchecked")
    private void verifyRegisterProviderResults(final boolean awaitGlobalRegistration, final String[] gbids) {
        verify(localDiscoveryAggregator).add(any(CallbackWithModeledError.class),
                                             discoveryEntryCaptor.capture(),
                                             eq(awaitGlobalRegistration),
                                             eq(gbids));
        final DiscoveryEntry actual = discoveryEntryCaptor.getValue();
        verifyDiscoveryEntry(actual);
        verify(providerDirectory).add(eq(participantId), eq(providerContainer));
    }

    @SuppressWarnings("unchecked")
    @Deprecated
    private void verifyRegisterInallKnownBackendsResults(final boolean awaitGlobalRegistration) {
        verify(localDiscoveryAggregator).addToAll(any(CallbackWithModeledError.class),
                                                  discoveryEntryCaptor.capture(),
                                                  eq(awaitGlobalRegistration));
        final DiscoveryEntry actual = discoveryEntryCaptor.getValue();
        verifyDiscoveryEntry(actual);
        verify(providerDirectory).add(eq(participantId), eq(providerContainer));
    }

    private void verifyDiscoveryEntry(final DiscoveryEntry discoveryEntry) {
        assertEquals(discoveryEntry.getProviderVersion(), testVersion);
        assertEquals(discoveryEntry.getDomain(), domain);
        assertEquals(discoveryEntry.getInterfaceName(), TestProvider.INTERFACE_NAME);
        assertEquals(discoveryEntry.getParticipantId(), participantId);
        assertEquals(discoveryEntry.getQos(), providerQos);
        assertTrue((System.currentTimeMillis() - discoveryEntry.getLastSeenDateMs()) < 5000);
        assertTrue((discoveryEntry.getExpiryDateMs() - expiryDateMs) < 5000);
        assertEquals("", discoveryEntry.getPublicKeyId());
    }

    @Test
    public void registerWithCapRegistrarWithoutAwaitGlobalRegistration() {
        final boolean awaitGlobalRegistration = false;
        registrar.registerProvider(domain, testProvider, providerQos, new String[]{}, awaitGlobalRegistration);
        verifyRegisterProviderResults(awaitGlobalRegistration, new String[]{});
    }

    @Test
    public void registerWithCapRegistrarWithAwaitGlobalRegistration() {
        final boolean awaitGlobalRegistration = true;
        registrar.registerProvider(domain, testProvider, providerQos, new String[]{}, awaitGlobalRegistration);
        verifyRegisterProviderResults(awaitGlobalRegistration, new String[]{});
    }

    @SuppressWarnings("unchecked")
    private void testRegistrationWithErrorFromAdd() {
        registrar.registerProvider(domain, testProvider, providerQos, new String[]{}, true);
        final InOrder inOrder = inOrder(messageRouter, localDiscoveryAggregator);
        inOrder.verify(messageRouter).addNextHop(eq(participantId), eq(dispatcherAddress), eq(true));
        inOrder.verify(localDiscoveryAggregator).add(any(CallbackWithModeledError.class),
                                                     any(DiscoveryEntry.class),
                                                     any(Boolean.class),
                                                     any(String[].class));
        inOrder.verify(messageRouter).removeNextHop(eq(participantId));
        verifyNoMoreInteractions(messageRouter, localDiscoveryAggregator);
    }

    @SuppressWarnings("unchecked")
    @Test
    public void registerWithCapRegistrarWithRuntimeException() {
        doAnswer((Answer<Future<Void>>) invocation -> {
            final Object[] args = invocation.getArguments();
            final CallbackWithModeledError<Void, DiscoveryError> callback = (CallbackWithModeledError<Void, DiscoveryError>) args[0];
            callback.onFailure(new JoynrRuntimeException("testException"));
            return null;
        }).when(localDiscoveryAggregator)
          .add(any(CallbackWithModeledError.class), any(DiscoveryEntry.class), any(Boolean.class), any(String[].class));
        testRegistrationWithErrorFromAdd();
    }

    //Just for coverage
    @SuppressWarnings("unchecked")
    @Test
    public void registerWithCapRegistrarWithRuntimeException_removeNextHopGoesWrong() {
        doAnswer((Answer<Future<Void>>) invocation -> {
            final Object[] args = invocation.getArguments();
            final CallbackWithModeledError<Void, DiscoveryError> callback = (CallbackWithModeledError<Void, DiscoveryError>) args[0];
            callback.onFailure(new JoynrRuntimeException("testException"));
            return null;
        }).when(localDiscoveryAggregator)
          .add(any(CallbackWithModeledError.class), any(DiscoveryEntry.class), any(Boolean.class), any(String[].class));
        //We don't care about the exception type here
        doThrow(NullPointerException.class).when(messageRouter).removeNextHop(eq(participantId));
        testRegistrationWithErrorFromAdd();
    }

    @SuppressWarnings("unchecked")
    @Test
    public void registerWithCapRegistrarWithDiscoveryError() {
        doAnswer((Answer<Future<Void>>) invocation -> {
            final Object[] args = invocation.getArguments();
            final CallbackWithModeledError<Void, DiscoveryError> callback = (CallbackWithModeledError<Void, DiscoveryError>) args[0];
            callback.onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
            return null;
        }).when(localDiscoveryAggregator)
          .add(any(CallbackWithModeledError.class), any(DiscoveryEntry.class), any(Boolean.class), any(String[].class));
        testRegistrationWithErrorFromAdd();
    }

    //Just for coverage
    @SuppressWarnings("unchecked")
    @Test
    public void registerWithCapRegistrarWithDiscoveryError_removeNextHopGoesWrong() {
        doAnswer((Answer<Future<Void>>) invocation -> {
            final Object[] args = invocation.getArguments();
            final CallbackWithModeledError<Void, DiscoveryError> callback = (CallbackWithModeledError<Void, DiscoveryError>) args[0];
            callback.onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
            return null;
        }).when(localDiscoveryAggregator)
          .add(any(CallbackWithModeledError.class), any(DiscoveryEntry.class), any(Boolean.class), any(String[].class));
        //We don't care about the exception type here
        doThrow(NullPointerException.class).when(messageRouter).removeNextHop(eq(participantId));
        testRegistrationWithErrorFromAdd();
    }

    @Test
    public void registerWithCapRegistrarWithAwaitGlobalRegistrationAndGbids() {
        final boolean awaitGlobalRegistration = true;
        final String[] gbids = new String[]{ "testgbid1" };
        registrar.registerProvider(domain, testProvider, providerQos, gbids, awaitGlobalRegistration);
        verifyRegisterProviderResults(awaitGlobalRegistration, gbids);
    }

    @Test
    @Deprecated
    public void testRegisterInAllKnownBackendsWithoutAwaitGlobalRegistration() {
        final boolean awaitGlobalRegistration = false;
        registrar.registerInAllKnownBackends(domain, testProvider, providerQos, awaitGlobalRegistration);
        verifyRegisterInallKnownBackendsResults(awaitGlobalRegistration);
    }

    @Test
    @Deprecated
    public void testRegisterInAllKnownBackendsWithAwaitGlobalRegistration() {
        final boolean awaitGlobalRegistration = true;
        registrar.registerInAllKnownBackends(domain, testProvider, providerQos, awaitGlobalRegistration);
        verifyRegisterInallKnownBackendsResults(awaitGlobalRegistration);
    }

    private static Answer<Future<Void>> createRemoveAnswerWithSuccess() {
        return invocation -> {
            final Future<Void> result = new Future<>();
            @SuppressWarnings("unchecked")
            final Callback<Void> callback = (Callback<Void>) invocation.getArguments()[0];
            callback.onSuccess(null);
            result.onSuccess(null);
            return result;
        };
    }

    private static Answer<Future<Void>> createRemoveAnswerWithException(JoynrRuntimeException exception) {
        return invocation -> {
            final Future<Void> result = new Future<>();
            @SuppressWarnings("unchecked")
            final Callback<Void> callback = (Callback<Void>) invocation.getArguments()[0];
            callback.onFailure(exception);
            result.onSuccess(null);
            return result;
        };
    }

    @Test
    public void unregisterProvider_succeeded() {
        doAnswer(createRemoveAnswerWithSuccess()).when(localDiscoveryAggregator).remove(any(), eq(participantId));
        try {
            final Future<Void> future = registrar.unregisterProvider(domain, testProvider);
            verify(localDiscoveryAggregator).remove(any(), eq(participantId));
            future.get(5000);
            verify(providerDirectory).remove(eq(participantId));
            verify(messageRouter).removeNextHop(eq(participantId));
            verify(requestInterpreter).removeAllMethodInformation(testProvider.getClass());
        } catch (final Exception e) {
            fail("Unexpected exception from unregisterProvider: " + e);
        }
    }

    @Test
    public void unregisterProvider_failsWithException() {
        final JoynrRuntimeException expectedException = new JoynrRuntimeException("remove failed");
        doAnswer(createRemoveAnswerWithException(expectedException)).when(localDiscoveryAggregator)
                                                                    .remove(any(), eq(participantId));

        try {
            final Future<Void> future = registrar.unregisterProvider(domain, testProvider);
            verify(localDiscoveryAggregator).remove(any(), eq(participantId));
            future.get(5000);
            fail("unregisterProvider expected to fail with exception");
        } catch (Exception exception) {
            verify(providerDirectory, times(0)).remove(eq(participantId));
            verify(messageRouter, times(0)).removeNextHop(eq(participantId));
            assertEquals(expectedException, exception);
        }
    }
}
