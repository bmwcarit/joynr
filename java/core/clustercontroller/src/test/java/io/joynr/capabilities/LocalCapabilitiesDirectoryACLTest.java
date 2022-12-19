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
import java.util.Collection;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.capabilities.helpers.AnswerCreateHelper;
import io.joynr.capabilities.helpers.DiscoveryEntryWithUpdatedLastSeenDateMsMatcher;
import io.joynr.capabilities.helpers.GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher;
import io.joynr.capabilities.helpers.PromiseChecker;
import io.joynr.provider.Promise;
import io.joynr.util.ObjectMapper;
import joynr.exceptions.ProviderRuntimeException;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.infrastructure.DacTypes.TrustLevel;
import joynr.system.DiscoveryProvider.Add1Deferred;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.CustomParameter;
import joynr.types.DiscoveryEntry;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@RunWith(MockitoJUnitRunner.class)
public class LocalCapabilitiesDirectoryACLTest extends AbstractLocalCapabilitiesDirectoryTest {

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectoryACLTest.class);

    private static final long freshnessUpdateIntervalMs = 30000;

    @Before
    public void setUp() throws Exception {
        enableAccessControl = true;
        promiseChecker = new PromiseChecker(DEFAULT_WAIT_TIME_MS);
        answerCreateHelper = new AnswerCreateHelper(logger, TEST_TIMEOUT);

        final ObjectMapper objectMapper = new ObjectMapper();
        globalAddress1 = new MqttAddress(knownGbids[0], "testTopic");

        final Field objectMapperField = CapabilityUtils.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(CapabilityUtils.class, objectMapper);

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

        final Collection<GlobalDiscoveryEntry> provisionedEntries = Set.of(globalCapabilitiesDirectoryDiscoveryEntry,
                                                                           provisionedGlobalDiscoveryEntry);
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

        final ProviderQos providerQos = new ProviderQos();
        final CustomParameter[] parameterList = { new CustomParameter("key1", "value1"),
                new CustomParameter("key2", "value2") };
        providerQos.setCustomParameters(parameterList);

        final String participantId = "testParticipantId";
        final String domain = "domain";
        discoveryEntry = new DiscoveryEntry(build47_11Version(),
                                            domain,
                                            INTERFACE_NAME,
                                            participantId,
                                            providerQos,
                                            System.currentTimeMillis(),
                                            expiryDateMs,
                                            PUBLIC_KEY_ID);
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

        final String[] gbids = new String[]{ knownGbids[0] };
        final Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, true, gbids);
        promiseChecker.checkPromiseException(promise,
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
        doAnswer(answerCreateHelper.createAnswerWithSuccess()).when(globalCapabilitiesDirectoryClient)
                                                              .add(any(),
                                                                   any(GlobalDiscoveryEntry.class),
                                                                   anyLong(),
                                                                   any());

        String[] gbids = new String[]{ knownGbids[0] };
        String[] expectedGbids = gbids.clone();
        Promise<Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry, true, gbids);
        promiseChecker.checkPromiseSuccess(promise, "add failed");

        verify(globalCapabilitiesDirectoryClient).add(any(),
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

    @Override
    protected Logger getLogger() {
        return logger;
    }
}
