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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
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
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.function.Function;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatcher;
import org.mockito.ArgumentMatchers;
import org.mockito.Captor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.accesscontrol.AccessController;
import io.joynr.capabilities.LocalCapabilitiesDirectoryImpl.GcdTaskSequencer;
import io.joynr.dispatching.Dispatcher;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrTimeoutException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.RoutingTable;
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

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectoryTest.class);

    private static final int TEST_TIMEOUT = 10000;
    private static final int DEFAULT_WAIT_TIME_MS = 5000; // value should be shorter than TEST_TIMEOUT
    private static final String INTERFACE_NAME = "interfaceName";
    private static final String TEST_URL = "mqtt://testUrl:42";
    private static final long ONE_DAY_IN_MS = 1 * 24 * 60 * 60 * 1000;
    private static final long freshnessUpdateIntervalMs = 300;
    private static final long DEFAULT_EXPIRY_TIME_MS = 3628800000l;
    private static final long RE_ADD_INTERVAL_DAYS = 7l;
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
    private RoutingTable routingTable;
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
    @Mock
    private AccessController accessController;

    @Captor
    private ArgumentCaptor<Collection<DiscoveryEntryWithMetaInfo>> capabilitiesCaptor;
    @Captor
    private ArgumentCaptor<Runnable> runnableCaptor;
    @Captor
    private ArgumentCaptor<GcdTaskSequencer> addRemoveQueueRunnableCaptor;
    @Captor
    ArgumentCaptor<CallbackWithModeledError<Void, DiscoveryError>> callbackCaptor;

    private GcdTaskSequencer gcdTaskSequencerSpy;
    private GcdTaskSequencer gcdTaskSequencer;
    private Thread addRemoveWorker;

    private boolean enableAccessControl = false;

    static class DiscoveryEntryWithUpdatedLastSeenDateMsMatcher implements ArgumentMatcher<DiscoveryEntry> {

        @Override
        public String toString() {
            String description = "expected: " + expected;
            return description;
        }

        private DiscoveryEntry expected;

        DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(DiscoveryEntry expected) {
            this.expected = expected;
        }

        @Override
        public boolean matches(DiscoveryEntry argument) {
            assertNotNull(argument);
            DiscoveryEntry actual = (DiscoveryEntry) argument;
            return discoveryEntriesMatchWithUpdatedLastSeenDate(expected, actual);
        }
    }

    static class GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher implements ArgumentMatcher<GlobalDiscoveryEntry> {

        private GlobalDiscoveryEntry expected;

        GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(GlobalDiscoveryEntry expected) {
            this.expected = expected;
        }

        @Override
        public boolean matches(GlobalDiscoveryEntry argument) {
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

    private static boolean discoveryEntriesWithMetaInfoMatchWithUpdatedLastSeenDate(DiscoveryEntryWithMetaInfo expected,
                                                                                    DiscoveryEntryWithMetaInfo actual) {
        return discoveryEntriesMatchWithUpdatedLastSeenDate(expected, actual)
                && expected.getIsLocal() == actual.getIsLocal();
    }

    private static boolean globalDiscoveryEntriesMatchWithUpdatedLastSeenDate(GlobalDiscoveryEntry expected,
                                                                              GlobalDiscoveryEntry actual) {
        return discoveryEntriesMatchWithUpdatedLastSeenDate(expected, actual)
                && expected.getAddress().equals(actual.getAddress());
    }

    private Field getPrivateField(Class<?> privateClass, String fieldName) {
        Field result = null;
        try {
            result = privateClass.getDeclaredField(fieldName);
        } catch (Exception e) {
            fail(e.getMessage());
        }
        return result;
    }

    private <T> void setFieldValue(Object object, String fieldName, T value) throws IllegalArgumentException,
                                                                             IllegalAccessException {
        Field objectField = getPrivateField(object.getClass(), fieldName);
        assertNotNull(objectField);
        objectField.setAccessible(true);
        objectField.set(object, value);
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
                                           .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                any(GlobalDiscoveryEntry.class),
                                                anyLong(),
                                                ArgumentMatchers.<String[]> any());

        doReturn(true).when(routingTable).put(any(String.class), any(Address.class), any(Boolean.class), anyLong());

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
                                                                        routingTable,
                                                                        globalCapabilitiesDirectoryClient,
                                                                        expiredDiscoveryEntryCacheCleaner,
                                                                        freshnessUpdateIntervalMs,
                                                                        capabilitiesFreshnessUpdateExecutor,
                                                                        shutdownNotifier,
                                                                        knownGbids,
                                                                        DEFAULT_EXPIRY_TIME_MS,
                                                                        accessController,
                                                                        enableAccessControl);

        verify(capabilitiesFreshnessUpdateExecutor).schedule(addRemoveQueueRunnableCaptor.capture(),
                                                             anyLong(),
                                                             eq(TimeUnit.MILLISECONDS));
        gcdTaskSequencer = addRemoveQueueRunnableCaptor.getValue();
        gcdTaskSequencerSpy = Mockito.spy(gcdTaskSequencer);
        addRemoveWorker = new Thread(gcdTaskSequencer);
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
        gcdTaskSequencer.stop();
        addRemoveWorker.join();
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testExpiredDiscoveryEntryCacheCleanerIsInitializenCorrectly() {
        verify(expiredDiscoveryEntryCacheCleaner).scheduleCleanUpForCaches(Mockito.<ExpiredDiscoveryEntryCacheCleaner.CleanupAction> any(),
                                                                           eq(globalDiscoveryEntryCacheMock),
                                                                           eq(localDiscoveryEntryStoreMock));
    }

    private void checkAddGlobal_invokesGcdThenLocalStoreWhenGlobalAddSucceeded(String[] expectedGbids,
                                                                               Function<Void, Promise<? extends AbstractDeferred>> addFunction) throws InterruptedException {
        ArgumentCaptor<GlobalDiscoveryEntry> argumentCaptor = ArgumentCaptor.forClass(GlobalDiscoveryEntry.class);
        ArgumentCaptor<Long> remainingTtlCapture = ArgumentCaptor.forClass(Long.class);
        Semaphore successCallbackSemaphore1 = new Semaphore(0);
        Semaphore successCallbackSemaphore2 = new Semaphore(0);
        doAnswer(createAnswerWithSuccess(successCallbackSemaphore1,
                                         successCallbackSemaphore2)).when(globalCapabilitiesDirectoryClient)
                                                                    .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                         any(GlobalDiscoveryEntry.class),
                                                                         anyLong(),
                                                                         ArgumentMatchers.<String[]> any());

        Promise<? extends AbstractDeferred> promise = addFunction.apply(null);
        assertTrue(successCallbackSemaphore1.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(localDiscoveryEntryStoreMock, times(0)).add(any(DiscoveryEntry.class));
        successCallbackSemaphore2.release();
        checkPromiseSuccess(promise, "add failed");
        verify(globalCapabilitiesDirectoryClient).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      argumentCaptor.capture(),
                                                      remainingTtlCapture.capture(),
                                                      eq(expectedGbids));
        GlobalDiscoveryEntry capturedGlobalDiscoveryEntry = argumentCaptor.getValue();
        assertNotNull(capturedGlobalDiscoveryEntry);

        checkRemainingTtl(remainingTtlCapture);

        assertTrue(globalDiscoveryEntriesMatchWithUpdatedLastSeenDate(expectedGlobalDiscoveryEntry,
                                                                      capturedGlobalDiscoveryEntry));

        verify(localDiscoveryEntryStoreMock).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
        verifyNoMoreInteractions(accessController);
    }

    private void checkRemainingTtl(ArgumentCaptor<Long> remainingTtlCaptor) {
        long remainingTtl = remainingTtlCaptor.getValue().longValue();
        assertTrue(remainingTtl <= MessagingQos.DEFAULT_TTL);
        assertTrue(remainingTtl > (MessagingQos.DEFAULT_TTL / 2.0));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void add_global_invokesGcdAndStore() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        String[] expectedGbids = knownGbids;

        Function<Void, Promise<? extends AbstractDeferred>> addFunction = (Void) -> localCapabilitiesDirectory.add(discoveryEntry,
                                                                                                                   awaitGlobalRegistration);
        checkAddGlobal_invokesGcdThenLocalStoreWhenGlobalAddSucceeded(expectedGbids, addFunction);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addWithGbids_global_singleNonDefaultGbid_invokesGcdAndStore() throws InterruptedException {
        String[] gbids = new String[]{ knownGbids[1] };
        String[] expectedGbids = gbids.clone();
        final boolean awaitGlobalRegistration = true;

        Function<Void, Promise<? extends AbstractDeferred>> addFunction = (Void) -> localCapabilitiesDirectory.add(discoveryEntry,
                                                                                                                   awaitGlobalRegistration,
                                                                                                                   gbids);
        checkAddGlobal_invokesGcdThenLocalStoreWhenGlobalAddSucceeded(expectedGbids, addFunction);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addWithGbids_global_multipleGbids_invokesGcdAndStore() throws InterruptedException {
        // expectedGbids element order intentionally differs from knownGbids element order
        String[] gbids = new String[]{ knownGbids[1], knownGbids[0] };
        String[] expectedGbids = gbids.clone();
        final boolean awaitGlobalRegistration = true;

        Function<Void, Promise<? extends AbstractDeferred>> addFunction = (Void) -> localCapabilitiesDirectory.add(discoveryEntry,
                                                                                                                   awaitGlobalRegistration,
                                                                                                                   gbids);
        checkAddGlobal_invokesGcdThenLocalStoreWhenGlobalAddSucceeded(expectedGbids, addFunction);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addWithGbids_global_emptyGbidArray_addsToKnownBackends() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        String[] gbids = new String[0];
        String[] expectedGbids = knownGbids;

        Function<Void, Promise<? extends AbstractDeferred>> addFunction = (Void) -> localCapabilitiesDirectory.add(discoveryEntry,
                                                                                                                   awaitGlobalRegistration,
                                                                                                                   gbids);
        checkAddGlobal_invokesGcdThenLocalStoreWhenGlobalAddSucceeded(expectedGbids, addFunction);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addToAll_global_invokesGcdAndStore() throws InterruptedException {
        String[] expectedGbids = knownGbids.clone();
        final boolean awaitGlobalRegistration = true;

        Function<Void, Promise<? extends AbstractDeferred>> addFunction = (Void) -> localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                                                        awaitGlobalRegistration);
        checkAddGlobal_invokesGcdThenLocalStoreWhenGlobalAddSucceeded(expectedGbids, addFunction);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void taskSequencerDoesNotCrashOnExceptionAfterAddTaskFinished() throws InterruptedException,
                                                                           IllegalAccessException {
        String[] expectedGbids = knownGbids.clone();
        final boolean awaitGlobalRegistration = true;
        reset(globalCapabilitiesDirectoryClient);

        CountDownLatch cdl1 = new CountDownLatch(1);
        CountDownLatch cdl2 = new CountDownLatch(1);
        @SuppressWarnings("serial")
        class TestGde extends GlobalDiscoveryEntry {
            TestGde(GlobalDiscoveryEntry gde) {
                super(gde);
            }

            @Override
            public Version getProviderVersion() {
                cdl1.countDown();
                try {
                    // block GcdTaskSequencer until taskFinished has been called
                    cdl2.await();
                } catch (InterruptedException e) {
                    // ignore
                }
                return super.getProviderVersion();
            }
        }
        AtomicBoolean cbCalled = new AtomicBoolean();
        TestGde gde = new TestGde(globalDiscoveryEntry);
        GcdTask.CallbackCreator callbackCreator = new GcdTask.CallbackCreator() {
            @Override
            public CallbackWithModeledError<Void, DiscoveryError> createCallback() {
                return new CallbackWithModeledError<Void, DiscoveryError>() {
                    @Override
                    public void onFailure(DiscoveryError errorEnum) {
                        // taskFinished is called manually
                        logger.error("onFailure callback called, DiscoveryError {}", errorEnum);
                        cbCalled.set(true);
                    }

                    @Override
                    public void onFailure(JoynrRuntimeException runtimeException) {
                        // taskFinished is called manually
                        logger.error("onFailure callback called:", runtimeException);
                        cbCalled.set(true);
                    }

                    @Override
                    public void onSuccess(Void result) {
                        // taskFinished is called manually
                        logger.error("onSuccess callback called");
                        cbCalled.set(true);
                    }
                };
            }
        };
        GcdTask task = GcdTask.createAddTask(callbackCreator, gde, expiryDateMs, knownGbids, true);
        gcdTaskSequencer.addTask(task);

        assertTrue(cdl1.await(DEFAULT_WAIT_TIME_MS * 100, TimeUnit.MILLISECONDS));
        // call taskFinished while task is processed
        gcdTaskSequencer.taskFinished();
        cdl2.countDown();

        verify(globalCapabilitiesDirectoryClient,
               timeout(DEFAULT_WAIT_TIME_MS).times(1)).add(any(),
                                                           argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)),
                                                           anyLong(),
                                                           eq(expectedGbids));

        // check that GcdTaskSequencer is still alive
        localCapabilitiesDirectory.addToAll(discoveryEntry, awaitGlobalRegistration);
        verify(globalCapabilitiesDirectoryClient, timeout(DEFAULT_WAIT_TIME_MS).times(2)).add(any(),
                                                                                              any(),
                                                                                              anyLong(),
                                                                                              eq(expectedGbids));

        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient);
        assertFalse(cbCalled.get());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void taskSequencerNotReleasedAfterAddSuccess() throws InterruptedException, IllegalAccessException {
        String[] expectedGbids = knownGbids.clone();
        final boolean awaitGlobalRegistration = true;
        reset(globalCapabilitiesDirectoryClient);
        setFieldValue(localCapabilitiesDirectory, "gcdTaskSequencer", gcdTaskSequencerSpy);

        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                cdl.countDown();
                return null;
            }
        }).when(globalCapabilitiesDirectoryClient).add(any(), any(), anyLong(), any());
        localCapabilitiesDirectory.addToAll(discoveryEntry, awaitGlobalRegistration);
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS * 100, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(callbackCaptor.capture(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)),
                             anyLong(),
                             eq(expectedGbids));
        verify(gcdTaskSequencerSpy).addTask(argThat(arg -> GcdTask.MODE.ADD.equals(arg.getMode())));

        callbackCaptor.getValue().onSuccess(null);
        verify(gcdTaskSequencerSpy).taskFinished();
        callbackCaptor.getValue().onSuccess(null);
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);

        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient, gcdTaskSequencerSpy);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void taskSequencerNotReleasedAfterAddTimeoutOnDisabledRetry() throws InterruptedException,
                                                                         IllegalAccessException {
        String[] expectedGbids = knownGbids.clone();
        // Retries are disabled when awaitGlobalRegistration is true
        final boolean awaitGlobalRegistration = true;
        reset(globalCapabilitiesDirectoryClient);
        setFieldValue(localCapabilitiesDirectory, "gcdTaskSequencer", gcdTaskSequencerSpy);

        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                cdl.countDown();
                return null;
            }
        }).when(globalCapabilitiesDirectoryClient).add(any(), any(), anyLong(), any());
        localCapabilitiesDirectory.addToAll(discoveryEntry, awaitGlobalRegistration);
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(callbackCaptor.capture(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)),
                             anyLong(),
                             eq(expectedGbids));
        verify(gcdTaskSequencerSpy).addTask(argThat(arg -> GcdTask.MODE.ADD.equals(arg.getMode())));

        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        verify(gcdTaskSequencerSpy).taskFinished();
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        callbackCaptor.getValue().onSuccess(null);
        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient, gcdTaskSequencerSpy);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void taskSequencerRetriesAddOnJoynrTimeoutExceptionOnly() throws InterruptedException,
                                                                     IllegalAccessException {
        String[] expectedGbids = knownGbids.clone();
        final boolean awaitGlobalRegistration = false;
        reset(globalCapabilitiesDirectoryClient);
        setFieldValue(localCapabilitiesDirectory, "gcdTaskSequencer", gcdTaskSequencerSpy);

        Semaphore semaphore = new Semaphore(0);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                semaphore.release();
                return null;
            }
        }).when(globalCapabilitiesDirectoryClient).add(any(), any(), anyLong(), any());
        localCapabilitiesDirectory.addToAll(discoveryEntry, awaitGlobalRegistration);
        assertTrue(semaphore.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient, times(1)).add(callbackCaptor.capture(),
                                                                any(),
                                                                anyLong(),
                                                                eq(expectedGbids));
        verify(gcdTaskSequencerSpy).addTask(argThat(arg -> GcdTask.MODE.ADD.equals(arg.getMode())));

        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        verify(gcdTaskSequencerSpy).retryTask();
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        callbackCaptor.getValue().onSuccess(null);
        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        assertTrue(semaphore.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient, times(2)).add(callbackCaptor.capture(),
                                                                any(),
                                                                anyLong(),
                                                                eq(expectedGbids));

        verify(gcdTaskSequencerSpy, never()).taskFinished();
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        verify(gcdTaskSequencerSpy).taskFinished();
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        callbackCaptor.getValue().onSuccess(null);
        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient, gcdTaskSequencerSpy);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void taskSequencerNotReleasedAfterAddDiscoveryError() throws InterruptedException, IllegalAccessException {
        String[] expectedGbids = new String[]{ knownGbids[0] };
        final boolean awaitGlobalRegistration = true;
        reset(globalCapabilitiesDirectoryClient);
        setFieldValue(localCapabilitiesDirectory, "gcdTaskSequencer", gcdTaskSequencerSpy);

        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                cdl.countDown();
                return null;
            }
        }).when(globalCapabilitiesDirectoryClient).add(any(), any(), anyLong(), any());
        localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, expectedGbids);
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient, times(1)).add(callbackCaptor.capture(),
                                                                any(),
                                                                anyLong(),
                                                                eq(expectedGbids));
        verify(gcdTaskSequencerSpy).addTask(argThat(arg -> GcdTask.MODE.ADD.equals(arg.getMode())));

        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        verify(gcdTaskSequencerSpy).taskFinished();

        callbackCaptor.getValue().onSuccess(null);
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient, gcdTaskSequencerSpy);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addRemoveAddInSequence_awaitGlobalRegistration_true() throws InterruptedException {
        // A sequence of add-remove-add for the same provider could lead to a non registered provider in earlier versions
        final Boolean awaitGlobalRegistration = true;
        final String participantId = discoveryEntry.getParticipantId();
        discoveryEntry.getQos().setScope(ProviderScope.GLOBAL);
        expectedDiscoveryEntry = new DiscoveryEntry(discoveryEntry);
        globalDiscoveryEntry = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry, globalAddress1);

        // checked in remove
        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock).lookup(eq(participantId), anyLong());

        Semaphore addSemaphore1 = new Semaphore(0);
        Semaphore addSemaphore2 = new Semaphore(0);
        doAnswer(createAnswerWithSuccess(addSemaphore1,
                                         addSemaphore2)).when(globalCapabilitiesDirectoryClient)
                                                        .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                             argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry)),
                                                             anyLong(),
                                                             any(String[].class));
        Semaphore removeSemaphore1 = new Semaphore(0);
        Semaphore removeSemaphore2 = new Semaphore(0);
        doAnswer(createAnswerWithSuccess(removeSemaphore1,
                                         removeSemaphore2)).when(globalCapabilitiesDirectoryClient)
                                                           .remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                   eq(participantId),
                                                                   any(String[].class));

        // 3 actions. 1 global add 1 lcd.remove and 1 global add
        Promise<DeferredVoid> promiseAdd1 = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        Promise<DeferredVoid> promiseRemove = localCapabilitiesDirectory.remove(discoveryEntry.getParticipantId());
        Promise<DeferredVoid> promiseAdd2 = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);

        // add1
        assertTrue(addSemaphore1.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                      anyLong(),
                                                      any(String[].class));
        verify(localDiscoveryEntryStoreMock, times(0)).add(any(DiscoveryEntry.class));
        addSemaphore2.release();
        checkPromiseSuccess(promiseAdd1, "add failed");
        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));

        // remove
        assertTrue(removeSemaphore1.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient).remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                         eq(participantId),
                                                         any(String[].class));
        verify(localDiscoveryEntryStoreMock, times(0)).remove(eq(participantId));
        removeSemaphore2.release();
        checkPromiseSuccess(promiseRemove, "remove failed");
        verify(localDiscoveryEntryStoreMock, timeout(DEFAULT_WAIT_TIME_MS).times(1)).remove(eq(participantId));

        // add2
        assertTrue(addSemaphore1.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient,
               times(2)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                             anyLong(),
                             any(String[].class));
        verify(localDiscoveryEntryStoreMock, times(1)).add(any(DiscoveryEntry.class));
        addSemaphore2.release();
        checkPromiseSuccess(promiseAdd2, "add failed");
        verify(localDiscoveryEntryStoreMock,
               times(2)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));

        InOrder inOrderGlobal = inOrder(globalCapabilitiesDirectoryClient);
        inOrderGlobal.verify(globalCapabilitiesDirectoryClient)
                     .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                          argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                          anyLong(),
                          any(String[].class));
        inOrderGlobal.verify(globalCapabilitiesDirectoryClient)
                     .remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             eq(participantId),
                             any(String[].class));
        inOrderGlobal.verify(globalCapabilitiesDirectoryClient)
                     .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                          argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                          anyLong(),
                          any(String[].class));

        InOrder inOrderLocal = inOrder(localDiscoveryEntryStoreMock);
        inOrderLocal.verify(localDiscoveryEntryStoreMock, times(1))
                    .add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        inOrderLocal.verify(localDiscoveryEntryStoreMock, times(1)).remove(eq(participantId));
        inOrderLocal.verify(localDiscoveryEntryStoreMock, times(1))
                    .add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));

        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addAndRemoveAndAddCalledInRowSameDiscoveryEntry_awaitGlobalRegistration_false() throws InterruptedException {
        // A sequence of add-remove-add for the same provider could lead to a non registered provider in earlier versions
        // This is still the case for awaitGlobalRegistration = false if the second add is executed before the remove has
        // returned from GCD
        final Boolean awaitGlobalRegistration = false;
        final String participantId = discoveryEntry.getParticipantId();
        discoveryEntry.getQos().setScope(ProviderScope.GLOBAL);
        expectedDiscoveryEntry = new DiscoveryEntry(discoveryEntry);
        globalDiscoveryEntry = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry, globalAddress1);

        // checked in remove
        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock).lookup(eq(participantId), anyLong());

        Semaphore addSemaphore1 = new Semaphore(0);
        Semaphore addSemaphore2 = new Semaphore(0);
        doAnswer(createAnswerWithSuccess(addSemaphore1,
                                         addSemaphore2)).when(globalCapabilitiesDirectoryClient)
                                                        .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                             argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry)),
                                                             anyLong(),
                                                             any(String[].class));
        Semaphore removeSemaphore1 = new Semaphore(0);
        Semaphore removeSemaphore2 = new Semaphore(0);
        doAnswer(createAnswerWithSuccess(removeSemaphore1,
                                         removeSemaphore2)).when(globalCapabilitiesDirectoryClient)
                                                           .remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                   eq(participantId),
                                                                   any(String[].class));

        // 3 actions. 1 global add 1 lcd.remove and 1 global add
        Promise<DeferredVoid> promiseAdd1 = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        checkPromiseSuccess(promiseAdd1, "add failed");
        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));

        Promise<DeferredVoid> promiseRemove = localCapabilitiesDirectory.remove(discoveryEntry.getParticipantId());
        checkPromiseSuccess(promiseRemove, "remove failed");

        Promise<DeferredVoid> promiseAdd2 = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        checkPromiseSuccess(promiseAdd2, "add failed");
        verify(localDiscoveryEntryStoreMock,
               times(2)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));

        // add1
        assertTrue(addSemaphore1.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                      anyLong(),
                                                      any(String[].class));
        addSemaphore2.release();

        // remove
        assertTrue(removeSemaphore1.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient).remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                         eq(participantId),
                                                         any(String[].class));
        verify(localDiscoveryEntryStoreMock, times(0)).remove(eq(participantId));
        removeSemaphore2.release();
        verify(localDiscoveryEntryStoreMock, timeout(DEFAULT_WAIT_TIME_MS).times(1)).remove(eq(participantId));

        // add2
        assertTrue(addSemaphore1.tryAcquire(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient,
               times(2)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                             anyLong(),
                             any(String[].class));
        addSemaphore2.release();

        InOrder inOrderGlobal = inOrder(globalCapabilitiesDirectoryClient);
        inOrderGlobal.verify(globalCapabilitiesDirectoryClient)
                     .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                          argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                          anyLong(),
                          any(String[].class));
        inOrderGlobal.verify(globalCapabilitiesDirectoryClient)
                     .remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             eq(participantId),
                             any(String[].class));
        inOrderGlobal.verify(globalCapabilitiesDirectoryClient)
                     .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                          argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                          anyLong(),
                          any(String[].class));

        InOrder inOrderLocal = inOrder(localDiscoveryEntryStoreMock);
        inOrderLocal.verify(localDiscoveryEntryStoreMock, times(1))
                    .add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        inOrderLocal.verify(localDiscoveryEntryStoreMock, times(1))
                    .add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        inOrderLocal.verify(localDiscoveryEntryStoreMock, times(1)).remove(eq(participantId));

        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void localAddRemoveAddInSequence_doesNotInvokeGcdAndCache() throws InterruptedException {
        final String participantId = discoveryEntry.getParticipantId();
        discoveryEntry.getQos().setScope(ProviderScope.LOCAL);
        expectedDiscoveryEntry.getQos().setScope(ProviderScope.LOCAL);
        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock).lookup(eq(participantId), anyLong());

        // 3 actions. 1 global add 1 lcd.remove and 1 global add
        Promise<DeferredVoid> promiseAdd1 = localCapabilitiesDirectory.add(discoveryEntry);
        Promise<DeferredVoid> promiseRemove = localCapabilitiesDirectory.remove(discoveryEntry.getParticipantId());
        Promise<DeferredVoid> promiseAdd2 = localCapabilitiesDirectory.add(discoveryEntry);

        checkPromiseSuccess(promiseAdd1, "add failed");
        checkPromiseSuccess(promiseRemove, "remove failed");
        checkPromiseSuccess(promiseAdd2, "add failed");

        verify(globalDiscoveryEntryCacheMock, never()).add(ArgumentMatchers.<GlobalDiscoveryEntry> any());
        verify(globalDiscoveryEntryCacheMock, never()).remove(anyString());
        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient);

        InOrder inOrder = inOrder(localDiscoveryEntryStoreMock);
        inOrder.verify(localDiscoveryEntryStoreMock, times(1))
               .add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        inOrder.verify(localDiscoveryEntryStoreMock, times(1)).remove(eq(participantId));
        inOrder.verify(localDiscoveryEntryStoreMock, times(1))
               .add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void add_local_doesNotInvokeGcdAndCache() throws InterruptedException {
        discoveryEntry.getQos().setScope(ProviderScope.LOCAL);
        expectedDiscoveryEntry.getQos().setScope(ProviderScope.LOCAL);

        Thread.sleep(100);
        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry);
        checkPromiseSuccess(promise, "add failed");
        verify(localDiscoveryEntryStoreMock).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(globalCapabilitiesDirectoryClient,
               never()).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                            any(GlobalDiscoveryEntry.class),
                            anyLong(),
                            ArgumentMatchers.<String[]> any());
        verify(globalDiscoveryEntryCacheMock, never()).add(ArgumentMatchers.<GlobalDiscoveryEntry> any());
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
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)),
                             remainingTtlCapture.capture(),
                             ArgumentMatchers.<String[]> any());

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
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry2)),
                             remainingTtlCapture.capture(),
                             ArgumentMatchers.<String[]> any());
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
        final DiscoveryEntry expectedDiscoveryEntry = new DiscoveryEntry(discoveryEntry);
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
                                                          .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                               argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                               anyLong(),
                                                               ArgumentMatchers.<String[]> any());

        final boolean awaitGlobalRegistration = true;
        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);

        checkPromiseException(promise, new ProviderRuntimeException(exception.toString()));
        ArgumentCaptor<Long> remainingTtlCaptor = ArgumentCaptor.forClass(Long.class);
        verify(globalCapabilitiesDirectoryClient).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                      remainingTtlCaptor.capture(),
                                                      ArgumentMatchers.<String[]> any());
        checkRemainingTtl(remainingTtlCaptor);

        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(localDiscoveryEntryStoreMock, times(0)).add(any(DiscoveryEntry.class));
        verify(localDiscoveryEntryStoreMock, times(0)).remove(anyString());

        reset(globalCapabilitiesDirectoryClient, localDiscoveryEntryStoreMock);

        doAnswer(createAnswerWithSuccess()).when(globalCapabilitiesDirectoryClient)
                                           .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                anyLong(),
                                                ArgumentMatchers.<String[]> any());

        DiscoveryEntry expectedDiscoveryEntry2 = new DiscoveryEntry(expectedDiscoveryEntry);
        expectedDiscoveryEntry2.setLastSeenDateMs(System.currentTimeMillis());
        GlobalDiscoveryEntry expectedGlobalDiscoveryEntry2 = new GlobalDiscoveryEntry(globalDiscoveryEntry);
        expectedGlobalDiscoveryEntry2.setLastSeenDateMs(System.currentTimeMillis());

        promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);

        checkPromiseSuccess(promise, "add failed");
        verify(globalCapabilitiesDirectoryClient).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry2)),
                                                      remainingTtlCaptor.capture(),
                                                      ArgumentMatchers.<String[]> any());
        checkRemainingTtl(remainingTtlCaptor);
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry2)));
        verify(localDiscoveryEntryStoreMock, times(0)).remove(anyString());
    }

    private void testAddWithGbidsIsProperlyRejected(DiscoveryError expectedError,
                                                    boolean awaitGlobalRegistration) throws InterruptedException {
        reset(globalCapabilitiesDirectoryClient, localDiscoveryEntryStoreMock);
        doAnswer(createVoidAnswerWithDiscoveryError(expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                   .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                        argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                        anyLong(),
                                                                        ArgumentMatchers.<String[]> any());

        String[] gbids = new String[]{ knownGbids[0] };
        String[] expectedGbids = gbids.clone();
        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids);
        if (awaitGlobalRegistration) {
            checkPromiseError(promise, expectedError);
        } else {
            checkPromiseSuccess(promise, "add withoud awaitGlobalRegistration failed");
        }

        verify(globalCapabilitiesDirectoryClient,
               timeout(DEFAULT_WAIT_TIME_MS).times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                           argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                           anyLong(),
                                                           eq(expectedGbids));
        verify(localDiscoveryEntryStoreMock, times(awaitGlobalRegistration ? 0 : 1)).add(any(DiscoveryEntry.class));
        verify(localDiscoveryEntryStoreMock, times(0)).remove(any(String.class));
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
    }

    private void testAddIsProperlyRejected(DiscoveryError expectedError) throws InterruptedException {
        reset(globalCapabilitiesDirectoryClient);
        String[] expectedGbids = knownGbids.clone();
        doAnswer(createVoidAnswerWithDiscoveryError(expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                   .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                        argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                        anyLong(),
                                                                        ArgumentMatchers.<String[]> any());

        final boolean awaitGlobalRegistration = true;
        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        checkPromiseErrorInProviderRuntimeException(promise, expectedError);

        verify(globalCapabilitiesDirectoryClient).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                      anyLong(),
                                                      eq(expectedGbids));
        verify(localDiscoveryEntryStoreMock, times(0)).add(any(DiscoveryEntry.class));
        verify(localDiscoveryEntryStoreMock, times(0)).remove(any(String.class));
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddWithGbidsIsProperlyRejectedAndHandlesDiscoveryError() throws InterruptedException {
        final boolean awaitGlobal = true;
        testAddWithGbidsIsProperlyRejected(DiscoveryError.INVALID_GBID, awaitGlobal);
        testAddWithGbidsIsProperlyRejected(DiscoveryError.UNKNOWN_GBID, awaitGlobal);
        testAddWithGbidsIsProperlyRejected(DiscoveryError.INTERNAL_ERROR, awaitGlobal);
        final boolean noAwaitGlobal = false;
        testAddWithGbidsIsProperlyRejected(DiscoveryError.INVALID_GBID, noAwaitGlobal);
        testAddWithGbidsIsProperlyRejected(DiscoveryError.UNKNOWN_GBID, noAwaitGlobal);
        testAddWithGbidsIsProperlyRejected(DiscoveryError.INTERNAL_ERROR, noAwaitGlobal);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddIsProperlyRejectedAndHandlesDiscoveryError() throws InterruptedException {
        testAddIsProperlyRejected(DiscoveryError.INVALID_GBID);
        testAddIsProperlyRejected(DiscoveryError.UNKNOWN_GBID);
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
    public void globalAdd_withoutAwaitGlobalRegistration_retryAfterTimeout() throws InterruptedException {
        CountDownLatch cdl = new CountDownLatch(2);
        doAnswer(createVoidAnswerWithException(cdl,
                                               new JoynrTimeoutException(0))).when(globalCapabilitiesDirectoryClient)
                                                                             .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                                  argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                                  anyLong(),
                                                                                  ArgumentMatchers.<String[]> any());

        final boolean awaitGlobalRegistration = false;
        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient,
               atLeast(2)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                               argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                               anyLong(),
                               any());
        checkPromiseSuccess(promise, "add failed");
    }

    @Test(timeout = TEST_TIMEOUT)
    public void globalAdd_withAwaitGlobalRegistration_noRetryAfterTimeout() throws InterruptedException {
        JoynrTimeoutException timeoutException = new JoynrTimeoutException(0);
        ProviderRuntimeException expectedException = new ProviderRuntimeException(timeoutException.toString());

        doAnswer(createVoidAnswerWithException(timeoutException)).when(globalCapabilitiesDirectoryClient)
                                                                 .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                      anyLong(),
                                                                      ArgumentMatchers.<String[]> any());

        final boolean awaitGlobalRegistration = true;
        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        checkPromiseException(promise, expectedException);

        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                             anyLong(),
                             any());
        verify(localDiscoveryEntryStoreMock, times(0)).remove(anyString());
        verify(localDiscoveryEntryStoreMock, times(0)).add(any(DiscoveryEntry.class));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void globalAdd_withoutAwaitGlobalRegistration_noRetryAfterRuntimeException() throws InterruptedException {
        JoynrRuntimeException runtimeException = new JoynrRuntimeException("custom runtime exception");

        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(createVoidAnswerWithException(cdl,
                                               runtimeException)).when(globalCapabilitiesDirectoryClient)
                                                                 .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                      anyLong(),
                                                                      ArgumentMatchers.<String[]> any());

        final boolean awaitGlobalRegistration = false;
        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                             anyLong(),
                             any());
        verify(localDiscoveryEntryStoreMock, times(0)).remove(eq(globalDiscoveryEntry.getParticipantId()));
        checkPromiseSuccess(promise, "add failed");
    }

    @Test(timeout = TEST_TIMEOUT)
    public void globalAdd_withoutAwaitGlobalRegistration_noRetryAfterDiscoveryError() throws InterruptedException {
        DiscoveryError expectedError = DiscoveryError.UNKNOWN_GBID;

        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(createVoidAnswerWithDiscoveryError(cdl,
                                                    expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                   .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                        argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                        anyLong(),
                                                                        ArgumentMatchers.<String[]> any());

        final boolean awaitGlobalRegistration = false;
        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                             anyLong(),
                             any());
        verify(localDiscoveryEntryStoreMock, times(0)).remove(eq(globalDiscoveryEntry.getParticipantId()));
        checkPromiseSuccess(promise, "add failed");
    }

    private void globalAddUsesCorrectRemainingTtl(boolean awaitGlobalRegistration) throws InterruptedException {
        int defaultTtl = MessagingQos.DEFAULT_TTL;

        DiscoveryEntry discoveryEntry1 = new DiscoveryEntry(discoveryEntry);
        discoveryEntry1.setParticipantId("participantId1");
        DiscoveryEntry discoveryEntry2 = new DiscoveryEntry(discoveryEntry);
        discoveryEntry2.setParticipantId("participantId2");

        GlobalDiscoveryEntry globalDiscoveryEntry1 = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry1,
                                                                                                         globalAddress1);
        GlobalDiscoveryEntry globalDiscoveryEntry2 = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry2,
                                                                                                         globalAddress1);

        ArgumentCaptor<Long> remainingTtlCapture = ArgumentCaptor.forClass(Long.class);

        CountDownLatch startOfFirstAddCdl = new CountDownLatch(1);
        CountDownLatch endOfFirstAddCdl = new CountDownLatch(1);
        long sleepTime = 1000l;
        doAnswer(createAnswerWithDelayedSuccess(startOfFirstAddCdl,
                                                endOfFirstAddCdl,
                                                sleepTime)).when(globalCapabilitiesDirectoryClient)
                                                           .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry1)),
                                                                anyLong(),
                                                                ArgumentMatchers.<String[]> any());

        CountDownLatch secondAddCdl = new CountDownLatch(1);
        doAnswer(createAnswerWithSuccess(secondAddCdl)).when(globalCapabilitiesDirectoryClient)
                                                       .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                            argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry2)),
                                                            anyLong(),
                                                            ArgumentMatchers.<String[]> any());

        localCapabilitiesDirectory.add(discoveryEntry1, awaitGlobalRegistration);
        localCapabilitiesDirectory.add(discoveryEntry2, awaitGlobalRegistration);
        assertTrue(startOfFirstAddCdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry1)),
                             remainingTtlCapture.capture(),
                             any());

        long firstNow = System.currentTimeMillis();
        long capturedFirstAddRemainingTtl = remainingTtlCapture.getValue();

        assertTrue(endOfFirstAddCdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        assertTrue(secondAddCdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry2)),
                             remainingTtlCapture.capture(),
                             any());

        long secondNow = System.currentTimeMillis();
        long delta = secondNow - firstNow;
        long capturedSecondAddRemainingTtl = remainingTtlCapture.getValue();
        long epsilon = 300;
        if (awaitGlobalRegistration) {
            assertTrue(capturedFirstAddRemainingTtl <= defaultTtl);
            assertTrue(capturedFirstAddRemainingTtl > defaultTtl - epsilon);
            assertTrue(capturedSecondAddRemainingTtl <= defaultTtl - delta + epsilon);
            assertTrue(capturedSecondAddRemainingTtl > defaultTtl - delta - epsilon);
        } else {
            assertEquals(capturedFirstAddRemainingTtl, defaultTtl);
            assertEquals(capturedSecondAddRemainingTtl, defaultTtl);
        }
    }

    @Test(timeout = TEST_TIMEOUT)
    public void globalAdd_withAwaitGlobalRegistration_usesCorrectRemainingTtl() throws InterruptedException {
        globalAddUsesCorrectRemainingTtl(true);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void globalAdd_withoutAwaitGlobalRegistration_usesCorrectRemainingTtl() throws InterruptedException {
        globalAddUsesCorrectRemainingTtl(false);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void addSameGbidTwiceInARow() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        String[] gbids = new String[]{ knownGbids[0] };
        String[] expectedGbids = gbids.clone();
        DiscoveryEntry discoveryEntry2 = new DiscoveryEntry(discoveryEntry);

        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                              .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                   argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry)),
                                                   anyLong(),
                                                   eq(expectedGbids));

        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids);

        checkPromiseSuccess(promise, "add failed");
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
        ArgumentCaptor<Long> remainingTtlCaptor = ArgumentCaptor.forClass(Long.class);
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
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
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
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
        DiscoveryEntryWithMetaInfo expectedEntryWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                                   discoveryEntry);
        DiscoveryEntry discoveryEntry2 = new DiscoveryEntry(discoveryEntry);

        doAnswer(createAnswerWithSuccess()).when(globalCapabilitiesDirectoryClient)
                                           .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                any(GlobalDiscoveryEntry.class),
                                                anyLong(),
                                                ArgumentMatchers.<String[]> any());

        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids1);
        checkPromiseSuccess(promise, "add failed");

        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
        ArgumentCaptor<Long> remainingTtlCaptor = ArgumentCaptor.forClass(Long.class);
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
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
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedGlobalDiscoveryEntry2)),
                             remainingTtlCaptor.capture(),
                             eq(expectedGbids2));
        checkRemainingTtl(remainingTtlCaptor);

        // provider is now registered for both GBIDs
        doReturn(Arrays.asList(discoveryEntry)).when(localDiscoveryEntryStoreMock).lookupGlobalEntries(eq(new String[]{
                expectedDiscoveryEntry.getDomain() }), eq(INTERFACE_NAME));

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
        assertTrue(discoveryEntriesWithMetaInfoMatchWithUpdatedLastSeenDate(expectedEntryWithMetaInfo, result1[0]));
        DiscoveryEntryWithMetaInfo[] result2 = (DiscoveryEntryWithMetaInfo[]) checkPromiseSuccess(promiseLookup2,
                                                                                                  "lookup failed")[0];
        assertEquals(1, result2.length);
        assertTrue(discoveryEntriesWithMetaInfoMatchWithUpdatedLastSeenDate(expectedEntryWithMetaInfo, result2[0]));

        verify(globalCapabilitiesDirectoryClient,
               times(0)).lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                ArgumentMatchers.<String[]> any(),
                                anyString(),
                                anyLong(),
                                ArgumentMatchers.<String[]> any());
    }

    void checkAddRemovesCachedEntryWithSameParticipantId(ProviderScope scope) throws InterruptedException {
        discoveryEntry.getQos().setScope(scope);
        expectedDiscoveryEntry.getQos().setScope(scope);

        doReturn(false).when(localDiscoveryEntryStoreMock)
                       .hasDiscoveryEntry(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));

        doReturn(Optional.of(globalDiscoveryEntry)).when(globalDiscoveryEntryCacheMock)
                                                   .lookup(eq(expectedDiscoveryEntry.getParticipantId()),
                                                           eq(Long.MAX_VALUE));

        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, true, knownGbids);

        checkPromiseSuccess(promise, "add failed");
        verify(localDiscoveryEntryStoreMock).hasDiscoveryEntry(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(localDiscoveryEntryStoreMock, never()).lookup(any(), any());

        verify(globalDiscoveryEntryCacheMock, times(1)).lookup(eq(expectedGlobalDiscoveryEntry.getParticipantId()),
                                                               eq(Long.MAX_VALUE));
        verify(globalDiscoveryEntryCacheMock, times(1)).remove(eq(expectedGlobalDiscoveryEntry.getParticipantId()));

        int calls = (scope == ProviderScope.GLOBAL ? 1 : 0);
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient, times(calls)).add(any(), any(), anyLong(), any());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void add_removesCachedEntryWithSameParticipantId_ProviderScope_LOCAL() throws InterruptedException {
        checkAddRemovesCachedEntryWithSameParticipantId(ProviderScope.LOCAL);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void add_removesCachedEntryWithSameParticipantId_ProviderScope_GLOBAL() throws InterruptedException {
        checkAddRemovesCachedEntryWithSameParticipantId(ProviderScope.GLOBAL);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddKnownLocalEntryDoesNothing() throws InterruptedException {
        discoveryEntry.getQos().setScope(ProviderScope.LOCAL);
        expectedDiscoveryEntry.getQos().setScope(ProviderScope.LOCAL);
        doReturn(true).when(localDiscoveryEntryStoreMock)
                      .hasDiscoveryEntry(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock)
                                             .lookup(eq(expectedDiscoveryEntry.getParticipantId()), eq(Long.MAX_VALUE));

        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, false, knownGbids);
        checkPromiseSuccess(promise, "add failed");

        verify(localDiscoveryEntryStoreMock).hasDiscoveryEntry(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
        verify(localDiscoveryEntryStoreMock).lookup(eq(expectedDiscoveryEntry.getParticipantId()), eq(Long.MAX_VALUE));

        verify(globalDiscoveryEntryCacheMock, never()).lookup(anyString(), anyLong());
        verify(globalDiscoveryEntryCacheMock, never()).remove(anyString());
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
        // check whether the local entry is in the global cache (unlikely). If so, then remove it
        verify(globalDiscoveryEntryCacheMock, times(1)).lookup(anyString(), anyLong());
        verify(globalDiscoveryEntryCacheMock, never()).remove(anyString());
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
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
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
               never()).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                            any(GlobalDiscoveryEntry.class),
                            anyLong(),
                            ArgumentMatchers.<String[]> any());
        verify(localDiscoveryEntryStoreMock,
               times(1)).add(argThat(new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry)));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddToAllIsProperlyRejected_exception() throws InterruptedException {
        String[] expectedGbids = knownGbids.clone();
        JoynrRuntimeException exception = new JoynrRuntimeException("add failed");
        ProviderRuntimeException expectedException = new ProviderRuntimeException(exception.toString());

        doAnswer(createVoidAnswerWithException(exception)).when(globalCapabilitiesDirectoryClient)
                                                          .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                               argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                               anyLong(),
                                                               ArgumentMatchers.<String[]> any());

        Promise<AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry, true);

        checkPromiseException(promise, expectedException);
        verify(globalCapabilitiesDirectoryClient).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                      anyLong(),
                                                      eq(expectedGbids));
        verify(localDiscoveryEntryStoreMock, times(0)).add(any(DiscoveryEntry.class));
        verify(localDiscoveryEntryStoreMock, times(0)).remove(any(String.class));
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
    }

    private void testAddToAllIsProperlyRejected(DiscoveryError expectedError) throws InterruptedException {
        String[] expectedGbids = knownGbids.clone();
        doAnswer(createVoidAnswerWithDiscoveryError(expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                   .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                        argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                        anyLong(),
                                                                        ArgumentMatchers.<String[]> any());

        Promise<AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry, true);

        checkPromiseError(promise, expectedError);
        verify(globalCapabilitiesDirectoryClient).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                      anyLong(),
                                                      eq(expectedGbids));
        verify(localDiscoveryEntryStoreMock, times(0)).add(any(DiscoveryEntry.class));
        verify(localDiscoveryEntryStoreMock, times(0)).remove(any(String.class));
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));

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

    static Answer<Void> createAnswerWithSuccess() {
        return createAnswerWithSuccess((Semaphore) null, null);
    }

    private static Answer<Void> createAnswerWithSuccess(Semaphore successCallbackSemaphore1,
                                                        Semaphore successCallbackSemaphore2) {
        return new Answer<Void>() {

            @SuppressWarnings("unchecked")
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                Object[] args = invocation.getArguments();
                if (successCallbackSemaphore1 != null) {
                    logger.debug("success callback called");
                    successCallbackSemaphore1.release();
                }
                if (successCallbackSemaphore2 != null) {
                    logger.debug("waiting for Semaphore in success callback");
                    successCallbackSemaphore2.tryAcquire(TEST_TIMEOUT, TimeUnit.MILLISECONDS);
                }
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
                                          .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
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
        verify(routingTable, never()).incrementReferenceCount(any());

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
        verify(routingTable, never()).incrementReferenceCount(any());

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
        verify(routingTable, never()).incrementReferenceCount(any());

        // add global entry
        String globalParticipantId = "globalParticipant";
        GlobalDiscoveryEntry capInfo = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                domain1,
                                                                interfaceName1,
                                                                globalParticipantId,
                                                                new ProviderQos(),
                                                                System.currentTimeMillis(),
                                                                expiryDateMs,
                                                                publicKeyId,
                                                                globalAddress1Serialized);
        caps.add(capInfo);
        doAnswer(createLookupAnswer(caps)).when(globalCapabilitiesDirectoryClient)
                                          .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
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
        verify(routingTable, times(1)).put(eq(globalParticipantId), any(Address.class), eq(true), anyLong());
        verify(routingTable, never()).incrementReferenceCount(any());

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        reset((Object) globalDiscoveryEntryCacheMock);
        reset((Object) routingTable);

        when(globalDiscoveryEntryCacheMock.lookup(eq(domains),
                                                  eq(interfaceName1),
                                                  eq(discoveryQos.getCacheMaxAge()))).thenReturn(Arrays.asList(capInfo));

        doReturn(true).when(routingTable).put(eq(globalParticipantId), any(Address.class), eq(true), anyLong());
        Promise<Lookup1Deferred> promise5 = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(4,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise5,
                                             1); // 1 cached entry
        verify(routingTable, times(1)).put(eq(globalParticipantId), any(Address.class), eq(true), anyLong());
        verify(routingTable, never()).incrementReferenceCount(any());
        reset((Object) routingTable);

        // and now, invalidate the existing cached global values, resulting in another call to globalcapclient
        discoveryQos.setCacheMaxAge(0L);
        Thread.sleep(1);

        doReturn(true).when(routingTable).put(eq(globalParticipantId), any(Address.class), eq(true), anyLong());
        // now, another lookup call shall call the globalCapabilitiesDirectoryClient, as the global cap dir is expired
        Promise<Lookup1Deferred> promise6 = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(5,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise6,
                                             1); // 1 global entry
        verify(routingTable, times(1)).put(eq(globalParticipantId), any(Address.class), eq(true), anyLong());
        verify(routingTable, never()).incrementReferenceCount(any());
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
               times(gcdTimesCalled)).lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                             org.mockito.hamcrest.MockitoHamcrest.argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
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
                                          .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
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
        verify(routingTable, never()).incrementReferenceCount(any());

        // add local entry
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        String localParticipantId = "localParticipant";
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                           domain1,
                                                           interfaceName1,
                                                           localParticipantId,
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
        verify(routingTable, times(1)).incrementReferenceCount(eq(localParticipantId));

        // add global entry
        String globalParticipantId = "globalParticipant";
        GlobalDiscoveryEntry capInfo = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                domain1,
                                                                interfaceName1,
                                                                globalParticipantId,
                                                                new ProviderQos(),
                                                                System.currentTimeMillis(),
                                                                expiryDateMs,
                                                                publicKeyId,
                                                                globalAddress1Serialized);
        caps.add(capInfo);
        promise = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(1,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             1); // 1 local entry
        verify(routingTable, times(2)).incrementReferenceCount(eq(localParticipantId));
        verify(routingTable, never()).put(anyString(), any(Address.class), eq(true), anyLong());

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
        verify(routingTable, times(1)).put(eq(globalParticipantId), any(Address.class), eq(true), anyLong());

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
        verify(routingTable, times(2)).put(eq(globalParticipantId), any(Address.class), eq(true), anyLong());

        // and now, invalidate the existing cached global values, resulting in another call to globalcapclient
        discoveryQos.setCacheMaxAge(0L);
        Thread.sleep(1);

        promise = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(3,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             1); // 1 global entry
        verify(routingTable, times(3)).put(eq(globalParticipantId), any(Address.class), eq(true), anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantId_defaultScopelocalAndGlobal_localEntry() throws InterruptedException {
        DiscoveryQos discoveryQos = new DiscoveryQos(Long.MAX_VALUE,
                                                     Long.MAX_VALUE,
                                                     DiscoveryScope.LOCAL_AND_GLOBAL,
                                                     false);

        DiscoveryEntryWithMetaInfo expectedDiscoveryEntryWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                                            expectedDiscoveryEntry);

        // add locally registered entry
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        when(localDiscoveryEntryStoreMock.lookup(eq(expectedDiscoveryEntry.getParticipantId()),
                                                 eq(discoveryQos.getCacheMaxAge()))).thenReturn(Optional.of(discoveryEntry));

        reset(globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);
        Promise<Lookup3Deferred> lookupPromise = localCapabilitiesDirectory.lookup(expectedDiscoveryEntry.getParticipantId());

        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo retrievedCapabilityEntry = (DiscoveryEntryWithMetaInfo) values[0];
        assertEquals(expectedDiscoveryEntryWithMetaInfo, retrievedCapabilityEntry);

        verify(localDiscoveryEntryStoreMock).lookup(eq(expectedDiscoveryEntry.getParticipantId()), eq(Long.MAX_VALUE));
        verifyNoMoreInteractions(globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);
        verify(routingTable, times(1)).incrementReferenceCount(eq(discoveryEntry.getParticipantId()));
        verify(routingTable, never()).put(eq(discoveryEntry.getParticipantId()),
                                          any(Address.class),
                                          any(Boolean.class),
                                          anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterface_localAndGlobal() throws InterruptedException {
        List<GlobalDiscoveryEntry> globalEntries = new ArrayList<GlobalDiscoveryEntry>();
        String domain1 = "domain1";
        String[] domains = new String[]{ domain1 };
        String interfaceName1 = "interfaceName1";
        DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.LOCAL_AND_GLOBAL, false);

        doAnswer(createLookupAnswer(globalEntries)).when(globalCapabilitiesDirectoryClient)
                                                   .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
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
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());

        // add local entry
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        String localParticipantId = "localParticipant";
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                           domain1,
                                                           interfaceName1,
                                                           localParticipantId,
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
        verify(routingTable, times(1)).incrementReferenceCount(eq(localParticipantId));
        verify(routingTable, never()).put(anyString(), any(Address.class), eq(true), anyLong());

        // add global entry
        String globalParticipantId = "globalParticipant";
        GlobalDiscoveryEntry capInfo = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                domain1,
                                                                interfaceName1,
                                                                globalParticipantId,
                                                                new ProviderQos(),
                                                                System.currentTimeMillis(),
                                                                expiryDateMs,
                                                                publicKeyId,
                                                                globalAddress1Serialized);
        globalEntries.add(capInfo);
        doAnswer(createLookupAnswer(globalEntries)).when(globalCapabilitiesDirectoryClient)
                                                   .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
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
        verify(routingTable, times(2)).incrementReferenceCount(eq(localParticipantId));
        verify(routingTable, times(1)).put(eq(globalParticipantId), any(Address.class), eq(true), anyLong());

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
        verify(routingTable, times(3)).incrementReferenceCount(eq(localParticipantId));
        verify(routingTable, times(2)).put(eq(globalParticipantId), any(Address.class), eq(true), anyLong());

        // and now, invalidate the existing cached global values, resulting in another call to glocalcapclient
        discoveryQos.setCacheMaxAge(0L);
        Thread.sleep(1);

        promise = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(4,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             2); // 1 local, 1 global entry
        verify(routingTable, times(4)).incrementReferenceCount(eq(localParticipantId));
        verify(routingTable, times(3)).put(eq(globalParticipantId), any(Address.class), eq(true), anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterface_emptyGbid_replacesReturnedGbidsWithEmpty() throws InterruptedException {
        String[] gbids = new String[]{ "" };

        LocalCapabilitiesDirectoryImpl localCapabilitiesDirectoryWithEmptyGbids = new LocalCapabilitiesDirectoryImpl(capabilitiesProvisioning,
                                                                                                                     globalAddressProvider,
                                                                                                                     localDiscoveryEntryStoreMock,
                                                                                                                     globalDiscoveryEntryCacheMock,
                                                                                                                     routingTable,
                                                                                                                     globalCapabilitiesDirectoryClient,
                                                                                                                     expiredDiscoveryEntryCacheCleaner,
                                                                                                                     freshnessUpdateIntervalMs,
                                                                                                                     capabilitiesFreshnessUpdateExecutor,
                                                                                                                     shutdownNotifier,
                                                                                                                     gbids,
                                                                                                                     DEFAULT_EXPIRY_TIME_MS,
                                                                                                                     accessController,
                                                                                                                     enableAccessControl);

        List<GlobalDiscoveryEntry> globalEntries = new ArrayList<GlobalDiscoveryEntry>();

        String domain1 = "domain1";
        String[] domains = new String[]{ domain1 };
        String interfaceName1 = "interfaceName1";
        DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.LOCAL_AND_GLOBAL, false);

        // add global entries
        String globalParticipantId = "globalParticipant";
        GlobalDiscoveryEntry capInfo = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                domain1,
                                                                interfaceName1,
                                                                globalParticipantId,
                                                                new ProviderQos(),
                                                                System.currentTimeMillis(),
                                                                expiryDateMs,
                                                                publicKeyId,
                                                                globalAddress1Serialized);
        globalEntries.add(capInfo);

        String globalParticipantId2 = "globalParticipant2";
        GlobalDiscoveryEntry capInfo2 = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                 domain1,
                                                                 interfaceName1,
                                                                 globalParticipantId2,
                                                                 new ProviderQos(),
                                                                 System.currentTimeMillis(),
                                                                 expiryDateMs,
                                                                 publicKeyId,
                                                                 globalAddress1Serialized);
        globalEntries.add(capInfo2);

        doAnswer(createLookupAnswer(globalEntries)).when(globalCapabilitiesDirectoryClient)
                                                   .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                           eq(new String[]{ domain1 }),
                                                           eq(interfaceName1),
                                                           eq(discoveryQos.getDiscoveryTimeout()),
                                                           eq(gbids));
        Promise<Lookup2Deferred> promise = localCapabilitiesDirectoryWithEmptyGbids.lookup(domains,
                                                                                           interfaceName1,
                                                                                           discoveryQos,
                                                                                           new String[]{});
        verifyGcdLookupAndPromiseFulfillment(1,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             gbids,
                                             promise,
                                             2);
        verify(routingTable, never()).incrementReferenceCount(any());
        ArgumentCaptor<Address> addressCaptor = ArgumentCaptor.forClass(Address.class);
        verify(routingTable, times(1)).put(eq(globalParticipantId), addressCaptor.capture(), eq(true), anyLong());
        MqttAddress address = (MqttAddress) addressCaptor.getValue();
        assertEquals(gbids[0], address.getBrokerUri());
        verify(routingTable, times(1)).put(eq(globalParticipantId2), addressCaptor.capture(), eq(true), anyLong());
        address = (MqttAddress) addressCaptor.getValue();
        assertEquals(gbids[0], address.getBrokerUri());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantId_emptyGbid_replacesReturnedGbidsWithEmpty() throws InterruptedException {
        String[] gbids = new String[]{ "" };

        LocalCapabilitiesDirectoryImpl localCapabilitiesDirectoryWithEmptyGbids = new LocalCapabilitiesDirectoryImpl(capabilitiesProvisioning,
                                                                                                                     globalAddressProvider,
                                                                                                                     localDiscoveryEntryStoreMock,
                                                                                                                     globalDiscoveryEntryCacheMock,
                                                                                                                     routingTable,
                                                                                                                     globalCapabilitiesDirectoryClient,
                                                                                                                     expiredDiscoveryEntryCacheCleaner,
                                                                                                                     freshnessUpdateIntervalMs,
                                                                                                                     capabilitiesFreshnessUpdateExecutor,
                                                                                                                     shutdownNotifier,
                                                                                                                     gbids,
                                                                                                                     DEFAULT_EXPIRY_TIME_MS,
                                                                                                                     accessController,
                                                                                                                     enableAccessControl);

        String domain1 = "domain1";
        String interfaceName1 = "interfaceName1";
        DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.LOCAL_AND_GLOBAL, false);

        // add global entry
        String globalParticipantId = "globalParticipant";
        GlobalDiscoveryEntry capInfo = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                domain1,
                                                                interfaceName1,
                                                                globalParticipantId,
                                                                new ProviderQos(),
                                                                System.currentTimeMillis(),
                                                                expiryDateMs,
                                                                publicKeyId,
                                                                globalAddress1Serialized);

        doAnswer(createLookupAnswer(capInfo)).when(globalCapabilitiesDirectoryClient)
                                             .lookup(ArgumentMatchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                                                     eq(globalParticipantId),
                                                     eq(discoveryQos.getDiscoveryTimeout()),
                                                     eq(gbids));
        Promise<Lookup4Deferred> promise = localCapabilitiesDirectoryWithEmptyGbids.lookup(globalParticipantId,
                                                                                           discoveryQos,
                                                                                           new String[]{});
        checkPromiseSuccess(promise, "lookup failed");
        verify(globalCapabilitiesDirectoryClient).lookup(ArgumentMatchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                                                         eq(globalParticipantId),
                                                         eq(discoveryQos.getDiscoveryTimeout()),
                                                         eq(gbids));
        verify(routingTable, never()).incrementReferenceCount(any());
        ArgumentCaptor<Address> addressCaptor = ArgumentCaptor.forClass(Address.class);
        verify(routingTable, times(1)).put(eq(globalParticipantId), addressCaptor.capture(), eq(true), anyLong());
        MqttAddress address = (MqttAddress) addressCaptor.getValue();
        assertEquals(gbids[0], address.getBrokerUri());
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
        verify(routingTable, times(1)).put(eq(expectedEntry2.getParticipantId()),
                                           any(Address.class),
                                           eq(true),
                                           anyLong());
        verify(routingTable, never()).put(eq(expectedEntry1.getParticipantId()),
                                          any(Address.class),
                                          eq(true),
                                          anyLong());
        verify(routingTable, never()).incrementReferenceCount(anyString());
        assertEquals(1, result1.length);
        assertEquals(expectedEntry2, result1[0]);

        reset((Object) routingTable);

        doReturn(true).when(routingTable)
                      .put(eq(expectedEntry1.getParticipantId()), any(Address.class), eq(true), anyLong());
        Promise<Lookup2Deferred> promise2 = localCapabilitiesDirectory.lookup(domainsForLookup,
                                                                              interfaceName,
                                                                              discoveryQos,
                                                                              new String[]{ knownGbids[0] });

        DiscoveryEntryWithMetaInfo[] result2 = (DiscoveryEntryWithMetaInfo[]) checkPromiseSuccess(promise2,
                                                                                                  "lookup failed")[0];
        verify(routingTable, times(1)).put(eq(expectedEntry1.getParticipantId()),
                                           any(Address.class),
                                           eq(true),
                                           anyLong());
        verify(routingTable, never()).put(eq(expectedEntry2.getParticipantId()),
                                          any(Address.class),
                                          eq(true),
                                          anyLong());
        verify(routingTable, never()).incrementReferenceCount(anyString());
        assertEquals(1, result2.length);
        assertEquals(expectedEntry1, result2[0]);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbids_globalOnly_filtersLocalEntriesByGbids() throws InterruptedException {
        String domain = "domain";
        String[] domainsForLookup = new String[]{ domain };
        String interfaceName = "interfaceName";
        DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.GLOBAL_ONLY, false);

        DiscoveryEntry localEntry1 = new DiscoveryEntry(new Version(47, 11),
                                                        domain,
                                                        interfaceName,
                                                        "participantId1",
                                                        new ProviderQos(),
                                                        System.currentTimeMillis(),
                                                        expiryDateMs,
                                                        publicKeyId);
        DiscoveryEntryWithMetaInfo expectedEntry1 = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                        localEntry1);
        DiscoveryEntry localEntry2 = new DiscoveryEntry(localEntry1);
        localEntry2.setParticipantId("participantId2");
        DiscoveryEntryWithMetaInfo expectedEntry2 = new DiscoveryEntryWithMetaInfo(expectedEntry1);
        expectedEntry2.setParticipantId(localEntry2.getParticipantId());

        doReturn(Arrays.asList(localEntry1, localEntry2)).when(localDiscoveryEntryStoreMock)
                                                         .lookupGlobalEntries(eq(domainsForLookup), eq(interfaceName));

        Promise<Add1Deferred> promiseAdd = localCapabilitiesDirectory.add(localEntry1, true, knownGbids);
        checkPromiseSuccess(promiseAdd, "add failed");
        promiseAdd = localCapabilitiesDirectory.add(localEntry2, true, new String[]{ knownGbids[1] });
        checkPromiseSuccess(promiseAdd, "add failed");

        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(localDiscoveryEntryStoreMock).add(eq(localEntry1));
        verify(localDiscoveryEntryStoreMock).add(eq(localEntry2));

        Promise<Lookup2Deferred> promiseLookup1 = localCapabilitiesDirectory.lookup(domainsForLookup,
                                                                                    interfaceName,
                                                                                    discoveryQos,
                                                                                    new String[]{ knownGbids[1] });

        DiscoveryEntryWithMetaInfo[] result1 = (DiscoveryEntryWithMetaInfo[]) checkPromiseSuccess(promiseLookup1,
                                                                                                  "lookup failed")[0];

        assertEquals(2, result1.length);
        int actualEntry1 = expectedEntry1.getParticipantId().equals(result1[0].getParticipantId()) ? 0 : 1;
        int actualEntry2 = (actualEntry1 + 1) % 2;
        assertTrue(discoveryEntriesWithMetaInfoMatchWithUpdatedLastSeenDate(expectedEntry1, result1[actualEntry1]));
        assertTrue(discoveryEntriesWithMetaInfoMatchWithUpdatedLastSeenDate(expectedEntry2, result1[actualEntry2]));
        verify(routingTable, times(1)).incrementReferenceCount(eq(expectedEntry1.getParticipantId()));
        verify(routingTable, times(1)).incrementReferenceCount(eq(expectedEntry2.getParticipantId()));

        Promise<Lookup2Deferred> promiseLookup2 = localCapabilitiesDirectory.lookup(domainsForLookup,
                                                                                    interfaceName,
                                                                                    discoveryQos,
                                                                                    new String[]{ knownGbids[0] });

        DiscoveryEntryWithMetaInfo[] result2 = (DiscoveryEntryWithMetaInfo[]) checkPromiseSuccess(promiseLookup2,
                                                                                                  "lookup failed")[0];
        assertEquals(1, result2.length);
        assertTrue(discoveryEntriesWithMetaInfoMatchWithUpdatedLastSeenDate(expectedEntry1, result2[0]));
        verify(routingTable, times(2)).incrementReferenceCount(eq(expectedEntry1.getParticipantId()));

        Promise<Lookup2Deferred> promiseLookup3 = localCapabilitiesDirectory.lookup(domainsForLookup,
                                                                                    interfaceName,
                                                                                    discoveryQos,
                                                                                    knownGbids);

        DiscoveryEntryWithMetaInfo[] result3 = (DiscoveryEntryWithMetaInfo[]) checkPromiseSuccess(promiseLookup3,
                                                                                                  "lookup failed")[0];
        assertEquals(2, result3.length);
        actualEntry1 = expectedEntry1.getParticipantId().equals(result3[0].getParticipantId()) ? 0 : 1;
        actualEntry2 = (actualEntry1 + 1) % 2;
        assertTrue(discoveryEntriesWithMetaInfoMatchWithUpdatedLastSeenDate(expectedEntry1, result3[actualEntry1]));
        assertTrue(discoveryEntriesWithMetaInfoMatchWithUpdatedLastSeenDate(expectedEntry2, result3[actualEntry2]));
        verify(routingTable, times(3)).incrementReferenceCount(eq(expectedEntry1.getParticipantId()));
        verify(routingTable, times(2)).incrementReferenceCount(eq(expectedEntry2.getParticipantId()));
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbids_globalOnly_filtersLocalEntriesByGbids() throws InterruptedException {
        DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.GLOBAL_ONLY, false);

        DiscoveryEntry localEntry = new DiscoveryEntry(new Version(47, 11),
                                                       "domain",
                                                       "interfaceName",
                                                       "participantId1",
                                                       new ProviderQos(),
                                                       System.currentTimeMillis(),
                                                       expiryDateMs,
                                                       publicKeyId);
        DiscoveryEntry localStoreEntry = new DiscoveryEntry(localEntry);
        DiscoveryEntryWithMetaInfo expectedLocalEntry = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                            localEntry);

        // register in knownGbids[1]
        Promise<Add1Deferred> promiseAdd = localCapabilitiesDirectory.add(localEntry,
                                                                          true,
                                                                          new String[]{ knownGbids[1] });
        checkPromiseSuccess(promiseAdd, "add failed");
        reset((Object) localDiscoveryEntryStoreMock,
              (Object) globalDiscoveryEntryCacheMock,
              (Object) globalCapabilitiesDirectoryClient);

        doReturn(Optional.of(localStoreEntry)).when(localDiscoveryEntryStoreMock)
                                              .lookup(eq(expectedLocalEntry.getParticipantId()), eq(Long.MAX_VALUE));

        // lookup knownGbids[1], expect local entry
        Promise<Lookup4Deferred> promiseLookup1 = localCapabilitiesDirectory.lookup(expectedLocalEntry.getParticipantId(),
                                                                                    discoveryQos,
                                                                                    new String[]{ knownGbids[1] });

        DiscoveryEntryWithMetaInfo result1 = (DiscoveryEntryWithMetaInfo) checkPromiseSuccess(promiseLookup1,
                                                                                              "lookup failed")[0];
        verify(localDiscoveryEntryStoreMock, times(1)).lookup(eq(expectedLocalEntry.getParticipantId()),
                                                              eq(Long.MAX_VALUE));
        verifyNoMoreInteractions(globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);
        assertEquals(expectedLocalEntry, result1);
        verify(routingTable, times(1)).incrementReferenceCount(eq(expectedLocalEntry.getParticipantId()));

        // lookup knownGbids[0], expect DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS
        Promise<Lookup4Deferred> promiseLookup2 = localCapabilitiesDirectory.lookup(expectedLocalEntry.getParticipantId(),
                                                                                    discoveryQos,
                                                                                    new String[]{ knownGbids[0] });

        checkPromiseError(promiseLookup2, DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
        verify(localDiscoveryEntryStoreMock, times(2)).lookup(eq(expectedLocalEntry.getParticipantId()),
                                                              eq(Long.MAX_VALUE));
        verifyNoMoreInteractions(globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);
        verify(routingTable, times(1)).incrementReferenceCount(eq(expectedLocalEntry.getParticipantId()));

        // lookup all gbids, expect local entry
        Promise<Lookup4Deferred> promiseLookup3 = localCapabilitiesDirectory.lookup(expectedLocalEntry.getParticipantId(),
                                                                                    discoveryQos,
                                                                                    knownGbids);

        DiscoveryEntryWithMetaInfo result3 = (DiscoveryEntryWithMetaInfo) checkPromiseSuccess(promiseLookup3,
                                                                                              "lookup failed")[0];
        verify(localDiscoveryEntryStoreMock, times(3)).lookup(eq(expectedLocalEntry.getParticipantId()),
                                                              eq(Long.MAX_VALUE));
        verifyNoMoreInteractions(globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);
        assertEquals(expectedLocalEntry, result3);
        verify(routingTable, times(2)).incrementReferenceCount(eq(expectedLocalEntry.getParticipantId()));
        verify(routingTable, never()).put(eq(expectedLocalEntry.getParticipantId()),
                                          any(Address.class),
                                          any(Boolean.class),
                                          anyLong());
    }

    private void testLookupByDomainInterfaceWithGbids_globalOnly_allLocal(String[] gbidsForLookup,
                                                                          DiscoveryEntry entryForGbid1,
                                                                          DiscoveryEntry entryForGbid2,
                                                                          DiscoveryEntry entryForGbid3,
                                                                          DiscoveryEntry entryForGbid2And3,
                                                                          Set<String> expectedParticipantIds) throws InterruptedException {
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

        doReturn(Arrays.asList(entryForGbid1,
                               entryForGbid2,
                               entryForGbid3,
                               entryForGbid2And3)).when(localDiscoveryEntryStoreMock)
                                                  .lookupGlobalEntries(eq(domainsForLookup), eq(INTERFACE_NAME));
        doReturn(new ArrayList<GlobalDiscoveryEntry>()).when(globalDiscoveryEntryCacheMock)
                                                       .lookup(eq(domainsForLookup), eq(INTERFACE_NAME), anyLong());

        Promise<Lookup2Deferred> lookupPromise = localCapabilitiesDirectory.lookup(domainsForLookup,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos,
                                                                                   gbidsForLookup);
        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo[] foundEntries = (DiscoveryEntryWithMetaInfo[]) values[0];
        assertEquals(expectedParticipantIds.size(), foundEntries.length);

        Set<String> foundParticipantIds = new HashSet<>();
        for (DiscoveryEntryWithMetaInfo foundEntry : foundEntries) {
            foundParticipantIds.add(foundEntry.getParticipantId());
        }
        assertEquals(expectedParticipantIds, foundParticipantIds);
        expectedParticipantIds.forEach((participantId) -> {
            verify(routingTable, times(1)).incrementReferenceCount(eq(participantId));
            verify(routingTable, never()).put(eq(participantId), any(Address.class), any(Boolean.class), anyLong());
        });
        reset((Object) routingTable);

        verify(globalCapabilitiesDirectoryClient,
               times(0)).lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                any(String[].class),
                                anyString(),
                                anyLong(),
                                any(String[].class));
    }

    private void testLookupByDomainInterfaceWithGbids_globalOnly_noneLocalOrCached(String[] gbidsForLookup,
                                                                                   String[] expectedGbids) throws InterruptedException {
        String[] domainsForLookup = new String[]{ discoveryEntry.getDomain() };
        String[] expectedDomains = domainsForLookup.clone();
        DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.GLOBAL_ONLY, false);

        List<GlobalDiscoveryEntry> globalEntries = new ArrayList<>();
        globalEntries.add(globalDiscoveryEntry);
        DiscoveryEntry entry2 = new DiscoveryEntry(discoveryEntry);
        entry2.setParticipantId("participantId2");
        globalEntries.add(CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(entry2, globalAddressWithoutGbid));

        doReturn(new ArrayList<GlobalDiscoveryEntry>()).when(globalDiscoveryEntryCacheMock)
                                                       .lookup(eq(expectedDomains), eq(INTERFACE_NAME), anyLong());
        doAnswer(createLookupAnswer(globalEntries)).when(globalCapabilitiesDirectoryClient)
                                                   .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                           eq(expectedDomains),
                                                           eq(INTERFACE_NAME),
                                                           eq(discoveryQos.getDiscoveryTimeout()),
                                                           eq(expectedGbids));

        doReturn(true).when(routingTable).put(anyString(), any(Address.class), anyBoolean(), anyLong());
        Promise<Lookup2Deferred> lookupPromise = localCapabilitiesDirectory.lookup(domainsForLookup,
                                                                                   INTERFACE_NAME,
                                                                                   discoveryQos,
                                                                                   gbidsForLookup);

        verify(globalCapabilitiesDirectoryClient).lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                         eq(expectedDomains),
                                                         eq(INTERFACE_NAME),
                                                         eq(discoveryQos.getDiscoveryTimeout()),
                                                         eq(expectedGbids));
        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo[] foundEntries = (DiscoveryEntryWithMetaInfo[]) values[0];
        assertEquals(2, foundEntries.length);
        Arrays.asList(foundEntries).forEach(entry -> {
            verify(routingTable, times(1)).put(eq(entry.getParticipantId()), any(Address.class), eq(true), anyLong());
        });

        verify(routingTable, never()).incrementReferenceCount(anyString());
        reset((Object) routingTable);
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

        testLookupByDomainInterfaceWithGbids_globalOnly_allLocal(gbidsForLookup,
                                                                 entryForGbid1,
                                                                 entryForGbid2,
                                                                 entryForGbid3,
                                                                 entryForGbid2And3,
                                                                 expectedParticipantIds);
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

        testLookupByDomainInterfaceWithGbids_globalOnly_allLocal(gbidsForLookup,
                                                                 entryForGbid1,
                                                                 entryForGbid2,
                                                                 entryForGbid3,
                                                                 entryForGbid2And3,
                                                                 expectedParticipantIds);
    }

    @Test
    public void testLookupByDomainInterfaceWithGbids_globalOnly_multipleGbids_noneCached() throws InterruptedException {
        String[] gbidsForLookup = new String[]{ knownGbids[0], knownGbids[2] };

        testLookupByDomainInterfaceWithGbids_globalOnly_noneLocalOrCached(gbidsForLookup, gbidsForLookup.clone());

        String[] gbidsForLookup2 = new String[]{ knownGbids[2], knownGbids[0] };

        testLookupByDomainInterfaceWithGbids_globalOnly_noneLocalOrCached(gbidsForLookup2, gbidsForLookup2.clone());
    }

    @Test
    public void testLookupByDomainInterfaceWithGbids_globalOnly_emptyGbidsArray_noneCached() throws InterruptedException {
        String[] gbidsForLookup = new String[0];

        testLookupByDomainInterfaceWithGbids_globalOnly_noneLocalOrCached(gbidsForLookup, knownGbids);
    }

    private void testLookupByParticipantIdWithGbids_globalOnly_allCached(String[] gbidsForLookup) throws InterruptedException {
        String participantId = discoveryEntry.getParticipantId();
        DiscoveryQos discoveryQos = new DiscoveryQos(30000L, 500L, DiscoveryScope.GLOBAL_ONLY, false);

        doReturn(Optional.of(globalDiscoveryEntry)).when(globalDiscoveryEntryCacheMock).lookup(eq(participantId),
                                                                                               anyLong());

        Promise<Lookup4Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId,
                                                                                   discoveryQos,
                                                                                   gbidsForLookup);
        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo foundEntry = (DiscoveryEntryWithMetaInfo) values[0];

        assertEquals(participantId, foundEntry.getParticipantId());
        verify(routingTable, never()).incrementReferenceCount(eq(globalDiscoveryEntry.getParticipantId()));
        verify(routingTable, times(1)).put(eq(globalDiscoveryEntry.getParticipantId()),
                                           any(Address.class),
                                           eq(true),
                                           anyLong());
        reset((Object) routingTable);

        verify(globalCapabilitiesDirectoryClient,
               times(0)).lookup(ArgumentMatchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
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
                                                          .lookup(ArgumentMatchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                                                                  eq(participantId),
                                                                  eq(discoveryQos.getDiscoveryTimeout()),
                                                                  eq(expectedGbids));
        doReturn(true).when(routingTable).put(eq(participantId), any(Address.class), any(Boolean.class), anyLong());

        Promise<Lookup4Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId,
                                                                                   discoveryQos,
                                                                                   gbidsForLookup);

        verify(globalCapabilitiesDirectoryClient).lookup(ArgumentMatchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                                                         eq(participantId),
                                                         eq(discoveryQos.getDiscoveryTimeout()),
                                                         eq(expectedGbids));
        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo foundEntry = (DiscoveryEntryWithMetaInfo) values[0];
        assertEquals(participantId, foundEntry.getParticipantId());
        verify(routingTable, times(1)).put(eq(foundEntry.getParticipantId()), any(Address.class), eq(true), anyLong());
        verify(routingTable, never()).incrementReferenceCount(anyString());
        reset((Object) routingTable);
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
        entries.forEach((entry) -> {
            verify(routingTable, times(1)).incrementReferenceCount(eq(entry.getParticipantId()));
            verify(routingTable, never()).put(eq(entry.getParticipantId()),
                                              any(Address.class),
                                              any(Boolean.class),
                                              anyLong());
        });
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
                                                                           .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                                   org.mockito.hamcrest.MockitoHamcrest.argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
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
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupMultipleDomains_globalOnly_allCached() throws InterruptedException {
        String[] domains = new String[]{ "domain1", "domain2" };
        String interfaceName = "interface1";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        List<GlobalDiscoveryEntry> entries = new ArrayList<>();
        for (String domain : domains) {
            GlobalDiscoveryEntry entry = new GlobalDiscoveryEntry();
            entry.setParticipantId("participantIdFor-" + domain);
            entry.setDomain(domain);
            entry.setAddress(globalAddress1Serialized);
            entries.add(entry);
        }

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
        entries.forEach((entry) -> {
            verify(routingTable, never()).incrementReferenceCount(eq(entry.getParticipantId()));
            verify(routingTable, times(1)).put(eq(entry.getParticipantId()), any(Address.class), eq(true), anyLong());
        });
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupMultipleDomains_globalOnly_allLocalGlobal() throws InterruptedException {
        String[] domains = new String[]{ "domain1", "domain2" };
        String interfaceName = "interface1";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setCacheMaxAge(ONE_DAY_IN_MS);

        List<DiscoveryEntry> entries = new ArrayList<>();
        List<Promise<Add1Deferred>> promises = new ArrayList<>();
        for (String domain : domains) {
            DiscoveryEntry entry = new GlobalDiscoveryEntry();
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

        doReturn(entries).when(localDiscoveryEntryStoreMock).lookupGlobalEntries(eq(domains), eq(interfaceName));

        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos);

        verifyGcdLookupAndPromiseFulfillment(0,
                                             domains,
                                             interfaceName,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             2); // 2 cached entries
        entries.forEach((entry) -> {
            verify(routingTable, times(1)).incrementReferenceCount(eq(entry.getParticipantId()));
            verify(routingTable, never()).put(eq(entry.getParticipantId()),
                                              any(Address.class),
                                              any(Boolean.class),
                                              anyLong());
        });
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
        doReturn(Optional.of(entry)).when(globalDiscoveryEntryCacheMock).lookup(eq(entry.getParticipantId()),
                                                                                eq(Long.MAX_VALUE));
        doAnswer(createLookupAnswer(new ArrayList<GlobalDiscoveryEntry>())).when(globalCapabilitiesDirectoryClient)
                                                                           .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                                   eq(domains),
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
                                             1);
        verify(routingTable, never()).incrementReferenceCount(anyString());
        verify(routingTable, times(1)).put(eq(entry.getParticipantId()), any(Address.class), eq(true), anyLong());
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
                                               .lookup(org.mockito.hamcrest.MockitoHamcrest.argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
                                                       eq(INTERFACE_NAME));

        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock)
                                             .lookup(eq(discoveryEntry.getParticipantId()), anyLong());

        // cached entry for participantId2 for cachedDomain
        GlobalDiscoveryEntry cachedRemoteEntry = new GlobalDiscoveryEntry();
        cachedRemoteEntry.setParticipantId("participantId2");
        cachedRemoteEntry.setInterfaceName(INTERFACE_NAME);
        cachedRemoteEntry.setDomain(cachedDomain);
        cachedRemoteEntry.setAddress(globalAddress1Serialized);
        doReturn(Arrays.asList(cachedRemoteEntry)).when(globalDiscoveryEntryCacheMock)
                                                  .lookup(org.mockito.hamcrest.MockitoHamcrest.argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
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
                                                                 .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                         eq(domains),
                                                                         eq(INTERFACE_NAME),
                                                                         eq(discoveryQos.getDiscoveryTimeout()),
                                                                         eq(knownGbids));

        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, INTERFACE_NAME, discoveryQos);

        verify(localDiscoveryEntryStoreMock).lookup(org.mockito.hamcrest.MockitoHamcrest.argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
                                                    eq(INTERFACE_NAME));
        verify(globalDiscoveryEntryCacheMock).lookup(org.mockito.hamcrest.MockitoHamcrest.argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
                                                     eq(INTERFACE_NAME),
                                                     eq(discoveryQos.getCacheMaxAge()));
        verify(globalCapabilitiesDirectoryClient).lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                         eq(domains),
                                                         eq(INTERFACE_NAME),
                                                         eq(discoveryQos.getDiscoveryTimeout()),
                                                         eq(knownGbids));
        Object[] values = verifyGcdLookupAndPromiseFulfillment(1,
                                                               domains,
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
        verify(routingTable, never()).put(eq(remoteEntry1.getParticipantId()),
                                          eq(globalAddressWithoutGbid),
                                          eq(true),
                                          anyLong());
        verify(routingTable, times(1)).incrementReferenceCount(eq(remoteEntry1.getParticipantId()));
        verify(routingTable, never()).incrementReferenceCount(eq(remoteEntry2.getParticipantId()));
        verify(routingTable, never()).incrementReferenceCount(eq(remoteEntry3.getParticipantId()));
        verify(routingTable).put(eq(remoteEntry2.getParticipantId()),
                                 eq(globalAddressWithoutGbid),
                                 eq(true),
                                 anyLong());
        verify(routingTable).put(eq(remoteEntry3.getParticipantId()),
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
                                                            .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
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
        verify(routingTable, times(1)).incrementReferenceCount(eq(capturedDiscoveryEntries[0].getParticipantId()));
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
        verify(localDiscoveryEntryStoreMock).lookup(eq(domains), eq(INTERFACE_NAME));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByDomainInterfaceGbids_globalOnly_invokesGcd_ignoresGlobalDuplicateOfLocalGlobalEntry() throws Exception {
        // test assumes that the local entry is registered after the global lookup has been triggered
        String[] domains = { discoveryEntry.getDomain() };
        List<GlobalDiscoveryEntry> globalDiscoveryEntries = Arrays.asList(globalDiscoveryEntry);
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        DiscoveryQos discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, DiscoveryScope.GLOBAL_ONLY, false);
        doAnswer(createLookupAnswer(globalDiscoveryEntries)).when(globalCapabilitiesDirectoryClient)
                                                            .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
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
        verify(routingTable, times(1)).incrementReferenceCount(eq(capturedDiscoveryEntries[0].getParticipantId()));
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
        verify(localDiscoveryEntryStoreMock).lookup(eq(discoveryEntry.getParticipantId()), anyLong());
    }

    @Test
    public void lookupDomIntf_globalOnlyWithCache_localGlobalEntryNoCachedEntry_doesNotInvokeGcd() throws Exception {
        final long cacheMaxAge = 1L;
        final long discoveryTimeout = 5000L;
        final String[] domains = new String[]{ discoveryEntry.getDomain() };
        final String interfaceName = discoveryEntry.getInterfaceName();
        DiscoveryQos discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, DiscoveryScope.GLOBAL_ONLY, false);

        // register in all gbids
        Promise<Add1Deferred> promiseAdd = localCapabilitiesDirectory.add(discoveryEntry, true, knownGbids);
        checkPromiseSuccess(promiseAdd, "add failed");
        reset(localDiscoveryEntryStoreMock, globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);

        doReturn(new HashSet<>(Arrays.asList(discoveryEntry))).when(localDiscoveryEntryStoreMock)
                                                              .lookupGlobalEntries(eq(domains), eq(interfaceName));

        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos);

        DiscoveryEntryWithMetaInfo[] values = (DiscoveryEntryWithMetaInfo[]) checkPromiseSuccess(promise,
                                                                                                 "lookup failed")[0];
        assertEquals(1, values.length);
        assertEquals(true, values[0].getIsLocal());
        verify(routingTable, times(1)).incrementReferenceCount(eq(discoveryEntry.getParticipantId()));
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());

        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient);
    }

    @Test
    public void lookupDomIntf_globalOnlyNoCache_localGlobalEntryNoCachedEntry_invokesGcd() throws Exception {
        final long cacheMaxAge = 0L;
        final long discoveryTimeout = 5000L;
        final String[] domains = new String[]{ discoveryEntry.getDomain() };
        final String interfaceName = discoveryEntry.getInterfaceName();
        DiscoveryQos discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, DiscoveryScope.GLOBAL_ONLY, false);

        // register in all gbids
        Promise<Add1Deferred> promiseAdd = localCapabilitiesDirectory.add(discoveryEntry, true, knownGbids);
        checkPromiseSuccess(promiseAdd, "add failed");
        reset(localDiscoveryEntryStoreMock, globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);

        doAnswer(createLookupAnswer(new ArrayList<GlobalDiscoveryEntry>())).when(globalCapabilitiesDirectoryClient)
                                                                           .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                                   Mockito.<String[]> any(),
                                                                                   anyString(),
                                                                                   anyLong(),
                                                                                   any());

        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos);

        DiscoveryEntryWithMetaInfo[] values = (DiscoveryEntryWithMetaInfo[]) checkPromiseSuccess(promise,
                                                                                                 "lookup failed")[0];
        assertEquals(0, values.length);
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());

        verify(globalCapabilitiesDirectoryClient).lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                         eq(domains),
                                                         eq(interfaceName),
                                                         eq(discoveryQos.getDiscoveryTimeout()),
                                                         eq(knownGbids));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdGbids_globalOnlyWithCache_invokesGcd_ignoresGlobalDuplicateOfLocalGlobalEntry() throws Exception {
        // test assumes that the local entry is registered after the global lookup has been triggered
        lookupByParticipantIdGbids_globalOnly_invokesGcd_ignoresGlobalDuplicateOfLocalGlobalEntry(10000L);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdGbids_globalOnlyNoCache_invokesGcd_ignoresGlobalDuplicateOfLocalGlobalEntry() throws Exception {
        // test assumes that the local entry is registered after the global lookup has been triggered
        lookupByParticipantIdGbids_globalOnly_invokesGcd_ignoresGlobalDuplicateOfLocalGlobalEntry(0L);
    }

    private void lookupByParticipantIdGbids_globalOnly_invokesGcd_ignoresGlobalDuplicateOfLocalGlobalEntry(long cacheMaxAge) throws Exception {
        // test assumes that the local entry is registered after the global lookup has been triggered
        final long discoveryTimeout = 5000L;

        DiscoveryQos discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, DiscoveryScope.GLOBAL_ONLY, false);
        doAnswer(createLookupAnswer(globalDiscoveryEntry)).when(globalCapabilitiesDirectoryClient)
                                                          .lookup(ArgumentMatchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                                                                  anyString(),
                                                                  anyLong(),
                                                                  any());

        doAnswer(new Answer<Optional<DiscoveryEntry>>() {
            // simulate provider registration after remote lookup has been triggered
            boolean firstCall = true;

            @Override
            public Optional<DiscoveryEntry> answer(InvocationOnMock invocation) throws Throwable {
                if (firstCall) {
                    firstCall = false;
                    return Optional.empty();
                }
                return Optional.of(discoveryEntry);
            }
        }).when(localDiscoveryEntryStoreMock).lookup(eq(discoveryEntry.getParticipantId()), anyLong());

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
        verify(routingTable, times(1)).incrementReferenceCount(eq(discoveryEntry.getParticipantId()));
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
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
        doReturn(Optional.of(globalEntry)).when(globalDiscoveryEntryCacheMock)
                                          .lookup(eq(globalEntry.getParticipantId()), eq(Long.MAX_VALUE));

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
                                                                      .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                              Mockito.<String[]> any(),
                                                                              anyString(),
                                                                              anyLong(),
                                                                              eq(knownGbids));

        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos);

        verify(globalCapabilitiesDirectoryClient).lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                         eq(domains),
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
        verify(routingTable, times(1)).incrementReferenceCount(eq(localEntry.getParticipantId()));
        verify(routingTable, never()).incrementReferenceCount(eq(globalEntry.getParticipantId()));
        verify(routingTable, never()).incrementReferenceCount(eq(remoteGlobalEntry.getParticipantId()));
        verify(routingTable, never()).put(eq(localEntry.getParticipantId()),
                                          any(Address.class),
                                          any(Boolean.class),
                                          anyLong());
        verify(routingTable, times(1)).put(eq(globalEntry.getParticipantId()), any(Address.class), eq(true), anyLong());
        verify(routingTable, times(1)).put(eq(remoteGlobalEntry.getParticipantId()),
                                           any(Address.class),
                                           eq(true),
                                           anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupMultipleDomains_localThenGlobal_oneLocalAllCachedDomains_returnsLocalAndCachedEntries() throws InterruptedException {
        String[] domains = new String[]{ "domain1", "domain2" };
        String interfaceName = "interface1";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);
        discoveryQos.setCacheMaxAge(ONE_DAY_IN_MS);

        DiscoveryEntry localEntry = new DiscoveryEntry();
        localEntry.setParticipantId("participantIdLocal");
        localEntry.setDomain(domains[0]);
        when(localDiscoveryEntryStoreMock.lookup(eq(domains), eq(interfaceName))).thenReturn(Arrays.asList(localEntry));

        GlobalDiscoveryEntry globalCachedEntry1 = new GlobalDiscoveryEntry();
        globalCachedEntry1.setParticipantId("participantIdCached1");
        globalCachedEntry1.setInterfaceName(interfaceName);
        globalCachedEntry1.setDomain(domains[0]);
        globalCachedEntry1.setAddress(globalAddress1Serialized);

        GlobalDiscoveryEntry globalCachedEntry2 = new GlobalDiscoveryEntry();
        globalCachedEntry2.setParticipantId("participantIdCached2");
        globalCachedEntry2.setInterfaceName(interfaceName);
        globalCachedEntry2.setDomain(domains[1]);
        globalCachedEntry2.setAddress(globalAddress1Serialized);

        Set<GlobalDiscoveryEntry> globalCachedEntries = new HashSet<GlobalDiscoveryEntry>(Arrays.asList(globalCachedEntry1,
                                                                                                        globalCachedEntry2));

        doReturn(globalCachedEntries).when(globalDiscoveryEntryCacheMock)
                                     .lookup(eq(domains), eq(interfaceName), eq(discoveryQos.getCacheMaxAge()));

        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos);
        Object[] values = checkPromiseSuccess(promise, "lookup failed");

        verify(localDiscoveryEntryStoreMock).lookup(eq(domains), eq(interfaceName));
        verify(globalDiscoveryEntryCacheMock).lookup(eq(domains), eq(interfaceName), eq(ONE_DAY_IN_MS));
        verify(globalCapabilitiesDirectoryClient, never()).lookup(any(), anyString(), anyLong(), any());

        Collection<DiscoveryEntry> captured = CapabilityUtils.convertToDiscoveryEntrySet(Arrays.asList((DiscoveryEntryWithMetaInfo[]) values[0]));
        assertEquals(3, captured.size());
        assertTrue(captured.contains(localEntry));
        assertTrue(captured.contains(new DiscoveryEntry(globalCachedEntry1)));
        assertTrue(captured.contains(new DiscoveryEntry(globalCachedEntry2)));
        verify(routingTable, times(1)).incrementReferenceCount(eq(localEntry.getParticipantId()));
        verify(routingTable, never()).incrementReferenceCount(eq(globalCachedEntry1.getParticipantId()));
        verify(routingTable, never()).incrementReferenceCount(eq(globalCachedEntry2.getParticipantId()));
        verify(routingTable, never()).put(eq(localEntry.getParticipantId()),
                                          any(Address.class),
                                          any(Boolean.class),
                                          anyLong());
        verify(routingTable, times(1)).put(eq(globalCachedEntry1.getParticipantId()),
                                           any(Address.class),
                                           eq(true),
                                           anyLong());
        verify(routingTable, times(1)).put(eq(globalCachedEntry2.getParticipantId()),
                                           any(Address.class),
                                           eq(true),
                                           anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupMultipleDomains_localThenGlobal_allDomainsLocal_returnsOnlyLocalEntries() throws InterruptedException {
        String[] domains = new String[]{ "domain1", "domain2" };
        String interfaceName = "interface1";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);
        discoveryQos.setCacheMaxAge(ONE_DAY_IN_MS);

        DiscoveryEntry localEntry1 = new DiscoveryEntry();
        localEntry1.setParticipantId("participantIdLocal1");
        localEntry1.setDomain(domains[0]);

        DiscoveryEntry localEntry2 = new DiscoveryEntry();
        localEntry2.setParticipantId("participantIdLocal2");
        localEntry2.setDomain(domains[1]);

        when(localDiscoveryEntryStoreMock.lookup(eq(domains),
                                                 eq(interfaceName))).thenReturn(Arrays.asList(localEntry1,
                                                                                              localEntry2));

        GlobalDiscoveryEntry globalCachedEntry = new GlobalDiscoveryEntry();
        globalCachedEntry.setParticipantId("participantIdCached1");
        globalCachedEntry.setInterfaceName(interfaceName);
        globalCachedEntry.setDomain(domains[0]);
        globalCachedEntry.setAddress(globalAddress1Serialized);

        doReturn(Arrays.asList(globalCachedEntry)).when(globalDiscoveryEntryCacheMock)
                                                  .lookup(eq(domains),
                                                          eq(interfaceName),
                                                          eq(discoveryQos.getCacheMaxAge()));

        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos);
        Object[] values = checkPromiseSuccess(promise, "lookup failed");

        verify(localDiscoveryEntryStoreMock).lookup(eq(domains), eq(interfaceName));
        verify(globalDiscoveryEntryCacheMock).lookup(eq(domains), eq(interfaceName), eq(ONE_DAY_IN_MS));
        verify(globalCapabilitiesDirectoryClient, never()).lookup(any(), anyString(), anyLong(), any());

        Collection<DiscoveryEntry> captured = CapabilityUtils.convertToDiscoveryEntrySet(Arrays.asList((DiscoveryEntryWithMetaInfo[]) values[0]));
        assertEquals(2, captured.size());
        assertTrue(captured.contains(localEntry1));
        assertTrue(captured.contains(localEntry2));
        assertFalse(captured.contains(new DiscoveryEntry(globalCachedEntry)));
        captured.forEach((entry) -> {
            verify(routingTable, times(1)).incrementReferenceCount(eq(entry.getParticipantId()));
            verify(routingTable, never()).put(eq(entry.getParticipantId()),
                                              any(Address.class),
                                              any(Boolean.class),
                                              anyLong());
        });
        verify(routingTable, never()).put(eq(globalCachedEntry.getParticipantId()),
                                          any(Address.class),
                                          any(Boolean.class),
                                          anyLong());
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
        verify(routingTable, times(1)).incrementReferenceCount(eq(capturedLocalEntry.getParticipantId()));
        verify(routingTable, never()).put(eq(capturedLocalEntry.getParticipantId()),
                                          any(Address.class),
                                          any(Boolean.class),
                                          anyLong());
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
        verify(routingTable, never()).incrementReferenceCount(eq(capturedCachedGlobalEntry.getParticipantId()));
        verify(routingTable, times(1)).put(eq(capturedCachedGlobalEntry.getParticipantId()),
                                           any(Address.class),
                                           eq(true),
                                           anyLong());
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
               never()).lookup(ArgumentMatchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                               anyString(),
                               anyLong(),
                               any());
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_localOnly_localGlobalEntry_doesNotInvokeGcd_returnsLocalEntry() throws Exception {
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
    public void lookupByParticipantIdWithGbids_localThenGlobal_localGlobalEntry_doesNotInvokeGcd_returnsLocalEntry() throws Exception {
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
    public void lookupByParticipantIdWithGbids_localAndGlobal_localGlobalEntry_doesNotInvokeGcd_returnsLocalEntry() throws Exception {
        DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_AND_GLOBAL;
        boolean localEntryAvailable = true;
        boolean invokesGcd = false;
        boolean returnsLocalEntry = true;
        lookupByParticipantIdDiscoveryScopeTest(discoveryScope, localEntryAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_globalOnly_localGlobalEntry_doesNotInvokeGcd_returnsLocalResult() throws Exception {
        DiscoveryScope discoveryScope = DiscoveryScope.GLOBAL_ONLY;
        boolean localEntryAvailable = true;
        boolean invokesGcd = false;
        boolean returnsLocalEntry = true;
        lookupByParticipantIdDiscoveryScopeTest(discoveryScope, localEntryAvailable, invokesGcd, returnsLocalEntry);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_globalOnly_noLocalEntry_invokesGcd_returnsRemoteResult() throws Exception {
        DiscoveryScope discoveryScope = DiscoveryScope.GLOBAL_ONLY;
        boolean localEntryAvailable = false;
        boolean invokesGcd = true;
        boolean returnsLocalEntry = false;
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
            // register in all gbids
            Promise<Add1Deferred> promiseAdd = localCapabilitiesDirectory.add(discoveryEntry, true, knownGbids);
            checkPromiseSuccess(promiseAdd, "add failed");

            doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock).lookup(eq(participantId),
                                                                                            eq(Long.MAX_VALUE));
        }
        doAnswer(createLookupAnswer(globalDiscoveryEntry)).when(globalCapabilitiesDirectoryClient)
                                                          .lookup(ArgumentMatchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
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
            verify(routingTable, times(1)).incrementReferenceCount(eq(expectedLocalDiscoveryEntry.getParticipantId()));
            verify(routingTable, never()).put(eq(expectedLocalDiscoveryEntry.getParticipantId()),
                                              any(Address.class),
                                              any(Boolean.class),
                                              anyLong());
        } else {
            DiscoveryEntryWithMetaInfo expectedGlobalDiscoveryEntry = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                                          globalDiscoveryEntry);
            assertEquals(expectedGlobalDiscoveryEntry, capturedDiscoveryEntry);
            verify(routingTable, never()).incrementReferenceCount(any());
            verify(routingTable, times(1)).put(eq(expectedGlobalDiscoveryEntry.getParticipantId()),
                                               any(Address.class),
                                               eq(true),
                                               anyLong());
        }
    }

    @Test(timeout = TEST_TIMEOUT)
    public void lookupByParticipantIdWithGbids_globalOnly_localOnlyEntry_doesNotInvokeGcd_noEntryForParticipant() throws Exception {
        DiscoveryScope discoveryScope = DiscoveryScope.GLOBAL_ONLY;

        String participantId = discoveryEntry.getParticipantId();
        final long cacheMaxAge = 10000L;
        final long discoveryTimeout = 5000L;

        DiscoveryQos discoveryQos = new DiscoveryQos(cacheMaxAge, discoveryTimeout, discoveryScope, false);
        // register local only
        discoveryEntry.getQos().setScope(ProviderScope.LOCAL);
        Promise<Add1Deferred> promiseAdd = localCapabilitiesDirectory.add(discoveryEntry, true, knownGbids);
        checkPromiseSuccess(promiseAdd, "add failed");
        reset(localDiscoveryEntryStoreMock, globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);

        doReturn(Optional.of(discoveryEntry)).when(localDiscoveryEntryStoreMock).lookup(eq(participantId),
                                                                                        eq(Long.MAX_VALUE));

        Promise<Lookup4Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId,
                                                                                   discoveryQos,
                                                                                   new String[0]);

        checkPromiseError(lookupPromise, DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        verify(localDiscoveryEntryStoreMock).lookup(eq(participantId), eq(Long.MAX_VALUE));
        verifyNoMoreInteractions(globalDiscoveryEntryCacheMock, globalCapabilitiesDirectoryClient);
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
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
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
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
                                                            .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
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
            verify(routingTable, times(1)).incrementReferenceCount(eq(expectedLocalDiscoveryEntry.getParticipantId()));
            verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
        } else {
            DiscoveryEntryWithMetaInfo expectedGlobalDiscoveryEntry = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                                          globalDiscoveryEntry);
            assertEquals(expectedGlobalDiscoveryEntry, capturedDiscoveryEntries[0]);
            verify(routingTable, never()).incrementReferenceCount(any());
            verify(routingTable, times(1)).put(eq(expectedGlobalDiscoveryEntry.getParticipantId()),
                                               any(Address.class),
                                               any(Boolean.class),
                                               anyLong());
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
          .lookup(ArgumentMatchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                  eq(participantId),
                  anyLong(),
                  eq(knownGbids));

        Promise<Lookup3Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId);

        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo capturedRemoteGlobalEntry = (DiscoveryEntryWithMetaInfo) values[0];
        assertEquals(remoteGlobalEntryWithMetaInfo, capturedRemoteGlobalEntry);
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, times(1)).put(eq(remoteGlobalEntryWithMetaInfo.getParticipantId()),
                                           any(Address.class),
                                           any(Boolean.class),
                                           anyLong());
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
          .lookup(ArgumentMatchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                  eq(participantId),
                  anyLong(),
                  eq(knownGbids));

        Promise<Lookup4Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId,
                                                                                   discoveryQos,
                                                                                   new String[0]);

        checkPromiseSuccess(lookupPromise, "lookup failed");
        verify(globalCapabilitiesDirectoryClient).lookup(any(), eq(participantId), eq(discoveryTimeout), any());
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, times(1)).put(eq(participantId), any(Address.class), any(Boolean.class), anyLong());
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
        doReturn(Optional.of(cachedGlobalEntry)).when(globalDiscoveryEntryCacheMock)
                                                .lookup(eq(cachedGlobalEntry.getParticipantId()), eq(Long.MAX_VALUE));

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
                                                                      .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                              eq(domains),
                                                                              eq(interfaceName),
                                                                              anyLong(),
                                                                              eq(knownGbids));

        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos);

        Object[] values = checkPromiseSuccess(promise, "lookup failed");
        List<DiscoveryEntryWithMetaInfo> capabilities = Arrays.asList((DiscoveryEntryWithMetaInfo[]) values[0]);
        assertEquals(3, capabilities.size());
        assertTrue(capabilities.contains(localEntryWithMetaInfo));
        verify(routingTable, times(1)).incrementReferenceCount(eq(localEntryWithMetaInfo.getParticipantId()));
        verify(routingTable, never()).put(eq(localEntryWithMetaInfo.getParticipantId()),
                                          any(Address.class),
                                          any(Boolean.class),
                                          anyLong());
        assertTrue(capabilities.contains(cachedGlobalEntryWithMetaInfo));
        verify(routingTable, never()).incrementReferenceCount(eq(cachedGlobalEntryWithMetaInfo.getParticipantId()));
        verify(routingTable, times(1)).put(eq(cachedGlobalEntryWithMetaInfo.getParticipantId()),
                                           any(Address.class),
                                           eq(true),
                                           anyLong());
        assertTrue(capabilities.contains(remoteGlobalEntryWithMetaInfo));
        verify(routingTable, never()).incrementReferenceCount(eq(remoteGlobalEntryWithMetaInfo.getParticipantId()));
        verify(routingTable, times(1)).put(eq(remoteGlobalEntryWithMetaInfo.getParticipantId()),
                                           any(Address.class),
                                           eq(true),
                                           anyLong());
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
                                                          .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                  eq(domains),
                                                                  eq(interfaceName),
                                                                  anyLong(),
                                                                  ArgumentMatchers.<String[]> any());

        Promise<Lookup2Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                             interfaceName,
                                                                             discoveryQos,
                                                                             knownGbids);

        verify(globalCapabilitiesDirectoryClient).lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                         eq(domains),
                                                         eq(interfaceName),
                                                         anyLong(),
                                                         ArgumentMatchers.<String[]> any());
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());

        checkPromiseException(promise, expectedException);
    }

    private void testLookupByDomainInterfaceWithGbidsIsProperlyRejected(DiscoveryError expectedError) throws InterruptedException {
        String domain = "domain";
        String[] domains = new String[]{ domain };
        String interfaceName = "interface";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        doAnswer(createVoidAnswerWithDiscoveryError(expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                   .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                           eq(domains),
                                                                           eq(interfaceName),
                                                                           anyLong(),
                                                                           ArgumentMatchers.<String[]> any());

        Promise<Lookup2Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                             interfaceName,
                                                                             discoveryQos,
                                                                             knownGbids);

        verify(globalCapabilitiesDirectoryClient).lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                         eq(domains),
                                                         eq(interfaceName),
                                                         anyLong(),
                                                         ArgumentMatchers.<String[]> any());
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());

        checkPromiseError(promise, expectedError);
    }

    private void testLookupByDomainInterfaceIsProperlyRejected(DiscoveryError expectedError) throws InterruptedException {
        String domain = "domain";
        String[] domains = new String[]{ domain };
        String interfaceName = "interface";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        doAnswer(createVoidAnswerWithDiscoveryError(expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                   .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                           eq(domains),
                                                                           eq(interfaceName),
                                                                           anyLong(),
                                                                           ArgumentMatchers.<String[]> any());

        Promise<Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos);

        verify(globalCapabilitiesDirectoryClient).lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                         eq(domains),
                                                         eq(interfaceName),
                                                         anyLong(),
                                                         ArgumentMatchers.<String[]> any());
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());

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
                                                          .lookup(ArgumentMatchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                                                                  eq(participantId),
                                                                  anyLong(),
                                                                  ArgumentMatchers.<String[]> any());

        Promise<Lookup4Deferred> promise = localCapabilitiesDirectory.lookup(participantId, discoveryQos, knownGbids);

        checkPromiseException(promise, expectedException);
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
    }

    private void testLookupByParticipantIdWithGbidsIsProperlyRejected(DiscoveryError expectedError) throws InterruptedException {
        String participantId = "participantId";

        doAnswer(createVoidAnswerWithDiscoveryError(expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                   .lookup(ArgumentMatchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                                                                           eq(participantId),
                                                                           anyLong(),
                                                                           ArgumentMatchers.<String[]> any());

        DiscoveryQos discoveryQos = new DiscoveryQos(10000L, 500L, DiscoveryScope.LOCAL_AND_GLOBAL, false);
        Promise<Lookup4Deferred> promise = localCapabilitiesDirectory.lookup(participantId, discoveryQos, knownGbids);

        checkPromiseError(promise, expectedError);
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
    }

    private void testLookupByParticipantIdIsProperlyRejected(DiscoveryError expectedError) throws InterruptedException {
        String participantId = "participantId";

        doAnswer(createVoidAnswerWithDiscoveryError(expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                   .lookup(ArgumentMatchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                                                                           eq(participantId),
                                                                           anyLong(),
                                                                           ArgumentMatchers.<String[]> any());

        Promise<Lookup3Deferred> promise = localCapabilitiesDirectory.lookup(participantId);

        checkPromiseErrorInProviderRuntimeException(promise, expectedError);
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
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
               never()).lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                               any(String[].class),
                               anyString(),
                               anyLong(),
                               any(String[].class));
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());

        checkPromiseError(promise, expectedError);
    }

    private void testLookupByParticipantIdWithDiscoveryError(String[] gbids,
                                                             DiscoveryError expectedError) throws InterruptedException {
        String participantId = "participantId";
        Promise<Lookup4Deferred> promise = localCapabilitiesDirectory.lookup(participantId, new DiscoveryQos(), gbids);

        verify(globalCapabilitiesDirectoryClient,
               never()).lookup(ArgumentMatchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                               anyString(),
                               anyLong(),
                               any(String[].class));
        checkPromiseError(promise, expectedError);
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
    }

    static void checkPromiseException(Promise<?> promise, Exception expectedException) throws InterruptedException {
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

    static Object[] checkPromiseSuccess(Promise<? extends AbstractDeferred> promise,
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
                                                      .remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                              eq(globalDiscoveryEntry.getParticipantId()),
                                                              any(String[].class));

        when(localDiscoveryEntryStoreMock.lookup(discoveryEntry.getParticipantId(),
                                                 Long.MAX_VALUE)).thenReturn(Optional.of(discoveryEntry));
        localCapabilitiesDirectory.remove(globalDiscoveryEntry.getParticipantId());

        assertTrue(cdlStart.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verifyNoMoreInteractions(routingTable);
        verify(globalCapabilitiesDirectoryClient).remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                         eq(discoveryEntry.getParticipantId()),
                                                         any(String[].class));
        verify(localDiscoveryEntryStoreMock, times(0)).remove(any(String.class));
        verify(globalDiscoveryEntryCacheMock, times(0)).remove(any(String.class));
        assertTrue(cdlDone.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(localDiscoveryEntryStoreMock, times(1)).remove(eq(discoveryEntry.getParticipantId()));
        verify(globalDiscoveryEntryCacheMock, times(0)).remove(anyString());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_localProvider_GcdNotCalled() throws InterruptedException {
        discoveryEntry.getQos().setScope(ProviderScope.LOCAL);
        Boolean awaitGlobalRegistration = true;
        Promise<DeferredVoid> addPromise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        checkPromiseSuccess(addPromise, "add failed");

        when(localDiscoveryEntryStoreMock.lookup(discoveryEntry.getParticipantId(),
                                                 Long.MAX_VALUE)).thenReturn(Optional.of(discoveryEntry));
        localCapabilitiesDirectory.remove(discoveryEntry.getParticipantId());

        Thread.sleep(500);

        verifyNoMoreInteractions(routingTable);
        verify(globalCapabilitiesDirectoryClient,
               times(0)).remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                anyString(),
                                any(String[].class));
        verify(globalDiscoveryEntryCacheMock, times(0)).remove(anyString());
        verify(localDiscoveryEntryStoreMock, times(1)).remove(eq(discoveryEntry.getParticipantId()));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_participantNotRegisteredNoGbids_GcdNotCalled() throws InterruptedException {
        String participantId = "unknownparticipantId";
        CountDownLatch cdl = new CountDownLatch(1);

        doReturn(Optional.empty()).when(localDiscoveryEntryStoreMock).lookup(eq(participantId), anyLong());

        localCapabilitiesDirectory.remove(participantId);

        assertFalse(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verifyNoMoreInteractions(routingTable);
        verify(globalCapabilitiesDirectoryClient,
               never()).remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
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
                                              .remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      eq(provisionedGlobalDiscoveryEntry.getParticipantId()),
                                                      any(String[].class));

        doReturn(Optional.empty()).when(localDiscoveryEntryStoreMock)
                                  .lookup(eq(provisionedGlobalDiscoveryEntry.getParticipantId()), anyLong());
        localCapabilitiesDirectory.remove(provisionedGlobalDiscoveryEntry.getParticipantId());

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verifyNoMoreInteractions(routingTable);
        verify(globalCapabilitiesDirectoryClient,
               times(1)).remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                eq(provisionedGlobalDiscoveryEntry.getParticipantId()),
                                any(String[].class));
        verify(localDiscoveryEntryStoreMock, times(1)).remove(eq(provisionedGlobalDiscoveryEntry.getParticipantId()));
        verify(globalDiscoveryEntryCacheMock, times(0)).remove(anyString());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_FailureStates_TimeoutException() throws InterruptedException {
        CountDownLatch cdl = new CountDownLatch(2);
        doAnswer(createVoidAnswerWithException(cdl,
                                               new JoynrTimeoutException(0))).when(globalCapabilitiesDirectoryClient)
                                                                             .remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                                     eq(provisionedGlobalDiscoveryEntry.getParticipantId()),
                                                                                     any(String[].class));

        localCapabilitiesDirectory.remove(provisionedGlobalDiscoveryEntry.getParticipantId());

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verifyNoMoreInteractions(routingTable);
        verify(globalCapabilitiesDirectoryClient,
               atLeast(2)).remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
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
                                                                                  .remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                                          eq(provisionedGlobalDiscoveryEntry.getParticipantId()),
                                                                                          any(String[].class));
        localCapabilitiesDirectory.remove(provisionedGlobalDiscoveryEntry.getParticipantId());
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verifyNoMoreInteractions(routingTable);
        verify(globalCapabilitiesDirectoryClient,
               times(1)).remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
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
                                                                                             .remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                                                     eq(provisionedGlobalDiscoveryEntry.getParticipantId()),
                                                                                                     any(String[].class));

        localCapabilitiesDirectory.remove(provisionedGlobalDiscoveryEntry.getParticipantId());

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verifyNoMoreInteractions(routingTable);
        verify(globalCapabilitiesDirectoryClient,
               times(1)).remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                eq(provisionedGlobalDiscoveryEntry.getParticipantId()),
                                any(String[].class));
        verify(globalDiscoveryEntryCacheMock, times(0)).remove(anyString());
        verify(localDiscoveryEntryStoreMock, times(1)).remove(eq(provisionedGlobalDiscoveryEntry.getParticipantId()));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void remove_FailureStates_DiscoveryError_InvalidGbid() throws InterruptedException {
        // Also covers UNKNOWN_GBID and INTERNAL_ERROR
        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(createVoidAnswerWithDiscoveryError(cdl,
                                                    DiscoveryError.INVALID_GBID)).when(globalCapabilitiesDirectoryClient)
                                                                                 .remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                                                         eq(provisionedGlobalDiscoveryEntry.getParticipantId()),
                                                                                         any(String[].class));

        localCapabilitiesDirectory.remove(provisionedGlobalDiscoveryEntry.getParticipantId());

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verifyNoMoreInteractions(routingTable);
        verify(globalCapabilitiesDirectoryClient,
               times(1)).remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
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
                                              .remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      any(String.class),
                                                      any(String[].class));

        when(localDiscoveryEntryStoreMock.lookup(globalDiscoveryEntry.getParticipantId(),
                                                 Long.MAX_VALUE)).thenReturn(Optional.of(globalDiscoveryEntry));
        localCapabilitiesDirectory.remove(globalDiscoveryEntry.getParticipantId());

        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient).remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                         any(String.class),
                                                         eq(expectedGbids));
        verifyNoMoreInteractions(routingTable);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testRemoveUsesSameGbidOrderAsAdd() throws InterruptedException {
        testRemoveUsesSameGbidOrderAsAdd(new String[]{ knownGbids[0] });

        testRemoveUsesSameGbidOrderAsAdd(new String[]{ knownGbids[1] });

        testRemoveUsesSameGbidOrderAsAdd(new String[]{ knownGbids[0], knownGbids[1] });

        testRemoveUsesSameGbidOrderAsAdd(new String[]{ knownGbids[1], knownGbids[0] });
    }

    @Test(timeout = TEST_TIMEOUT)
    public void taskSequencerDoesNotCrashOnExceptionAfterRemoveTaskFinished() throws InterruptedException,
                                                                              IllegalAccessException {
        /// We have to add before we can remove anything
        String[] expectedGbids = knownGbids.clone();
        final boolean awaitGlobalRegistration = true;
        Promise<AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                awaitGlobalRegistration);
        checkPromiseSuccess(promise, "add failed");
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             any(),
                             anyLong(),
                             eq(expectedGbids));
        reset(globalCapabilitiesDirectoryClient);
        ///
        ///The real test starts here
        CountDownLatch cdl1 = new CountDownLatch(1);
        CountDownLatch cdl2 = new CountDownLatch(1);

        AtomicBoolean cbCalled = new AtomicBoolean();
        GcdTask.CallbackCreator callbackCreator = new GcdTask.CallbackCreator() {
            @Override
            public CallbackWithModeledError<Void, DiscoveryError> createCallback() {
                return new CallbackWithModeledError<Void, DiscoveryError>() {
                    @Override
                    public void onFailure(DiscoveryError errorEnum) {
                        // taskFinished is called manually
                        logger.error("onFailure callback called, DiscoveryError {}", errorEnum);
                        cbCalled.set(true);
                    }

                    @Override
                    public void onFailure(JoynrRuntimeException runtimeException) {
                        // taskFinished is called manually
                        logger.error("onFailure callback called:", runtimeException);
                        cbCalled.set(true);
                    }

                    @Override
                    public void onSuccess(Void result) {
                        // taskFinished is called manually
                        logger.error("onSuccess callback called");
                        cbCalled.set(true);
                    }
                };
            }
        };
        class TestGcdRemoveTask extends GcdTask {
            public TestGcdRemoveTask(CallbackCreator callbackCreator, String participantId) {
                super(MODE.REMOVE, callbackCreator, participantId, null, null, 0l, true);
            }

            @Override
            public String getParticipantId() {
                cdl1.countDown();
                try {
                    // block GcdTaskSequencer until taskFinished has been called
                    cdl2.await();
                } catch (InterruptedException e) {
                    // ignore
                }
                return super.getParticipantId();
            }
        }
        TestGcdRemoveTask task = new TestGcdRemoveTask(callbackCreator, globalDiscoveryEntry.getParticipantId());
        gcdTaskSequencer.addTask(task);

        assertTrue(cdl1.await(DEFAULT_WAIT_TIME_MS * 100, TimeUnit.MILLISECONDS));
        // call taskFinished while task is processed
        gcdTaskSequencer.taskFinished();
        cdl2.countDown();

        verify(globalCapabilitiesDirectoryClient,
               timeout(DEFAULT_WAIT_TIME_MS).times(1)).remove(any(),
                                                              eq(globalDiscoveryEntry.getParticipantId()),
                                                              eq(expectedGbids));

        // check that GcdTaskSequencer is still alive
        localCapabilitiesDirectory.addToAll(discoveryEntry, awaitGlobalRegistration);
        verify(globalCapabilitiesDirectoryClient, timeout(DEFAULT_WAIT_TIME_MS).times(1)).add(any(),
                                                                                              any(),
                                                                                              anyLong(),
                                                                                              eq(expectedGbids));

        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient);
        assertFalse(cbCalled.get());
    }

    @Test(timeout = TEST_TIMEOUT)
    public void taskSequencerNotReleasedAfterRemoveSuccess() throws InterruptedException, IllegalAccessException {
        /// We have to add before we can remove anything
        String[] expectedGbids = knownGbids.clone();
        final boolean awaitGlobalRegistration = true;
        Promise<AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                awaitGlobalRegistration);
        checkPromiseSuccess(promise, "add failed");
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             any(),
                             anyLong(),
                             eq(expectedGbids));
        reset(globalCapabilitiesDirectoryClient);
        ///
        ///The real test starts here
        setFieldValue(localCapabilitiesDirectory, "gcdTaskSequencer", gcdTaskSequencerSpy);
        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                cdl.countDown();
                return null;
            }
        }).when(globalCapabilitiesDirectoryClient).remove(any(), any(), any());
        localCapabilitiesDirectory.remove(discoveryEntry.getParticipantId());
        verify(gcdTaskSequencerSpy).addTask(argThat(arg -> GcdTask.MODE.REMOVE.equals(arg.getMode())));
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient, times(1)).remove(callbackCaptor.capture(),
                                                                   eq(discoveryEntry.getParticipantId()),
                                                                   eq(expectedGbids));

        callbackCaptor.getValue().onSuccess(null);
        verify(gcdTaskSequencerSpy).taskFinished();
        callbackCaptor.getValue().onSuccess(null);
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient, gcdTaskSequencerSpy);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void taskSequencerRetriesRemoveOnJoynrTimeoutExceptionOnly() throws InterruptedException,
                                                                        IllegalAccessException {
        ///We need to add before we can remove
        String[] expectedGbids = knownGbids.clone();
        final boolean awaitGlobalRegistration = true;
        Promise<AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                awaitGlobalRegistration);
        checkPromiseSuccess(promise, "add failed");
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             any(),
                             anyLong(),
                             eq(expectedGbids));
        reset(globalCapabilitiesDirectoryClient);
        ///
        ///The real test starts here
        setFieldValue(localCapabilitiesDirectory, "gcdTaskSequencer", gcdTaskSequencerSpy);
        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                cdl.countDown();
                return null;
            }
        }).when(globalCapabilitiesDirectoryClient).remove(any(), any(), any());
        localCapabilitiesDirectory.remove(discoveryEntry.getParticipantId());
        verify(gcdTaskSequencerSpy).addTask(argThat(arg -> GcdTask.MODE.REMOVE.equals(arg.getMode())));
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient, times(1)).remove(callbackCaptor.capture(),
                                                                   eq(discoveryEntry.getParticipantId()),
                                                                   eq(expectedGbids));

        CountDownLatch cdl2 = new CountDownLatch(1);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                cdl2.countDown();
                return null;
            }
        }).when(globalCapabilitiesDirectoryClient).remove(any(), any(), any());
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        verify(gcdTaskSequencerSpy).retryTask();
        callbackCaptor.getValue().onSuccess(null);
        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        assertTrue(cdl2.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient, times(2)).remove(callbackCaptor.capture(),
                                                                   eq(discoveryEntry.getParticipantId()),
                                                                   eq(expectedGbids));
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        verify(gcdTaskSequencerSpy).taskFinished();
        // After handling a non-timeout exception, the callback is 'disabled'
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        callbackCaptor.getValue().onSuccess(null);
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient, gcdTaskSequencerSpy);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void taskSequencerNotReleasedAfterRemoveDiscoveryError() throws InterruptedException,
                                                                    IllegalAccessException {
        ///We need to add before we can remove
        String[] expectedGbids = knownGbids.clone();
        final boolean awaitGlobalRegistration = true;
        Promise<AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                awaitGlobalRegistration);
        checkPromiseSuccess(promise, "add failed");
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             any(),
                             anyLong(),
                             eq(expectedGbids));
        reset(globalCapabilitiesDirectoryClient);
        ///
        ///The real test starts here
        setFieldValue(localCapabilitiesDirectory, "gcdTaskSequencer", gcdTaskSequencerSpy);
        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                cdl.countDown();
                return null;
            }
        }).when(globalCapabilitiesDirectoryClient).remove(any(), any(), any());
        localCapabilitiesDirectory.remove(discoveryEntry.getParticipantId());
        verify(gcdTaskSequencerSpy).addTask(argThat(arg -> GcdTask.MODE.REMOVE.equals(arg.getMode())));
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient, times(1)).remove(callbackCaptor.capture(),
                                                                   eq(discoveryEntry.getParticipantId()),
                                                                   eq(expectedGbids));

        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        verify(gcdTaskSequencerSpy).taskFinished();
        callbackCaptor.getValue().onFailure(new JoynrTimeoutException(12345));
        callbackCaptor.getValue().onSuccess(null);
        callbackCaptor.getValue().onFailure(new JoynrRuntimeException());
        callbackCaptor.getValue().onFailure(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient, gcdTaskSequencerSpy);
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
                                              .touch(ArgumentMatchers.<Callback<Void>> any(),
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

        verify(globalCapabilitiesDirectoryClient, times(1)).touch(ArgumentMatchers.<Callback<Void>> any(),
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

        verify(globalCapabilitiesDirectoryClient, times(0)).touch(ArgumentMatchers.<Callback<Void>> any(),
                                                                  any(),
                                                                  anyString());
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
                                              .touch(ArgumentMatchers.<Callback<Void>> any(),
                                                     eq(participantIdsToTouch),
                                                     anyString());
        Thread.sleep(freshnessUpdateIntervalMs); // make sure that the initial delay has expired before starting the runnable
        Runnable runnable = runnableCaptor.getValue();
        runnable.run();
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient, times(1)).touch(ArgumentMatchers.<Callback<Void>> any(),
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
                                              .touch(ArgumentMatchers.<Callback<Void>> any(),
                                                     eq(participantIdsToTouch),
                                                     anyString());
        Thread.sleep(freshnessUpdateIntervalMs); // make sure that the initial delay has expired before starting the runnable
        Runnable runnable = runnableCaptor.getValue();
        runnable.run();
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));
        verify(globalCapabilitiesDirectoryClient, times(1)).touch(ArgumentMatchers.<Callback<Void>> any(),
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
                                              .touch(ArgumentMatchers.<Callback<Void>> any(),
                                                     ArgumentMatchers.<String[]> any(),
                                                     anyString());
        Thread.sleep(freshnessUpdateIntervalMs); // make sure that the initial delay has expired before starting the runnable
        Runnable runnable = runnableCaptor.getValue();
        runnable.run();
        assertTrue(cdl.await(DEFAULT_WAIT_TIME_MS, TimeUnit.MILLISECONDS));

        verify(globalCapabilitiesDirectoryClient, times(1)).touch(ArgumentMatchers.<Callback<Void>> any(),
                                                                  eq(expectedParticipantIds1),
                                                                  eq(gbid1));

        verify(globalCapabilitiesDirectoryClient, times(1)).touch(ArgumentMatchers.<Callback<Void>> any(),
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
        verify(globalCapabilitiesDirectoryClient,
               times(knownGbids.length)).removeStale(ArgumentMatchers.<Callback<Void>> any(),
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
              .removeStale(ArgumentMatchers.<Callback<Void>> any(), anyLong(), eq(gbid));
        }

        localCapabilitiesDirectory.removeStaleProvidersOfClusterController();

        int numberOfCalls = numberOfOnFailureCalls + 1; // one time success

        for (String gbid : knownGbids) {
            verify(globalCapabilitiesDirectoryClient,
                   times(numberOfCalls)).removeStale(ArgumentMatchers.<Callback<Void>> any(), anyLong(), eq(gbid));
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
        }).when(globalCapabilitiesDirectoryClient)
          .removeStale(ArgumentMatchers.<Callback<Void>> any(), anyLong(), anyString());

        localCapabilitiesDirectory.removeStaleProvidersOfClusterController();

        for (String gbid : knownGbids) {
            verify(globalCapabilitiesDirectoryClient, times(1)).removeStale(ArgumentMatchers.<Callback<Void>> any(),
                                                                            anyLong(),
                                                                            eq(gbid));
        }
    }

    @Test
    public void removeStaleProvidersOfClusterController_noRetryIfRetryDurationExceeded() {
        final long removeStaleMaxRetryMs = 3600000;
        // Set a custom value of cluster controller start time to simulate timeout for removeStale retries
        final long ccStartUpDateMs = removeStaleMaxRetryMs + 1;
        try {
            setFieldValue(localCapabilitiesDirectory, "ccStartUpDateInMs", ccStartUpDateMs);
        } catch (Exception e) {
            fail("Couldn't set start date of cluster controller in milliseconds.");
        }

        JoynrRuntimeException exception = new JoynrRuntimeException("removeStale failed");
        for (String gbid : knownGbids) {
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
            }).when(globalCapabilitiesDirectoryClient)
              .removeStale(ArgumentMatchers.<Callback<Void>> any(), anyLong(), eq(gbid));
        }

        localCapabilitiesDirectory.removeStaleProvidersOfClusterController();

        for (String gbid : knownGbids) {
            verify(globalCapabilitiesDirectoryClient, times(1)).removeStale(ArgumentMatchers.<Callback<Void>> any(),
                                                                            eq(ccStartUpDateMs),
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
               .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                    argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry1)),
                    remainingTtlCapture.capture(),
                    any(String[].class));

        checkRemainingTtl(remainingTtlCapture);

        inOrder.verify(globalCapabilitiesDirectoryClient)
               .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                    argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry2)),
                    remainingTtlCapture.capture(),
                    any(String[].class));

        checkRemainingTtl(remainingTtlCapture);

        CountDownLatch cdl = new CountDownLatch(2);
        doAnswer(createAnswerWithSuccess(cdl)).when(globalCapabilitiesDirectoryClient)
                                              .remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
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
               .remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                       eq(participantId2),
                       any(String[].class));
        inOrder.verify(globalCapabilitiesDirectoryClient)
               .remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                       eq(participantId1),
                       any(String[].class));
    }

    private static class GlobalDiscoveryEntryWithParticipantIdMatcher implements ArgumentMatcher<GlobalDiscoveryEntry> {
        private GlobalDiscoveryEntry expected;

        private GlobalDiscoveryEntryWithParticipantIdMatcher(GlobalDiscoveryEntry expected) {
            this.expected = expected;
        }

        @Override
        public boolean matches(GlobalDiscoveryEntry argument) {
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
                                                       .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                            argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry1)),
                                                            anyLong(),
                                                            any(String[].class));

        CountDownLatch cdlRemove = new CountDownLatch(1);
        doAnswer(createAnswerWithSuccess(cdlRemove)).when(globalCapabilitiesDirectoryClient)
                                                    .remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
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
               .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                    argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry1)),
                    anyLong(),
                    any(String[].class));

        inOrder.verify(globalCapabilitiesDirectoryClient, times(1))
               .remove(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                       eq(participantId1),
                       any(String[].class));

        verify(globalCapabilitiesDirectoryClient,
               times(0)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
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
                                                   .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                        argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry1)),
                                                        anyLong(),
                                                        eq(gbids1));

        doAnswer(createAnswerWithSuccess(cdlReAdd)).when(globalCapabilitiesDirectoryClient)
                                                   .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                        argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry2)),
                                                        anyLong(),
                                                        eq(gbids2));

        Set<DiscoveryEntry> globalEntries = new HashSet<>();
        globalEntries.add(discoveryEntry1);
        globalEntries.add(discoveryEntry2);
        when(localDiscoveryEntryStoreMock.getAllGlobalEntries()).thenReturn(globalEntries);

        verify(globalCapabilitiesDirectoryClient, times(0)).add(any(), any(), anyLong(), any());

        verify(capabilitiesFreshnessUpdateExecutor).scheduleAtFixedRate(runnableCaptor.capture(),
                                                                        eq(RE_ADD_INTERVAL_DAYS),
                                                                        eq(RE_ADD_INTERVAL_DAYS),
                                                                        eq(TimeUnit.DAYS));

        // capture the runnable and execute it to schedule the re-add task
        Runnable runnable = runnableCaptor.getValue();
        runnable.run();

        assertTrue(cdlReAdd.await(defaultTtlAddAndRemove, TimeUnit.MILLISECONDS));

        // check whether add method has been called for 2 non expired entries
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry1)),
                             eq(defaultTtlAddAndRemove),
                             eq(expectedGbids1));

        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             argThat(new GlobalDiscoveryEntryWithParticipantIdMatcher(globalDiscoveryEntry2)),
                             eq(defaultTtlAddAndRemove),
                             eq(expectedGbids2));
    }
}
