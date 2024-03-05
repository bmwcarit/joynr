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

import static io.joynr.runtime.SystemServicesSettings.PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.lenient;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.hamcrest.MockitoHamcrest.argThat;

import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.Properties;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import joynr.types.ProviderScope;
import org.junit.After;
import org.junit.Before;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.slf4j.Logger;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.util.Modules;
import com.google.inject.Module;
import com.google.inject.name.Names;
import com.google.inject.TypeLiteral;

import io.joynr.accesscontrol.AccessController;
import io.joynr.capabilities.LocalCapabilitiesDirectoryImpl.GcdTaskSequencer;
import io.joynr.capabilities.helpers.AnswerCreateHelper;
import io.joynr.capabilities.helpers.PromiseChecker;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.provider.Promise;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.runtime.ClusterControllerRuntimeModule;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.util.ObjectMapper;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.system.DiscoveryProvider;
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
import joynr.types.Version;

public abstract class AbstractLocalCapabilitiesDirectoryTest {

    protected static final int TEST_TIMEOUT = 10000;
    protected static final int DEFAULT_WAIT_TIME_MS = 5000; // value should be shorter than TEST_TIMEOUT
    protected static final long ONE_DAY_IN_MS = 24 * 60 * 60 * 1000;
    protected static final String TEST_URL = "mqtt://testUrl:42";
    protected static final long FRESHNESS_UPDATE_INTERVAL_MS = 300;
    protected static final long DEFAULT_EXPIRY_TIME_MS = 3628800000L;
    protected static final String INTERFACE_NAME = "interfaceName";
    protected static final String TEST_TOPIC = "testTopic";
    protected static final String PUBLIC_KEY_ID = "publicKeyId";
    protected static final String PROVISIONED_PUBLIC_KEY = "provisionedPublicKey";

    protected static final String DOMAIN_1 = "domain1";
    protected static final String DOMAIN_2 = "domain2";
    protected static final String DOMAIN_3 = "domain3";
    protected static final String PARTICIPANT_ID_1 = "participantId1";
    protected static final String PARTICIPANT_ID_2 = "participantId2";
    protected static final String PARTICIPANT_ID_3 = "participantId3";
    protected static final String MSG_LOOKUP_FAILED = "lookup failed";
    protected static final String MSG_ON_ADD_REJECT = "add failed";

    protected String[] knownGbids = { "testDEFAULTgbid", "testgbid2", "testGbid" };
    protected MqttAddress globalAddress1;
    protected String globalAddress1Serialized;
    protected MqttAddress globalAddress2;
    protected String globalAddress2Serialized;
    protected MqttAddress globalAddressWithoutGbid;
    protected String globalAddressWithoutGbidSerialized;
    protected Long expiryDateMs = System.currentTimeMillis() + ONE_DAY_IN_MS;
    protected GlobalDiscoveryEntry provisionedGlobalDiscoveryEntry;
    protected LocalCapabilitiesDirectory localCapabilitiesDirectory;
    protected boolean enableAccessControl = false;
    protected GcdTaskSequencer gcdTaskSequencer;
    protected GcdTaskSequencer gcdTaskSequencerSpy;
    protected Thread addRemoveWorker;
    protected DiscoveryEntry discoveryEntry;
    protected DiscoveryEntry expectedDiscoveryEntry;
    protected GlobalDiscoveryEntry globalDiscoveryEntry;
    protected GlobalDiscoveryEntry expectedGlobalDiscoveryEntry;
    protected AnswerCreateHelper answerCreateHelper;
    protected PromiseChecker promiseChecker;
    protected String[] domains;
    protected DiscoveryQos discoveryQos;
    @Mock
    protected GlobalCapabilitiesDirectoryClient globalCapabilitiesDirectoryClient;
    @Mock
    protected RoutingTable routingTable;
    @Mock
    protected CapabilitiesProvisioning capabilitiesProvisioning;
    @Mock
    protected DiscoveryEntryStore<DiscoveryEntry> localDiscoveryEntryStoreMock;
    @Mock
    protected DiscoveryEntryStore<GlobalDiscoveryEntry> globalDiscoveryEntryCacheMock;
    @Mock
    protected ExpiredDiscoveryEntryCacheCleaner expiredDiscoveryEntryCacheCleaner;
    @Mock
    protected ScheduledExecutorService capabilitiesFreshnessUpdateExecutor;
    @Mock
    protected ShutdownNotifier shutdownNotifier;
    @Mock
    protected AccessController accessController;
    @Captor
    protected ArgumentCaptor<GcdTaskSequencer> addRemoveQueueRunnableCaptor;
    @Captor
    protected ArgumentCaptor<CallbackWithModeledError<Void, DiscoveryError>> callbackCaptor;
    @Captor
    protected ArgumentCaptor<Runnable> runnableCaptor;

