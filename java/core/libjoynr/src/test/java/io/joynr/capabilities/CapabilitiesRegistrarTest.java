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

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatchers;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import io.joynr.JoynrVersion;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.ProviderDirectory;
import io.joynr.dispatching.RequestCaller;
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
    private Address dispatcherAddress;

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
        dispatcherAddress = new InProcessAddress(new InProcessLibjoynrMessagingSkeleton(dispatcher));
        registrar = new CapabilitiesRegistrarImpl(localDiscoveryAggregator,
                                                  providerContainerFactory,
                                                  messageRouter,
                                                  providerDirectory,
                                                  participantIdStorage,
                                                  ONE_DAY_IN_MS,
                                                  dispatcherAddress);
        currentJoynrVersion = (JoynrVersion) TestProvider.class.getAnnotation(JoynrVersion.class);
        testVersion = new Version(currentJoynrVersion.major(), currentJoynrVersion.minor());

        when(providerContainer.getInterfaceName()).thenReturn(TestProvider.INTERFACE_NAME);
        requestCallerMock = mock(RequestCaller.class);
        lenient().when(providerContainer.getRequestCaller()).thenReturn(requestCallerMock);
        lenient().when(providerContainer.getSubscriptionPublisher()).thenReturn(subscriptionPublisher);
        when(participantIdStorage.getProviderParticipantId(eq(domain),
                                                           eq(TestProvider.INTERFACE_NAME),
                                                           anyInt())).thenReturn(participantId);
        when(providerContainerFactory.create(testProvider)).thenReturn(providerContainer);

        discoveryEntryCaptor = ArgumentCaptor.forClass(DiscoveryEntry.class);

        // Trigger onSuccess by default to make sure that the onSuccess callback is invoked (for coverage)
        doAnswer(new Answer<Future<Void>>() {
            public Future<Void> answer(InvocationOnMock invocation) {
                Object[] args = invocation.getArguments();
                @SuppressWarnings("unchecked")
                CallbackWithModeledError<Void, DiscoveryError> callback = (CallbackWithModeledError<Void, DiscoveryError>) args[0];
                callback.onSuccess(null);
                return null;
            }
        }).when(localDiscoveryAggregator)
          .add(any(CallbackWithModeledError.class), any(DiscoveryEntry.class), any(Boolean.class), any(String[].class));
    }

    @SuppressWarnings("unchecked")
    private void verifyRegisterProviderResults(boolean awaitGlobalRegistration, String[] gbids) {
        verify(localDiscoveryAggregator).add(any(CallbackWithModeledError.class),
                                             discoveryEntryCaptor.capture(),
                                             eq(awaitGlobalRegistration),
                                             eq(gbids));
        DiscoveryEntry actual = discoveryEntryCaptor.getValue();
        verifyDiscoveryEntry(actual);
        verify(providerDirectory).add(eq(participantId), eq(providerContainer));
    }

    @SuppressWarnings("unchecked")
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

    @Test
    public void registerWithCapRegistrarWithoutAwaitGlobalRegistration() {
        boolean awaitGlobalRegistration = false;
        registrar.registerProvider(domain, testProvider, providerQos, new String[]{}, awaitGlobalRegistration);
        verifyRegisterProviderResults(awaitGlobalRegistration, new String[]{});
    }

    @Test
    public void registerWithCapRegistrarWithAwaitGlobalRegistration() {
        boolean awaitGlobalRegistration = true;
        registrar.registerProvider(domain, testProvider, providerQos, new String[]{}, awaitGlobalRegistration);
        verifyRegisterProviderResults(awaitGlobalRegistration, new String[]{});
    }

    @SuppressWarnings("unchecked")
    private void testRegistrationWithErrorFromAdd() {
        registrar.registerProvider(domain, testProvider, providerQos, new String[]{}, true);
        InOrder inOrder = inOrder(messageRouter, localDiscoveryAggregator);
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
        doAnswer(new Answer<Future<Void>>() {
            public Future<Void> answer(InvocationOnMock invocation) {
                Object[] args = invocation.getArguments();
                CallbackWithModeledError<Void, DiscoveryError> callback = (CallbackWithModeledError<Void, DiscoveryError>) args[0];
                callback.onFailure(new JoynrRuntimeException("testException"));
                return null;
            }
        }).when(localDiscoveryAggregator)
          .add(any(CallbackWithModeledError.class), any(DiscoveryEntry.class), any(Boolean.class), any(String[].class));
        testRegistrationWithErrorFromAdd();
    }

    //Just for coverage
    @SuppressWarnings("unchecked")
    @Test
    public void registerWithCapRegistrarWithRuntimeException_removeNextHopGoesWrong() {
        doAnswer(new Answer<Future<Void>>() {
            public Future<Void> answer(InvocationOnMock invocation) {
                Object[] args = invocation.getArguments();
                CallbackWithModeledError<Void, DiscoveryError> callback = (CallbackWithModeledError<Void, DiscoveryError>) args[0];
                callback.onFailure(new JoynrRuntimeException("testException"));
                return null;
            }
        }).when(localDiscoveryAggregator)
          .add(any(CallbackWithModeledError.class), any(DiscoveryEntry.class), any(Boolean.class), any(String[].class));
        //We don't care about the exception type here
        doThrow(NullPointerException.class).when(messageRouter).removeNextHop(eq(participantId));
        testRegistrationWithErrorFromAdd();
    }

    @SuppressWarnings("unchecked")
    @Test
    public void registerWithCapRegistrarWithDiscoveryError() {
        doAnswer(new Answer<Future<Void>>() {
            public Future<Void> answer(InvocationOnMock invocation) {
                Object[] args = invocation.getArguments();
                CallbackWithModeledError<Void, DiscoveryError> callback = (CallbackWithModeledError<Void, DiscoveryError>) args[0];
                callback.onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
                return null;
            }
        }).when(localDiscoveryAggregator)
          .add(any(CallbackWithModeledError.class), any(DiscoveryEntry.class), any(Boolean.class), any(String[].class));
        testRegistrationWithErrorFromAdd();
    }

    //Just for coverage
    @SuppressWarnings("unchecked")
    @Test
    public void registerWithCapRegistrarWithDiscoveryError_removeNextHopGoesWrong() {
        doAnswer(new Answer<Future<Void>>() {
            public Future<Void> answer(InvocationOnMock invocation) {
                Object[] args = invocation.getArguments();
                CallbackWithModeledError<Void, DiscoveryError> callback = (CallbackWithModeledError<Void, DiscoveryError>) args[0];
                callback.onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
                return null;
            }
        }).when(localDiscoveryAggregator)
          .add(any(CallbackWithModeledError.class), any(DiscoveryEntry.class), any(Boolean.class), any(String[].class));
        //We don't care about the exception type here
        doThrow(NullPointerException.class).when(messageRouter).removeNextHop(eq(participantId));
        testRegistrationWithErrorFromAdd();
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

    private static Answer<Future<Void>> createRemoveAnswerWithSuccess() {
        return new Answer<Future<Void>>() {
            @Override
            public Future<Void> answer(InvocationOnMock invocation) throws Throwable {
                Future<Void> result = new Future<Void>();
                @SuppressWarnings("unchecked")
                Callback<Void> callback = (Callback<Void>) invocation.getArguments()[0];
                callback.onSuccess(null);
                result.onSuccess(null);
                return result;
            }
        };
    }

    private static Answer<Future<Void>> createRemoveAnswerWithException(JoynrRuntimeException exception) {
        return new Answer<Future<Void>>() {
            @Override
            public Future<Void> answer(InvocationOnMock invocation) throws Throwable {
                Future<Void> result = new Future<Void>();
                @SuppressWarnings("unchecked")
                Callback<Void> callback = (Callback<Void>) invocation.getArguments()[0];
                callback.onFailure(exception);
                result.onSuccess(null);
                return result;
            }
        };
    }

    @Test
    public void unregisterProvider_succeeded() {
        doAnswer(createRemoveAnswerWithSuccess()).when(localDiscoveryAggregator)
                                                 .remove(ArgumentMatchers.<Callback<Void>> any(), eq(participantId));
        try {
            Future<Void> future = registrar.unregisterProvider(domain, testProvider);
            verify(localDiscoveryAggregator).remove(ArgumentMatchers.<Callback<Void>> any(), eq(participantId));
            future.get(5000);
            verify(providerDirectory).remove(eq(participantId));
            verify(messageRouter).removeNextHop(eq(participantId));
        } catch (Exception e) {
            Assert.fail("Unexpected exception from unregisterProvider: " + e);
        }
    }

    @Test
    public void unregisterProvider_failsWithException() {
        JoynrRuntimeException expectedException = new JoynrRuntimeException("remove failed");
        doAnswer(createRemoveAnswerWithException(expectedException)).when(localDiscoveryAggregator)
                                                                    .remove(ArgumentMatchers.<Callback<Void>> any(),
                                                                            eq(participantId));

        try {
            Future<Void> future = registrar.unregisterProvider(domain, testProvider);
            verify(localDiscoveryAggregator).remove(ArgumentMatchers.<Callback<Void>> any(), eq(participantId));
            future.get(5000);
            Assert.fail("unregisterProvider expected to fail with exception");
        } catch (Exception exception) {
            verify(providerDirectory, times(0)).remove(eq(participantId));
            verify(messageRouter, times(0)).removeNextHop(eq(participantId));
            Assert.assertEquals(expectedException, exception);
        }
    }
}
