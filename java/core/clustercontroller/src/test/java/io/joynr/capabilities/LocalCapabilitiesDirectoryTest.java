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

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.argThat;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.hamcrest.TypeSafeMatcher;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatcher;
import org.mockito.Captor;
import org.mockito.Matchers;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.internal.matchers.VarargMatcher;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.fasterxml.jackson.databind.ObjectMapper;

import io.joynr.dispatching.Dispatcher;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrRuntimeException;
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
import joynr.exceptions.ApplicationException;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.infrastructure.GlobalDomainAccessController;
import joynr.system.DiscoveryProvider.Add1Deferred;
import joynr.system.DiscoveryProvider.AddToAllDeferred;
import joynr.system.DiscoveryProvider.Lookup1Deferred;
import joynr.system.DiscoveryProvider.Lookup2Deferred;
import joynr.system.DiscoveryProvider.Lookup3Deferred;
import joynr.system.RoutingTypes.ChannelAddress;
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
    private static final String TEST_URL = "http://testUrl";
    private static final long ONE_DAY_IN_MS = 1 * 24 * 60 * 60 * 1000;
    private static final long defaultDiscoveryRetryIntervalMs = 2000L;
    private Long expiryDateMs = System.currentTimeMillis() + ONE_DAY_IN_MS;
    private String publicKeyId = "publicKeyId";
    private String[] knownGbids = { "testDEFAULTgbid", "testgbid2" };

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

    private LocalCapabilitiesDirectory localCapabilitiesDirectory;
    private MqttAddress globalAddress;
    private String globalAddressSerialized;
    private DiscoveryEntry discoveryEntry;
    private GlobalDiscoveryEntry globalDiscoveryEntry;

    public interface TestInterface {
        public static final String INTERFACE_NAME = "interfaceName";
    }

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

    @Before
    public void setUp() throws Exception {

        globalAddress = new MqttAddress(knownGbids[0], "testTopic");
        ObjectMapper objectMapper = new ObjectMapper();
        globalAddressSerialized = objectMapper.writeValueAsString(globalAddress);

        Field objectMapperField = CapabilityUtils.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(CapabilityUtils.class, objectMapper);

        doAnswer(createAddAnswerWithSuccess()).when(globalCapabilitiesDirectoryClient)
                                              .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                   any(GlobalDiscoveryEntry.class),
                                                   Matchers.<String[]> any());

        String discoveryDirectoriesDomain = "io.joynr";
        String capabilitiesDirectoryParticipantId = "capDir_participantId";
        String capabiltitiesDirectoryChannelId = "dirchannelId";
        String domainAccessControllerParticipantId = "domainAccessControllerParticipantId";
        String domainAccessControllerChannelId = "domainAccessControllerChannelId";
        GlobalDiscoveryEntry globalCapabilitiesDirectoryDiscoveryEntry = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0,
                                                                                                                             1),
                                                                                                                 discoveryDirectoriesDomain,
                                                                                                                 GlobalCapabilitiesDirectory.INTERFACE_NAME,
                                                                                                                 capabilitiesDirectoryParticipantId,
                                                                                                                 new ProviderQos(),
                                                                                                                 System.currentTimeMillis(),
                                                                                                                 expiryDateMs,
                                                                                                                 domainAccessControllerChannelId,
                                                                                                                 new ChannelAddress(TEST_URL,
                                                                                                                                    capabiltitiesDirectoryChannelId));

        GlobalDiscoveryEntry domainAccessControllerDiscoveryEntry = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0,
                                                                                                                        1),
                                                                                                            discoveryDirectoriesDomain,
                                                                                                            GlobalDomainAccessController.INTERFACE_NAME,
                                                                                                            domainAccessControllerParticipantId,
                                                                                                            new ProviderQos(),
                                                                                                            System.currentTimeMillis(),
                                                                                                            expiryDateMs,
                                                                                                            domainAccessControllerChannelId,
                                                                                                            new ChannelAddress(TEST_URL,
                                                                                                                               domainAccessControllerChannelId));

        when(capabilitiesProvisioning.getDiscoveryEntries()).thenReturn(new HashSet<GlobalDiscoveryEntry>(Arrays.asList(globalCapabilitiesDirectoryDiscoveryEntry,
                                                                                                                        domainAccessControllerDiscoveryEntry)));
        // use default freshnessUpdateIntervalMs: 3600000ms (1h)
        localCapabilitiesDirectory = new LocalCapabilitiesDirectoryImpl(capabilitiesProvisioning,
                                                                        globalAddressProvider,
                                                                        localDiscoveryEntryStoreMock,
                                                                        globalDiscoveryEntryCacheMock,
                                                                        messageRouter,
                                                                        globalCapabilitiesDirectoryClient,
                                                                        expiredDiscoveryEntryCacheCleaner,
                                                                        3600000,
                                                                        capabilitiesFreshnessUpdateExecutor,
                                                                        defaultDiscoveryRetryIntervalMs,
                                                                        shutdownNotifier,
                                                                        knownGbids);
        verify(expiredDiscoveryEntryCacheCleaner).scheduleCleanUpForCaches(Mockito.<ExpiredDiscoveryEntryCacheCleaner.CleanupAction> any(),
                                                                           argThat(new DiscoveryEntryStoreVarargMatcher(globalDiscoveryEntryCacheMock,
                                                                                                                        localDiscoveryEntryStoreMock)));
        verify(capabilitiesFreshnessUpdateExecutor).scheduleAtFixedRate(runnableCaptor.capture(),
                                                                        anyLong(),
                                                                        anyLong(),
                                                                        eq(TimeUnit.MILLISECONDS));

        ProviderQos providerQos = new ProviderQos();
        CustomParameter[] parameterList = { new CustomParameter("key1", "value1"),
                new CustomParameter("key2", "value2") };
        providerQos.setCustomParameters(parameterList);

        String participantId = "testParticipantId";
        String domain = "domain";
        discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                            domain,
                                            TestInterface.INTERFACE_NAME,
                                            participantId,
                                            providerQos,
                                            System.currentTimeMillis(),
                                            expiryDateMs,
                                            publicKeyId);
        globalDiscoveryEntry = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry, globalAddress);
        when(globalAddressProvider.get()).thenReturn(globalAddress);
    }

    private void checkCallToGlobalCapabilitiesDirectoryClientAndCache(DiscoveryEntry discoveryEntry,
                                                                      String[] expectedGbids) {
        ArgumentCaptor<GlobalDiscoveryEntry> argumentCaptor = ArgumentCaptor.forClass(GlobalDiscoveryEntry.class);
        verify(globalCapabilitiesDirectoryClient,
               timeout(200)).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                 argumentCaptor.capture(),
                                 eq(expectedGbids));
        GlobalDiscoveryEntry capturedGlobalDiscoveryEntry = argumentCaptor.getValue();
        assertNotNull(capturedGlobalDiscoveryEntry);
        assertEquals(discoveryEntry.getDomain(), capturedGlobalDiscoveryEntry.getDomain());
        assertEquals(discoveryEntry.getInterfaceName(), capturedGlobalDiscoveryEntry.getInterfaceName());
        verify(globalDiscoveryEntryCacheMock, times(1)).add(eq(globalDiscoveryEntry));
    }

    @Test(timeout = 1000)
    public void addCapability() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        String[] expectedGbids = new String[]{ knownGbids[0] };

        localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        checkCallToGlobalCapabilitiesDirectoryClientAndCache(discoveryEntry, expectedGbids);
    }

    @Test(timeout = 1000)
    public void addCapabilityWithSingleNonDefaultGbid() throws InterruptedException {
        String[] gbids = new String[]{ knownGbids[1] };
        String[] expectedGbids = gbids.clone();
        final boolean awaitGlobalRegistration = true;
        localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids);
        checkCallToGlobalCapabilitiesDirectoryClientAndCache(discoveryEntry, expectedGbids);
    }

    @Test(timeout = 1000)
    public void addCapabilityWithMultipleGbids() throws InterruptedException {
        // expectedGbids element order intentionally differs from knownGbids element order
        String[] gbids = new String[]{ knownGbids[1], knownGbids[0] };
        String[] expectedGbids = gbids.clone();
        final boolean awaitGlobalRegistration = true;
        localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids);
        checkCallToGlobalCapabilitiesDirectoryClientAndCache(discoveryEntry, expectedGbids);
    }

    @Test(timeout = 1000)
    public void addCapabilityWithGbidsWithoutElements() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        String[] gbids = new String[]{};
        String[] expectedGbids = new String[]{ knownGbids[0] };
        localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids);
        checkCallToGlobalCapabilitiesDirectoryClientAndCache(discoveryEntry, expectedGbids);
    }

    @Test(timeout = 1000)
    public void addCapabilityWithAddToAll() throws InterruptedException {
        String[] expectedGbids = new String[]{ knownGbids[0], knownGbids[1] };
        final boolean awaitGlobalRegistration = true;
        localCapabilitiesDirectory.addToAll(discoveryEntry, awaitGlobalRegistration);
        checkCallToGlobalCapabilitiesDirectoryClientAndCache(discoveryEntry, expectedGbids);
    }

    private static void checkDiscoveryError(Promise<Add1Deferred> promise, DiscoveryError expectedDiscoveryError) {
        assertEquals(promise.isRejected(), true);
        promise.then(new PromiseListener() {
            @Override
            public void onFulfillment(Object... values) {
                fail("localCapabilitiesDirectory.add succeeded unexpectedly ");
            }

            @Override
            public void onRejection(JoynrException error) {
                assertTrue(error instanceof ApplicationException);
                assertTrue(((ApplicationException) error).getError() == expectedDiscoveryError);
            }
        });
    }

    @Test(timeout = 1000)
    public void addCapabilityWithNullGbids() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        String[] gbids = null;
        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids);
        checkDiscoveryError(promise, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = 1000)
    public void addCapabilityWithGbidsWithNullEntry() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        String[] gbids = new String[]{ knownGbids[0], null };
        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids);
        checkDiscoveryError(promise, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = 1000)
    public void addCapabilityWithGbidsWithEmptyStringEntry() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        String[] gbids = new String[]{ knownGbids[0], "" };
        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids);
        checkDiscoveryError(promise, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = 1000)
    public void addCapabilityWithGbidsWithDuplicateStringEntries() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        String[] gbids = new String[]{ knownGbids[0], knownGbids[0] };
        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids);
        checkDiscoveryError(promise, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = 1000)
    public void addCapabilityWithUnknownGbidEntry() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        String[] gbids = new String[]{ knownGbids[0], "unknownGbid" };
        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids);
        checkDiscoveryError(promise, DiscoveryError.UNKNOWN_GBID);
    }

    @Test(timeout = 2000)
    public void addLocalOnlyCapability() throws InterruptedException {

        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                            "test",
                                            TestInterface.INTERFACE_NAME,
                                            "participantId",
                                            providerQos,
                                            System.currentTimeMillis(),
                                            expiryDateMs,
                                            publicKeyId);

        localCapabilitiesDirectory.add(discoveryEntry);
        Thread.sleep(1000);
        verify(localDiscoveryEntryStoreMock).add(discoveryEntry);
        verify(globalCapabilitiesDirectoryClient,
               never()).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                            any(GlobalDiscoveryEntry.class),
                            Matchers.<String[]> any());
        verify(globalDiscoveryEntryCacheMock, never()).add(Matchers.<GlobalDiscoveryEntry> any());
    }

    @Test(timeout = 1000)
    public void addGlobalCapSucceeds_NextAddShallNotAddGlobalAgain() throws InterruptedException {
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.GLOBAL);

        String participantId = LocalCapabilitiesDirectoryTest.class.getName()
                + ".addGlobalCapSucceeds_NextAddShallNotAddGlobalAgain";
        String domain = "testDomain";
        long currentTime = System.currentTimeMillis();
        final DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                                 domain,
                                                                 TestInterface.INTERFACE_NAME,
                                                                 participantId,
                                                                 providerQos,
                                                                 currentTime,
                                                                 expiryDateMs,
                                                                 publicKeyId);
        globalDiscoveryEntry = new GlobalDiscoveryEntry(new Version(47, 11),
                                                        domain,
                                                        TestInterface.INTERFACE_NAME,
                                                        participantId,
                                                        providerQos,
                                                        currentTime,
                                                        expiryDateMs,
                                                        publicKeyId,
                                                        globalAddressSerialized);

        final boolean awaitGlobalRegistration = true;
        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);

        verify(globalDiscoveryEntryCacheMock).add(eq(globalDiscoveryEntry));
        verify(globalCapabilitiesDirectoryClient).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      eq(globalDiscoveryEntry),
                                                      Matchers.<String[]> any());
        checkPromiseSuccess(promise, "add failed");
        reset(globalCapabilitiesDirectoryClient);

        doReturn(true).when(localDiscoveryEntryStoreMock).hasDiscoveryEntry(discoveryEntry);
        when(globalDiscoveryEntryCacheMock.lookup(eq(discoveryEntry.getParticipantId()),
                                                  anyLong())).thenReturn(globalDiscoveryEntry);
        Promise<DeferredVoid> promise2 = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);

        verify(globalCapabilitiesDirectoryClient,
               never()).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                            eq(globalDiscoveryEntry),
                            Matchers.<String[]> any());
        checkPromiseSuccess(promise2, "add failed");
    }

    @Test(timeout = 3000)
    public void addGlobalCapFails_NextAddShallAddGlobalAgain() throws InterruptedException {

        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.GLOBAL);

        String participantId = LocalCapabilitiesDirectoryTest.class.getName() + ".addLocalAndThanGlobalShallWork";
        String domain = "testDomain";
        final DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                                 domain,
                                                                 TestInterface.INTERFACE_NAME,
                                                                 participantId,
                                                                 providerQos,
                                                                 System.currentTimeMillis(),
                                                                 expiryDateMs,
                                                                 publicKeyId);
        globalDiscoveryEntry = new GlobalDiscoveryEntry(new Version(47, 11),
                                                        domain,
                                                        TestInterface.INTERFACE_NAME,
                                                        participantId,
                                                        providerQos,
                                                        System.currentTimeMillis(),
                                                        expiryDateMs,
                                                        publicKeyId,
                                                        globalAddressSerialized);

        doAnswer(createAddAnswerWithException()).when(globalCapabilitiesDirectoryClient)
                                                .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                     eq(globalDiscoveryEntry),
                                                     Matchers.<String[]> any());

        final boolean awaitGlobalRegistration = true;
        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);

        checkPromiseException(promise);
        verify(globalCapabilitiesDirectoryClient).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      eq(globalDiscoveryEntry),
                                                      Matchers.<String[]> any());
        verify(globalDiscoveryEntryCacheMock, never()).add(eq(globalDiscoveryEntry));

        reset(globalCapabilitiesDirectoryClient);

        doAnswer(createAddAnswerWithSuccess()).when(globalCapabilitiesDirectoryClient)
                                              .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                   eq(globalDiscoveryEntry),
                                                   Matchers.<String[]> any());

        promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);

        verify(globalCapabilitiesDirectoryClient).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      eq(globalDiscoveryEntry),
                                                      Matchers.<String[]> any());
        checkPromiseSuccess(promise, "add failed");

    }

    @Test
    public void addSameGbidTwiceInARow() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        String[] gbids = new String[]{ knownGbids[0] };

        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids);

        verify(localDiscoveryEntryStoreMock, times(1)).add(eq(discoveryEntry));
        verify(globalDiscoveryEntryCacheMock, times(1)).add(eq(globalDiscoveryEntry));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             eq(globalDiscoveryEntry),
                             Matchers.<String[]> any());
        checkPromiseSuccess(promise, "add failed");

        doReturn(true).when(localDiscoveryEntryStoreMock).hasDiscoveryEntry(any(DiscoveryEntry.class));
        doReturn(globalDiscoveryEntry).when(globalDiscoveryEntryCacheMock)
                                      .lookup(eq(globalDiscoveryEntry.getParticipantId()), anyLong());

        Promise<Add1Deferred> promise2 = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids);

        checkPromiseSuccess(promise2, "add failed");
        // entry is not added again
        verify(localDiscoveryEntryStoreMock, times(1)).add(eq(discoveryEntry));
        verify(globalDiscoveryEntryCacheMock, times(1)).add(eq(globalDiscoveryEntry));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             eq(globalDiscoveryEntry),
                             Matchers.<String[]> any());
    }

    @Test
    public void addDifferentGbidsAfterEachOther() throws InterruptedException {
        final boolean awaitGlobalRegistration = true;
        String[] gbids1 = new String[]{ knownGbids[0] };
        String[] expectedGbids1 = gbids1.clone();
        String[] gbids2 = new String[]{ knownGbids[1] };
        String[] expectedGbids2 = gbids2.clone();
        DiscoveryEntryWithMetaInfo expectedEntry = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                       discoveryEntry);

        doAnswer(createAddAnswerWithSuccess()).when(globalCapabilitiesDirectoryClient)
                                              .add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                   eq(globalDiscoveryEntry),
                                                   Matchers.<String[]> any());

        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration, gbids1);

        verify(localDiscoveryEntryStoreMock, times(1)).add(eq(discoveryEntry));
        verify(globalDiscoveryEntryCacheMock, times(1)).add(eq(globalDiscoveryEntry));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             eq(globalDiscoveryEntry),
                             eq(expectedGbids1));
        checkPromiseSuccess(promise, "add failed");

        doReturn(true).when(localDiscoveryEntryStoreMock).hasDiscoveryEntry(any(DiscoveryEntry.class));
        doReturn(globalDiscoveryEntry).when(globalDiscoveryEntryCacheMock)
                                      .lookup(eq(globalDiscoveryEntry.getParticipantId()), anyLong());

        Promise<Add1Deferred> promise2 = localCapabilitiesDirectory.add(discoveryEntry,
                                                                        awaitGlobalRegistration,
                                                                        gbids2);

        verify(localDiscoveryEntryStoreMock, times(1)).add(eq(discoveryEntry));
        verify(globalDiscoveryEntryCacheMock, times(2)).add(eq(globalDiscoveryEntry));
        verify(globalCapabilitiesDirectoryClient,
               times(1)).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                             eq(globalDiscoveryEntry),
                             eq(expectedGbids2));
        checkPromiseSuccess(promise2, "add failed");

        // provider is now registered for both GBIDs
        doReturn(Arrays.asList(globalDiscoveryEntry)).when(globalDiscoveryEntryCacheMock)
                                                     .lookup(any(String[].class), anyString(), anyLong());
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setCacheMaxAge(ONE_DAY_IN_MS);

        Promise<Lookup2Deferred> promiseLookup1 = localCapabilitiesDirectory.lookup(new String[]{
                discoveryEntry.getDomain() }, discoveryEntry.getInterfaceName(), discoveryQos, gbids1);
        Promise<Lookup2Deferred> promiseLookup2 = localCapabilitiesDirectory.lookup(new String[]{
                discoveryEntry.getDomain() }, discoveryEntry.getInterfaceName(), discoveryQos, gbids2);

        DiscoveryEntryWithMetaInfo[] result1 = (DiscoveryEntryWithMetaInfo[]) checkPromiseSuccess(promiseLookup1,
                                                                                                  "lookup failed")[0];
        assertEquals(1, result1.length);
        assertEquals(expectedEntry, result1[0]);
        DiscoveryEntryWithMetaInfo[] result2 = (DiscoveryEntryWithMetaInfo[]) checkPromiseSuccess(promiseLookup2,
                                                                                                  "lookup failed")[0];
        assertEquals(1, result2.length);
        assertEquals(expectedEntry, result2[0]);
    }

    @Test
    public void testAddAlreadyKnownWithLocalScopeDoesNothing() throws InterruptedException {
        discoveryEntry.getQos().setScope(ProviderScope.LOCAL);
        doReturn(true).when(localDiscoveryEntryStoreMock).hasDiscoveryEntry(discoveryEntry);

        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, false, knownGbids);

        checkPromiseSuccess(promise, "add failed");
        verify(localDiscoveryEntryStoreMock, never()).add(any(DiscoveryEntry.class));
        verify(globalDiscoveryEntryCacheMock, never()).lookup(anyString(), anyLong());
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient, never()).add(any(), any(), any());
    }

    @Test
    public void testAddWithGlobalAddressProviderThrowingException() throws InterruptedException {
        when(globalAddressProvider.get()).thenThrow(new JoynrRuntimeException());

        final boolean awaitGlobalRegistration = true;
        localCapabilitiesDirectory.add(globalDiscoveryEntry, awaitGlobalRegistration, knownGbids);

        Thread.sleep(200);
        verify(globalAddressProvider).registerGlobalAddressesReadyListener((TransportReadyListener) localCapabilitiesDirectory);
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient, times(0)).add(any(), any(), any());
    }

    @Test
    public void testAddToAll() throws InterruptedException {
        boolean awaitGlobalRegistration = true;
        Promise<AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                awaitGlobalRegistration);

        verify(localDiscoveryEntryStoreMock, times(1)).add(eq(discoveryEntry));
        verify(globalDiscoveryEntryCacheMock).add(eq(globalDiscoveryEntry));
        verify(globalCapabilitiesDirectoryClient).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      eq(globalDiscoveryEntry),
                                                      eq(knownGbids));
        checkPromiseSuccess(promise, "addToAll failed");
    }

    @Test
    public void testAddToAllLocal() throws InterruptedException {
        discoveryEntry.getQos().setScope(ProviderScope.LOCAL);
        boolean awaitGlobalRegistration = true;

        Promise<AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                awaitGlobalRegistration);

        checkPromiseSuccess(promise, "addToAll failed");
        verify(globalDiscoveryEntryCacheMock, never()).add(any(GlobalDiscoveryEntry.class));
        verify(globalCapabilitiesDirectoryClient,
               never()).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                            any(GlobalDiscoveryEntry.class),
                            Matchers.<String[]> any());
        verify(localDiscoveryEntryStoreMock, times(1)).add(eq(discoveryEntry));
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

    private static Answer<Future<Void>> createAddAnswerWithSuccess() {
        return new Answer<Future<Void>>() {

            @SuppressWarnings("unchecked")
            @Override
            public Future<Void> answer(InvocationOnMock invocation) throws Throwable {
                Future<Void> result = new Future<Void>();
                Object[] args = invocation.getArguments();
                ((Callback<Void>) args[0]).onSuccess(null);
                result.onSuccess(null);
                return result;
            }
        };
    }

    private static Answer<Future<Void>> createAddAnswerWithException() {
        return new Answer<Future<Void>>() {

            @SuppressWarnings("unchecked")
            @Override
            public Future<Void> answer(InvocationOnMock invocation) throws Throwable {
                Future<Void> result = new Future<Void>();
                Object[] args = invocation.getArguments();
                ((Callback<Void>) args[0]).onFailure(new JoynrRuntimeException("Simulating a JoynrRuntimeException on callback"));
                result.onSuccess(null);
                return result;
            }
        };
    }

    private static Answer<Future<Void>> createAddAnswerWithDiscoveryError(DiscoveryError error) {
        return new Answer<Future<Void>>() {

            @Override
            public Future<Void> answer(InvocationOnMock invocation) throws Throwable {
                Future<Void> result = new Future<Void>();
                Object[] args = invocation.getArguments();
                @SuppressWarnings("unchecked")
                CallbackWithModeledError<Void, DiscoveryError> callback = ((CallbackWithModeledError<Void, DiscoveryError>) args[0]);
                callback.onFailure(error);
                result.onSuccess(null);
                return result;
            }
        };
    }

    @Test(timeout = 3000)
    public void lookupWithScopeGlobalOnly() throws InterruptedException {
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
        localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        Promise<Lookup1Deferred> promise2 = localCapabilitiesDirectory.lookup(domains, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(2,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise2,
                                             0);

        // even deleting local cap entries shall have no effect, the global cap dir shall be invoked
        localCapabilitiesDirectory.remove(discoveryEntry);
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
                                                                globalAddressSerialized);
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

    private void verifyGcdLookupAndPromiseFulfillment(int gcdTimesCalled,
                                                      String[] domains,
                                                      String interfaceName,
                                                      long discoveryTimeout,
                                                      String[] gbids,
                                                      Promise<?> promise,
                                                      int numberOfReturnedValues) throws InterruptedException {

        Object[] values = checkPromiseSuccess(promise, "Unexpected rejection in global lookup");
        assertEquals(numberOfReturnedValues, ((DiscoveryEntryWithMetaInfo[]) values[0]).length);
        verify(globalCapabilitiesDirectoryClient,
               times(gcdTimesCalled)).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                             argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
                                             eq(interfaceName),
                                             eq(discoveryTimeout),
                                             eq(gbids));
    }

    @Test(timeout = 1000)
    public void lookupWithScopeLocalThenGlobal() throws InterruptedException {
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
                                                                globalAddressSerialized);
        caps.add(capInfo);
        doAnswer(createLookupAnswer(caps)).when(globalCapabilitiesDirectoryClient)
                                          .lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                  eq(domains),
                                                  eq(interfaceName1),
                                                  eq(discoveryQos.getDiscoveryTimeout()),
                                                  eq(new String[]{ knownGbids[0] }));
        promise = localCapabilitiesDirectory.lookup(new String[]{ domain1 }, interfaceName1, discoveryQos);
        verifyGcdLookupAndPromiseFulfillment(1,
                                             domains,
                                             interfaceName1,
                                             discoveryQos.getDiscoveryTimeout(),
                                             knownGbids,
                                             promise,
                                             1); // 1 local entry

        // without local entry, the global cap dir is called
        reset((Object) localDiscoveryEntryStoreMock);
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

    @Test(timeout = 1000)
    public void lookupByParticipantIdWithScopeLocalSync() throws InterruptedException {
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
                                                 eq(discoveryQos.getCacheMaxAge()))).thenReturn(discoveryEntry);

        Promise<Lookup3Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId1);

        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo retrievedCapabilityEntry = (DiscoveryEntryWithMetaInfo) values[0];
        assertEquals(expectedDiscoveryEntry, retrievedCapabilityEntry);
    }

    @Test(timeout = 3000)
    public void lookupWithScopeLocalAndGlobal() throws InterruptedException {
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
                                                                globalAddressSerialized);
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

    @Test(timeout = 2000)
    public void lookupLocalAndGlobalFiltersDuplicates() throws InterruptedException {
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
                                                                globalAddressSerialized);

        doReturn(Arrays.asList(capInfo)).when(globalDiscoveryEntryCacheMock).lookup(eq(domainsForLookup),
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

    @Test
    public void testLookupMultipleDomainsLocalOnly() throws InterruptedException {
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

    @Test
    public void testLookupMultipleDomainsGlobalOnly() throws InterruptedException {
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

    @Test
    public void testLookupMultipleDomainsGlobalOnlyAllCached() throws InterruptedException {
        String[] domains = new String[]{ "domain1", "domain2" };
        String interfaceName = "interface1";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        List<GlobalDiscoveryEntry> entries = new ArrayList<>();
        for (String domain : domains) {
            GlobalDiscoveryEntry entry = new GlobalDiscoveryEntry();
            entry.setParticipantId("participantIdFor-" + domain);
            entry.setDomain(domain);
            entries.add(entry);
            localCapabilitiesDirectory.add(entry, true, knownGbids);
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
    }

    @Test
    public void testLookupMultipleDomainsGlobalOnlyOneCached() throws InterruptedException {
        String[] domains = new String[]{ "domain1", "domain2" };
        String interfaceName = "interface1";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setCacheMaxAge(ONE_DAY_IN_MS);

        GlobalDiscoveryEntry entry = new GlobalDiscoveryEntry();
        entry.setParticipantId("participantId1");
        entry.setInterfaceName(interfaceName);
        entry.setDomain(domains[0]);
        entry.setAddress(globalAddressSerialized);
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

    @Test
    public void testLookupMultipleDomainsLocalThenGlobal() throws InterruptedException {
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
        globalEntry.setAddress(globalAddressSerialized);
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
                                                                                globalAddressSerialized);

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

    @Test
    public void testLookupByParticipantId_localEntry_DiscoveryEntryWithMetaInfoContainsExpectedIsLocalValue() throws Exception {
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
                                                 eq(discoveryQos.getCacheMaxAge()))).thenReturn(localEntry);

        Promise<Lookup3Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId);

        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo capturedLocalEntry = (DiscoveryEntryWithMetaInfo) values[0];
        assertEquals(localEntryWithMetaInfo, capturedLocalEntry);
    }

    @Test
    public void testLookupByParticipantId_cachedEntry_DiscoveryEntryWithMetaInfoContainsExpectedIsLocalValue() throws Exception {
        String participantId = "participantId";
        String interfaceName = "interfaceName";
        DiscoveryQos discoveryQos = new DiscoveryQos(Long.MAX_VALUE,
                                                     Long.MAX_VALUE,
                                                     DiscoveryScope.LOCAL_AND_GLOBAL,
                                                     false);

        // cached global DiscoveryEntry
        String globalDomain = "globalDomain";
        GlobalDiscoveryEntry cachedGlobalEntry = new GlobalDiscoveryEntry();
        cachedGlobalEntry.setDomain(globalDomain);
        cachedGlobalEntry.setInterfaceName(interfaceName);
        cachedGlobalEntry.setParticipantId(participantId);
        cachedGlobalEntry.setAddress(globalAddressSerialized);
        DiscoveryEntryWithMetaInfo cachedGlobalEntryWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                                       cachedGlobalEntry);
        when(globalDiscoveryEntryCacheMock.lookup(eq(participantId),
                                                  eq(discoveryQos.getCacheMaxAge()))).thenReturn(cachedGlobalEntry);
        Promise<Lookup3Deferred> lookupPromise = localCapabilitiesDirectory.lookup(participantId);

        Object[] values = checkPromiseSuccess(lookupPromise, "lookup failed");
        DiscoveryEntryWithMetaInfo capturedCachedGlobalEntry = (DiscoveryEntryWithMetaInfo) values[0];
        assertEquals(cachedGlobalEntryWithMetaInfo, capturedCachedGlobalEntry);
    }

    @Test
    public void testLookupByParticipantId_globalEntry_DiscoveryEntryWithMetaInfoContainsExpectedIsLocalValue() throws Exception {
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
                                                                                globalAddressSerialized);
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

    @Test
    public void testLookup_DiscoveryEntriesWithMetaInfoContainExpectedIsLocalValue() throws InterruptedException {
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
        cachedGlobalEntry.setAddress(globalAddressSerialized);
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
                                                                                globalAddressSerialized);
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

    private static void checkPromiseException(Promise<?> promise) throws InterruptedException {
        CountDownLatch countDownLatch = new CountDownLatch(1);
        promise.then(new PromiseListener() {

            @Override
            public void onRejection(JoynrException exception) {
                assertTrue(JoynrRuntimeException.class.isInstance(exception));
                countDownLatch.countDown();
            }

            @Override
            public void onFulfillment(Object... values) {
                fail("Unexpected fulfillment when expecting rejection.");
            }
        });
        assertTrue(countDownLatch.await(1, TimeUnit.SECONDS));
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
        assertTrue(countDownLatch.await(1, TimeUnit.SECONDS));
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

    @Test(timeout = 1000)
    public void removeCapabilities() throws InterruptedException {
        when(globalAddressProvider.get()).thenReturn(new MqttAddress("testgbid", "testtopic"));
        localCapabilitiesDirectory.add(discoveryEntry);
        localCapabilitiesDirectory.remove(discoveryEntry);

        verify(globalCapabilitiesDirectoryClient,
               timeout(1000)).remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                     eq(globalDiscoveryEntry.getParticipantId()),
                                     any(String[].class));
    }

    @Test
    public void testGCDRemoveNotCalledIfParticipantIsNotRegistered() throws InterruptedException {
        localCapabilitiesDirectory.remove(discoveryEntry);
        verify(globalCapabilitiesDirectoryClient,
               never()).remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                               any(String.class),
                               any(String[].class));
    }

    @Test
    public void callTouchPeriodically() throws InterruptedException {
        Runnable runnable = runnableCaptor.getValue();
        runnable.run();
        verify(globalCapabilitiesDirectoryClient).touch();
    }
}