    @Before
    public void setUp() throws Exception {
        domains = new String[]{ DOMAIN_1 };
        discoveryQos = new DiscoveryQos(30000L, 1000L, DiscoveryScope.GLOBAL_ONLY, false);
        answerCreateHelper = new AnswerCreateHelper(getLogger(), TEST_TIMEOUT);
        promiseChecker = new PromiseChecker(DEFAULT_WAIT_TIME_MS);

        final ObjectMapper objectMapper = new ObjectMapper();
        globalAddress1 = new MqttAddress(knownGbids[0], TEST_TOPIC);
        globalAddress1Serialized = objectMapper.writeValueAsString(globalAddress1);
        globalAddress2 = new MqttAddress(knownGbids[1], TEST_TOPIC);
        globalAddress2Serialized = objectMapper.writeValueAsString(globalAddress2);
        globalAddressWithoutGbid = new MqttAddress("brokerUri", TEST_TOPIC);
        globalAddressWithoutGbidSerialized = objectMapper.writeValueAsString(globalAddressWithoutGbid);

        final Field objectMapperField = CapabilityUtils.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(CapabilityUtils.class, objectMapper);

        lenient().doAnswer(answerCreateHelper.createAnswerWithSuccess())
                 .when(globalCapabilitiesDirectoryClient)
                 .add(any(), any(GlobalDiscoveryEntry.class), anyLong(), any());

        lenient().doReturn(true)
                 .when(routingTable)
                 .put(any(String.class), any(Address.class), any(Boolean.class), anyLong());

        final String discoveryDirectoriesDomain = "io.joynr";
        final String capabilitiesDirectoryParticipantId = "capDir_participantId";
        final String capabilitiesDirectoryTopic = "dirTopic";
        final GlobalDiscoveryEntry globalCapabilitiesDirectoryDiscoveryEntry = CapabilityUtils.newGlobalDiscoveryEntry(build0_1Version(),
                                                                                                                       discoveryDirectoriesDomain,
                                                                                                                       GlobalCapabilitiesDirectory.INTERFACE_NAME,
                                                                                                                       capabilitiesDirectoryParticipantId,
                                                                                                                       new ProviderQos(),
                                                                                                                       System.currentTimeMillis(),
                                                                                                                       expiryDateMs,
                                                                                                                       PROVISIONED_PUBLIC_KEY,
                                                                                                                       new MqttAddress(TEST_URL,
                                                                                                                                       capabilitiesDirectoryTopic));

        provisionedGlobalDiscoveryEntry = CapabilityUtils.newGlobalDiscoveryEntry(build0_1Version(),
                                                                                  "provisioneddomain",
                                                                                  "provisionedInterface",
                                                                                  "provisionedParticipantId",
                                                                                  new ProviderQos(),
                                                                                  System.currentTimeMillis(),
                                                                                  expiryDateMs,
                                                                                  PROVISIONED_PUBLIC_KEY,
                                                                                  new MqttAddress("provisionedbrokeruri",
                                                                                                  "provisionedtopic"));

        when(capabilitiesProvisioning.getDiscoveryEntries()).thenReturn(new HashSet<>(Arrays.asList(globalCapabilitiesDirectoryDiscoveryEntry,
                                                                                                    provisionedGlobalDiscoveryEntry)));
        Module injectionModule = Modules.override(createBaseInjectionModule()).with(new AbstractModule() {
            @Override
            protected void configure() {
                bind(long.class).annotatedWith(Names.named(PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS))
                                .toInstance(FRESHNESS_UPDATE_INTERVAL_MS);
            }
        });
        localCapabilitiesDirectory = Guice.createInjector(injectionModule)
                                          .getInstance(LocalCapabilitiesDirectory.class);

        verify(capabilitiesFreshnessUpdateExecutor).schedule(addRemoveQueueRunnableCaptor.capture(),
                                                             anyLong(),
                                                             eq(TimeUnit.MILLISECONDS));
        gcdTaskSequencer = addRemoveQueueRunnableCaptor.getValue();
        gcdTaskSequencerSpy = Mockito.spy(gcdTaskSequencer);
        addRemoveWorker = new Thread(gcdTaskSequencer);
        addRemoveWorker.start();

        final ProviderQos providerQos = new ProviderQos();
        final CustomParameter[] parameterList = { new CustomParameter("key1", "value1"),
                new CustomParameter("key2", "value2") };
        providerQos.setCustomParameters(parameterList);

        final String participantId = "testParticipantId";
        final String domain = "domain";
        discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                            domain,
                                            INTERFACE_NAME,
                                            participantId,
                                            providerQos,
                                            System.currentTimeMillis(),
                                            expiryDateMs,
                                            PUBLIC_KEY_ID);
        expectedDiscoveryEntry = new DiscoveryEntry(discoveryEntry);
        globalDiscoveryEntry = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry, globalAddress1);
        expectedGlobalDiscoveryEntry = new GlobalDiscoveryEntry(globalDiscoveryEntry);

        lenient().when(localDiscoveryEntryStoreMock.lookup(anyString(), anyLong())).thenReturn(Optional.empty());
        lenient().when(globalDiscoveryEntryCacheMock.lookup(anyString(), anyLong())).thenReturn(Optional.empty());
    }

    protected Module createBaseInjectionModule() {
        return Modules.override(new JoynrPropertiesModule(new Properties())).with(new AbstractModule() {
            @Override
            protected void configure() {
                bind(LocalCapabilitiesDirectory.class).to(LocalCapabilitiesDirectoryImpl.class);
                bind(CapabilitiesProvisioning.class).toInstance(capabilitiesProvisioning);
                bind(Address.class).annotatedWith(Names.named(MessagingPropertyKeys.GLOBAL_ADDRESS))
                                   .toInstance(globalAddress1);
                bind(new TypeLiteral<DiscoveryEntryStore<DiscoveryEntry>>() {
                }).toInstance(localDiscoveryEntryStoreMock);
                bind(new TypeLiteral<DiscoveryEntryStore<GlobalDiscoveryEntry>>() {
                }).toInstance(globalDiscoveryEntryCacheMock);
                bind(RoutingTable.class).toInstance(routingTable);
                bind(GlobalCapabilitiesDirectoryClient.class).toInstance(globalCapabilitiesDirectoryClient);
                bind(ExpiredDiscoveryEntryCacheCleaner.class).toInstance(expiredDiscoveryEntryCacheCleaner);
                bind(ScheduledExecutorService.class).annotatedWith(Names.named(LocalCapabilitiesDirectory.JOYNR_SCHEDULER_CAPABILITIES_FRESHNESS))
                                                    .toInstance(capabilitiesFreshnessUpdateExecutor);
                bind(ShutdownNotifier.class).toInstance(shutdownNotifier);
                bind(String[].class).annotatedWith(Names.named(MessagingPropertyKeys.GBID_ARRAY))
                                    .toInstance(knownGbids);
                bind(long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS))
                                .toInstance(DEFAULT_EXPIRY_TIME_MS);
                bind(AccessController.class).toInstance(accessController);
                bind(boolean.class).annotatedWith(Names.named(ClusterControllerRuntimeModule.PROPERTY_ACCESSCONTROL_ENABLE))
                                   .toInstance(enableAccessControl);
            }
        });
    }

    @After
    public void tearDown() throws Exception {
        gcdTaskSequencer.stop();
        addRemoveWorker.join();
    }

    protected Version build0_0Version() {
        return new Version(0, 0);
    }

    protected Version build0_1Version() {
        return new Version(0, 1);
    }

    protected Version build47_1Version() {
        return new Version(47, 1);
    }

    protected Version build47_11Version() {
        return new Version(47, 11);
    }

    protected void checkRemainingTtl(final ArgumentCaptor<Long> remainingTtlCaptor) {
        final long remainingTtl = remainingTtlCaptor.getValue();
        assertTrue(remainingTtl <= MessagingQos.DEFAULT_TTL);
        assertTrue(remainingTtl > (MessagingQos.DEFAULT_TTL / 2.0));
    }

    protected <T> void setFieldValue(final Object object,
                                     final String fieldName,
                                     final T value) throws IllegalArgumentException, IllegalAccessException {
        final Field objectField = getPrivateField(object.getClass(), fieldName);
        assertNotNull(objectField);
        objectField.setAccessible(true);
        objectField.set(object, value);
    }

    protected Field getPrivateField(final Class<?> privateClass, final String fieldName) {
        try {
            return privateClass.getDeclaredField(fieldName);
        } catch (Exception e) {
            fail(e.getMessage());
        }
        return null;
    }

    protected Object[] verifyGcdLookupAndPromiseFulfillment(final int gcdTimesCalled,
                                                            final String[] domains,
                                                            final long discoveryTimeout,
                                                            final String[] gbids,
                                                            final Promise<?> promise,
                                                            final int numberOfReturnedValues) throws InterruptedException {
        verify(globalCapabilitiesDirectoryClient,
               times(gcdTimesCalled)).lookup(any(),
                                             argThat(org.hamcrest.Matchers.arrayContainingInAnyOrder(domains)),
                                             eq(INTERFACE_NAME),
                                             eq(discoveryTimeout),
                                             eq(gbids));

        final Object[] values = promiseChecker.checkPromiseSuccess(promise, "Unexpected rejection in global lookup");
        assertEquals(numberOfReturnedValues, ((DiscoveryEntryWithMetaInfo[]) values[0]).length);
        return values;
    }

    protected void mockGcdLookup(final List<GlobalDiscoveryEntry> caps, final String[] knownGbids) {
        doAnswer(answerCreateHelper.createLookupAnswer(caps)).when(globalCapabilitiesDirectoryClient)
                                                             .lookup(any(),
                                                                     any(),
                                                                     anyString(),
                                                                     anyLong(),
                                                                     eq(knownGbids));
    }

    protected void mockGcdLookup(final List<GlobalDiscoveryEntry> caps,
                                 final String[] domains,
                                 final DiscoveryQos discoveryQos,
                                 final String[] knownGbids) {
        mockGcdLookup(caps, domains, discoveryQos.getDiscoveryTimeout(), knownGbids);
    }

    protected void mockGcdLookup(final List<GlobalDiscoveryEntry> caps,
                                 final String[] domains,
                                 final long discoveryTimeout,
                                 final String[] knownGbids) {
        doAnswer(answerCreateHelper.createLookupAnswer(caps)).when(globalCapabilitiesDirectoryClient)
                                                             .lookup(any(),
                                                                     eq(domains),
                                                                     eq(INTERFACE_NAME),
                                                                     eq(discoveryTimeout),
                                                                     eq(knownGbids));
    }

    protected void mockGcdLookup(final List<GlobalDiscoveryEntry> caps,
                                 final String[] domains,
                                 final String[] knownGbids) {
        doAnswer(answerCreateHelper.createLookupAnswer(caps)).when(globalCapabilitiesDirectoryClient)
                                                             .lookup(any(),
                                                                     eq(domains),
                                                                     eq(INTERFACE_NAME),
                                                                     anyLong(),
                                                                     eq(knownGbids));
    }

    protected void mockGcdLookup(final GlobalDiscoveryEntry caps,
                                 final String participantId,
                                 final long timeout,
                                 final String[] knownGbids) {
        doAnswer(answerCreateHelper.createLookupAnswer(caps)).when(globalCapabilitiesDirectoryClient)
                                                             .lookup(any(),
                                                                     eq(participantId),
                                                                     eq(timeout),
                                                                     eq(knownGbids));
    }

    protected void mockGcdLookup(final List<GlobalDiscoveryEntry> caps) {
        doAnswer(answerCreateHelper.createLookupAnswer(caps)).when(globalCapabilitiesDirectoryClient)
                                                             .lookup(any(), any(), anyString(), anyLong(), any());

    }

    protected void mockGcdLookup(final GlobalDiscoveryEntry entry) {
        doAnswer(answerCreateHelper.createLookupAnswer(entry)).when(globalCapabilitiesDirectoryClient)
                                                              .lookup(any(), anyString(), anyLong(), any());

    }

    protected void mockGcdLookupException(final JoynrRuntimeException exception, final String[] domains) {
        doAnswer(answerCreateHelper.createVoidAnswerWithException(exception)).when(globalCapabilitiesDirectoryClient)
                                                                             .lookup(any(),
                                                                                     eq(domains),
                                                                                     eq(INTERFACE_NAME),
                                                                                     anyLong(),
                                                                                     any());
    }

    @SuppressWarnings("SameParameterValue")
    protected void mockGcdLookupException(final JoynrRuntimeException exception, final String participantId) {
        doAnswer(answerCreateHelper.createVoidAnswerWithException(exception)).when(globalCapabilitiesDirectoryClient)
                                                                             .lookup(any(),
                                                                                     eq(participantId),
                                                                                     anyLong(),
                                                                                     any());
    }

    protected void mockGcdLookupError(final DiscoveryError discoveryError, final String[] domains) {
        doAnswer(answerCreateHelper.createVoidAnswerWithDiscoveryError(discoveryError)).when(globalCapabilitiesDirectoryClient)
                                                                                       .lookup(any(),
                                                                                               eq(domains),
                                                                                               eq(INTERFACE_NAME),
                                                                                               anyLong(),
                                                                                               any());
    }

    @SuppressWarnings("SameParameterValue")
    protected void mockGcdLookupError(final DiscoveryError discoveryError, final String participantId) {
        doAnswer(answerCreateHelper.createVoidAnswerWithDiscoveryError(discoveryError)).when(globalCapabilitiesDirectoryClient)
                                                                                       .lookup(any(),
                                                                                               eq(participantId),
                                                                                               anyLong(),
                                                                                               any());
    }

    protected GlobalDiscoveryEntry createGlobalDiscoveryEntry(final String globalParticipantId,
                                                              final ProviderQos providerQos) {
        return new GlobalDiscoveryEntry(build47_11Version(),
                                        DOMAIN_1,
                                        INTERFACE_NAME,
                                        globalParticipantId,
                                        providerQos,
                                        System.currentTimeMillis(),
                                        expiryDateMs,
                                        PUBLIC_KEY_ID,
                                        globalAddress1Serialized);
    }

    protected DiscoveryEntry createDiscoveryEntry(final String participantId, final ProviderQos providerQos) {
        return new DiscoveryEntry(build47_1Version(),
                                  DOMAIN_1,
                                  INTERFACE_NAME,
                                  participantId,
                                  providerQos,
                                  System.currentTimeMillis(),
                                  expiryDateMs,
                                  PUBLIC_KEY_ID);
    }

    @SuppressWarnings("SameParameterValue")
    protected Promise<DiscoveryProvider.Add1Deferred> addEntry(final DiscoveryEntry entry,
                                                               final boolean awaitGlobalRegistration,
                                                               final String... knownGbids) {
        return localCapabilitiesDirectory.add(entry, awaitGlobalRegistration, knownGbids);
    }

    protected void sleep1ms() {
        sleep(1);
    }

    protected void sleep(final long ms) {
        try {
            Thread.sleep(ms);
        } catch (final InterruptedException exception) {
            fail("Unexpected exception during waiting: " + exception.getMessage());
        }
    }

    protected void setProviderQos(DiscoveryEntry discoveryEntry, ProviderScope providerScope) {
        ProviderQos providerQos = discoveryEntry.getQos();
        providerQos.setScope(providerScope);
        discoveryEntry.setQos(providerQos);
    }

    protected abstract Logger getLogger();
}
