/*
 * #%L
 * %%
 * Copyright (C) 2022 BMW Car IT GmbH
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

import static io.joynr.capabilities.LocalCapabilitiesDirectoryTest.checkPromiseException;
import static io.joynr.capabilities.LocalCapabilitiesDirectoryTest.checkPromiseSuccess;
import static io.joynr.capabilities.LocalCapabilitiesDirectoryTest.createAnswerWithSuccess;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.Optional;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatchers;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.accesscontrol.AccessController;
import io.joynr.capabilities.LocalCapabilitiesDirectoryImpl.GcdTaskSequencer;
import io.joynr.capabilities.LocalCapabilitiesDirectoryTest.DiscoveryEntryWithUpdatedLastSeenDateMsMatcher;
import io.joynr.capabilities.LocalCapabilitiesDirectoryTest.GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher;
import io.joynr.dispatching.Dispatcher;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.provider.Promise;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.runtime.GlobalAddressProvider;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.util.ObjectMapper;
import joynr.exceptions.ProviderRuntimeException;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.infrastructure.DacTypes.TrustLevel;
import joynr.system.DiscoveryProvider.Add1Deferred;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.CustomParameter;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

@RunWith(MockitoJUnitRunner.class)
public class LocalCapabilitiesDirectoryACLTest {

    private static final int TEST_TIMEOUT = 10000;
    private static final String INTERFACE_NAME = "interfaceName";
    private static final String TEST_URL = "mqtt://testUrl:42";
    private static final long ONE_DAY_IN_MS = 1 * 24 * 60 * 60 * 1000;
    private static final long freshnessUpdateIntervalMs = 30000;
    private static final long DEFAULT_EXPIRY_TIME_MS = 3628800000l;

    private LocalCapabilitiesDirectory localCapabilitiesDirectory;

    private String[] knownGbids = { "testDEFAULTgbid", "testgbid2", "testGbid" };
    private Long expiryDateMs = System.currentTimeMillis() + ONE_DAY_IN_MS;
    private String publicKeyId = "publicKeyId";

    private MqttAddress globalAddress1;

    private DiscoveryEntry discoveryEntry;
    private DiscoveryEntry expectedDiscoveryEntry;
    private GlobalDiscoveryEntry globalDiscoveryEntry;
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

    private GcdTaskSequencer gcdTaskSequencer;
    private Thread addRemoveWorker;

    private boolean enableAccessControl = true;

    @Before
    public void setUp() throws Exception {
        ObjectMapper objectMapper = new ObjectMapper();
        globalAddress1 = new MqttAddress(knownGbids[0], "testTopic");

        Field objectMapperField = CapabilityUtils.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(CapabilityUtils.class, objectMapper);

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

        Collection<GlobalDiscoveryEntry> provisionedEntries = new HashSet<GlobalDiscoveryEntry>(Arrays.asList(globalCapabilitiesDirectoryDiscoveryEntry,
                                                                                                              provisionedGlobalDiscoveryEntry));
        when(capabilitiesProvisioning.getDiscoveryEntries()).thenReturn(provisionedEntries);
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
        verify(globalDiscoveryEntryCacheMock).add(eq(provisionedEntries));

        verify(capabilitiesFreshnessUpdateExecutor).schedule(addRemoveQueueRunnableCaptor.capture(),
                                                             anyLong(),
                                                             eq(TimeUnit.MILLISECONDS));
        gcdTaskSequencer = addRemoveQueueRunnableCaptor.getValue();
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

        when(globalAddressProvider.get()).thenReturn(globalAddress1);
        when(globalDiscoveryEntryCacheMock.lookup(anyString(), anyLong())).thenReturn(Optional.empty());
    }

    @After
    public void tearDown() throws Exception {
        gcdTaskSequencer.stop();
        addRemoveWorker.join();
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddWithoutProviderPermissionIsProperlyRejected() throws InterruptedException {
        doReturn(false).when(accessController).hasProviderPermission(anyString(),
                                                                     any(TrustLevel.class),
                                                                     anyString(),
                                                                     anyString(),
                                                                     anyString());

        String[] gbids = new String[]{ knownGbids[0] };
        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, true, gbids);
        checkPromiseException(promise,
                              new ProviderRuntimeException("Provider does not have permissions to register interface interfaceName on domain domain with participantId testParticipantId"));

        verify(accessController).hasProviderPermission(eq("creatorUserId"),
                                                       eq(TrustLevel.HIGH),
                                                       eq(expectedDiscoveryEntry.getDomain()),
                                                       eq(expectedDiscoveryEntry.getInterfaceName()),
                                                       eq(expectedDiscoveryEntry.getParticipantId()));
        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient,
                                 localDiscoveryEntryStoreMock,
                                 globalDiscoveryEntryCacheMock,
                                 accessController);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddWithProviderPermission() throws InterruptedException {
        doReturn(true).when(accessController)
                      .hasProviderPermission(anyString(), any(TrustLevel.class), anyString(), anyString(), anyString());
        doAnswer(createAnswerWithSuccess()).when(globalCapabilitiesDirectoryClient)
                                           .add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                any(GlobalDiscoveryEntry.class),
                                                anyLong(),
                                                ArgumentMatchers.<String[]> any());

        String[] gbids = new String[]{ knownGbids[0] };
        String[] expectedGbids = gbids.clone();
        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, true, gbids);
        checkPromiseSuccess(promise, "add failed");

        verify(globalCapabilitiesDirectoryClient).add(ArgumentMatchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                      anyLong(),
                                                      eq(expectedGbids));
        DiscoveryEntryWithUpdatedLastSeenDateMsMatcher matcher = new DiscoveryEntryWithUpdatedLastSeenDateMsMatcher(expectedDiscoveryEntry);
        verify(localDiscoveryEntryStoreMock).hasDiscoveryEntry(argThat(matcher));
        verify(localDiscoveryEntryStoreMock).add(argThat(matcher));
        verify(globalDiscoveryEntryCacheMock).lookup(eq(expectedDiscoveryEntry.getParticipantId()), eq(Long.MAX_VALUE));
        verify(accessController).hasProviderPermission(eq("creatorUserId"),
                                                       eq(TrustLevel.HIGH),
                                                       eq(expectedDiscoveryEntry.getDomain()),
                                                       eq(expectedDiscoveryEntry.getInterfaceName()),
                                                       eq(expectedDiscoveryEntry.getParticipantId()));
        verifyNoMoreInteractions(globalCapabilitiesDirectoryClient,
                                 localDiscoveryEntryStoreMock,
                                 globalDiscoveryEntryCacheMock,
                                 accessController);
    }

}
