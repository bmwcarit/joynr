/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.argThat;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.hamcrest.TypeSafeMatcher;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatcher;
import org.mockito.Captor;
import org.mockito.InOrder;
import org.mockito.Matchers;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.internal.matchers.VarargMatcher;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import io.joynr.capabilities.LocalCapabilitiesDirectoryImpl.GcdTaskSequencer;
import io.joynr.dispatching.Dispatcher;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrTimeoutException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.TransportReadyListener;
import io.joynr.provider.AbstractDeferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;
import io.joynr.proxy.Callback;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.runtime.GlobalAddressProvider;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.util.ObjectMapper;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.system.DiscoveryProvider.Add1Deferred;
import joynr.system.DiscoveryProvider.AddToAllDeferred;
import joynr.system.DiscoveryProvider.Lookup1Deferred;
import joynr.system.DiscoveryProvider.Lookup2Deferred;
import joynr.system.DiscoveryProvider.Lookup3Deferred;
import joynr.system.DiscoveryProvider.Lookup4Deferred;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.CustomParameter;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.DiscoveryError;
import joynr.types.DiscoveryQos;
import joynr.types.DiscoveryScope;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;
import joynr.types.Version;

@RunWith(MockitoJUnitRunner.class)
public class LocalCapabilitiesDirectoryTest {
    private static final int TEST_TIMEOUT = 10000;
    private static final int DEFAULT_WAIT_TIME_MS = 5000; // value should be shorter than TEST_TIMEOUT
    private static final String INTERFACE_NAME = "interfaceName";
    private static final String TEST_URL = "mqtt://testUrl:42";
    private static final long ONE_DAY_IN_MS = 1 * 24 * 60 * 60 * 1000;
    private static final long freshnessUpdateIntervalMs = 300;
    private static final long DEFAULT_EXPIRY_TIME_MS = 3628800000l;
    private static final long READD_INTERVAL_DAYS = 7l;
    private static final long defaultTtlAddAndRemove = MessagingQos.DEFAULT_TTL;

    private LocalCapabilitiesDirectory localCapabilitiesDirectory;

    private String[] knownGbids = { "testDEFAULTgbid", "testgbid2", "testGbid" };
    private Long expiryDateMs = System.currentTimeMillis() + ONE_DAY_IN_MS;
    private String publicKeyId = "publicKeyId";

    private MqttAddress globalAddress1;
    private String globalAddress1Serialized;
    private MqttAddress globalAddress2;
    private String globalAddress2Serialized;
    private MqttAddress globalAddressWithoutGbid;
    private String globalAddressWithoutGbidSerialized;

    private DiscoveryEntry discoveryEntry;
    private DiscoveryEntry expectedDiscoveryEntry;
    private GlobalDiscoveryEntry globalDiscoveryEntry;
    private GlobalDiscoveryEntry expectedGlobalDiscoveryEntry;
    private GlobalDiscoveryEntry provisionedGlobalDiscoveryEntry;

    @Mock
    JoynrRuntime runtime;
    @Mock
    private GlobalCapabilitiesDirectoryClient globalCapabilitiesDirectoryClient;
    @Mock
    private ExpiredDiscoveryEntryCacheCleaner expiredDiscoveryEntryCacheCleaner;
    @Mock
    private MessageRouter messageRouter;
    @Mock
    private Dispatcher dispatcher;
    @Mock
    private ProxyBuilderFactory proxyBuilderFactoryMock;
    @Mock
    private DiscoveryEntryStore<DiscoveryEntry> localDiscoveryEntryStoreMock;
    @Mock
    private DiscoveryEntryStore<GlobalDiscoveryEntry> globalDiscoveryEntryCacheMock;
    @Mock
    private GlobalAddressProvider globalAddressProvider;
    @Mock
    private CapabilitiesProvisioning capabilitiesProvisioning;
    @Mock
    private ScheduledExecutorService capabilitiesFreshnessUpdateExecutor;
    @Mock
    private ShutdownNotifier shutdownNotifier;

    @Captor
    private ArgumentCaptor<Collection<DiscoveryEntryWithMetaInfo>> capabilitiesCaptor;
    @Captor
    private ArgumentCaptor<Runnable> runnableCaptor;

    @Captor
    private ArgumentCaptor<GcdTaskSequencer> addRemoveQueueRunnableCaptor;

    private Thread addRemoveWorker;

    private static class DiscoveryEntryStoreVarargMatcher
            extends ArgumentMatcher<DiscoveryEntryStore<? extends DiscoveryEntry>[]> implements VarargMatcher {
        private static final long serialVersionUID = 1L;
        private final DiscoveryEntryStore<? extends DiscoveryEntry>[] matchAgainst;

        private DiscoveryEntryStoreVarargMatcher(DiscoveryEntryStore<?>... matchAgainst) {
            this.matchAgainst = matchAgainst;
        }

        @Override
        public boolean matches(Object argument) {
            assertNotNull(argument);
            assertArrayEquals(matchAgainst, (DiscoveryEntryStore[]) argument);
            return true;
        }
    }

    private static class DiscoveryEntryWithUpdatedLastSeenDateMsMatcher extends ArgumentMatcher<DiscoveryEntry> {

        private DiscoveryEntry expected;

        private DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(DiscoveryEntry expected) {
            this.expected = expected;
        }

        @Override
        public boolean matches(Object argument) {
            assertNotNull(argument);
            DiscoveryEntry actual = (DiscoveryEntry) argument;
            return discoveryEntriesMatchWithUpdatedLastSeenDate(expected, actual);
        }
    }

    private static class GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher
            extends ArgumentMatcher<GlobalDiscoveryEntry> {

        private GlobalDiscoveryEntry expected;

        private GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(GlobalDiscoveryEntry expected) {
            this.expected = expected;
        }

        @Override
        public boolean matches(Object argument) {
            assertNotNull(argument);
            GlobalDiscoveryEntry actual = (GlobalDiscoveryEntry) argument;
            return globalDiscoveryEntriesMatchWithUpdatedLastSeenDate(expected, actual);
        }
    }

    private static boolean discoveryEntriesMatchWithUpdatedLastSeenDate(DiscoveryEntry expected,
                                                                        DiscoveryEntry actual) {
        return expected.getDomain() == actual.getDomain() && expected.getExpiryDateMs() == actual.getExpiryDateMs()
                && expected.getInterfaceName() == actual.getInterfaceName()
                && expected.getParticipantId() == actual.getParticipantId()
                && expected.getProviderVersion().equals(actual.getProviderVersion())
                && expected.getPublicKeyId() == actual.getPublicKeyId() && expected.getQos().equals(actual.getQos())
                && expected.getLastSeenDateMs() <= actual.getLastSeenDateMs()
                && (expected.getLastSeenDateMs() + 1000) >= actual.getLastSeenDateMs();
    }

    private static boolean globalDiscoveryEntriesMatchWithUpdatedLastSeenDate(GlobalDiscoveryEntry expected,
                                                                              GlobalDiscoveryEntry actual) {
        return discoveryEntriesMatchWithUpdatedLastSeenDate(expected, actual)
                && expected.getAddress().equals(actual.getAddress());
    }

    @Before
    public void setUp() throws Exception {
        ObjectMapper objectMapper = new ObjectMapper();
        globalAddress1 = new MqttAddress(knownGbids[0], "testTopic");
        globalAddress1Serialized = objectMapper.writeValueAsString(globalAddress1);
        globalAddress2 = new MqttAddress(knownGbids[1], "testTopic");
        globalAddress2Serialized = objectMapper.writeValueAsString(globalAddress2);
        globalAddressWithoutGbid = new MqttAddress("brokerUri", "testTopic");
        globalAddressWithoutGbidSerialized = objectMapper.writeValueAsString(globalAddressWithoutGbid);

        Field objectMapperField = CapabilityUtils.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(CapabilityUtils.class, objectMapper);

        doAnswer(createAnswerWithSuccess()).when(globalCapabilitiesDirectoryClient)
                                           .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                any(GlobalDiscoveryEntry.class),
                                                anyLong(),
                                                Matchers.<String[]> any());

        doAnswer(createAnswerWithSuccess()).when(globalCapabilitiesDirectoryClient)
                                           .remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                   anyString(),
                                                   Matchers.<String[]> any());

        String discoveryDirectoriesDomain = "io.joynr";
        String capabilitiesDirectoryParticipantId = "capDir_participantId";
        String capabiltitiesDirectoryTopic = "dirTopic";
        GlobalDiscoveryEntry globalCapabilitiesDirectoryDiscoveryEntry = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0,
                                                                                                                             1),
                                                                                                                 discoveryDirectoriesDomain,
                                                                                                                 GlobalCapabilitiesDirectory.INTERFACE_NAME,
                                                                                                                 capabilitiesDirectoryParticipantId,
                                                                                                                 new ProviderQos(),
                                                                                                                 System.currentTimeMillis(),
                                                                                                                 expiryDateMs,
                                                                                                                 "provisionedPublicKey",
                                                                                                                 new MqttAddress(TEST_URL,
                                                                                                                                 capabiltitiesDirectoryTopic));

        provisionedGlobalDiscoveryEntry = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0, 1),
                                                                                  "provisioneddomain",
                                                                                  "provisionedInterface",
                                                                                  "provisionedParticipantId",
                                                                                  new ProviderQos(),
                                                                                  System.currentTimeMillis(),
                                                                                  expiryDateMs,
                                                                                  "provisionedPublicKey",
                                                                                  new MqttAddress("provisionedbrokeruri",
                                                                                                  "provisionedtopic"));

        when(capabilitiesProvisioning.getDiscoveryEntries()).thenReturn(new HashSet<GlobalDiscoveryEntry>(Arrays.asList(globalCapabilitiesDirectoryDiscoveryEntry,
                                                                                                                        provisionedGlobalDiscoveryEntry)));
        localCapabilitiesDirectory = new LocalCapabilitiesDirectoryImpl(capabilitiesProvisioning,
                                                                        globalAddressProvider,
                                                                        localDiscoveryEntryStoreMock,
                                                                        globalDiscoveryEntryCacheMock,
                                                                        messageRouter,
                                                                        globalCapabilitiesDirectoryClient,
                                                                        expiredDiscoveryEntryCacheCleaner,
                                                                        freshnessUpdateIntervalMs,
                                                                        capabilitiesFreshnessUpdateExecutor,
                                                                        shutdownNotifier,
                                                                        knownGbids,
                                                                        DEFAULT_EXPIRY_TIME_MS);

        verify(capabilitiesFreshnessUpdateExecutor).schedule(addRemoveQueueRunnableCaptor.capture(),
                                                             anyLong(),
                                                             eq(TimeUnit.MILLISECONDS));
        Runnable runnable = addRemoveQueueRunnableCaptor.getValue();
        addRemoveWorker = new Thread(runnable);
        addRemoveWorker.start();

        ProviderQos providerQos = new ProviderQos();
        CustomParameter[] parameterList = { new CustomParameter("key1", "value1"),
                new CustomParameter("key2", "value2") };
        providerQos.setCustomParameters(parameterList);

        String participantId = "testParticipantId";
        String domain = "domain";
        discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                            domain,
                                            INTERFACE_NAME,
                                            participantId,
                                            providerQos,
                                            System.currentTimeMillis(),
                                            expiryDateMs,
                                            publicKeyId);
        expectedDiscoveryEntry = new DiscoveryEntry(discoveryEntry);
        globalDiscoveryEntry = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry, globalAddress1);
        expectedGlobalDiscoveryEntry = new GlobalDiscoveryEntry(globalDiscoveryEntry);

        when(globalAddressProvider.get()).thenReturn(globalAddress1);
        when(localDiscoveryEntryStoreMock.lookup(anyString(), anyLong())).thenReturn(Optional.empty());
        when(globalDiscoveryEntryCacheMock.lookup(anyString(), anyLong())).thenReturn(Optional.empty());
    }

    @After
    public void tearDown() throws Exception {
        GcdTaskSequencer runnable = (GcdTaskSequencer) addRemoveQueueRunnableCaptor.getValue();
        runnable.stop();
        addRemoveWorker.join();
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testExpiredDiscoveryEntryCacheCleanerIsInitializenCorrectly() {
        verify(expiredDiscoveryEntryCacheCleaner).scheduleCleanUpForCaches(Mockito.<ExpiredDiscoveryEntryCacheCleaner.CleanupAction> any(),
                                                                           argThat(new DiscoveryEntryStoreVarargMatcher(globalDiscoveryEntryCacheMock,
                                                                                                                        localDiscoveryEntryStoreMock)));
    }

    private void checkAddToGlobalCapabilitiesDirectoryClientAndCache(Promise<? extends AbstractDeferred> promise,
                                                                     String[] expectedGbids) throws InterruptedException {
        ArgumentCaptor<GlobalDiscoveryEntry> argumentCaptor = ArgumentCaptor.forClass(GlobalDiscoveryEntry.class);
        ArgumentCaptor<Long> remainingTtlCapture = ArgumentCaptor.forClass(Long.class);
        checkPromiseSuccess(promise, "add failed");
        verify(globalCapabilitiesDirectoryClient).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      argumentCaptor.capture(),
                                                      remainingTtlCapture.capture(),
                                                      eq(expectedGbids));
        GlobalDiscoveryEntry capturedGlobalDiscoveryEntry = argumentCaptor.getValue();
        assertNotNull(capturedGlobalDiscoveryEntry);

        checkRemainingTtl(remainingTtlCapture);

        assertTrue(globalDiscoveryEntriesMatchWithUpdatedLastSeenDate(expectedGlobalDiscoveryEntry,
                                                                      capturedGlobalDiscoveryEntry));

        verify(localDiscoveryEntryStoreMock).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(globalDiscoveryEntryCacheMock,
               times(1)).add(argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)));
    }

    private void checkRemainingTtl(ArgumentCaptor<Long> remainingTtlCaptor) {
        long remainingTtl = remainingTtlCaptor.getValue().longValue();
        assertTrue(remainingTtl <= MessagingQos.DEFAULT_TTL);
        assertTrue(remainingTtl > (MessagingQos.DEFAULT_TTL / 2.0));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void add_global_invokesGcdAndCache() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        String[] expectedGbids = knownGbids;

        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        checkAddToGlobalCapabilitiesDirectoryClientAndCache(promise, expectedGbids);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addWithGbids_global_singleNonDefaultGbid_invokesGcdAndCache() throws InterruptedException {
        String[] gbids = new String[]{ knownGbids[1] };
        String[] expectedGbids = gbids.clone();
        final boolean awaitGlobalRegistration = true;
        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids);
        checkAddToGlobalCapabilitiesDirectoryClientAndCache(promise, expectedGbids);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addWithGbids_global_multipleGbids_invokesGcdAndCache() throws InterruptedException {
        // expectedGbids element order intentionally differs from knownGbids element order
        String[] gbids = new String[]{ knownGbids[1], knownGbids[0] };
        String[] expectedGbids = gbids.clone();
        final boolean awaitGlobalRegistration = true;
        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids);
        checkAddToGlobalCapabilitiesDirectoryClientAndCache(promise, expectedGbids);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addWithGbids_global_emptyGbidArray_addsToKnownBackends() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        String[] gbids = new String[0];
        String[] expectedGbids = knownGbids;
        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids);
        checkAddToGlobalCapabilitiesDirectoryClientAndCache(promise, expectedGbids);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addToAll_global_invokesGcdAndCache() throws InterruptedException {
        String[] expectedGbids = knownGbids.clone();
        final boolean awaitGlobalRegistration = true;
        Promise<AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                awaitGlobalRegistration);
        checkAddToGlobalCapabilitiesDirectoryClientAndCache(promise, expectedGbids);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void add_local_doesNotInvokeGcdAndCache() throws InterruptedException {
        discoveryEntry.getQos().setScope(ProviderScope.LOCAL);

        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry);
        checkPromiseSuccess(promise, "add failed");
        verify(localDiscoveryEntryStoreMock).add(discoveryEntry);
        verify(globalCapabilitiesDirectoryClient,
               never()).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                            any(GlobalDiscoveryEntry.class),
                            anyLong(),
                            Matchers.<String[]> any());
        verify(globalDiscoveryEntryCacheMock, never()).add(Matchers.<GlobalDiscoveryEntry> any());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addGlobalCapSucceeds_NextAddShallAddGlobalAgain() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        DiscoveryEntry discoveryEntry2 = new DiscoveryEntry(discoveryEntry);
        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);

        checkPromiseSuccess(promise, "add failed");

        ArgumentCaptor<Long> remainingTtlCapture = ArgumentCaptor.forClass(Long.class);

        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(globalDiscoveryEntryCacheMock,
               times(1)).add(argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)),
                             remainingTtlCapture.capture(),
                             Matchers.<String[]> any());

        checkRemainingTtl(remainingTtlCapture);

        Thread.sleep(1); // make sure that the lastSeenDate of expected entry 2 is larger than the lastSeenDateMs of expected entry 1
        DiscoveryEntry expectedDiscoveryEntry2 = new DiscoveryEntry(expectedDiscoveryEntry);
        expectedDiscoveryEntry2.setLastSeenDateMs(System.currentTimeMillis());
        GlobalDiscoveryEntry expectedGlobalDiscoveryEntry2 = new GlobalDiscoveryEntry(expectedGlobalDiscoveryEntry);
        expectedGlobalDiscoveryEntry2.setLastSeenDateMs(System.currentTimeMillis());
        verify(localDiscoveryEntryStoreMock,
               times(0)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry2)));

        doReturn(true).when(localDiscoveryEntryStoreMock).hasDiscoveryEntry(any(DiscoveryEntry.class));
        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock).lookup(anyString(), anyLong());

        Promise<DeferredVoid> promise2 = localCapabilitiesDirectory.add(discoveryEntry2, awaitGlobalRegistration);

        checkPromiseSuccess(promise2, "add failed");

        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry2)));
        verify(globalDiscoveryEntryCacheMock,
               times(1)).add(argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry2)));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry2)),
                             remainingTtlCapture.capture(),
                             Matchers.<String[]> any());
        checkRemainingTtl(remainingTtlCapture);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addGlobalCapFails_NextAddShallAddGlobalAgain() throws InterruptedException {

        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.GLOBAL);

        String participantId = LocalCapabilitiesDirectoryTest.class.getName() + ".addLocalAndThanGlobalShallWork";
        String domain = "testDomain";
        final DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                                 domain,
                                                                 INTERFACE_NAME,
                                                                 participantId,
                                                                 providerQos,
                                                                 System.currentTimeMillis(),
                                                                 expiryDateMs,
                                                                 publicKeyId);
        globalDiscoveryEntry = new GlobalDiscoveryEntry(new Version(47, 11),
                                                        domain,
                                                        INTERFACE_NAME,
                                                        participantId,
                                                        providerQos,
                                                        System.currentTimeMillis(),
                                                        expiryDateMs,
                                                        publicKeyId,
                                                        globalAddress1Serialized);

        ProviderRuntimeException exception = new ProviderRuntimeException("add failed");
        doAnswer(createVoidAnswerWithException(exception)).when(globalCapabilitiesDirectoryClient)
                                                          .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                               argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                               anyLong(),
                                                               Matchers.<String[]> any());

        final boolean awaitGlobalRegistration = true;
        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);

        checkPromiseException(promise, new ProviderRuntimeException(exception.toString()));
        ArgumentCaptor<Long> remainingTtlCaptor = ArgumentCaptor.forClass(Long.class);
        verify(globalCapabilitiesDirectoryClient).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                      remainingTtlCaptor.capture(),
                                                      Matchers.<String[]> any());
        checkRemainingTtl(remainingTtlCaptor);

        verify(globalDiscoveryEntryCacheMock,
               never()).add(argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)));

        reset(globalCapabilitiesDirectoryClient);

        doAnswer(createAnswerWithSuccess()).when(globalCapabilitiesDirectoryClient)
                                           .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                anyLong(),
                                                Matchers.<String[]> any());

        GlobalDiscoveryEntry expectedGlobalDiscoveryEntry2 = new GlobalDiscoveryEntry(globalDiscoveryEntry);
        expectedGlobalDiscoveryEntry2.setLastSeenDateMs(System.currentTimeMillis());

        promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);

        checkPromiseSuccess(promise, "add failed");
        verify(globalCapabilitiesDirectoryClient).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry2)),
                                                      remainingTtlCaptor.capture(),
                                                      Matchers.<String[]> any());
        checkRemainingTtl(remainingTtlCaptor);
    }

    private void testAddWithGbidsIsProperlyRejected(DiscoveryError expectedError) throws InterruptedException {
        doAnswer(createVoidAnswerWithDiscoveryError(expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                   .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                        argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                        anyLong(),
                                                                        Matchers.<String[]> any());

        final boolean awaitGlobalRegistration = true;
        String[] gbids = new String[]{ knownGbids[0] };
        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids);
        checkPromiseError(promise, expectedError);
    }

    private void testAddIsProperlyRejected(DiscoveryError expectedError) throws InterruptedException {
        doAnswer(createVoidAnswerWithDiscoveryError(expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                   .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                        argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                        anyLong(),
                                                                        Matchers.<String[]> any());

        final boolean awaitGlobalRegistration = true;
        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        checkPromiseErrorInProviderRuntimeException(promise, expectedError);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddWithGbidsIsProperlyRejected_invalidGbid() throws InterruptedException {
        testAddWithGbidsIsProperlyRejected(DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddWithGbidsIsProperlyRejected_unknownGbid() throws InterruptedException {
        testAddWithGbidsIsProperlyRejected(DiscoveryError.UNKNOWN_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddWithGbidsIsProperlyRejected_internalError() throws InterruptedException {
        testAddWithGbidsIsProperlyRejected(DiscoveryError.INTERNAL_ERROR);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddIsProperlyRejected_invalidGbid() throws InterruptedException {
        testAddIsProperlyRejected(DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddIsProperlyRejected_unknownGbid() throws InterruptedException {
        testAddIsProperlyRejected(DiscoveryError.UNKNOWN_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddIsProperlyRejected_internalError() throws InterruptedException {
        testAddIsProperlyRejected(DiscoveryError.INTERNAL_ERROR);
    }

    private void testAddReturnsDiscoveryError(String[] gbids,
                                              DiscoveryError expectedError) throws InterruptedException {
        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, true, gbids);
        checkPromiseError(promise, expectedError);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddWithGbids_unknownGbid() throws InterruptedException {
        String[] gbids = new String[]{ knownGbids[1], "unknown" };
        testAddReturnsDiscoveryError(gbids, DiscoveryError.UNKNOWN_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddWithGbids_invalidGbid_emptyGbid() throws InterruptedException {
        String[] gbids = new String[]{ knownGbids[1], "" };
        testAddReturnsDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddWithGbids_invalidGbid_duplicateGbid() throws InterruptedException {
        String[] gbids = new String[]{ knownGbids[1], knownGbids[1] };
        testAddReturnsDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddWithGbids_invalidGbid_nullGbid() throws InterruptedException {
        String[] gbids = new String[]{ knownGbids[1], null };
        testAddReturnsDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddWithGbids_invalidGbid_nullGbidArray() throws InterruptedException {
        String[] gbids = null;
        testAddReturnsDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addSameGbidTwiceInARow() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        String[] gbids = new String[]{ knownGbids[0] };
        String[] expectedGbids = gbids.clone();
        DiscoveryEntry discoveryEntry2 = new DiscoveryEntry(discoveryEntry);

        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                              .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                   argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)),
                                                   anyLong(),
                                                   eq(expectedGbids));

        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids);

        checkPromiseSuccess(promise, "add failed");
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(globalDiscoveryEntryCacheMock,
               times(1)).add(argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)));
        ArgumentCaptor<Long> remainingTtlCaptor = ArgumentCaptor.forClass(Long.class);
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)),
                             remainingTtlCaptor.capture(),
                             eq(expectedGbids));
        checkRemainingTtl(remainingTtlCaptor);

        Thread.sleep(1); // make sure that the lastSeenDate of expected entry 2 is larger than the lastSeenDateMs of expected entry 1
        DiscoveryEntry expectedDiscoveryEntry2 = new DiscoveryEntry(expectedDiscoveryEntry);
        expectedDiscoveryEntry2.setLastSeenDateMs(System.currentTimeMillis());
        GlobalDiscoveryEntry expectedGlobalDiscoveryEntry2 = new GlobalDiscoveryEntry(expectedGlobalDiscoveryEntry);
        expectedGlobalDiscoveryEntry2.setLastSeenDateMs(System.currentTimeMillis());

        doReturn(true).when(localDiscoveryEntryStoreMock).hasDiscoveryEntry(any(DiscoveryEntry.class));
        doReturn(Optional.of(globalDiscoveryEntry)).when(globalDiscoveryEntryCacheMock)
                                                   .lookup(eq(expectedDiscoveryEntry.getParticipantId()), anyLong());

        Promise<Add1Deferred> promise2 = localCapabilitiesDirectory.add(discoveryEntry2,
                                                                        awaitGlobalRegistration,
                                                                        gbids);

        checkPromiseSuccess(promise2, "add failed");

        // entry is added again (with newer lastSeenDateMs)
        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry2)));
        verify(globalDiscoveryEntryCacheMock,
               times(1)).add(argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry2)));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry2)),
                             remainingTtlCaptor.capture(),
                             eq(expectedGbids));
        checkRemainingTtl(remainingTtlCaptor);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addDifferentGbidsAfterEachOther() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        String[] gbids1 = new String[]{ knownGbids[0] };
        String[] expectedGbids1 = gbids1.clone();
        String[] gbids2 = new String[]{ knownGbids[1] };
        String[] expectedGbids2 = gbids2.clone();
        DiscoveryEntryWithMetaInfo expectedEntryWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                                   discoveryEntry);
        DiscoveryEntry discoveryEntry2 = new DiscoveryEntry(discoveryEntry);

        doAnswer(createAnswerWithSuccess()).when(globalCapabilitiesDirectoryClient)
                                           .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                any(GlobalDiscoveryEntry.class),
                                                anyLong(),
                                                Matchers.<String[]> any());

        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids1);
        checkPromiseSuccess(promise, "add failed");

        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(globalDiscoveryEntryCacheMock,
               times(1)).add(argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)));
        ArgumentCaptor<Long> remainingTtlCaptor = ArgumentCaptor.forClass(Long.class);
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)),
                             remainingTtlCaptor.capture(),
                             eq(expectedGbids1));
        checkRemainingTtl(remainingTtlCaptor);

        Thread.sleep(1); // make sure that the lastSeenDate of expected entry 2 is larger than the lastSeenDateMs of expected entry 1
        DiscoveryEntry expectedDiscoveryEntry2 = new DiscoveryEntry(expectedDiscoveryEntry);
        expectedDiscoveryEntry2.setLastSeenDateMs(System.currentTimeMillis());
        GlobalDiscoveryEntry expectedGlobalDiscoveryEntry2 = new GlobalDiscoveryEntry(expectedGlobalDiscoveryEntry);
        expectedGlobalDiscoveryEntry2.setLastSeenDateMs(System.currentTimeMillis());
        verify(localDiscoveryEntryStoreMock,
               times(0)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry2)));

        doReturn(true).when(localDiscoveryEntryStoreMock).hasDiscoveryEntry(any(DiscoveryEntry.class));

        Promise<Add1Deferred> promise2 = localCapabilitiesDirectory.add(discoveryEntry2,
                                                                        awaitGlobalRegistration,
                                                                        gbids2);
        checkPromiseSuccess(promise2, "add failed");

        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry2)));
        verify(globalDiscoveryEntryCacheMock,
               times(1)).add(argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry2)));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry2)),
                             remainingTtlCaptor.capture(),
                             eq(expectedGbids2));
        checkRemainingTtl(remainingTtlCaptor);

        // provider is now registered for both GBIDs
        doReturn(Arrays.asList(globalDiscoveryEntry)).when(globalDiscoveryEntryCacheMock)
                                                     .lookup(any(String[].class), anyString(), anyLong());
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setCacheMaxAge(ONE_DAY_IN_MS);

        Promise<Lookup2Deferred> promiseLookup1 = localCapabilitiesDirectory.lookup(new String[]{
                expectedDiscoveryEntry.getDomain() }, expectedDiscoveryEntry.getInterfaceName(), discoveryQos, gbids1);
        Promise<Lookup2Deferred> promiseLookup2 = localCapabilitiesDirectory.lookup(new String[]{
                expectedDiscoveryEntry.getDomain() }, expectedDiscoveryEntry.getInterfaceName(), discoveryQos, gbids2);

        DiscoveryEntryWithMetaInfo[] result1 = (DiscoveryEntryWithMetaInfo[]) checkPromiseSuccess(promiseLookup1,
                                                                                                  "lookup failed")[0];
        assertEquals(1, result1.length);
        assertEquals(expectedEntryWithMetaInfo, result1[0]);
        DiscoveryEntryWithMetaInfo[] result2 = (DiscoveryEntryWithMetaInfo[]) checkPromiseSuccess(promiseLookup2,
                                                                                                  "lookup failed")[0];
        assertEquals(1, result2.length);
        assertEquals(expectedEntryWithMetaInfo, result2[0]);

        verify(globalCapabilitiesDirectoryClient,
               times(0)).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                Matchers.<String[]> any(),
                                anyString(),
                                anyLong(),
                                Matchers.<String[]> any());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddKnownLocalEntryDoesNothing() throws InterruptedException {
        discoveryEntry.getQos().setScope(ProviderScope.LOCAL);
        expectedDiscoveryEntry.getQos().setScope(ProviderScope.LOCAL);
        doReturn(true).when(localDiscoveryEntryStoreMock)
                      .hasDiscoveryEntry(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock)
                                             .lookup(eq(expectedDiscoveryEntry.getParticipantId()), eq(Long.MAX_VALUE));

        DiscoveryEntry newDiscoveryEntry = new DiscoveryEntry(discoveryEntry);
        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(newDiscoveryEntry, false, knownGbids);

        checkPromiseSuccess(promise, "add failed");
        verify(localDiscoveryEntryStoreMock).hasDiscoveryEntry(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(localDiscoveryEntryStoreMock).lookup(eq(expectedDiscoveryEntry.getParticipantId()), eq(Long.MAX_VALUE));
        verify(globalDiscoveryEntryCacheMock, never()).lookup(anyString(), anyLong());
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient, never()).add(any(), any(), anyLong(), any());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddKnownLocalEntryWithDifferentExpiryDateAddsAgain() throws InterruptedException {
        DiscoveryEntry newDiscoveryEntry = new DiscoveryEntry(discoveryEntry);
        newDiscoveryEntry.setExpiryDateMs(discoveryEntry.getExpiryDateMs() + 1);
        newDiscoveryEntry.getQos().setScope(ProviderScope.LOCAL);
        doReturn(true).when(localDiscoveryEntryStoreMock).hasDiscoveryEntry(newDiscoveryEntry);
        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock)
                                             .lookup(eq(newDiscoveryEntry.getParticipantId()), eq(Long.MAX_VALUE));

        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(newDiscoveryEntry, false, knownGbids);

        checkPromiseSuccess(promise, "add failed");
        verify(localDiscoveryEntryStoreMock).hasDiscoveryEntry(newDiscoveryEntry);
        verify(localDiscoveryEntryStoreMock).lookup(eq(newDiscoveryEntry.getParticipantId()), eq(Long.MAX_VALUE));
        verify(localDiscoveryEntryStoreMock).add(eq(newDiscoveryEntry));
        verify(globalDiscoveryEntryCacheMock, never()).lookup(anyString(), anyLong());
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient, never()).add(any(), any(), anyLong(), any());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddWithGlobalAddressProviderThrowingException() throws InterruptedException {
        when(globalAddressProvider.get()).thenThrow(new JoynrRuntimeException());

        final boolean awaitGlobalRegistration = true;
        localCapabilitiesDirectory.add(globalDiscoveryEntry, awaitGlobalRegistration, knownGbids);

        verify(globalAddressProvider).registerGlobalAddressesReadyListener((TransportReadyListener) localCapabilitiesDirectory);
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient, times(0)).add(any(), any(), anyLong(), any());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddToAll() throws InterruptedException {
        boolean awaitGlobalRegistration = true;
        Promise<AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                awaitGlobalRegistration);

        ArgumentCaptor<Long> remainingTtlCapture = ArgumentCaptor.forClass(Long.class);

        checkPromiseSuccess(promise, "addToAll failed");
        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(globalDiscoveryEntryCacheMock).add(argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)));
        verify(globalCapabilitiesDirectoryClient).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)),
                                                      remainingTtlCapture.capture(),
                                                      eq(knownGbids));
        checkRemainingTtl(remainingTtlCapture);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddToAll_local() throws InterruptedException {
        discoveryEntry.getQos().setScope(ProviderScope.LOCAL);
        expectedDiscoveryEntry.getQos().setScope(ProviderScope.LOCAL);
        boolean awaitGlobalRegistration = true;

        Promise<AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                awaitGlobalRegistration);

        checkPromiseSuccess(promise, "addToAll failed");
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient,
               never()).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                            any(GlobalDiscoveryEntry.class),
                            anyLong(),
                            Matchers.<String[]> any());
        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddToAllIsProperlyRejected_exception() throws InterruptedException {
        JoynrRuntimeException exception = new JoynrRuntimeException("add failed");
        ProviderRuntimeException expectedException = new ProviderRuntimeException(exception.toString());

        doAnswer(createVoidAnswerWithException(exception)).when(globalCapabilitiesDirectoryClient)
                                                          .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                               argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                               anyLong(),
                                                               Matchers.<String[]> any());

        Promise<AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry, true);

        checkPromiseException(promise, expectedException);
        verify(localDiscoveryEntryStoreMock, times(1)).remove(eq(discoveryEntry.getParticipantId()));
    }

    private void testAddToAllIsProperlyRejected(DiscoveryError expectedError) throws InterruptedException {
        doAnswer(createVoidAnswerWithDiscoveryError(expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                   .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                        argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                        anyLong(),
                                                                        Matchers.<String[]> any());

        Promise<AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry, true);

        checkPromiseError(promise, expectedError);
        verify(localDiscoveryEntryStoreMock, times(1)).remove(eq(globalDiscoveryEntry.getParticipantId()));

    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddToAllIsProperlyRejected_internalError() throws InterruptedException {
        DiscoveryError expectedError = DiscoveryError.INTERNAL_ERROR;
        testAddToAllIsProperlyRejected(expectedError);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddToAllIsProperlyRejected_invalidGbid() throws InterruptedException {
        DiscoveryError expectedError = DiscoveryError.INVALID_GBID;
        testAddToAllIsProperlyRejected(expectedError);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddToAllIsProperlyRejected_unknownGbid() throws InterruptedException {
        DiscoveryError expectedError = DiscoveryError.UNKNOWN_GBID;
        testAddToAllIsProperlyRejected(expectedError);
    }

    private static Answer<Future<List<GlobalDiscoveryEntry>>> createLookupAnswer(final List<GlobalDiscoveryEntry> caps) {
        return new Answer<Future<List<GlobalDiscoveryEntry>>>() {

            @Override
            public Future<List<GlobalDiscoveryEntry>> answer(InvocationOnMock invocation) throws Throwable {
                Future<List<GlobalDiscoveryEntry>> result = new Future<List<GlobalDiscoveryEntry>>();
                @SuppressWarnings("unchecked")
                Callback<List<GlobalDiscoveryEntry>> callback = (Callback<List<GlobalDiscoveryEntry>>) invocation.getArguments()[0];
                callback.onSuccess(caps);
                result.onSuccess(caps);
                return result;
            }
        };
    }

    private static Answer<Future<GlobalDiscoveryEntry>> createLookupAnswer(final GlobalDiscoveryEntry caps) {
        return new Answer<Future<GlobalDiscoveryEntry>>() {

            @Override
            public Future<GlobalDiscoveryEntry> answer(InvocationOnMock invocation) throws Throwable {
                Future<GlobalDiscoveryEntry> result = new Future<GlobalDiscoveryEntry>();
                @SuppressWarnings("unchecked")
                Callback<GlobalDiscoveryEntry> callback = (Callback<GlobalDiscoveryEntry>) invocation.getArguments()[0];
                callback.onSuccess(caps);
                result.onSuccess(caps);
                return result;
            }
        };
    }

    private static Answer<Void> createAnswerWithSuccess() {
        return new Answer<Void>() {

            @SuppressWarnings("unchecked")
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                Object[] args = invocation.getArguments();
                ((Callback<Void>) args[0]).onSuccess(null);
                return null;
            }
        };
    }

    private static Answer<Void> createAnswerWithSuccess(CountDownLatch cdl) {
        return new Answer<Void>() {

            @SuppressWarnings("unchecked")
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                Object[] args = invocation.getArguments();
                ((Callback<Void>) args[0]).onSuccess(null);
                cdl.countDown();
                return null;
            }
        };
    }

    private static Answer<Void> createAnswerWithDelayedSuccess(CountDownLatch cdlStart,
                                                               CountDownLatch cdlDone,
                                                               long delay) {
        return new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                Callback<Void> callback = (Callback<Void>) invocation.getArguments()[0];
                new Thread(new Runnable() {

                    @Override
                    public void run() {
                        cdlStart.countDown();
                        try {
                            Thread.sleep(delay);
                        } catch (Exception e) {
                            fail("SLEEP INTERRUPTED");
                        }
                        callback.onSuccess(null);
                        cdlDone.countDown();
                    }
                }).start();
                return null;
            }
        };
    }

    private static Answer<Void> createVoidAnswerWithException(JoynrRuntimeException exception) {
        CountDownLatch cdl = new CountDownLatch(0);
        return createVoidAnswerWithException(cdl, exception);
    }

    private static Answer<Void> createVoidAnswerWithDiscoveryError(DiscoveryError error) {
        CountDownLatch cdl = new CountDownLatch(0);
        return createVoidAnswerWithDiscoveryError(cdl, error);
    }

    private static Answer<Void> createVoidAnswerWithException(CountDownLatch cdl, JoynrRuntimeException exception) {
        return new Answer<Void>() {

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                Callback<Void> callback = (Callback<Void>) invocation.getArguments()[0];
                callback.onFailure(exception);
                cdl.countDown();
                return null;
            }
        };
    }

    private static Answer<Void> createVoidAnswerWithDiscoveryError(CountDownLatch cdl, DiscoveryError error) {
        return new Answer<Void>() {

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                Object[] args = invocation.getArguments();
                @SuppressWarnings("unchecked")
                CallbackWithModeledError<Void, DiscoveryError> callback = ((CallbackWithModeledError<Void, DiscoveryError>) args[0]);
                callback.onFailure(error);
                cdl.countDown();
                return null;
            }
        };
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterface_globalOnly() throws InterruptedException {
        List<GlobalDiscoveryEntry> caps = new ArrayList<GlobalDiscoveryEntry>();
        String domain1 = "domain1";
        String[] domains = new String[]{ domain1 };
        String interfaceName1 = "interfaceName1";
        DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 1000L, DiscoveryScope.GLOBAL_ONLY, false);

        when(globalDiscoveryEntryCacheMock.lookup(eq(domains),
                                                  eq(interfaceName1),
                                                  eq(discoveryQos.getCacheMaxAge()))).thenReturn(new ArrayList<GlobalDiscoveryEntry>());
        doAnswer(createLookupAnswer(caps)).when(globalCapabilitiesDirectoryClient)
                                          .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                  eq(domains),
                                                  eq(interfaceName1),
                                                  eq(discoveryQos.getDiscoveryTimeout()),
                                                  eq(knownGbids));
        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(1,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             0);

        // add local entry
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                           domain1,
                                                           interfaceName1,
                                                           "localParticipant",
                                                           providerQos,
                                                           System.currentTimeMillis(),
                                                           expiryDateMs,
                                                           publicKeyId);
        final boolean awaitGlobalRegistration = true;
        Promise<DeferredVoid> promiseAdd = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        checkPromiseSuccess(promiseAdd, "add failed");
        Promise<Lookup1Deferred> promise2 = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(2,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise2,
                                             0);

        // even deleting local cap entries shall have no effect, the global cap dir shall be invoked
        when(localDiscoveryEntryStoreMock.lookup(discoveryEntry.getParticipantId(),
                                                 Long.MAX_VALUE)).thenReturn(Optional.of(discoveryEntry));
        localCapabilitiesDirectory.remove(discoveryEntry.getParticipantId());
        verify(localDiscoveryEntryStoreMock).remove(discoveryEntry.getParticipantId());
        Promise<Lookup1Deferred> promise3 = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(3,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise3,
                                             0);

        // add global entry
        GlobalDiscoveryEntry capInfo = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                domain1,
                                                                interfaceName1,
                                                                "globalParticipant",
                                                                new ProviderQos(),
                                                                System.currentTimeMillis(),
                                                                expiryDateMs,
                                                                publicKeyId,
                                                                globalAddress1Serialized);
        caps.add(capInfo);
        doAnswer(createLookupAnswer(caps)).when(globalCapabilitiesDirectoryClient)
                                          .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                  eq(domains),
                                                  eq(interfaceName1),
                                                  eq(discoveryQos.getDiscoveryTimeout()),
                                                  eq(knownGbids));
        Promise<Lookup1Deferred> promise4 = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(4,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise4,
                                             1); // 1 global entry

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        reset((Object) globalDiscoveryEntryCacheMock);

        when(globalDiscoveryEntryCacheMock.lookup(eq(domains),
                                                  eq(interfaceName1),
                                                  eq(discoveryQos.getCacheMaxAge()))).thenReturn(Arrays.asList(capInfo));

        Promise<Lookup1Deferred> promise5 = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(4,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise5,
                                             1); // 1 cached entry

        // and now, invalidate the existing cached global values, resulting in another call to globalcapclient
        discoveryQos.setCacheMaxAge(0L);
        Thread.sleep(1);

        // now, another lookup call shall call the globalCapabilitiesDirectoryClient, as the global cap dir is expired
        Promise<Lookup1Deferred> promise6 = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(5,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise6,
                                             1); // 1 global entry
        reset(globalCapabilitiesDirectoryClient);
    }

    private Object[] verifyGcdLookupAndPromiseFulfillment(int gcdTimesCalled,
                                                          String[] domains,
                                                          String interfaceName,
                                                          long discoveryTimeout,
                                                          String[] gbids,
                                                          Promise<?> promise,
                                                          int numberOfReturnedValues) throws InterruptedException {
        verify(globalCapabilitiesDirectoryClient,
               times(gcdTimesCalled)).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                             argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
                                             eq(interfaceName),
                                             eq(discoveryTimeout),
                                             eq(gbids));

        Object[] values = checkPromiseSuccess(promise, "Unexpected rejection in global lookup");
        assertEquals(numberOfReturnedValues, ((DiscoveryEntryWithMetaInfo[]) values[0]).length);
        return values;
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterface_localThenGlobal() throws InterruptedException {
        List<GlobalDiscoveryEntry> caps = new ArrayList<GlobalDiscoveryEntry>();
        String domain1 = "domain1";
        String[] domains = new String[]{ domain1 };
        String interfaceName1 = "interfaceName1";
        DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 1000L, DiscoveryScope.LOCAL_THEN_GLOBAL, false);

        doAnswer(createLookupAnswer(caps)).when(globalCapabilitiesDirectoryClient)
                                          .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                  eq(domains),
                                                  eq(interfaceName1),
                                                  eq(discoveryQos.getDiscoveryTimeout()),
                                                  eq(knownGbids));
        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);

        verifyGcdLookupAndPromiseFulfillment(1,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             0);

        // add local entry
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                           domain1,
                                                           interfaceName1,
                                                           "localParticipant",
                                                           providerQos,
                                                           System.currentTimeMillis(),
                                                           expiryDateMs,
                                                           publicKeyId);
        reset((Object) localDiscoveryEntryStoreMock);
        when(localDiscoveryEntryStoreMock.lookup(eq(domains),
                                                 eq(interfaceName1))).thenReturn(Arrays.asList(discoveryEntry));
        promise = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(1,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             1); // 1 local entry

        // add global entry
        GlobalDiscoveryEntry capInfo = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                domain1,
                                                                interfaceName1,
                                                                "globalParticipant",
                                                                new ProviderQos(),
                                                                System.currentTimeMillis(),
                                                                expiryDateMs,
                                                                publicKeyId,
                                                                globalAddress1Serialized);
        caps.add(capInfo);
        doAnswer(createLookupAnswer(caps)).when(globalCapabilitiesDirectoryClient)
                                          .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                  eq(domains),
                                                  eq(interfaceName1),
                                                  eq(discoveryQos.getDiscoveryTimeout()),
                                                  eq(new String[]{ knownGbids[0] }));
        promise = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(1,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             1); // 1 local entry

        // without local entry, the global cap dir is called
        reset((Object) localDiscoveryEntryStoreMock);
        when(localDiscoveryEntryStoreMock.lookup(anyString(), anyLong())).thenReturn(Optional.empty());
        promise = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(2,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             1); // 1 global entry

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        when(globalDiscoveryEntryCacheMock.lookup(eq(domains),
                                                  eq(interfaceName1),
                                                  eq(discoveryQos.getCacheMaxAge()))).thenReturn(Arrays.asList(capInfo));
        promise = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(2,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             1); // 1 cached entry

        // and now, invalidate the existing cached global values, resulting in another call to globalcapclient
        discoveryQos.setCacheMaxAge(0L);
        Thread.sleep(1);

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        promise = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(3,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             1); // 1 global entry
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantId_localEntry() throws InterruptedException {
        String domain1 = "domain1";
        String interfaceName1 = "interfaceName1";
        String participantId1 = "participantId1";
        DiscoveryQos discoveryQos = new DiscoveryQos(Long.MAX_VALUE,
                                                     Long.MAX_VALUE,
                                                     DiscoveryScope.LOCAL_AND_GLOBAL,
                                                     false);

        // add local entry
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                           domain1,
                                                           interfaceName1,
                                                           participantId1,
                                                           providerQos,
                                                           System.currentTimeMillis(),
                                                           expiryDateMs,
                                                           publicKeyId);
        DiscoveryEntryWithMetaInfo expectedDiscoveryEntry = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                                discoveryEntry);
        when(localDiscoveryEntryStoreMock.lookup(eq(participantId1),
                                                 eq(discoveryQos.getCacheMaxAge()))).thenReturn(Optional.of(discoveryEntry));

        Promise<Lookup3Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId1);

        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo retrievedCapabilityEntry = (DiscoveryEntryWithMetaInfo) values[0];
        assertEquals(expectedDiscoveryEntry, retrievedCapabilityEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterface_localAndGlobal() throws InterruptedException {
        List<GlobalDiscoveryEntry> globalEntries = new ArrayList<GlobalDiscoveryEntry>();
        String domain1 = "domain1";
        String[] domains = new String[]{ domain1 };
        String interfaceName1 = "interfaceName1";
        DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.LOCAL_AND_GLOBAL, false);

        doAnswer(createLookupAnswer(globalEntries)).when(globalCapabilitiesDirectoryClient)
                                                   .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                           eq(new String[]{ domain1 }),
                                                           eq(interfaceName1),
                                                           eq(discoveryQos.getDiscoveryTimeout()),
                                                           eq(knownGbids));
        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(1,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             0);

        // add local entry
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                           domain1,
                                                           interfaceName1,
                                                           "localParticipant",
                                                           providerQos,
                                                           System.currentTimeMillis(),
                                                           expiryDateMs,
                                                           publicKeyId);
        when(localDiscoveryEntryStoreMock.lookup(eq(domains),
                                                 eq(interfaceName1))).thenReturn(Arrays.asList(discoveryEntry));
        promise = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(2,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             1); // 1 local entry

        // add global entry
        GlobalDiscoveryEntry capInfo = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                domain1,
                                                                interfaceName1,
                                                                "globalParticipant",
                                                                new ProviderQos(),
                                                                System.currentTimeMillis(),
                                                                expiryDateMs,
                                                                publicKeyId,
                                                                globalAddress1Serialized);
        globalEntries.add(capInfo);
        doAnswer(createLookupAnswer(globalEntries)).when(globalCapabilitiesDirectoryClient)
                                                   .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                           eq(domains),
                                                           eq(interfaceName1),
                                                           eq(discoveryQos.getDiscoveryTimeout()),
                                                           eq(knownGbids));
        promise = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(3,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             2); // 1 local, 1 global entry

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        when(globalDiscoveryEntryCacheMock.lookup(eq(domains),
                                                  eq(interfaceName1),
                                                  eq(discoveryQos.getCacheMaxAge()))).thenReturn(Arrays.asList(capInfo));
        promise = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(3,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             2); // 1 local, 1 cached entry

        // and now, invalidate the existing cached global values, resulting in another call to glocalcapclient
        discoveryQos.setCacheMaxAge(0L);
        Thread.sleep(1);

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        promise = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(4,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             2); // 1 local, 1 global entry
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterface_localAndGlobal_filtersDuplicates() throws InterruptedException {
        String domain = "domain";
        String[] domainsForLookup = new String[]{ domain };
        String interfaceName = "interfaceName";
        String participant = "participant";
        DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.LOCAL_AND_GLOBAL, false);

        // add same discovery entry to localCapabilitiesDirectory and cached GlobalCapabilitiesDirectory
        ProviderQos providerQos = new ProviderQos();
        long currentTime = System.currentTimeMillis();
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                           domain,
                                                           interfaceName,
                                                           participant,
                                                           providerQos,
                                                           currentTime,
                                                           expiryDateMs,
                                                           publicKeyId);

        when(localDiscoveryEntryStoreMock.lookup(eq(new String[]{ domain }),
                                                 eq(interfaceName))).thenReturn(Arrays.asList(discoveryEntry));

        GlobalDiscoveryEntry capInfo = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                domain,
                                                                interfaceName,
                                                                participant,
                                                                providerQos,
                                                                currentTime,
                                                                expiryDateMs,
                                                                publicKeyId,
                                                                globalAddress1Serialized);

        doReturn(Arrays.asList(capInfo)).when(globalDiscoveryEntryCacheMock)
                                        .lookup(eq(domainsForLookup),
                                                eq(interfaceName),
                                                eq(discoveryQos.getCacheMaxAge()));

        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domainsForLookup,
                                                                             interfaceName,
                                                                             discoveryQos);

        Object[] values = checkPromiseSuccess(promise, "lookup failed");
        DiscoveryEntryWithMetaInfo[] receivedValues = (DiscoveryEntryWithMetaInfo[]) values[0];
        assertEquals(1, receivedValues.length);
        assertTrue(Arrays.asList(receivedValues)
                         .contains(CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true, discoveryEntry)));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbids_globalOnly_filtersRemoteCachedEntriesByGbids() throws InterruptedException {
        String domain = "domain";
        String[] domainsForLookup = new String[]{ domain };
        String interfaceName = "interfaceName";
        DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.GLOBAL_ONLY, false);

        GlobalDiscoveryEntry cachedEntryForGbid1 = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                            domain,
                                                                            interfaceName,
                                                                            "participantId1",
                                                                            new ProviderQos(),
                                                                            System.currentTimeMillis(),
                                                                            expiryDateMs,
                                                                            publicKeyId,
                                                                            globalAddress1Serialized);
        GlobalDiscoveryEntry cachedEntryForGbid2 = new GlobalDiscoveryEntry(cachedEntryForGbid1);
        cachedEntryForGbid2.setParticipantId("participantId2");
        cachedEntryForGbid2.setAddress(globalAddress2Serialized);
        DiscoveryEntryWithMetaInfo expectedEntry1 = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                        cachedEntryForGbid1);
        DiscoveryEntryWithMetaInfo expectedEntry2 = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                        cachedEntryForGbid2);

        doReturn(Arrays.asList(cachedEntryForGbid1, cachedEntryForGbid2)).when(globalDiscoveryEntryCacheMock)
                                                                         .lookup(eq(domainsForLookup),
                                                                                 eq(interfaceName),
                                                                                 eq(discoveryQos.getCacheMaxAge()));

        Promise<Lookup2Deferred> promise1 = localCapabilitiesDirectory.lookup(domainsForLookup,
                                                                              interfaceName,
                                                                              discoveryQos,
                                                                              new String[]{ knownGbids[1] });

        DiscoveryEntryWithMetaInfo[] result1 = (DiscoveryEntryWithMetaInfo[]) checkPromiseSuccess(promise1,
                                                                                                  "lookup failed")[0];
        assertEquals(1, result1.length);
        assertEquals(expectedEntry2, result1[0]);

        Promise<Lookup2Deferred> promise2 = localCapabilitiesDirectory.lookup(domainsForLookup,
                                                                              interfaceName,
                                                                              discoveryQos,
                                                                              new String[]{ knownGbids[0] });

        DiscoveryEntryWithMetaInfo[] result2 = (DiscoveryEntryWithMetaInfo[]) checkPromiseSuccess(promise2,
                                                                                                  "lookup failed")[0];
        assertEquals(1, result2.length);
        assertEquals(expectedEntry1, result2[0]);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbids_globalOnly_filtersLocalCachedEntriesByGbids() throws InterruptedException {
        String domain = "domain";
        String[] domainsForLookup = new String[]{ domain };
        String interfaceName = "interfaceName";
        DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.GLOBAL_ONLY, false);

        DiscoveryEntry localEntry = new DiscoveryEntry(new Version(47, 11),
                                                       domain,
                                                       interfaceName,
                                                       "participantId1",
                                                       new ProviderQos(),
                                                       System.currentTimeMillis(),
                                                       expiryDateMs,
                                                       publicKeyId);
        GlobalDiscoveryEntry cachedEntry = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(localEntry,
                                                                                               globalAddressWithoutGbid);
        DiscoveryEntryWithMetaInfo expectedEntry = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                       cachedEntry);

        doReturn(Arrays.asList(cachedEntry)).when(globalDiscoveryEntryCacheMock)
                                            .lookup(eq(domainsForLookup),
                                                    eq(interfaceName),
                                                    eq(discoveryQos.getCacheMaxAge()));

        Promise<Add1Deferred> promiseAdd = localCapabilitiesDirectory.add(localEntry, true, knownGbids);
        checkPromiseSuccess(promiseAdd, "add failed");

        Promise<Lookup2Deferred> promiseLookup1 = localCapabilitiesDirectory.lookup(domainsForLookup,
                                                                                    interfaceName,
                                                                                    discoveryQos,
                                                                                    new String[]{ knownGbids[1] });

        DiscoveryEntryWithMetaInfo[] result1 = (DiscoveryEntryWithMetaInfo[]) checkPromiseSuccess(promiseLookup1,
                                                                                                  "lookup failed")[0];
        assertEquals(1, result1.length);
        assertEquals(expectedEntry, result1[0]);

        Promise<Lookup2Deferred> promiseLookup2 = localCapabilitiesDirectory.lookup(domainsForLookup,
                                                                                    interfaceName,
                                                                                    discoveryQos,
                                                                                    new String[]{ knownGbids[0] });

        DiscoveryEntryWithMetaInfo[] result2 = (DiscoveryEntryWithMetaInfo[]) checkPromiseSuccess(promiseLookup2,
                                                                                                  "lookup failed")[0];
        assertEquals(1, result2.length);
        assertEquals(expectedEntry, result2[0]);

        Promise<Lookup2Deferred> promiseLookup3 = localCapabilitiesDirectory.lookup(domainsForLookup,
                                                                                    interfaceName,
                                                                                    discoveryQos,
                                                                                    knownGbids);

        DiscoveryEntryWithMetaInfo[] result3 = (DiscoveryEntryWithMetaInfo[]) checkPromiseSuccess(promiseLookup3,
                                                                                                  "lookup failed")[0];
        assertEquals(1, result3.length);
        assertEquals(expectedEntry, result3[0]);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbids_globalOnly_filtersLocalCachedEntriesByGbids() throws InterruptedException {
        DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.GLOBAL_ONLY, false);

        DiscoveryEntry localEntry = new DiscoveryEntry(new Version(47, 11),
                                                       "domain",
                                                       "interfaceName",
                                                       "participantId1",
                                                       new ProviderQos(),
                                                       System.currentTimeMillis(),
                                                       expiryDateMs,
                                                       publicKeyId);
        GlobalDiscoveryEntry cachedEntry = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(localEntry,
                                                                                               globalAddressWithoutGbid);
        DiscoveryEntryWithMetaInfo expectedEntry = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                       cachedEntry);

        doReturn(Optional.of(cachedEntry)).when(globalDiscoveryEntryCacheMock)
                                          .lookup(eq(expectedEntry.getParticipantId()), eq(30000L));

        Promise<Add1Deferred> promiseAdd = localCapabilitiesDirectory.add(localEntry, true, knownGbids);
        checkPromiseSuccess(promiseAdd, "add failed");

        Promise<Lookup4Deferred> promiseLookup1 = localCapabilitiesDirectory.lookup(expectedEntry.getParticipantId(),
                                                                                    discoveryQos,
                                                                                    new String[]{ knownGbids[1] });

        verify(globalDiscoveryEntryCacheMock).lookup(eq(expectedEntry.getParticipantId()), eq(30000L));
        DiscoveryEntryWithMetaInfo result1 = (DiscoveryEntryWithMetaInfo) checkPromiseSuccess(promiseLookup1,
                                                                                              "lookup failed")[0];
        assertEquals(expectedEntry, result1);

        Promise<Lookup4Deferred> promiseLookup2 = localCapabilitiesDirectory.lookup(expectedEntry.getParticipantId(),
                                                                                    discoveryQos,
                                                                                    new String[]{ knownGbids[0] });

        DiscoveryEntryWithMetaInfo result2 = (DiscoveryEntryWithMetaInfo) checkPromiseSuccess(promiseLookup2,
                                                                                              "lookup failed")[0];
        assertEquals(expectedEntry, result2);

        Promise<Lookup4Deferred> promiseLookup3 = localCapabilitiesDirectory.lookup(expectedEntry.getParticipantId(),
                                                                                    discoveryQos,
                                                                                    knownGbids);

        DiscoveryEntryWithMetaInfo result3 = (DiscoveryEntryWithMetaInfo) checkPromiseSuccess(promiseLookup3,
                                                                                              "lookup failed")[0];
        assertEquals(expectedEntry, result3);
    }

    private void testLookupByDomainInterfaceWithGbids_globalOnly_allCached(String[] gbidsForLookup,
                                                                           DiscoveryEntry entryForGbid1,
                                                                           DiscoveryEntry entryForGbid2,
                                                                           DiscoveryEntry entryForGbid3,
                                                                           DiscoveryEntry entryForGbid2And3,
                                                                           Set<String> expectedParticipantIds,
                                                                           int expectedResultSize) throws InterruptedException {
        String[] domainsForLookup = new String[]{ discoveryEntry.getDomain() };
        DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.GLOBAL_ONLY, false);
        final boolean awaitGlobalRegistration = true;
        Promise<Add1Deferred> promise1 = localCapabilitiesDirectory.add(entryForGbid1,
                                                                        awaitGlobalRegistration,
                                                                        new String[]{ knownGbids[0] });
        checkPromiseSuccess(promise1, "add failed");
        Promise<Add1Deferred> promise2 = localCapabilitiesDirectory.add(entryForGbid2,
                                                                        awaitGlobalRegistration,
                                                                        new String[]{ knownGbids[1] });
        checkPromiseSuccess(promise2, "add failed");
        Promise<Add1Deferred> promise3 = localCapabilitiesDirectory.add(entryForGbid3,
                                                                        awaitGlobalRegistration,
                                                                        new String[]{ knownGbids[2] });
        checkPromiseSuccess(promise3, "add failed");
        Promise<Add1Deferred> promise4 = localCapabilitiesDirectory.add(entryForGbid2And3,
                                                                        awaitGlobalRegistration,
                                                                        new String[]{ knownGbids[1], knownGbids[2] });
        checkPromiseSuccess(promise4, "add failed");

        List<GlobalDiscoveryEntry> cachedEntries = new ArrayList<>(4);
        cachedEntries.add(CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(entryForGbid1, globalAddressWithoutGbid));
        cachedEntries.add(CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(entryForGbid2, globalAddressWithoutGbid));
        cachedEntries.add(CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(entryForGbid3, globalAddressWithoutGbid));
        cachedEntries.add(CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(entryForGbid2And3,
                                                                              globalAddressWithoutGbid));
        doReturn(cachedEntries).when(globalDiscoveryEntryCacheMock)
                               .lookup(eq(domainsForLookup), eq(INTERFACE_NAME), anyLong());

        Promise<Lookup2Deferred> lookupPromise = localCapabilitiesDirectory.lookup(domainsForLookup,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos,
                                                                                   gbidsForLookup);
        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo[] foundEntries = (DiscoveryEntryWithMetaInfo[]) values[0];
        assertEquals(expectedResultSize, foundEntries.length);

        Set<String> foundParticipantIds = new HashSet<>();
        for (DiscoveryEntryWithMetaInfo foundEntry : foundEntries) {
            foundParticipantIds.add(foundEntry.getParticipantId());
        }
        assertEquals(expectedParticipantIds, foundParticipantIds);

        verify(globalCapabilitiesDirectoryClient,
               times(0)).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                any(String[].class),
                                anyString(),
                                anyLong(),
                                any(String[].class));
    }

    private void testLookupByDomainInterfaceWithGbids_globalOnly_noneCached(String[] gbidsForLookup,
                                                                            String[] expectedGbids) throws InterruptedException {
        String[] domainsForLookup = new String[]{ discoveryEntry.getDomain() };
        String[] expectedDomains = domainsForLookup.clone();
        DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.GLOBAL_ONLY, false);

        List<GlobalDiscoveryEntry> globalEntries = new ArrayList<>(4);
        globalEntries.add(globalDiscoveryEntry);
        DiscoveryEntry entry2 = new DiscoveryEntry(discoveryEntry);
        entry2.setParticipantId("participantId2");
        globalEntries.add(CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(entry2, globalAddressWithoutGbid));

        doReturn(new ArrayList<GlobalDiscoveryEntry>()).when(globalDiscoveryEntryCacheMock)
                                                       .lookup(eq(expectedDomains), eq(INTERFACE_NAME), anyLong());
        doAnswer(createLookupAnswer(globalEntries)).when(globalCapabilitiesDirectoryClient)
                                                   .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                           eq(expectedDomains),
                                                           eq(INTERFACE_NAME),
                                                           eq(discoveryQos.getDiscoveryTimeout()),
                                                           eq(expectedGbids));

        Promise<Lookup2Deferred> lookupPromise = localCapabilitiesDirectory.lookup(domainsForLookup,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos,
                                                                                   gbidsForLookup);

        verify(globalCapabilitiesDirectoryClient).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                         eq(expectedDomains),
                                                         eq(INTERFACE_NAME),
                                                         eq(discoveryQos.getDiscoveryTimeout()),
                                                         eq(expectedGbids));
        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo[] foundEntries = (DiscoveryEntryWithMetaInfo[]) values[0];
        assertEquals(2, foundEntries.length);
    }

    @Test
    public void testLookupByDomainInterfaceWithGbids_globalOnly_multipleGbids_allCached() throws InterruptedException {
        String[] gbidsForLookup = new String[]{ knownGbids[0], knownGbids[2] };

        DiscoveryEntry entryForGbid1 = new DiscoveryEntry(discoveryEntry);
        DiscoveryEntry entryForGbid2 = new DiscoveryEntry(discoveryEntry);
        entryForGbid2.setParticipantId("participantId2");
        DiscoveryEntry entryForGbid3 = new DiscoveryEntry(discoveryEntry);
        entryForGbid3.setParticipantId("participantId3");
        DiscoveryEntry entryForGbid2And3 = new DiscoveryEntry(discoveryEntry);
        entryForGbid2And3.setParticipantId("participantId4");

        Set<String> expectedParticipantIds = new HashSet<>();
        expectedParticipantIds.add(entryForGbid1.getParticipantId());
        expectedParticipantIds.add(entryForGbid3.getParticipantId());
        expectedParticipantIds.add(entryForGbid2And3.getParticipantId());

        testLookupByDomainInterfaceWithGbids_globalOnly_allCached(gbidsForLookup,
                                                                  entryForGbid1,
                                                                  entryForGbid2,
                                                                  entryForGbid3,
                                                                  entryForGbid2And3,
                                                                  expectedParticipantIds,
                                                                  3);
    }

    @Test
    public void testLookupByDomainInterfaceWithGbids_globalOnly_emptyGbidsArray_allCached() throws InterruptedException {
        String[] gbidsForLookup = new String[0];

        DiscoveryEntry entryForGbid1 = new DiscoveryEntry(discoveryEntry);
        DiscoveryEntry entryForGbid2 = new DiscoveryEntry(discoveryEntry);
        entryForGbid2.setParticipantId("participantId2");
        DiscoveryEntry entryForGbid3 = new DiscoveryEntry(discoveryEntry);
        entryForGbid3.setParticipantId("participantId3");
        DiscoveryEntry entryForGbid2And3 = new DiscoveryEntry(discoveryEntry);
        entryForGbid2And3.setParticipantId("participantId4");

        Set<String> expectedParticipantIds = new HashSet<>();
        expectedParticipantIds.add(entryForGbid1.getParticipantId());
        expectedParticipantIds.add(entryForGbid2.getParticipantId());
        expectedParticipantIds.add(entryForGbid3.getParticipantId());
        expectedParticipantIds.add(entryForGbid2And3.getParticipantId());

        testLookupByDomainInterfaceWithGbids_globalOnly_allCached(gbidsForLookup,
                                                                  entryForGbid1,
                                                                  entryForGbid2,
                                                                  entryForGbid3,
                                                                  entryForGbid2And3,
                                                                  expectedParticipantIds,
                                                                  4);
    }

    @Test
    public void testLookupByDomainInterfaceWithGbids_globalOnly_multipleGbids_noneCached() throws InterruptedException {
        String[] gbidsForLookup = new String[]{ knownGbids[0], knownGbids[2] };

        testLookupByDomainInterfaceWithGbids_globalOnly_noneCached(gbidsForLookup, gbidsForLookup.clone());

        String[] gbidsForLookup2 = new String[]{ knownGbids[2], knownGbids[0] };

        testLookupByDomainInterfaceWithGbids_globalOnly_noneCached(gbidsForLookup2, gbidsForLookup2.clone());
    }

    @Test
    public void testLookupByDomainInterfaceWithGbids_globalOnly_emptyGbidsArray_noneCached() throws InterruptedException {
        String[] gbidsForLookup = new String[0];

        testLookupByDomainInterfaceWithGbids_globalOnly_noneCached(gbidsForLookup, knownGbids);
    }

    private void testLookupByParticipantIdWithGbids_globalOnly_allCached(String[] gbidsForLookup) throws InterruptedException {
        String participantId = discoveryEntry.getParticipantId();
        DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.GLOBAL_ONLY, false);

        doReturn(Optional.of(globalDiscoveryEntry)).when(globalDiscoveryEntryCacheMock)
                                                   .lookup(eq(participantId), anyLong());

        Promise<Lookup4Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId,
                                                                                   discoveryQos,
                                                                                   gbidsForLookup);
        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo foundEntry = (DiscoveryEntryWithMetaInfo) values[0];

        assertEquals(participantId, foundEntry.getParticipantId());

        verify(globalCapabilitiesDirectoryClient,
               times(0)).lookup(Matchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                                anyString(),
                                anyLong(),
                                any(String[].class));
    }

    private void testLookupByParticipantIdWithGbids_globalOnly_noneCached(String[] gbidsForLookup,
                                                                          String[] expectedGbids) throws InterruptedException {
        String participantId = discoveryEntry.getParticipantId();
        DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.GLOBAL_ONLY, false);

        doReturn(Optional.empty()).when(globalDiscoveryEntryCacheMock).lookup(eq(participantId), anyLong());
        doAnswer(createLookupAnswer(globalDiscoveryEntry)).when(globalCapabilitiesDirectoryClient)
                                                          .lookup(Matchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                                                                  eq(participantId),
                                                                  eq(discoveryQos.getDiscoveryTimeout()),
                                                                  eq(expectedGbids));

        Promise<Lookup4Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId,
                                                                                   discoveryQos,
                                                                                   gbidsForLookup);

        verify(globalCapabilitiesDirectoryClient).lookup(Matchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                                                         eq(participantId),
                                                         eq(discoveryQos.getDiscoveryTimeout()),
                                                         eq(expectedGbids));
        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo foundEntry = (DiscoveryEntryWithMetaInfo) values[0];
        assertEquals(participantId, foundEntry.getParticipantId());
    }

    @Test
    public void testLookupByParticipantIdWithGbids_globalOnly_multipleGbids_allCached() throws InterruptedException {
        String[] gbidsForLookup = new String[]{ knownGbids[0], knownGbids[2] };

        testLookupByParticipantIdWithGbids_globalOnly_allCached(gbidsForLookup);
    }

    @Test
    public void testLookupByParticipantIdWithGbids_globalOnly_emptyGbidsArray_allCached() throws InterruptedException {
        String[] gbidsForLookup = new String[0];

        testLookupByParticipantIdWithGbids_globalOnly_allCached(gbidsForLookup);
    }

    @Test
    public void testLookupByParticipantIdWithGbids_globalOnly_multipleGbids_noneCached() throws InterruptedException {
        String[] gbidsForLookup = new String[]{ knownGbids[0], knownGbids[2] };

        testLookupByParticipantIdWithGbids_globalOnly_noneCached(gbidsForLookup, gbidsForLookup.clone());

        String[] gbidsForLookup2 = new String[]{ knownGbids[2], knownGbids[0] };

        testLookupByParticipantIdWithGbids_globalOnly_noneCached(gbidsForLookup2, gbidsForLookup2.clone());
    }

    @Test
    public void testLookupByParticipantIdWithGbids_globalOnly_emptyGbidsArray_noneCached() throws InterruptedException {
        String[] gbidsForLookup = new String[0];

        testLookupByParticipantIdWithGbids_globalOnly_noneCached(gbidsForLookup, knownGbids);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupMultipleDomains_localOnly() throws InterruptedException {
        String[] domains = new String[]{ "domain1", "domain2" };
        String interfaceName = "interface1";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);

        Collection<DiscoveryEntry> entries = Arrays.asList(new DiscoveryEntry(new Version(0, 0),
                                                                              "domain1",
                                                                              interfaceName,
                                                                              "participantId1",
                                                                              new ProviderQos(),
                                                                              System.currentTimeMillis(),
                                                                              expiryDateMs,
                                                                              interfaceName),
                                                           new DiscoveryEntry(new Version(0, 0),
                                                                              "domain2",
                                                                              interfaceName,
                                                                              "participantId2",
                                                                              new ProviderQos(),
                                                                              System.currentTimeMillis(),
                                                                              expiryDateMs,
                                                                              interfaceName));
        when(localDiscoveryEntryStoreMock.lookup(eq(domains), eq(interfaceName))).thenReturn(entries);

        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos);

        Object[] values = checkPromiseSuccess(promise, "lookup failed");
        assertEquals(2, ((DiscoveryEntryWithMetaInfo[]) values[0]).length);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupMultipleDomains_globalOnly_noneCached() throws InterruptedException {
        String[] domains = new String[]{ "domain1", "domain2" };
        String interfaceName = "interface1";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        when(globalDiscoveryEntryCacheMock.lookup(eq(domains),
                                                  eq(interfaceName),
                                                  eq(discoveryQos.getCacheMaxAge()))).thenReturn(new ArrayList<GlobalDiscoveryEntry>());
        doAnswer(createLookupAnswer(new ArrayList<GlobalDiscoveryEntry>())).when(globalCapabilitiesDirectoryClient)
                                                                           .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                                   argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
                                                                                   eq(interfaceName),
                                                                                   eq(discoveryQos.getDiscoveryTimeout()),
                                                                                   eq(knownGbids));

        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos);

        verifyGcdLookupAndPromiseFulfillment(1,
                                             domains,
                                             interfaceName,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             0);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupMultipleDomains_globalOnly_allCached() throws InterruptedException {
        String[] domains = new String[]{ "domain1", "domain2" };
        String interfaceName = "interface1";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        List<GlobalDiscoveryEntry> entries = new ArrayList<>();
        List<Promise<Add1Deferred>> promises = new ArrayList<>();
        for (String domain : domains) {
            GlobalDiscoveryEntry entry = new GlobalDiscoveryEntry();
            entry.setParticipantId("participantIdFor-" + domain);
            entry.setDomain(domain);
            entries.add(entry);
            promises.add(localCapabilitiesDirectory.add(entry, true, knownGbids));
        }
        promises.forEach(promise -> {
            try {
                checkPromiseSuccess(promise, "addFailed");
            } catch (InterruptedException e) {
                fail("add failed: " + e);
            }
        });

        when(globalDiscoveryEntryCacheMock.lookup(eq(domains),
                                                  eq(interfaceName),
                                                  eq(discoveryQos.getCacheMaxAge()))).thenReturn(entries);

        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos);

        verifyGcdLookupAndPromiseFulfillment(0,
                                             domains,
                                             interfaceName,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             2); // 2 cached entries
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupMultipleDomains_globalOnly_oneCached() throws InterruptedException {
        String[] domains = new String[]{ "domain1", "domain2" };
        String interfaceName = "interface1";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setCacheMaxAge(ONE_DAY_IN_MS);

        GlobalDiscoveryEntry entry = new GlobalDiscoveryEntry();
        entry.setParticipantId("participantId1");
        entry.setInterfaceName(interfaceName);
        entry.setDomain(domains[0]);
        entry.setAddress(globalAddress1Serialized);
        doReturn(Arrays.asList(entry)).when(globalDiscoveryEntryCacheMock)
                                      .lookup(eq(domains), eq(interfaceName), eq(discoveryQos.getCacheMaxAge()));
        doAnswer(createLookupAnswer(new ArrayList<GlobalDiscoveryEntry>())).when(globalCapabilitiesDirectoryClient)
                                                                           .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                                   argThat(org.hamcrest.Matchers.arrayContaining(domains[1])),
                                                                                   eq(interfaceName),
                                                                                   eq(discoveryQos.getDiscoveryTimeout()),
                                                                                   eq(knownGbids));

        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos);

        verifyGcdLookupAndPromiseFulfillment(1,
                                             new String[]{ domains[1] },
                                             interfaceName,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             1);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupMultipleDomains_localThenGlobal_oneLocalGlobalOneCached_sameParticipantIdsRemote() throws InterruptedException {
        String localDomain = "localDomain";
        String cachedDomain = "cachedDomain";
        String remoteDomain = "remoteDomain";
        String[] domains = new String[]{ localDomain, cachedDomain, remoteDomain };
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);
        discoveryQos.setCacheMaxAge(ONE_DAY_IN_MS);

        // local entry for participantId1 and domain1
        discoveryEntry.setParticipantId("participantId1");
        discoveryEntry.setDomain(localDomain);
        doReturn(Arrays.asList(discoveryEntry)).when(localDiscoveryEntryStoreMock)
                                               .lookup(argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
                                                       eq(INTERFACE_NAME));

        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock)
                                             .lookup(eq(discoveryEntry.getParticipantId()), anyLong());

        // cached entry for local provider and cached entry for participantId2 for cachedDomain
        GlobalDiscoveryEntry cachedLocalEntry = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry,
                                                                                                    globalAddressWithoutGbid);
        GlobalDiscoveryEntry cachedRemoteEntry = new GlobalDiscoveryEntry();
        cachedRemoteEntry.setParticipantId("participantId2");
        cachedRemoteEntry.setInterfaceName(INTERFACE_NAME);
        cachedRemoteEntry.setDomain(cachedDomain);
        cachedRemoteEntry.setAddress(globalAddress1Serialized);
        doReturn(Arrays.asList(cachedLocalEntry,
                               cachedRemoteEntry)).when(globalDiscoveryEntryCacheMock)
                                                  .lookup(argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
                                                          eq(INTERFACE_NAME),
                                                          eq(discoveryQos.getCacheMaxAge()));

        // remote entries for local provider and for remoteDomain for participantIds 2 and 3
        GlobalDiscoveryEntry remoteEntry1 = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry,
                                                                                                globalAddressWithoutGbid);
        remoteEntry1.setDomain(remoteDomain);
        GlobalDiscoveryEntry remoteEntry2 = new GlobalDiscoveryEntry(cachedRemoteEntry);
        remoteEntry2.setDomain(remoteDomain);
        remoteEntry2.setAddress(globalAddressWithoutGbidSerialized);
        GlobalDiscoveryEntry remoteEntry3 = new GlobalDiscoveryEntry(cachedRemoteEntry);
        remoteEntry3.setParticipantId("participantId3");
        remoteEntry3.setDomain(remoteDomain);
        remoteEntry3.setAddress(globalAddressWithoutGbidSerialized);
        doAnswer(createLookupAnswer(Arrays.asList(remoteEntry1,
                                                  remoteEntry2,
                                                  remoteEntry3))).when(globalCapabilitiesDirectoryClient)
                                                                 .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                         eq(new String[]{ remoteDomain }),
                                                                         eq(INTERFACE_NAME),
                                                                         eq(discoveryQos.getDiscoveryTimeout()),
                                                                         eq(knownGbids));

        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, INTERFACE_NAME, discoveryQos);

        verify(localDiscoveryEntryStoreMock).lookup(argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
                                                    eq(INTERFACE_NAME));
        verify(globalDiscoveryEntryCacheMock).lookup(argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
                                                     eq(INTERFACE_NAME),
                                                     eq(discoveryQos.getCacheMaxAge()));
        verify(globalCapabilitiesDirectoryClient).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                         eq(new String[]{ remoteDomain }),
                                                         eq(INTERFACE_NAME),
                                                         eq(discoveryQos.getDiscoveryTimeout()),
                                                         eq(knownGbids));
        Object[] values = verifyGcdLookupAndPromiseFulfillment(1,
                                                               new String[]{ remoteDomain },
                                                               INTERFACE_NAME,
                                                               discoveryQos.getDiscoveryTimeout(),
                                                               knownGbids,
                                                               promise,
                                                               3);
        DiscoveryEntryWithMetaInfo[] result = (DiscoveryEntryWithMetaInfo[]) values[0];
        assertEquals(3, result.length);
        boolean discoveryEntryFound = false;
        boolean remoteEntry2Found = false;
        boolean remoteEntry3Found = false;
        for (DiscoveryEntryWithMetaInfo entry : result) {
            if (entry.getParticipantId() == discoveryEntry.getParticipantId() && entry.getDomain().equals(localDomain)
                    && entry.getIsLocal()) {
                discoveryEntryFound = true;
            }
            if (entry.getParticipantId() == remoteEntry2.getParticipantId() && entry.getDomain().equals(remoteDomain)
                    && !entry.getIsLocal()) {
                remoteEntry2Found = true;
            }
            if (entry.getParticipantId() == remoteEntry3.getParticipantId() && entry.getDomain().equals(remoteDomain)
                    && !entry.getIsLocal()) {
                remoteEntry3Found = true;
            }
        }
        verify(globalDiscoveryEntryCacheMock, never()).add(remoteEntry1);
        verify(globalDiscoveryEntryCacheMock).add(remoteEntry2);
        verify(globalDiscoveryEntryCacheMock).add(remoteEntry3);
        verify(messageRouter, never()).addToRoutingTable(eq(remoteEntry1.getParticipantId()),
                                                         eq(globalAddressWithoutGbid),
                                                         eq(true),
                                                         anyLong());
        verify(messageRouter).addToRoutingTable(eq(remoteEntry2.getParticipantId()),
                                                eq(globalAddressWithoutGbid),
                                                eq(true),
                                                anyLong());
        verify(messageRouter).addToRoutingTable(eq(remoteEntry3.getParticipantId()),
                                                eq(globalAddressWithoutGbid),
                                                eq(true),
                                                anyLong());
        assertTrue(discoveryEntryFound && remoteEntry2Found && remoteEntry3Found);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterfaceGbids_localAndGlobal_localGlobalEntry_invokesGcd_filtersCombinedResult() throws Exception {
        String[] domains = { discoveryEntry.getDomain() };
        List<DiscoveryEntry> localDiscoveryEntries = Arrays.asList(discoveryEntry);
        List<GlobalDiscoveryEntry> globalDiscoveryEntries = Arrays.asList(globalDiscoveryEntry);
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        DiscoveryQos discoveryQos = new DiscoveryQos(cacheMaxAge,
                                                     discoveryTimeout,
                                                     DiscoveryScope.LOCAL_AND_GLOBAL,
                                                     false);
        when(localDiscoveryEntryStoreMock.lookup(eq(domains), eq(INTERFACE_NAME))).thenReturn(localDiscoveryEntries);
        doAnswer(createLookupAnswer(globalDiscoveryEntries)).when(globalCapabilitiesDirectoryClient)
                                                            .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                    Mockito.<String[]> any(),
                                                                    anyString(),
                                                                    anyLong(),
                                                                    any());

        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock)
                                             .lookup(eq(discoveryEntry.getParticipantId()), anyLong());

        Promise<Lookup2Deferred> lookupPromise = localCapabilitiesDirectory.lookup(domains,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos,
                                                                                   new String[0]);

        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo[] capturedDiscoveryEntries = (DiscoveryEntryWithMetaInfo[]) values[0];
        assertEquals(1, capturedDiscoveryEntries.length);
        assertTrue(capturedDiscoveryEntries[0].getIsLocal());
        verify(globalCapabilitiesDirectoryClient).lookup(any(),
                                                         eq(domains),
                                                         eq(INTERFACE_NAME),
                                                         eq(discoveryTimeout),
                                                         any());
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(messageRouter, never()).addToRoutingTable(anyString(),
                                                         any(Address.class),
                                                         any(Boolean.class),
                                                         anyLong());
        verify(localDiscoveryEntryStoreMock).lookup(eq(domains), eq(INTERFACE_NAME));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterfaceGbids_globalOnly_localGlobalEntry_invokesGcd_ignoresGlobalDuplicate() throws Exception {
        String[] domains = { discoveryEntry.getDomain() };
        List<GlobalDiscoveryEntry> globalDiscoveryEntries = Arrays.asList(globalDiscoveryEntry);
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        DiscoveryQos discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, DiscoveryScope.GLOBAL_ONLY, false);
        doAnswer(createLookupAnswer(globalDiscoveryEntries)).when(globalCapabilitiesDirectoryClient)
                                                            .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                    Mockito.<String[]> any(),
                                                                    anyString(),
                                                                    anyLong(),
                                                                    any());

        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock)
                                             .lookup(eq(discoveryEntry.getParticipantId()), anyLong());

        Promise<Lookup2Deferred> lookupPromise = localCapabilitiesDirectory.lookup(domains,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos,
                                                                                   new String[0]);

        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo[] capturedDiscoveryEntries = (DiscoveryEntryWithMetaInfo[]) values[0];
        assertEquals(1, capturedDiscoveryEntries.length);
        assertEquals(CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true, discoveryEntry),
                     capturedDiscoveryEntries[0]);
        verify(globalCapabilitiesDirectoryClient).lookup(any(),
                                                         eq(domains),
                                                         eq(INTERFACE_NAME),
                                                         eq(discoveryTimeout),
                                                         any());
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(messageRouter, never()).addToRoutingTable(anyString(),
                                                         any(Address.class),
                                                         any(Boolean.class),
                                                         anyLong());
        verify(localDiscoveryEntryStoreMock).lookup(eq(discoveryEntry.getParticipantId()), anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdGbids_globalOnlyWithCache_localGlobalEntry_invokesGcd_ignoresGlobalDuplicate() throws Exception {
        lookupByParticipantIdGbids_globalOnly_localGlobalEntry_invokesGcd_ignoresGlobalDuplicate(10000L);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdGbids_globalOnlyNoCache_localGlobalEntry_invokesGcd_ignoresGlobalDuplicate() throws Exception {
        lookupByParticipantIdGbids_globalOnly_localGlobalEntry_invokesGcd_ignoresGlobalDuplicate(0L);
    }

    public void lookupByParticipantIdGbids_globalOnly_localGlobalEntry_invokesGcd_ignoresGlobalDuplicate(long cacheMaxAge) throws Exception {
        final long discoveryTimeout = 5000L;

        DiscoveryQos discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, DiscoveryScope.GLOBAL_ONLY, false);
        doAnswer(createLookupAnswer(globalDiscoveryEntry)).when(globalCapabilitiesDirectoryClient)
                                                          .lookup(Matchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                                                                  anyString(),
                                                                  anyLong(),
                                                                  any());

        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock)
                                             .lookup(eq(discoveryEntry.getParticipantId()), anyLong());

        Promise<Lookup4Deferred> lookupPromise = localCapabilitiesDirectory.lookup(discoveryEntry.getParticipantId(),
                                                                                   discoveryQos,
                                                                                   new String[0]);

        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo capturedDiscoveryEntry = (DiscoveryEntryWithMetaInfo) values[0];
        assertTrue(capturedDiscoveryEntry.getIsLocal());
        verify(globalCapabilitiesDirectoryClient).lookup(any(),
                                                         eq(discoveryEntry.getParticipantId()),
                                                         eq(discoveryTimeout),
                                                         any());
        verify(localDiscoveryEntryStoreMock, times(2)).lookup(eq(discoveryEntry.getParticipantId()), anyLong());
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(messageRouter, never()).addToRoutingTable(anyString(),
                                                         any(Address.class),
                                                         any(Boolean.class),
                                                         anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupMultipleDomains_localThenGlobal() throws InterruptedException {
        String[] domains = new String[]{ "domain1", "domain2", "domain3" };
        String interfaceName = "interface1";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);
        discoveryQos.setCacheMaxAge(ONE_DAY_IN_MS);

        DiscoveryEntry localEntry = new DiscoveryEntry();
        localEntry.setParticipantId("participantIdLocal");
        localEntry.setDomain(domains[0]);
        when(localDiscoveryEntryStoreMock.lookup(eq(domains), eq(interfaceName))).thenReturn(Arrays.asList(localEntry));

        GlobalDiscoveryEntry globalEntry = new GlobalDiscoveryEntry();
        globalEntry.setParticipantId("participantIdCached");
        globalEntry.setInterfaceName(interfaceName);
        globalEntry.setDomain(domains[1]);
        globalEntry.setAddress(globalAddress1Serialized);
        doReturn(Arrays.asList(globalEntry)).when(globalDiscoveryEntryCacheMock)
                                            .lookup(eq(domains), eq(interfaceName), eq(discoveryQos.getCacheMaxAge()));

        final GlobalDiscoveryEntry remoteGlobalEntry = new GlobalDiscoveryEntry(new Version(0, 0),
                                                                                domains[2],
                                                                                interfaceName,
                                                                                "participantIdRemote",
                                                                                new ProviderQos(),
                                                                                System.currentTimeMillis(),
                                                                                System.currentTimeMillis() + 10000L,
                                                                                "publicKeyId",
                                                                                globalAddress1Serialized);

        doAnswer(createLookupAnswer(Arrays.asList(remoteGlobalEntry))).when(globalCapabilitiesDirectoryClient)
                                                                      .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                              Mockito.<String[]> any(),
                                                                              anyString(),
                                                                              anyLong(),
                                                                              eq(knownGbids));

        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos);

        verify(globalCapabilitiesDirectoryClient).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                         eq(new String[]{ "domain3" }),
                                                         eq(interfaceName),
                                                         eq(discoveryQos.getDiscoveryTimeout()),
                                                         eq(knownGbids));
        Object[] values = checkPromiseSuccess(promise, "lookup failed");
        Collection<DiscoveryEntry> captured = CapabilityUtils.convertToDiscoveryEntrySet(Arrays.asList((DiscoveryEntryWithMetaInfo[]) values[0]));
        assertNotNull(captured);
        assertEquals(3, captured.size());
        assertTrue(captured.contains(localEntry));
        assertTrue(captured.contains(new DiscoveryEntry(globalEntry)));
        assertTrue(captured.contains(new DiscoveryEntry(remoteGlobalEntry)));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantId_DiscoveryEntryWithMetaInfoContainsExpectedIsLocalValue_localEntry() throws Exception {
        String participantId = "participantId";
        String interfaceName = "interfaceName";
        DiscoveryQos discoveryQos = new DiscoveryQos(Long.MAX_VALUE, Long.MAX_VALUE, DiscoveryScope.LOCAL_ONLY, false);

        // local DiscoveryEntry
        String localDomain = "localDomain";
        DiscoveryEntry localEntry = new DiscoveryEntry();
        localEntry.setDomain(localDomain);
        localEntry.setInterfaceName(interfaceName);
        localEntry.setParticipantId(participantId);
        DiscoveryEntryWithMetaInfo localEntryWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                                localEntry);
        when(localDiscoveryEntryStoreMock.lookup(eq(participantId),
                                                 eq(discoveryQos.getCacheMaxAge()))).thenReturn(Optional.of(localEntry));

        Promise<Lookup3Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId);

        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo capturedLocalEntry = (DiscoveryEntryWithMetaInfo) values[0];
        assertEquals(localEntryWithMetaInfo, capturedLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantId_DiscoveryEntryWithMetaInfoContainsExpectedIsLocalValue_cachedEntry() throws Exception {
        String participantId = discoveryEntry.getParticipantId();
        String interfaceName = "interfaceName";

        // cached global DiscoveryEntry
        String globalDomain = "globalDomain";
        GlobalDiscoveryEntry cachedGlobalEntry = new GlobalDiscoveryEntry();
        cachedGlobalEntry.setDomain(globalDomain);
        cachedGlobalEntry.setInterfaceName(interfaceName);
        cachedGlobalEntry.setParticipantId(participantId);
        cachedGlobalEntry.setAddress(globalAddress1Serialized);
        DiscoveryEntryWithMetaInfo cachedGlobalEntryWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                                       cachedGlobalEntry);
        when(globalDiscoveryEntryCacheMock.lookup(eq(participantId),
                                                  eq(Long.MAX_VALUE))).thenReturn(Optional.of(cachedGlobalEntry));
        Promise<Lookup3Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId);

        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo capturedCachedGlobalEntry = (DiscoveryEntryWithMetaInfo) values[0];
        assertEquals(cachedGlobalEntryWithMetaInfo, capturedCachedGlobalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_localOnly_noLocalEntry_doesNotInvokeGcd_returnsNoEntryForParticipant() throws Exception {
        String participantId = discoveryEntry.getParticipantId();
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        DiscoveryQos discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, DiscoveryScope.LOCAL_ONLY, false);

        Promise<Lookup4Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId,
                                                                                   discoveryQos,
                                                                                   new String[0]);

        checkPromiseError(lookupPromise, DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        verify(globalCapabilitiesDirectoryClient,
               never()).lookup(Matchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                               anyString(),
                               anyLong(),
                               any());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_localOnly_localEntry_doesNotInvokeGcd_returnsLocalEntry() throws Exception {
        DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_ONLY;
        boolean localEntryAvailable = true;
        boolean invokesGcd = false;
        boolean returnsLocalEntry = true;
        lookupByParticipantIdDiscoveryScopeTest(discoveryScope, localEntryAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_localThenGlobal_noLocalEntry_invokesGcd_returnsRemoteResult() throws Exception {
        DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_THEN_GLOBAL;
        boolean localEntryAvailable = false;
        boolean invokesGcd = true;
        boolean returnsLocalEntry = false;
        lookupByParticipantIdDiscoveryScopeTest(discoveryScope, localEntryAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_localThenGlobal_localEntry_doesNotInvokeGcd_returnsLocalEntry() throws Exception {
        DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_THEN_GLOBAL;
        boolean localEntryAvailable = true;
        boolean invokesGcd = false;
        boolean returnsLocalEntry = true;
        lookupByParticipantIdDiscoveryScopeTest(discoveryScope, localEntryAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_localAndGlobal_noLocalEntry_invokesGcd_returnsRemoteResult() throws Exception {
        DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_AND_GLOBAL;
        boolean localEntryAvailable = false;
        boolean invokesGcd = true;
        boolean returnsLocalEntry = false;
        lookupByParticipantIdDiscoveryScopeTest(discoveryScope, localEntryAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_localAndGlobal_localEntry_doesNotInvokeGcd_returnsLocalEntry() throws Exception {
        DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_AND_GLOBAL;
        boolean localEntryAvailable = true;
        boolean invokesGcd = false;
        boolean returnsLocalEntry = true;
        lookupByParticipantIdDiscoveryScopeTest(discoveryScope, localEntryAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_globalOnly_localEntry_invokesGcd_returnsLocalResult() throws Exception {
        DiscoveryScope discoveryScope = DiscoveryScope.GLOBAL_ONLY;
        boolean localEntryAvailable = true;
        boolean invokesGcd = true;
        boolean returnsLocalEntry = true;
        lookupByParticipantIdDiscoveryScopeTest(discoveryScope, localEntryAvailable, invokesGcd, returnsLocalEntry);
    }

    private void lookupByParticipantIdDiscoveryScopeTest(DiscoveryScope discoveryScope,
                                                         boolean localEntryAvalaible,
                                                         boolean invokesGcd,
                                                         boolean returnsLocalEntry) throws Exception {
        String participantId = discoveryEntry.getParticipantId();
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        DiscoveryQos discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, discoveryScope, false);
        if (localEntryAvalaible) {
            when(localDiscoveryEntryStoreMock.lookup(eq(participantId),
                                                     eq(Long.MAX_VALUE))).thenReturn(Optional.of(discoveryEntry));
        }
        doAnswer(createLookupAnswer(globalDiscoveryEntry)).when(globalCapabilitiesDirectoryClient)
                                                          .lookup(Matchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                                                                  anyString(),
                                                                  anyLong(),
                                                                  any());

        Promise<Lookup4Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId,
                                                                                   discoveryQos,
                                                                                   new String[0]);

        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo capturedDiscoveryEntry = (DiscoveryEntryWithMetaInfo) values[0];
        if (invokesGcd) {
            verify(globalCapabilitiesDirectoryClient).lookup(any(), eq(participantId), eq(discoveryTimeout), any());
        } else {
            verify(globalCapabilitiesDirectoryClient, never()).lookup(any(), anyString(), anyLong(), any());
        }
        if (returnsLocalEntry) {
            DiscoveryEntryWithMetaInfo expectedLocalDiscoveryEntry = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                                         discoveryEntry);
            assertEquals(expectedLocalDiscoveryEntry, capturedDiscoveryEntry);
        } else {
            DiscoveryEntryWithMetaInfo expectedGlobalDiscoveryEntry = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                                          globalDiscoveryEntry);
            assertEquals(expectedGlobalDiscoveryEntry, capturedDiscoveryEntry);
        }
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterfaceWithGbids_localOnly_noLocalEntry_doesNotInvokeGcd_returnsEmptyArray() throws Exception {
        String[] domains = { discoveryEntry.getDomain() };
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        DiscoveryQos discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, DiscoveryScope.LOCAL_ONLY, false);
        when(localDiscoveryEntryStoreMock.lookup(eq(domains), eq(INTERFACE_NAME))).thenReturn(new ArrayList<>());
        Promise<Lookup2Deferred> lookupPromise = localCapabilitiesDirectory.lookup(domains,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos,
                                                                                   new String[0]);

        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo[] capturedDiscoveryEntries = (DiscoveryEntryWithMetaInfo[]) values[0];
        assertEquals(0, capturedDiscoveryEntries.length);
        verify(globalCapabilitiesDirectoryClient,
               never()).lookup(any(), any(String[].class), anyString(), anyLong(), any());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterfaceWithGbids_localOnly_localEntries_doesNotInvokeGcd_returnsLocalEntries() throws Exception {
        DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_ONLY;
        boolean localEntriesAvailable = true;
        boolean invokesGcd = false;
        boolean returnsLocalEntry = true;
        lookupByDomainInterfaceDiscoveryScopeTest(discoveryScope, localEntriesAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterfaceWithGbids_localThenGlobal_noLocalEntry_invokesGcd_returnsRemoteResult() throws Exception {
        DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_THEN_GLOBAL;
        boolean localEntriesAvailable = false;
        boolean invokesGcd = true;
        boolean returnsLocalEntry = false;
        lookupByDomainInterfaceDiscoveryScopeTest(discoveryScope, localEntriesAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterfaceWithGbids_localThenGlobal_localEntries_doesNotInvokeGcd_returnsLocalEntries() throws Exception {
        DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_THEN_GLOBAL;
        boolean localEntriesAvailable = true;
        boolean invokesGcd = false;
        boolean returnsLocalEntry = true;
        lookupByDomainInterfaceDiscoveryScopeTest(discoveryScope, localEntriesAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterfaceWithGbids_localAndGlobal_noLocalEntry_invokesGcd_returnsRemoteResult() throws Exception {
        DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_AND_GLOBAL;
        boolean localEntriesAvailable = false;
        boolean invokesGcd = true;
        boolean returnsLocalEntry = false;
        lookupByDomainInterfaceDiscoveryScopeTest(discoveryScope, localEntriesAvailable, invokesGcd, returnsLocalEntry);
    }

    private void lookupByDomainInterfaceDiscoveryScopeTest(DiscoveryScope discoveryScope,
                                                           boolean localEntriesAvailable,
                                                           boolean invokesGcd,
                                                           boolean returnsLocalEntry) throws Exception {
        String[] domains = { discoveryEntry.getDomain() };
        List<DiscoveryEntry> discoveryEntries = Arrays.asList(discoveryEntry);
        List<GlobalDiscoveryEntry> globalDiscoveryEntries = Arrays.asList(globalDiscoveryEntry);
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        DiscoveryQos discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, discoveryScope, false);
        if (localEntriesAvailable) {
            when(localDiscoveryEntryStoreMock.lookup(eq(domains), eq(INTERFACE_NAME))).thenReturn(discoveryEntries);
        }
        doAnswer(createLookupAnswer(globalDiscoveryEntries)).when(globalCapabilitiesDirectoryClient)
                                                            .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                    Mockito.<String[]> any(),
                                                                    anyString(),
                                                                    anyLong(),
                                                                    any());

        Promise<Lookup2Deferred> lookupPromise = localCapabilitiesDirectory.lookup(domains,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos,
                                                                                   new String[0]);

        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo[] capturedDiscoveryEntries = (DiscoveryEntryWithMetaInfo[]) values[0];
        if (invokesGcd) {
            verify(globalCapabilitiesDirectoryClient).lookup(any(),
                                                             eq(domains),
                                                             eq(INTERFACE_NAME),
                                                             eq(discoveryTimeout),
                                                             any());
        } else {
            verify(globalCapabilitiesDirectoryClient,
                   never()).lookup(any(), any(String[].class), anyString(), anyLong(), any());
        }
        if (returnsLocalEntry) {
            DiscoveryEntryWithMetaInfo expectedLocalDiscoveryEntry = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                                         discoveryEntry);
            assertEquals(expectedLocalDiscoveryEntry, capturedDiscoveryEntries[0]);
        } else {
            DiscoveryEntryWithMetaInfo expectedGlobalDiscoveryEntry = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                                          globalDiscoveryEntry);
            assertEquals(expectedGlobalDiscoveryEntry, capturedDiscoveryEntries[0]);
        }
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_respectsCacheMaxAge() throws Exception {
        String participantId = discoveryEntry.getParticipantId();
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        DiscoveryQos discoveryQos = new DiscoveryQos(cacheMaxAge,
                                                     discoveryTimeout,
                                                     DiscoveryScope.LOCAL_AND_GLOBAL,
                                                     false);

        localCapabilitiesDirectory.lookup(participantId, discoveryQos, new String[0]);

        verify(globalDiscoveryEntryCacheMock).lookup(eq(participantId), eq(cacheMaxAge));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterfaceWithGbids_respectsCacheMaxAge() throws Exception {
        String[] domains = { discoveryEntry.getDomain() };
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        DiscoveryQos discoveryQos = new DiscoveryQos(cacheMaxAge,
                                                     discoveryTimeout,
                                                     DiscoveryScope.LOCAL_AND_GLOBAL,
                                                     false);

        localCapabilitiesDirectory.lookup(domains, INTERFACE_NAME, discoveryQos, new String[0]);

        verify(globalDiscoveryEntryCacheMock).lookup(eq(domains), eq(INTERFACE_NAME), eq(cacheMaxAge));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantId_DiscoveryEntryWithMetaInfoContainsExpectedIsLocalValue_globalEntry() throws Exception {
        String participantId = "participantId";
        String interfaceName = "interfaceName";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);

        // remote global DiscoveryEntry
        String remoteGlobalDomain = "remoteglobaldomain";
        final GlobalDiscoveryEntry remoteGlobalEntry = new GlobalDiscoveryEntry(new Version(0, 0),
                                                                                remoteGlobalDomain,
                                                                                interfaceName,
                                                                                participantId,
                                                                                new ProviderQos(),
                                                                                System.currentTimeMillis(),
                                                                                System.currentTimeMillis() + 10000L,
                                                                                "publicKeyId",
                                                                                globalAddress1Serialized);
        DiscoveryEntryWithMetaInfo remoteGlobalEntryWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                                       remoteGlobalEntry);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                Callback<GlobalDiscoveryEntry> callback = (Callback<GlobalDiscoveryEntry>) invocation.getArguments()[0];
                callback.onSuccess(remoteGlobalEntry);
                return null;
            }
        }).when(globalCapabilitiesDirectoryClient)
          .lookup(Matchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                  eq(participantId),
                  anyLong(),
                  eq(knownGbids));

        Promise<Lookup3Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId);

        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo capturedRemoteGlobalEntry = (DiscoveryEntryWithMetaInfo) values[0];
        assertEquals(remoteGlobalEntryWithMetaInfo, capturedRemoteGlobalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantId_DiscoveryQosTtlIsUsed() throws Exception {
        String participantId = "participantId";
        String interfaceName = "interfaceName";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);
        long discoveryTimeout = 1000000000;
        discoveryQos.setDiscoveryTimeout(discoveryTimeout);

        // remote global DiscoveryEntry
        String remoteGlobalDomain = "remoteglobaldomain";
        final GlobalDiscoveryEntry remoteGlobalEntry = new GlobalDiscoveryEntry(new Version(0, 0),
                                                                                remoteGlobalDomain,
                                                                                interfaceName,
                                                                                participantId,
                                                                                new ProviderQos(),
                                                                                System.currentTimeMillis(),
                                                                                System.currentTimeMillis() + 10000L,
                                                                                "publicKeyId",
                                                                                globalAddress1Serialized);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                Callback<GlobalDiscoveryEntry> callback = (Callback<GlobalDiscoveryEntry>) invocation.getArguments()[0];
                callback.onSuccess(remoteGlobalEntry);
                return null;
            }
        }).when(globalCapabilitiesDirectoryClient)
          .lookup(Matchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                  eq(participantId),
                  anyLong(),
                  eq(knownGbids));

        Promise<Lookup4Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId,
                                                                                   discoveryQos,
                                                                                   new String[0]);

        checkPromiseSuccess(lookupPromise, "lookup failed");
        verify(globalCapabilitiesDirectoryClient).lookup(any(), eq(participantId), eq(discoveryTimeout), any());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterface_DiscoveryEntriesWithMetaInfoContainExpectedIsLocalValue_localCachedAndGlobalEntries() throws InterruptedException {
        String globalDomain = "globaldomain";
        String remoteGlobalDomain = "remoteglobaldomain";
        String[] domains = new String[]{ "localdomain", globalDomain, remoteGlobalDomain };
        String interfaceName = "interfaceName";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);

        // local DiscoveryEntry
        DiscoveryEntry localEntry = new DiscoveryEntry();
        localEntry.setParticipantId("participantIdLocal");
        localEntry.setDomain(domains[0]);
        DiscoveryEntryWithMetaInfo localEntryWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                                localEntry);
        when(localDiscoveryEntryStoreMock.lookup(eq(domains), eq(interfaceName))).thenReturn(Arrays.asList(localEntry));

        // cached global DiscoveryEntry
        GlobalDiscoveryEntry cachedGlobalEntry = new GlobalDiscoveryEntry();
        cachedGlobalEntry.setParticipantId("participantIdCached");
        cachedGlobalEntry.setInterfaceName(interfaceName);
        cachedGlobalEntry.setDomain(globalDomain);
        cachedGlobalEntry.setAddress(globalAddress1Serialized);
        DiscoveryEntryWithMetaInfo cachedGlobalEntryWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                                       cachedGlobalEntry);
        doReturn(Arrays.asList(cachedGlobalEntry)).when(globalDiscoveryEntryCacheMock)
                                                  .lookup(eq(domains),
                                                          eq(interfaceName),
                                                          eq(discoveryQos.getCacheMaxAge()));

        // remote global DiscoveryEntry
        final GlobalDiscoveryEntry remoteGlobalEntry = new GlobalDiscoveryEntry(new Version(0, 0),
                                                                                remoteGlobalDomain,
                                                                                interfaceName,
                                                                                "participantIdRemote",
                                                                                new ProviderQos(),
                                                                                System.currentTimeMillis(),
                                                                                System.currentTimeMillis() + 10000L,
                                                                                "publicKeyId",
                                                                                globalAddress1Serialized);
        DiscoveryEntryWithMetaInfo remoteGlobalEntryWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                                       remoteGlobalEntry);
        doAnswer(createLookupAnswer(Arrays.asList(remoteGlobalEntry))).when(globalCapabilitiesDirectoryClient)
                                                                      .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                              eq(new String[]{ remoteGlobalDomain }),
                                                                              eq(interfaceName),
                                                                              anyLong(),
                                                                              eq(knownGbids));

        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos);

        Object[] values = checkPromiseSuccess(promise, "lookup failed");
        List<DiscoveryEntryWithMetaInfo> capabilities = Arrays.asList((DiscoveryEntryWithMetaInfo[]) values[0]);
        assertEquals(3, capabilities.size());
        assertTrue(capabilities.contains(localEntryWithMetaInfo));
        assertTrue(capabilities.contains(cachedGlobalEntryWithMetaInfo));
        assertTrue(capabilities.contains(remoteGlobalEntryWithMetaInfo));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbidsIsProperlyRejected_exception() throws InterruptedException {
        String domain = "domain";
        String[] domains = new String[]{ domain };
        String interfaceName = "interface";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        JoynrRuntimeException exception = new JoynrRuntimeException("lookup failed");
        ProviderRuntimeException expectedException = new ProviderRuntimeException(exception.toString());
        doAnswer(createVoidAnswerWithException(exception)).when(globalCapabilitiesDirectoryClient)
                                                          .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                  eq(domains),
                                                                  eq(interfaceName),
                                                                  anyLong(),
                                                                  Matchers.<String[]> any());

        Promise<Lookup2Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                             interfaceName,
                                                                             discoveryQos,
                                                                             knownGbids);

        verify(globalCapabilitiesDirectoryClient).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                         eq(domains),
                                                         eq(interfaceName),
                                                         anyLong(),
                                                         Matchers.<String[]> any());

        checkPromiseException(promise, expectedException);
    }

    private void testLookupByDomainInterfaceWithGbidsIsProperlyRejected(DiscoveryError expectedError) throws InterruptedException {
        String domain = "domain";
        String[] domains = new String[]{ domain };
        String interfaceName = "interface";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        doAnswer(createVoidAnswerWithDiscoveryError(expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                   .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                           eq(domains),
                                                                           eq(interfaceName),
                                                                           anyLong(),
                                                                           Matchers.<String[]> any());

        Promise<Lookup2Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                             interfaceName,
                                                                             discoveryQos,
                                                                             knownGbids);

        verify(globalCapabilitiesDirectoryClient).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                         eq(domains),
                                                         eq(interfaceName),
                                                         anyLong(),
                                                         Matchers.<String[]> any());

        checkPromiseError(promise, expectedError);
    }

    private void testLookupByDomainInterfaceIsProperlyRejected(DiscoveryError expectedError) throws InterruptedException {
        String domain = "domain";
        String[] domains = new String[]{ domain };
        String interfaceName = "interface";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        doAnswer(createVoidAnswerWithDiscoveryError(expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                   .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                           eq(domains),
                                                                           eq(interfaceName),
                                                                           anyLong(),
                                                                           Matchers.<String[]> any());

        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos);

        verify(globalCapabilitiesDirectoryClient).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                         eq(domains),
                                                         eq(interfaceName),
                                                         anyLong(),
                                                         Matchers.<String[]> any());

        checkPromiseErrorInProviderRuntimeException(promise, expectedError);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceIsProperlyRejected_invalidGbid() throws InterruptedException {
        testLookupByDomainInterfaceIsProperlyRejected(DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceIsProperlyRejected_unknownGbid() throws InterruptedException {
        testLookupByDomainInterfaceIsProperlyRejected(DiscoveryError.UNKNOWN_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceIsProperlyRejected_internalError() throws InterruptedException {
        testLookupByDomainInterfaceIsProperlyRejected(DiscoveryError.INTERNAL_ERROR);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceIsProperlyRejected_noEntryForSelectedBackend() throws InterruptedException {
        testLookupByDomainInterfaceIsProperlyRejected(DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbidsIsProperlyRejected_invalidGbid() throws InterruptedException {
        testLookupByDomainInterfaceWithGbidsIsProperlyRejected(DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbidsIsProperlyRejected_unknownGbid() throws InterruptedException {
        testLookupByDomainInterfaceWithGbidsIsProperlyRejected(DiscoveryError.UNKNOWN_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbidsIsProperlyRejected_internalError() throws InterruptedException {
        testLookupByDomainInterfaceWithGbidsIsProperlyRejected(DiscoveryError.INTERNAL_ERROR);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbidsIsProperlyRejected_noEntryForSelectedBackend() throws InterruptedException {
        testLookupByDomainInterfaceWithGbidsIsProperlyRejected(DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbidsIsProperlyRejected_exception() throws InterruptedException {
        String participantId = "participantId";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        JoynrRuntimeException exception = new JoynrRuntimeException("lookup failed");
        ProviderRuntimeException expectedException = new ProviderRuntimeException(exception.toString());
        doAnswer(createVoidAnswerWithException(exception)).when(globalCapabilitiesDirectoryClient)
                                                          .lookup(Matchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                                                                  eq(participantId),
                                                                  anyLong(),
                                                                  Matchers.<String[]> any());

        Promise<Lookup4Deferred> promise = localCapabilitiesDirectory.lookup(participantId, discoveryQos, knownGbids);

        checkPromiseException(promise, expectedException);
    }

    private void testLookupByParticipantIdWithGbidsIsProperlyRejected(DiscoveryError expectedError) throws InterruptedException {
        String participantId = "participantId";

        doAnswer(createVoidAnswerWithDiscoveryError(expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                   .lookup(Matchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                                                                           eq(participantId),
                                                                           anyLong(),
                                                                           Matchers.<String[]> any());

        DiscoveryQos discoveryQos = new DiscoveryQos(10000L, 500L, DiscoveryScope.LOCAL_AND_GLOBAL, false);
        Promise<Lookup4Deferred> promise = localCapabilitiesDirectory.lookup(participantId, discoveryQos, knownGbids);

        checkPromiseError(promise, expectedError);
    }

    private void testLookupByParticipantIdIsProperlyRejected(DiscoveryError expectedError) throws InterruptedException {
        String participantId = "participantId";

        doAnswer(createVoidAnswerWithDiscoveryError(expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                   .lookup(Matchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                                                                           eq(participantId),
                                                                           anyLong(),
                                                                           Matchers.<String[]> any());

        Promise<Lookup3Deferred> promise = localCapabilitiesDirectory.lookup(participantId);

        checkPromiseErrorInProviderRuntimeException(promise, expectedError);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdIsProperlyRejected_invalidGbid() throws InterruptedException {
        testLookupByParticipantIdIsProperlyRejected(DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdIsProperlyRejected_unknownGbid() throws InterruptedException {
        testLookupByParticipantIdIsProperlyRejected(DiscoveryError.UNKNOWN_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdIsProperlyRejected_internalError() throws InterruptedException {
        testLookupByParticipantIdIsProperlyRejected(DiscoveryError.INTERNAL_ERROR);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdIsProperlyRejected_noEntryForSelectedBackend() throws InterruptedException {
        testLookupByParticipantIdIsProperlyRejected(DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdIsProperlyRejected_noEntryForParticipant() throws InterruptedException {
        testLookupByParticipantIdIsProperlyRejected(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbidsIsProperlyRejected_invalidGbid() throws InterruptedException {
        testLookupByParticipantIdWithGbidsIsProperlyRejected(DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbidsIsProperlyRejected_unknownGbid() throws InterruptedException {
        testLookupByParticipantIdWithGbidsIsProperlyRejected(DiscoveryError.UNKNOWN_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbidsIsProperlyRejected_internalError() throws InterruptedException {
        testLookupByParticipantIdWithGbidsIsProperlyRejected(DiscoveryError.INTERNAL_ERROR);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbidsIsProperlyRejected_noEntryForSelectedBackend() throws InterruptedException {
        testLookupByParticipantIdWithGbidsIsProperlyRejected(DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbidsIsProperlyRejected_noEntryForParticipant() throws InterruptedException {
        testLookupByParticipantIdWithGbidsIsProperlyRejected(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbids_unknownGbids() throws InterruptedException {
        String[] gbids = new String[]{ "not", "known" };
        testLookupByDomainInterfaceWithDiscoveryError(gbids, DiscoveryError.UNKNOWN_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbids_unknownGbids() throws InterruptedException {
        String[] gbids = new String[]{ "not", "known" };
        testLookupByParticipantIdWithDiscoveryError(gbids, DiscoveryError.UNKNOWN_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbids_invalidGbid_emptyGbid() throws InterruptedException {
        String[] gbids = new String[]{ "" };
        testLookupByDomainInterfaceWithDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbids_invalidGbid_emptyGbid() throws InterruptedException {
        String[] gbids = new String[]{ "" };
        testLookupByParticipantIdWithDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbids_invalidGbid_duplicateGbid() throws InterruptedException {
        String[] gbids = new String[]{ knownGbids[1], knownGbids[0], knownGbids[1] };
        testLookupByDomainInterfaceWithDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbids_invalidGbid_duplicateGbid() throws InterruptedException {
        String[] gbids = new String[]{ knownGbids[1], knownGbids[0], knownGbids[1] };
        testLookupByParticipantIdWithDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbids_invalidGbid_nullGbid() throws InterruptedException {
        String[] gbids = new String[]{ null };
        testLookupByDomainInterfaceWithDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbids_invalidGbid_nullGbid() throws InterruptedException {
        String[] gbids = new String[]{ null };
        testLookupByParticipantIdWithDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbids_invalidGbid_nullGbidArray() throws InterruptedException {
        String[] gbids = null;
        testLookupByDomainInterfaceWithDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbids_invalidGbid_nullGbidArray() throws InterruptedException {
        String[] gbids = null;
        testLookupByParticipantIdWithDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    private void testLookupByDomainInterfaceWithDiscoveryError(String[] gbids,
                                                               DiscoveryError expectedError) throws InterruptedException {
        String[] domains = new String[]{ "domain1", "domain2" };
        String interfaceName = "interfaceName";
        DiscoveryQos discoveryQos = new DiscoveryQos();

        Promise<Lookup2Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                             interfaceName,
                                                                             discoveryQos,
                                                                             gbids);

        verify(globalCapabilitiesDirectoryClient,
               never()).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                               any(String[].class),
                               anyString(),
                               anyLong(),
                               any(String[].class));

        checkPromiseError(promise, expectedError);
    }

    private void testLookupByParticipantIdWithDiscoveryError(String[] gbids,
                                                             DiscoveryError expectedError) throws InterruptedException {
        String participantId = "participantId";
        Promise<Lookup4Deferred> promise = localCapabilitiesDirectory.lookup(participantId, new DiscoveryQos(), gbids);

        verify(globalCapabilitiesDirectoryClient,
               never()).lookup(Matchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                               anyString(),
                               anyLong(),
                               any(String[].class));
        checkPromiseError(promise, expectedError);
    }

    private static void checkPromiseException(Promise<?> promise,
                                              Exception expectedException) throws InterruptedException {
        CountDownLatch countDownLatch = new CountDownLatch(1);
        promise.then(new PromiseListener() {

            @Override
            public void onRejection(JoynrException exception) {
                assertTrue(expectedException.getClass().isInstance(exception));
                assertEquals(expectedException, exception);
                countDownLatch.countDown();
            }

            @Override
            public void onFulfillment(Object... values) {
                fail("Unexpected fulfillment when expecting rejection.");
            }
        });
        assertTrue(countDownLatch.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
    }

    private static void checkPromiseError(Promise<?> promise,
                                          DiscoveryError exptectedError) throws InterruptedException {
        CountDownLatch countDownLatch = new CountDownLatch(1);
        promise.then(new PromiseListener() {

            @Override
            public void onRejection(JoynrException exception) {
                if (exception instanceof ApplicationException) {
                    DiscoveryError error = ((ApplicationException) exception).getError();
                    assertEquals(exptectedError, error);
                    countDownLatch.countDown();
                } else {
                    fail("Did not receive an ApplicationException on rejection.");
                }
            }

            @Override
            public void onFulfillment(Object... values) {
                fail("Unexpected fulfillment when expecting rejection.");
            }
        });
        assertTrue(countDownLatch.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
    }

    private static void checkPromiseErrorInProviderRuntimeException(Promise<?> promise,
                                                                    DiscoveryError exptectedError) throws InterruptedException {
        CountDownLatch countDownLatch = new CountDownLatch(1);
        promise.then(new PromiseListener() {

            @Override
            public void onRejection(JoynrException exception) {
                if (exception instanceof ProviderRuntimeException) {
                    assertTrue(((ProviderRuntimeException) exception).getMessage().contains(exptectedError.name()));
                    countDownLatch.countDown();
                } else {
                    fail("Did not receive a ProviderRuntimeException on rejection.");
                }
            }

            @Override
            public void onFulfillment(Object... values) {
                fail("Unexpected fulfillment when expecting rejection.");
            }
        });
        assertTrue(countDownLatch.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
    }

    private static Object[] checkPromiseSuccess(Promise<? extends AbstractDeferred> promise,
                                                String onRejectionMessage) throws InterruptedException {
        ArrayList<Object> result = new ArrayList<>();
        CountDownLatch countDownLatch = new CountDownLatch(1);
        promise.then(new PromiseListener() {
            @Override
            public void onRejection(JoynrException error) {
                fail(onRejectionMessage + ": " + error);
            }

            @Override
            public void onFulfillment(Object... values) {
                result.addAll(Arrays.asList(values));
                countDownLatch.countDown();
            }
        });
        assertTrue(onRejectionMessage + ": promise timeout",
                   countDownLatch.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        return result.toArray(new Object[result.size()]);
    }

    private class MyCollectionMatcher extends TypeSafeMatcher<Collection<DiscoveryEntryWithMetaInfo>> {

        private int n;

        public MyCollectionMatcher(int n) {
            this.n = n;
        }

        @Override
        public void describeTo(Description description) {
            description.appendText("list has " + n + " entries");
        }

        @Override
        protected boolean matchesSafely(Collection<DiscoveryEntryWithMetaInfo> item) {
            return item.size() == n;
        }

    }

    Matcher<Collection<DiscoveryEntryWithMetaInfo>> hasNEntries(int n) {
        return new MyCollectionMatcher(n);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_globallyRegistered_GcdCalled() throws InterruptedException {
        when(globalAddressProvider.get()).thenReturn(new MqttAddress("testgbid", "testtopic"));
        Boolean awaitGlobalRegistration = true;
        Promise<DeferredVoid> addPromise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        checkPromiseSuccess(addPromise, "add failed");

        CountDownLatch cdlStart = new CountDownLatch(1);
        CountDownLatch cdlDone = new CountDownLatch(1);
        doAnswer(createAnswerWithDelayedSuccess(cdlStart,
                                                cdlDone,
                                                1500)).when(globalCapabilitiesDirectoryClient)
                                                      .remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                              eq(globalDiscoveryEntry.getParticipantId()),
                                                              any(String[].class));

        when(localDiscoveryEntryStoreMock.lookup(discoveryEntry.getParticipantId(),
                                                 Long.MAX_VALUE)).thenReturn(Optional.of(discoveryEntry));
        localCapabilitiesDirectory.remove(globalDiscoveryEntry.getParticipantId());

        assertTrue(cdlStart.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient).remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                         eq(discoveryEntry.getParticipantId()),
                                                         any(String[].class));
        verify(localDiscoveryEntryStoreMock, times(0)).remove(any(String.class));
        verify(globalDiscoveryEntryCacheMock, times(0)).remove(any(String.class));
        assertTrue(cdlDone.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(localDiscoveryEntryStoreMock, times(1)).remove(eq(discoveryEntry.getParticipantId()));
        verify(globalDiscoveryEntryCacheMock, times(1)).remove(eq(discoveryEntry.getParticipantId()));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_localProvider_GcdNotCalled() throws InterruptedException {
        when(globalAddressProvider.get()).thenReturn(new MqttAddress("testgbid", "testtopic"));
        discoveryEntry.getQos().setScope(ProviderScope.LOCAL);
        Boolean awaitGlobalRegistration = true;
        Promise<DeferredVoid> addPromise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        checkPromiseSuccess(addPromise, "add failed");

        when(localDiscoveryEntryStoreMock.lookup(discoveryEntry.getParticipantId(),
                                                 Long.MAX_VALUE)).thenReturn(Optional.of(discoveryEntry));
        localCapabilitiesDirectory.remove(discoveryEntry.getParticipantId());

        Thread.sleep(500);
        verify(globalCapabilitiesDirectoryClient,
               times(0)).remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                anyString(),
                                any(String[].class));
        verify(globalDiscoveryEntryCacheMock, times(0)).remove(anyString());
        verify(localDiscoveryEntryStoreMock, times(1)).remove(eq(discoveryEntry.getParticipantId()));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_participantNotRegisteredNoGbids_GcdNotCalled() throws InterruptedException {
        String participantId = "unknownparticipantId";
        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                              .remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      eq(participantId),
                                                      any(String[].class));

        doReturn(Optional.empty()).when(localDiscoveryEntryStoreMock).lookup(eq(participantId), anyLong());

        localCapabilitiesDirectory.remove(participantId);

        assertFalse(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient,
               never()).remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                               any(String.class),
                               any(String[].class));
        verify(localDiscoveryEntryStoreMock, never()).remove(any(String.class));
        verify(globalDiscoveryEntryCacheMock, never()).remove(any(String.class));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_participantNotRegisteredGbidsMapped_GcdCalled() throws InterruptedException {
        // this test assumes that the participant gets registered by a queued add task after enqueuing the remove task
        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                              .remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      eq(provisionedGlobalDiscoveryEntry.getParticipantId()),
                                                      any(String[].class));

        doReturn(Optional.empty()).when(localDiscoveryEntryStoreMock)
                                  .lookup(eq(provisionedGlobalDiscoveryEntry.getParticipantId()), anyLong());
        localCapabilitiesDirectory.remove(provisionedGlobalDiscoveryEntry.getParticipantId());

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                eq(provisionedGlobalDiscoveryEntry.getParticipantId()),
                                any(String[].class));
        verify(localDiscoveryEntryStoreMock, times(1)).remove(eq(provisionedGlobalDiscoveryEntry.getParticipantId()));
        verify(globalDiscoveryEntryCacheMock, times(1)).remove(eq(provisionedGlobalDiscoveryEntry.getParticipantId()));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_FailureStates_TimeoutException() throws InterruptedException {
        CountDownLatch cdl = new CountDownLatch(2);
        doAnswer(createVoidAnswerWithException(cdl,
                                               new JoynrTimeoutException(0))).when(globalCapabilitiesDirectoryClient)
                                                                             .remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                                     eq(provisionedGlobalDiscoveryEntry.getParticipantId()),
                                                                                     any(String[].class));

        localCapabilitiesDirectory.remove(provisionedGlobalDiscoveryEntry.getParticipantId());

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient,
               atLeast(2)).remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                  eq(provisionedGlobalDiscoveryEntry.getParticipantId()),
                                  any(String[].class));
        verify(globalDiscoveryEntryCacheMock, times(0)).remove(anyString());
        verify(localDiscoveryEntryStoreMock, times(0)).remove(anyString());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_FailureStates_NonTimeoutException() throws InterruptedException {
        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(createVoidAnswerWithException(cdl,
                                               new JoynrCommunicationException())).when(globalCapabilitiesDirectoryClient)
                                                                                  .remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                                          eq(provisionedGlobalDiscoveryEntry.getParticipantId()),
                                                                                          any(String[].class));
        localCapabilitiesDirectory.remove(provisionedGlobalDiscoveryEntry.getParticipantId());
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient,
               times(1)).remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                eq(provisionedGlobalDiscoveryEntry.getParticipantId()),
                                any(String[].class));
        verify(globalDiscoveryEntryCacheMock, times(0)).remove(anyString());
        verify(localDiscoveryEntryStoreMock, times(0)).remove(anyString());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_FailureStates_DiscoveryError_NoEntry() throws InterruptedException {
        // covers NO_ENTRY_FOR_PARTICIPANT as well as NO_ENTRY_FOR_SELECTED_BACKENDS
        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(createVoidAnswerWithDiscoveryError(cdl,
                                                    DiscoveryError.NO_ENTRY_FOR_PARTICIPANT)).when(globalCapabilitiesDirectoryClient)
                                                                                             .remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                                                     eq(provisionedGlobalDiscoveryEntry.getParticipantId()),
                                                                                                     any(String[].class));

        localCapabilitiesDirectory.remove(provisionedGlobalDiscoveryEntry.getParticipantId());

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient,
               times(1)).remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                eq(provisionedGlobalDiscoveryEntry.getParticipantId()),
                                any(String[].class));
        verify(globalDiscoveryEntryCacheMock, times(1)).remove(eq(provisionedGlobalDiscoveryEntry.getParticipantId()));
        verify(localDiscoveryEntryStoreMock, times(1)).remove(eq(provisionedGlobalDiscoveryEntry.getParticipantId()));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_FailureStates_DiscoveryError_InvalidGbid() throws InterruptedException {
        // Also covers UNKNOWN_GBID and INTERNAL_ERROR
        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(createVoidAnswerWithDiscoveryError(cdl,
                                                    DiscoveryError.INVALID_GBID)).when(globalCapabilitiesDirectoryClient)
                                                                                 .remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                                         eq(provisionedGlobalDiscoveryEntry.getParticipantId()),
                                                                                         any(String[].class));

        localCapabilitiesDirectory.remove(provisionedGlobalDiscoveryEntry.getParticipantId());

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient,
               times(1)).remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                eq(provisionedGlobalDiscoveryEntry.getParticipantId()),
                                any(String[].class));
        verify(globalDiscoveryEntryCacheMock, times(0)).remove(anyString());
        verify(localDiscoveryEntryStoreMock, times(0)).remove(anyString());
    }

    private void testRemoveUsesSameGbidOrderAsAdd(String[] selectedGbids) throws InterruptedException {
        String[] expectedGbids = selectedGbids.clone();

        String participantId = LocalCapabilitiesDirectoryTest.class.getName() + ".removeUsesSameGbidOrderAsAdd."
                + Arrays.toString(selectedGbids);
        String domain = "testDomain";
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.GLOBAL);
        globalDiscoveryEntry = new GlobalDiscoveryEntry(new Version(47, 11),
                                                        domain,
                                                        INTERFACE_NAME,
                                                        participantId,
                                                        providerQos,
                                                        System.currentTimeMillis(),
                                                        expiryDateMs,
                                                        publicKeyId,
                                                        globalAddress1Serialized);

        boolean awaitGlobalRegistration = true;
        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(globalDiscoveryEntry,
                                                                       awaitGlobalRegistration,
                                                                       selectedGbids);
        checkPromiseSuccess(promise, "add failed in testRemoveUsesSameGbidOrderAsAdd");

        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                              .remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      any(String.class),
                                                      any(String[].class));

        when(localDiscoveryEntryStoreMock.lookup(globalDiscoveryEntry.getParticipantId(),
                                                 Long.MAX_VALUE)).thenReturn(Optional.of(globalDiscoveryEntry));
        localCapabilitiesDirectory.remove(globalDiscoveryEntry.getParticipantId());

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient).remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                         any(String.class),
                                                         eq(expectedGbids));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testRemoveUsesSameGbidOrderAsAdd() throws InterruptedException {
        testRemoveUsesSameGbidOrderAsAdd(new String[]{ knownGbids[0] });

        testRemoveUsesSameGbidOrderAsAdd(new String[]{ knownGbids[1] });

        testRemoveUsesSameGbidOrderAsAdd(new String[]{ knownGbids[0], knownGbids[1] });

        testRemoveUsesSameGbidOrderAsAdd(new String[]{ knownGbids[1], knownGbids[0] });
    }

    @Test(timeout = TEST_TIMEOUT)
    public void callTouchForGlobalParticipantIds() throws InterruptedException {
        final String participantId1 = "participantId1";
        final String participantId2 = "participantId2";

        final long toleranceMs = freshnessUpdateIntervalMs * 2 / 3;

        GlobalDiscoveryEntry entry1 = new GlobalDiscoveryEntry(globalDiscoveryEntry);

        entry1.getQos().setScope(ProviderScope.GLOBAL);
        entry1.setParticipantId(participantId1);

        GlobalDiscoveryEntry entry2 = new GlobalDiscoveryEntry(entry1);
        entry2.setParticipantId(participantId2);

        Promise<DeferredVoid> promiseAdd1 = localCapabilitiesDirectory.add(entry1, true);
        Promise<DeferredVoid> promiseAdd2 = localCapabilitiesDirectory.add(entry2, true);
        checkPromiseSuccess(promiseAdd1, "add failed");
        checkPromiseSuccess(promiseAdd2, "add failed");

        ArgumentCaptor<Long> lastSeenDateCaptor = ArgumentCaptor.forClass(Long.class);
        ArgumentCaptor<Long> expiryDateCaptor = ArgumentCaptor.forClass(Long.class);

        String[] touchedParticipantIds = new String[]{ participantId1, participantId2 };
        String[] expectedParticipantIds = touchedParticipantIds.clone();
        when(localDiscoveryEntryStoreMock.touchDiscoveryEntries(anyLong(),
                                                                anyLong())).thenReturn(touchedParticipantIds);

        verify(capabilitiesFreshnessUpdateExecutor).scheduleAtFixedRate(runnableCaptor.capture(),
                                                                        eq(freshnessUpdateIntervalMs),
                                                                        eq(freshnessUpdateIntervalMs),
                                                                        eq(TimeUnit.MILLISECONDS));

        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                              .touch(Matchers.<Callback<Void>> any(),
                                                     eq(expectedParticipantIds),
                                                     anyString());
        Thread.sleep(freshnessUpdateIntervalMs); // make sure that the inital delay has expired before starting the runnable
        final long expectedLastSeenDateMs = System.currentTimeMillis();
        final long expectedExpiryDateMs = expectedLastSeenDateMs + DEFAULT_EXPIRY_TIME_MS;
        Runnable runnable = runnableCaptor.getValue();
        runnable.run();

        verify(localDiscoveryEntryStoreMock, times(1)).touchDiscoveryEntries(lastSeenDateCaptor.capture(),
                                                                             expiryDateCaptor.capture());

        assertTrue(Math.abs(lastSeenDateCaptor.getValue() - expectedLastSeenDateMs) <= toleranceMs);
        assertTrue(Math.abs(expiryDateCaptor.getValue() - expectedExpiryDateMs) <= toleranceMs);

        verify(globalDiscoveryEntryCacheMock, times(1)).touchDiscoveryEntries(eq(expectedParticipantIds),
                                                                              eq(lastSeenDateCaptor.getValue()),
                                                                              eq(expiryDateCaptor.getValue()));

        assertTrue(Math.abs(lastSeenDateCaptor.getValue() - expectedLastSeenDateMs) <= toleranceMs);
        assertTrue(Math.abs(expiryDateCaptor.getValue() - expectedExpiryDateMs) <= toleranceMs);

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient, times(1)).touch(Matchers.<Callback<Void>> any(),
                                                                  eq(expectedParticipantIds),
                                                                  anyString());
    }

    @Test
    public void touchNotCalled_noParticipantIdsToTouch() throws InterruptedException {
        String[] participantIdsToTouch = new String[0];
        when(localDiscoveryEntryStoreMock.touchDiscoveryEntries(anyLong(),
                                                                anyLong())).thenReturn(participantIdsToTouch);

        verify(capabilitiesFreshnessUpdateExecutor).scheduleAtFixedRate(runnableCaptor.capture(),
                                                                        eq(freshnessUpdateIntervalMs),
                                                                        eq(freshnessUpdateIntervalMs),
                                                                        eq(TimeUnit.MILLISECONDS));

        Runnable runnable = runnableCaptor.getValue();
        runnable.run();

        verify(globalCapabilitiesDirectoryClient, times(0)).touch(Matchers.<Callback<Void>> any(), any(), anyString());
    }

    @Test
    public void touchCalledOnce_multipleParticipantIdsForSingleGbid() throws InterruptedException {
        String participantId1 = "participantId1";
        String participantId2 = "participantId2";

        String gbid = knownGbids[1];
        String[] gbids = { gbid };

        GlobalDiscoveryEntry entry1 = new GlobalDiscoveryEntry(globalDiscoveryEntry);

        entry1.getQos().setScope(ProviderScope.GLOBAL);
        entry1.setParticipantId(participantId1);
        entry1.setExpiryDateMs(0l);
        entry1.setLastSeenDateMs(0l);

        GlobalDiscoveryEntry entry2 = new GlobalDiscoveryEntry(entry1);
        entry2.setParticipantId(participantId2);

        Promise<Add1Deferred> promiseAdd1 = localCapabilitiesDirectory.add(entry1, true, gbids);
        Promise<Add1Deferred> promiseAdd2 = localCapabilitiesDirectory.add(entry2, true, gbids);
        checkPromiseSuccess(promiseAdd1, "add failed");
        checkPromiseSuccess(promiseAdd2, "add failed");

        // Mock return values of localDiscoveryEntryStore.touchDiscoveryEntries
        String[] participantIdsToTouch = new String[]{ participantId1, participantId2 };
        when(localDiscoveryEntryStoreMock.touchDiscoveryEntries(anyLong(),
                                                                anyLong())).thenReturn(participantIdsToTouch);

        verify(capabilitiesFreshnessUpdateExecutor).scheduleAtFixedRate(runnableCaptor.capture(),
                                                                        eq(freshnessUpdateIntervalMs),
                                                                        eq(freshnessUpdateIntervalMs),
                                                                        eq(TimeUnit.MILLISECONDS));

        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                              .touch(Matchers.<Callback<Void>> any(),
                                                     eq(participantIdsToTouch),
                                                     anyString());
        Thread.sleep(freshnessUpdateIntervalMs); // make sure that the initial delay has expired before starting the runnable
        Runnable runnable = runnableCaptor.getValue();
        runnable.run();
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient, times(1)).touch(Matchers.<Callback<Void>> any(),
                                                                  eq(participantIdsToTouch),
                                                                  eq(gbid));
    }

    @Test
    public void touchCalledOnce_singleParticipantIdForMultipleGbids() throws InterruptedException {
        String participantId1 = "participantId1";

        String gbid1 = knownGbids[1];
        String gbid2 = knownGbids[2];
        String[] gbids = { gbid1, gbid2 };

        GlobalDiscoveryEntry entry1 = new GlobalDiscoveryEntry(globalDiscoveryEntry);

        entry1.getQos().setScope(ProviderScope.GLOBAL);
        entry1.setParticipantId(participantId1);
        entry1.setExpiryDateMs(0l);
        entry1.setLastSeenDateMs(0l);

        Promise<Add1Deferred> promiseAdd = localCapabilitiesDirectory.add(entry1, true, gbids);
        checkPromiseSuccess(promiseAdd, "add failed");

        // Mock return values of localDiscoveryEntryStore.touchDiscoveryEntries
        String[] participantIdsToTouch = new String[]{ participantId1 };
        when(localDiscoveryEntryStoreMock.touchDiscoveryEntries(anyLong(),
                                                                anyLong())).thenReturn(participantIdsToTouch);

        verify(capabilitiesFreshnessUpdateExecutor).scheduleAtFixedRate(runnableCaptor.capture(),
                                                                        eq(freshnessUpdateIntervalMs),
                                                                        eq(freshnessUpdateIntervalMs),
                                                                        eq(TimeUnit.MILLISECONDS));

        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                              .touch(Matchers.<Callback<Void>> any(),
                                                     eq(participantIdsToTouch),
                                                     anyString());
        Thread.sleep(freshnessUpdateIntervalMs); // make sure that the initial delay has expired before starting the runnable
        Runnable runnable = runnableCaptor.getValue();
        runnable.run();
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient, times(1)).touch(Matchers.<Callback<Void>> any(),
                                                                  eq(participantIdsToTouch),
                                                                  eq(gbid1));
    }

    @Test
    public void touchCalledTwice_twoParticipantIdsForDifferentGbids() throws InterruptedException {
        String participantId1 = "participantId1";
        String participantId2 = "participantId2";

        String gbid1 = knownGbids[1];
        String gbid2 = knownGbids[2];
        String[] gbids1 = { gbid1 };
        String[] gbids2 = { gbid2 };

        GlobalDiscoveryEntry entry1 = new GlobalDiscoveryEntry(globalDiscoveryEntry);

        entry1.getQos().setScope(ProviderScope.GLOBAL);
        entry1.setParticipantId(participantId1);
        entry1.setExpiryDateMs(0l);
        entry1.setLastSeenDateMs(0l);

        GlobalDiscoveryEntry entry2 = new GlobalDiscoveryEntry(entry1);
        entry2.setParticipantId(participantId2);

        Promise<Add1Deferred> promiseAdd1 = localCapabilitiesDirectory.add(entry1, true, gbids1);
        Promise<Add1Deferred> promiseAdd2 = localCapabilitiesDirectory.add(entry2, true, gbids2);
        checkPromiseSuccess(promiseAdd1, "add failed");
        checkPromiseSuccess(promiseAdd2, "add failed");

        // Mock return values of localDiscoveryEntryStore.touchDiscoveryEntries
        String[] participantIdsToTouch = new String[]{ participantId1, participantId2 };
        when(localDiscoveryEntryStoreMock.touchDiscoveryEntries(anyLong(),
                                                                anyLong())).thenReturn(participantIdsToTouch);

        verify(capabilitiesFreshnessUpdateExecutor).scheduleAtFixedRate(runnableCaptor.capture(),
                                                                        eq(freshnessUpdateIntervalMs),
                                                                        eq(freshnessUpdateIntervalMs),
                                                                        eq(TimeUnit.MILLISECONDS));

        String[] expectedParticipantIds1 = new String[]{ participantId1 };
        String[] expectedParticipantIds2 = new String[]{ participantId2 };

        CountDownLatch cdl = new CountDownLatch(2);
        doAnswer(createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                              .touch(Matchers.<Callback<Void>> any(),
                                                     Matchers.<String[]> any(),
                                                     anyString());
        Thread.sleep(freshnessUpdateIntervalMs); // make sure that the initial delay has expired before starting the runnable
        Runnable runnable = runnableCaptor.getValue();
        runnable.run();
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient, times(1)).touch(Matchers.<Callback<Void>> any(),
                                                                  eq(expectedParticipantIds1),
                                                                  eq(gbid1));

        verify(globalCapabilitiesDirectoryClient, times(1)).touch(Matchers.<Callback<Void>> any(),
                                                                  eq(expectedParticipantIds2),
                                                                  eq(gbid2));
    }

    @Test
    public void removeStaleProvidersOfClusterController_invokesGcdClient() {
        // Test whether removeStale() of GlobalCapabiltiesDirectoryClient is called once for all known backends
        // and captured argument of maxLastSeenDateMs differs from current time less than threshold.
        final long currentDateMs = System.currentTimeMillis();
        ArgumentCaptor<Long> maxLastSeenDateCaptor = ArgumentCaptor.forClass(Long.class);
        final long toleranceMs = 200L;

        localCapabilitiesDirectory.removeStaleProvidersOfClusterController();
        ArgumentCaptor<String> gbidCaptor = ArgumentCaptor.forClass(String.class);
        verify(globalCapabilitiesDirectoryClient, times(knownGbids.length)).removeStale(Matchers.<Callback<Void>> any(),
                                                                                        maxLastSeenDateCaptor.capture(),
                                                                                        gbidCaptor.capture());

        assertTrue(maxLastSeenDateCaptor.getValue() <= currentDateMs);
        assertTrue(currentDateMs - maxLastSeenDateCaptor.getValue() <= toleranceMs);
        List<String> actualGbids = gbidCaptor.getAllValues();
        assertEquals(Arrays.asList(knownGbids), actualGbids);
    }

    @Test
    public void removeStaleProvidersOfClusterController_callsItselfOnCallbackFailure() {
        // Test whether removeStaleProvidersOfClusterController() is calling itself n-times
        // when callback function is calling onFailure(exception) function.
        int numberOfOnFailureCalls = 2;
        JoynrRuntimeException exception = new JoynrRuntimeException("removeStale failed");

        for (String gbid : knownGbids) {
            doAnswer(new Answer<Future<Void>>() {
                private int count = 0;

                @Override
                public Future<Void> answer(InvocationOnMock invocation) throws Throwable {
                    Future<Void> result = new Future<Void>();
                    @SuppressWarnings("unchecked")
                    Callback<Void> callback = (Callback<Void>) invocation.getArguments()[0];
                    if (count++ == numberOfOnFailureCalls) {
                        callback.onSuccess(null);
                        result.onSuccess(null);
                        return result;
                    }
                    callback.onFailure(exception);
                    result.onSuccess(null);
                    return result;
                }
            }).when(globalCapabilitiesDirectoryClient)
              .removeStale(Matchers.<Callback<Void>> any(), anyLong(), eq(gbid));
        }

        localCapabilitiesDirectory.removeStaleProvidersOfClusterController();

        int numberOfCalls = numberOfOnFailureCalls + 1; // one time success

        for (String gbid : knownGbids) {
            verify(globalCapabilitiesDirectoryClient, times(numberOfCalls)).removeStale(Matchers.<Callback<Void>> any(),
                                                                                        anyLong(),
                                                                                        eq(gbid));
        }
    }

    @Test
    public void removeStaleProvidersOfClusterController_calledOnceIfMessageNotSent() {
        // Test whether removeStale() of GlobalCapabiltiesDirectoryClient is called once when exception
        // in a gbid has a type JoynrMessageNotSentException and contains "Address type not supported" message
        JoynrRuntimeException exception = new JoynrMessageNotSentException("Address type not supported");

        doAnswer(new Answer<Future<Void>>() {
            @Override
            public Future<Void> answer(InvocationOnMock invocation) throws Throwable {
                Future<Void> result = new Future<Void>();
                @SuppressWarnings("unchecked")
                Callback<Void> callback = (Callback<Void>) invocation.getArguments()[0];
                callback.onFailure(exception);
                result.onSuccess(null);
                return result;
            }
        }).when(globalCapabilitiesDirectoryClient).removeStale(Matchers.<Callback<Void>> any(), anyLong(), anyString());

        localCapabilitiesDirectory.removeStaleProvidersOfClusterController();

        for (String gbid : knownGbids) {
            verify(globalCapabilitiesDirectoryClient, times(1)).removeStale(Matchers.<Callback<Void>> any(),
                                                                            anyLong(),
                                                                            eq(gbid));
        }
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addAndRemoveAreCalledInOrder() throws InterruptedException {
        final String participantId1 = "participantId1";
        final String participantId2 = "participantId2";

        DiscoveryEntry discoveryEntry1 = new DiscoveryEntry(discoveryEntry);
        discoveryEntry1.getQos().setScope(ProviderScope.GLOBAL);
        discoveryEntry1.setParticipantId(participantId1);

        DiscoveryEntry discoveryEntry2 = new DiscoveryEntry(discoveryEntry);
        discoveryEntry2.setParticipantId(participantId2);

        GlobalDiscoveryEntry globalDiscoveryEntry1 = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry1,
                                                                                                         globalAddress1);
        GlobalDiscoveryEntry globalDiscoveryEntry2 = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry2,
                                                                                                         globalAddress1);

        final boolean awaitGlobalRegistration = true;
        Promise<DeferredVoid> promiseAdd1 = localCapabilitiesDirectory.add(discoveryEntry1, awaitGlobalRegistration);
        Promise<DeferredVoid> promiseAdd2 = localCapabilitiesDirectory.add(discoveryEntry2, awaitGlobalRegistration);

        checkPromiseSuccess(promiseAdd1, "add failed");
        checkPromiseSuccess(promiseAdd2, "add failed");

        InOrder inOrder = inOrder(globalCapabilitiesDirectoryClient);

        ArgumentCaptor<Long> remainingTtlCapture = ArgumentCaptor.forClass(Long.class);

        inOrder.verify(globalCapabilitiesDirectoryClient)
               .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                    argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry1)),
                    remainingTtlCapture.capture(),
                    any(String[].class));

        checkRemainingTtl(remainingTtlCapture);

        inOrder.verify(globalCapabilitiesDirectoryClient)
               .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                    argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry2)),
                    remainingTtlCapture.capture(),
                    any(String[].class));

        checkRemainingTtl(remainingTtlCapture);

        CountDownLatch cdl = new CountDownLatch(2);
        doAnswer(createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                              .remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      anyString(),
                                                      any(String[].class));
        when(localDiscoveryEntryStoreMock.lookup(discoveryEntry2.getParticipantId(),
                                                 Long.MAX_VALUE)).thenReturn(Optional.of(discoveryEntry2));
        when(localDiscoveryEntryStoreMock.lookup(discoveryEntry1.getParticipantId(),
                                                 Long.MAX_VALUE)).thenReturn(Optional.of(discoveryEntry1));
        localCapabilitiesDirectory.remove(discoveryEntry2.getParticipantId());
        localCapabilitiesDirectory.remove(discoveryEntry1.getParticipantId());

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        inOrder.verify(globalCapabilitiesDirectoryClient)
               .remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                       eq(participantId2),
                       any(String[].class));
        inOrder.verify(globalCapabilitiesDirectoryClient)
               .remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                       eq(participantId1),
                       any(String[].class));
    }

    private static class GlobalDiscoveryEntryWithParticipantIdMatcher extends ArgumentMatcher<GlobalDiscoveryEntry> {
        private GlobalDiscoveryEntry expected;

        private GlobalDiscoveryEntryWithParticipantIdMatcher(GlobalDiscoveryEntry expected) {
            this.expected = expected;
        }

        @Override
        public boolean matches(Object argument) {
            assertNotNull(argument);
            GlobalDiscoveryEntry actual = (GlobalDiscoveryEntry) argument;
            return expected.getParticipantId() == actual.getParticipantId()
                    && expected.getAddress().equals(actual.getAddress());
        }
    }

    private void setNewDefaultTtlAddAndRemove(long defaulTtlMs) throws ReflectiveOperationException {

        Field defaulTtlMsField = LocalCapabilitiesDirectoryImpl.class.getDeclaredField("defaultTtlAddAndRemove");
        defaulTtlMsField.setAccessible(true);
        defaulTtlMsField.set((Object) localCapabilitiesDirectory, defaulTtlMs);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testProcessingExpiredQueuedGcdActions() throws Exception {
        reset(globalCapabilitiesDirectoryClient);

        // defaultTtlAddAndRemove = 60000ms (MessagingQos.DEFAULT_TTL) is too long, we reduce it to 1000ms for the test
        setNewDefaultTtlAddAndRemove(1000);

        final String participantId1 = "participantId1";
        final String participantId2 = "participantId2";

        DiscoveryEntry discoveryEntry1 = new DiscoveryEntry(discoveryEntry);
        discoveryEntry1.getQos().setScope(ProviderScope.GLOBAL);
        discoveryEntry1.setParticipantId(participantId1);

        DiscoveryEntry discoveryEntry2 = new DiscoveryEntry(discoveryEntry1);
        discoveryEntry2.setParticipantId(participantId2);

        GlobalDiscoveryEntry globalDiscoveryEntry1 = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry1,
                                                                                                         globalAddress1);
        GlobalDiscoveryEntry globalDiscoveryEntry2 = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry2,
                                                                                                         globalAddress1);

        final long delay = 1500;
        CountDownLatch cdlAddDelayStarted = new CountDownLatch(1);
        CountDownLatch cdlAddDone = new CountDownLatch(1);
        doAnswer(createAnswerWithDelayedSuccess(cdlAddDelayStarted,
                                                cdlAddDone,
                                                delay)).when(globalCapabilitiesDirectoryClient)
                                                       .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                            argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry1)),
                                                            anyLong(),
                                                            any(String[].class));

        CountDownLatch cdlRemove = new CountDownLatch(1);
        doAnswer(createAnswerWithSuccess(cdlRemove)).when(globalCapabilitiesDirectoryClient)
                                                    .remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                            eq(participantId1),
                                                            any(String[].class));

        // 3 actions. 2 lcd.add and 1 lcd.remove
        final Boolean awaitGlobalRegistration = true;
        Promise<DeferredVoid> promiseAdd1 = localCapabilitiesDirectory.add(discoveryEntry1, awaitGlobalRegistration);
        Promise<DeferredVoid> promiseAdd2 = localCapabilitiesDirectory.add(discoveryEntry2, awaitGlobalRegistration);
        localCapabilitiesDirectory.remove(discoveryEntry1.getParticipantId());
        assertTrue(cdlAddDelayStarted.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        JoynrRuntimeException expectedException = new JoynrRuntimeException("Failed to process global registration in time, please try again");
        checkPromiseException(promiseAdd2, new ProviderRuntimeException(expectedException.toString()));

        // second add failed before first add has finished, remove not yet executed
        assertEquals(1, cdlAddDone.getCount());
        assertEquals(1, cdlRemove.getCount());

        checkPromiseSuccess(promiseAdd1, "add failed");
        assertTrue(cdlAddDone.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        assertTrue(cdlRemove.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        InOrder inOrder = inOrder(globalCapabilitiesDirectoryClient);

        inOrder.verify(globalCapabilitiesDirectoryClient, times(1))
               .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                    argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry1)),
                    anyLong(),
                    any(String[].class));

        inOrder.verify(globalCapabilitiesDirectoryClient, times(1))
               .remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                       eq(participantId1),
                       any(String[].class));

        verify(globalCapabilitiesDirectoryClient,
               times(0)).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry2)),
                             anyLong(),
                             any(String[].class));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testReAddAllGlobalDiscoveryEntriesPeriodically() throws InterruptedException {
        final String participantId1 = "participantId1";
        final String participantId2 = "participantId2";

        DiscoveryEntry discoveryEntry1 = new DiscoveryEntry(discoveryEntry);
        discoveryEntry1.getQos().setScope(ProviderScope.GLOBAL);
        discoveryEntry1.setParticipantId(participantId1);

        DiscoveryEntry discoveryEntry2 = new DiscoveryEntry(discoveryEntry1);
        discoveryEntry2.setParticipantId(participantId2);

        GlobalDiscoveryEntry globalDiscoveryEntry1 = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry1,
                                                                                                         globalAddress1);
        GlobalDiscoveryEntry globalDiscoveryEntry2 = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry2,
                                                                                                         globalAddress1);

        final boolean awaitGlobalRegistration = true;
        String[] gbids1 = new String[]{ knownGbids[0] };
        String[] expectedGbids1 = gbids1.clone();
        String[] gbids2 = new String[]{ knownGbids[1] };
        String[] expectedGbids2 = gbids2.clone();
        Promise<Add1Deferred> promiseAdd1 = localCapabilitiesDirectory.add(discoveryEntry1,
                                                                           awaitGlobalRegistration,
                                                                           gbids1);
        Promise<Add1Deferred> promiseAdd2 = localCapabilitiesDirectory.add(discoveryEntry2,
                                                                           awaitGlobalRegistration,
                                                                           gbids2);

        checkPromiseSuccess(promiseAdd1, "add failed");
        checkPromiseSuccess(promiseAdd2, "add failed");

        reset(globalCapabilitiesDirectoryClient);

        CountDownLatch cdlReAdd = new CountDownLatch(2);
        doAnswer(createAnswerWithSuccess(cdlReAdd)).when(globalCapabilitiesDirectoryClient)
                                                   .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                        argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry1)),
                                                        anyLong(),
                                                        eq(gbids1));

        doAnswer(createAnswerWithSuccess(cdlReAdd)).when(globalCapabilitiesDirectoryClient)
                                                   .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                        argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry2)),
                                                        anyLong(),
                                                        eq(gbids2));

        Set<DiscoveryEntry> globalEntries = new HashSet<>();
        globalEntries.add(discoveryEntry1);
        globalEntries.add(discoveryEntry2);
        when(localDiscoveryEntryStoreMock.getAllGlobalEntries()).thenReturn(globalEntries);

        verify(globalCapabilitiesDirectoryClient, times(0)).add(any(), any(), anyLong(), any());

        verify(capabilitiesFreshnessUpdateExecutor).scheduleAtFixedRate(runnableCaptor.capture(),
                                                                        eq(READD_INTERVAL_DAYS),
                                                                        eq(READD_INTERVAL_DAYS),
                                                                        eq(TimeUnit.DAYS));

        // capture the runnable and execute it to schedule the re-add task
        Runnable runnable = runnableCaptor.getValue();
        runnable.run();

        assertTrue(cdlReAdd.await(defaultTtlAddAndRemove, TimeUnit.MILLISECONDS));

        // check whether add method has been called for 2 non expired entries
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry1)),
                             eq(defaultTtlAddAndRemove),
                             eq(expectedGbids1));

        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry2)),
                             eq(defaultTtlAddAndRemove),
                             eq(expectedGbids2));
    }
}
