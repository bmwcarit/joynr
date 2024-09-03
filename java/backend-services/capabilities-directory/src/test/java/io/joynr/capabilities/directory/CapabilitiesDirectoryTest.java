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
package io.joynr.capabilities.directory;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Optional;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import io.joynr.capabilities.CapabilityUtils;
import io.joynr.capabilities.GlobalDiscoveryEntryPersisted;
import io.joynr.capabilities.GlobalDiscoveryEntryStore;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;
import io.joynr.util.ObjectMapper;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProvider.Add1Deferred;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProvider.Lookup1Deferred;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProvider.Lookup2Deferred;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProvider.Lookup3Deferred;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProvider.Lookup4Deferred;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProvider.Remove1Deferred;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

public class CapabilitiesDirectoryTest {
    private static final String GCD_GBID = "ownGbid";
    private static final String VALID_GBIDS_STRING = "testGbid1, testGbid2, " + GCD_GBID;
    private String[] validGbids;

    private static final String PARTICIPANT_ID = "testParticipantId";
    private static final String DOMAIN = "example.com";
    private static final String[] DOMAINS = new String[]{ DOMAIN };
    private static final String INTERFACE_NAME = "interfaceName";
    private static final String TOPIC_NAME = "my/topic";
    private GlobalDiscoveryEntry testGlobalDiscoveryEntry;
    private GlobalDiscoveryEntry expectedGlobalDiscoveryEntry;

    CapabilitiesDirectoryImpl subject;

    @Mock
    private GlobalDiscoveryEntryStore<GlobalDiscoveryEntryPersisted> discoveryEntryStoreMock;

    @Captor
    private ArgumentCaptor<GlobalDiscoveryEntryPersisted> gdepCaptor;

    @Before
    public void setUp() throws NoSuchFieldException, IllegalAccessException {
        MockitoAnnotations.initMocks(this);
        validGbids = Arrays.asList(VALID_GBIDS_STRING.split(","))
                           .stream()
                           .map(gbid -> gbid.trim())
                           .toArray(String[]::new);

        subject = new CapabilitiesDirectoryImpl(discoveryEntryStoreMock, GCD_GBID, VALID_GBIDS_STRING);

        Field field = CapabilityUtils.class.getDeclaredField("objectMapper");
        field.setAccessible(true);
        field.set(CapabilityUtils.class, new ObjectMapper());

        testGlobalDiscoveryEntry = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0, 1),
                                                                           DOMAIN,
                                                                           INTERFACE_NAME,
                                                                           PARTICIPANT_ID,
                                                                           new ProviderQos(),
                                                                           System.currentTimeMillis(),
                                                                           System.currentTimeMillis() + 1000L,
                                                                           "public key ID",
                                                                           new MqttAddress("tcp://mqttbroker:1883",
                                                                                           TOPIC_NAME));

        expectedGlobalDiscoveryEntry = new GlobalDiscoveryEntry(testGlobalDiscoveryEntry);
    }

    private static void checkDiscoveryEntry(GlobalDiscoveryEntry expected, GlobalDiscoveryEntry actual) {
        assertNotNull(actual);
        assertEquals(expected.getParticipantId(), actual.getParticipantId());
        assertEquals(expected.getInterfaceName(), actual.getInterfaceName());
        assertEquals(expected.getDomain(), actual.getDomain());
        assertEquals(expected.getExpiryDateMs(), actual.getExpiryDateMs());
        assertEquals(expected.getLastSeenDateMs(), actual.getLastSeenDateMs());
        assertEquals(expected.getProviderVersion(), actual.getProviderVersion());
        assertEquals(expected.getQos(), actual.getQos());
        assertEquals(expected.getAddress(), actual.getAddress());
    }

    private static void checkDiscoveryEntryPersisted(GlobalDiscoveryEntry expected,
                                                     GlobalDiscoveryEntryPersisted actual) {
        checkDiscoveryEntry(expected, actual);
        assertEquals(TOPIC_NAME, actual.getClusterControllerId());
    }

    private static Object[] checkPromiseSuccess(Promise<?> promise) throws InterruptedException {
        ArrayList<Object> result = new ArrayList<>();
        CountDownLatch countDownLatch = new CountDownLatch(1);
        promise.then(new PromiseListener() {

            @Override
            public void onRejection(JoynrException exception) {
                fail("unexpected rejection: " + exception);
            }

            @Override
            public void onFulfillment(Object... values) {
                result.addAll(Arrays.asList(values));
                countDownLatch.countDown();
            }
        });
        assertTrue("promise success timeout", countDownLatch.await(1000, TimeUnit.MILLISECONDS));
        return result.toArray(new Object[result.size()]);
    }

    private static void checkPromiseError(Promise<?> promise,
                                          DiscoveryError expectedError) throws InterruptedException {
        CountDownLatch countDownLatch = new CountDownLatch(1);
        promise.then(new PromiseListener() {

            @Override
            public void onRejection(JoynrException exception) {
                assertTrue("Exception is not ApplicationException: " + exception,
                           exception instanceof ApplicationException);
                DiscoveryError error = ((ApplicationException) exception).getError();
                assertEquals(expectedError, error);
                countDownLatch.countDown();
            }

            @Override
            public void onFulfillment(Object... values) {
                fail("expected rejection");
            }
        });
        assertTrue("promise error timeout", countDownLatch.await(1000, TimeUnit.MILLISECONDS));
    }

    private static void checkPromiseException(Promise<?> promise,
                                              Class<? extends Exception> expectedClass,
                                              String... expectedSubstrings) throws InterruptedException {
        CountDownLatch countDownLatch = new CountDownLatch(1);
        promise.then(new PromiseListener() {

            @Override
            public void onRejection(JoynrException exception) {
                assertTrue("Exception is not " + expectedClass + ": " + exception, expectedClass.isInstance(exception));
                for (String substring : expectedSubstrings) {
                    assertTrue("Exception does not contain " + substring + ": " + exception,
                               ((JoynrRuntimeException) exception).getMessage().contains(substring));
                }
                countDownLatch.countDown();
            }

            @Override
            public void onFulfillment(Object... values) {
                fail("expected rejection");
            }
        });
        assertTrue("promise exception timeout", countDownLatch.await(1000, TimeUnit.MILLISECONDS));
    }

    @Test
    public void add_callsStore() throws InterruptedException {
        Promise<DeferredVoid> promise = subject.add(testGlobalDiscoveryEntry);
        verify(discoveryEntryStoreMock).add(gdepCaptor.capture(), eq(new String[]{ GCD_GBID }));
        checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry, gdepCaptor.getValue());
        checkPromiseSuccess(promise);
    }

    @Test
    public void addWithGbids_callsStore() throws InterruptedException {
        String[] selectedGbids = new String[]{ validGbids[2], validGbids[0] };
        String[] expectedGbids = selectedGbids.clone();
        Promise<Add1Deferred> promise = subject.add(testGlobalDiscoveryEntry, selectedGbids);
        verify(discoveryEntryStoreMock).add(gdepCaptor.capture(), eq(expectedGbids));
        checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry, gdepCaptor.getValue());
        checkPromiseSuccess(promise);
    }

    @Test
    public void addWithGbids_singleEmptyGbid_callsStoreWithGcdGbid() throws InterruptedException {
        String[] selectedGbids = new String[]{ "" };
        String[] expectedGbids = new String[]{ GCD_GBID };
        Promise<Add1Deferred> promise = subject.add(testGlobalDiscoveryEntry, selectedGbids);
        verify(discoveryEntryStoreMock).add(gdepCaptor.capture(), eq(expectedGbids));
        checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry, gdepCaptor.getValue());
        checkPromiseSuccess(promise);
    }

    @Test
    public void addWithGbids_multipleGbidsWithEmptyGbid_callsStoreWithEmptyGbidReplaced() throws InterruptedException {
        String[] selectedGbids = new String[]{ validGbids[1], "" };
        String[] expectedGbids = new String[]{ validGbids[1], GCD_GBID };
        Promise<Add1Deferred> promise = subject.add(testGlobalDiscoveryEntry, selectedGbids);
        verify(discoveryEntryStoreMock).add(gdepCaptor.capture(), eq(expectedGbids));
        checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry, gdepCaptor.getValue());
        checkPromiseSuccess(promise);
    }

    @Test
    public void addWithGbids_emptyAndGcdGbid_callsStoreWithEmptyGbidReplaced() throws InterruptedException {
        String[] selectedGbids = new String[]{ "", GCD_GBID };
        String[] expectedGbids = new String[]{ GCD_GBID, GCD_GBID };
        Promise<Add1Deferred> promise = subject.add(testGlobalDiscoveryEntry, selectedGbids);
        verify(discoveryEntryStoreMock).add(gdepCaptor.capture(), eq(expectedGbids));
        checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry, gdepCaptor.getValue());
        checkPromiseSuccess(promise);
    }

    private void testAddWithGbids_discoveryError(String[] gbids,
                                                 DiscoveryError expectedError) throws InterruptedException {
        Promise<Add1Deferred> promise = subject.add(testGlobalDiscoveryEntry, gbids);
        checkPromiseError(promise, expectedError);
        verify(discoveryEntryStoreMock, times(0)).add(any(GlobalDiscoveryEntryPersisted.class), any(String[].class));
    }

    @Test
    public void addWithGbids_unknownGbid() throws InterruptedException {
        final String[] invalidGbidsArray = { "unknownGbid" };
        testAddWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.UNKNOWN_GBID);
    }

    @Test
    public void addWithGbids_invalidGbid_nullGbid() throws InterruptedException {
        final String[] invalidGbidsArray = { null };
        testAddWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void addWithGbids_invalidGbid_nullGbidsArray() throws InterruptedException {
        final String[] invalidGbidsArray = null;
        testAddWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void addWithGbids_invalidGbid_emptyGbidsArray() throws InterruptedException {
        final String[] invalidGbidsArray = new String[0];
        testAddWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void addWithGbids_invalidGbid_duplicateGbid() throws InterruptedException {
        final String[] invalidGbidsArray = { validGbids[2], validGbids[1], validGbids[2] };
        testAddWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void addWithGbids_internalError() throws InterruptedException {
        doThrow(new RuntimeException("error in DiscoveryEntryStore")).when(discoveryEntryStoreMock)
                                                                     .add(any(GlobalDiscoveryEntryPersisted.class),
                                                                          any(String[].class));
        Promise<Add1Deferred> promise = subject.add(testGlobalDiscoveryEntry, validGbids.clone());
        verify(discoveryEntryStoreMock).add(gdepCaptor.capture(), eq(validGbids));
        checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry, gdepCaptor.getValue());
        checkPromiseError(promise, DiscoveryError.INTERNAL_ERROR);
    }

    @Test
    public void add_internalError() throws InterruptedException {
        doThrow(new RuntimeException("error in DiscoveryEntryStore")).when(discoveryEntryStoreMock)
                                                                     .add(any(GlobalDiscoveryEntryPersisted.class),
                                                                          any(String[].class));
        Promise<DeferredVoid> promise = subject.add(testGlobalDiscoveryEntry);
        verify(discoveryEntryStoreMock).add(gdepCaptor.capture(), eq(new String[]{ GCD_GBID }));
        checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry, gdepCaptor.getValue());
        checkPromiseException(promise,
                              ProviderRuntimeException.class,
                              DiscoveryError.INTERNAL_ERROR.name(),
                              PARTICIPANT_ID);
    }

    @Test
    public void remove_callsStore() throws InterruptedException {
        Promise<DeferredVoid> promise = subject.remove(PARTICIPANT_ID);
        verify(discoveryEntryStoreMock).remove(eq(PARTICIPANT_ID), eq(new String[]{ GCD_GBID }));
        checkPromiseSuccess(promise);
    }

    @Test
    public void removeWithGbids_multipleGbids_callsStore() throws InterruptedException {
        String[] selectedGbids = new String[]{ validGbids[2], validGbids[1] };
        String[] expectedGbids = selectedGbids.clone();
        doReturn(1).when(discoveryEntryStoreMock).remove(anyString(), any(String[].class));
        Promise<Remove1Deferred> promise = subject.remove(PARTICIPANT_ID, selectedGbids);
        verify(discoveryEntryStoreMock).remove(eq(PARTICIPANT_ID), eq(expectedGbids));
        checkPromiseSuccess(promise);
    }

    @Test
    public void removeWithGbids_emptyGbid_callsStoreWithGcdGbid() throws InterruptedException {
        String[] selectedGbids = new String[]{ "" };
        String[] expectedGbids = new String[]{ GCD_GBID };
        doReturn(1).when(discoveryEntryStoreMock).remove(anyString(), any(String[].class));
        Promise<Remove1Deferred> promise = subject.remove(PARTICIPANT_ID, selectedGbids);
        verify(discoveryEntryStoreMock).remove(eq(PARTICIPANT_ID), eq(expectedGbids));
        checkPromiseSuccess(promise);
    }

    @Test
    public void removeWithGbids_multipleGbidsWithEmptyGbid_callsStoreWithEmptyGbidReplaced() throws InterruptedException {
        String[] selectedGbids = new String[]{ validGbids[2], "" };
        String[] expectedGbids = new String[]{ validGbids[2], GCD_GBID };
        doReturn(1).when(discoveryEntryStoreMock).remove(anyString(), any(String[].class));
        Promise<Remove1Deferred> promise = subject.remove(PARTICIPANT_ID, selectedGbids);
        verify(discoveryEntryStoreMock).remove(eq(PARTICIPANT_ID), eq(expectedGbids));
        checkPromiseSuccess(promise);
    }

    private void testRemoveWithGbids_discoveryError(String[] gbids,
                                                    DiscoveryError expectedError) throws InterruptedException {
        Promise<Remove1Deferred> promise = subject.remove(PARTICIPANT_ID, gbids);
        checkPromiseError(promise, expectedError);
        verify(discoveryEntryStoreMock, times(0)).remove(anyString(), any(String[].class));
    }

    @Test
    public void removeWithGbids_unknownGbid() throws InterruptedException {
        final String[] gbids = new String[]{ "unknown" };
        testRemoveWithGbids_discoveryError(gbids, DiscoveryError.UNKNOWN_GBID);
    }

    @Test
    public void removeWithGbids_invalidGbid_nullGbid() throws InterruptedException {
        final String[] gbids = new String[]{ null };
        testRemoveWithGbids_discoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void removeWithGbids_invalidGbid_nullGbidsArray() throws InterruptedException {
        final String[] gbids = null;
        testRemoveWithGbids_discoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void removeWithGbids_invalidGbid_emptyGbidsArray() throws InterruptedException {
        final String[] gbids = new String[0];
        testRemoveWithGbids_discoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void removeWithGbids_invalidGbid_duplicateGbid() throws InterruptedException {
        final String[] gbids = new String[]{ validGbids[1], validGbids[1] };
        testRemoveWithGbids_discoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void removeWithGbids_internalError() throws InterruptedException {
        doThrow(new RuntimeException("error in DiscoveryEntryStore")).when(discoveryEntryStoreMock)
                                                                     .remove(anyString(), any(String[].class));
        Promise<Remove1Deferred> promise = subject.remove(PARTICIPANT_ID, validGbids.clone());
        checkPromiseError(promise, DiscoveryError.INTERNAL_ERROR);
        verify(discoveryEntryStoreMock).remove(eq(PARTICIPANT_ID), eq(validGbids));
    }

    @Test
    public void removeWithGbids_noEntryForParticipant() throws InterruptedException {
        String[] selectedGbids = new String[]{ validGbids[2], validGbids[1] };
        String[] expectedGbids = selectedGbids.clone();
        doReturn(0).when(discoveryEntryStoreMock).remove(anyString(), any(String[].class));
        Promise<Remove1Deferred> promise = subject.remove(PARTICIPANT_ID, selectedGbids);
        verify(discoveryEntryStoreMock).remove(eq(PARTICIPANT_ID), eq(expectedGbids));
        checkPromiseError(promise, DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
    }

    @Test
    public void removeWithGbids_noEntryForSelectedBackends() throws InterruptedException {
        String[] selectedGbids = new String[]{ validGbids[2], validGbids[1] };
        String[] expectedGbids = selectedGbids.clone();
        doReturn(-1).when(discoveryEntryStoreMock).remove(anyString(), any(String[].class));
        Promise<Remove1Deferred> promise = subject.remove(PARTICIPANT_ID, selectedGbids);
        verify(discoveryEntryStoreMock).remove(eq(PARTICIPANT_ID), eq(expectedGbids));
        checkPromiseError(promise, DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
    }

    @Test
    public void lookupByDomainInterface_callsStoreAndFiltersByOwnGbid() throws InterruptedException {
        GlobalDiscoveryEntryPersisted gdep1 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                TOPIC_NAME,
                                                                                validGbids[0]);
        gdep1.setParticipantId(gdep1.getGbid());
        GlobalDiscoveryEntryPersisted gdep2 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                TOPIC_NAME,
                                                                                validGbids[1]);
        gdep2.setParticipantId(gdep2.getGbid());
        GlobalDiscoveryEntryPersisted gdep3 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                TOPIC_NAME,
                                                                                validGbids[2]);
        gdep3.setParticipantId(gdep3.getGbid());
        GlobalDiscoveryEntry expectedEntry = new GlobalDiscoveryEntry(gdep3);
        doReturn(Arrays.asList(gdep1, gdep2, gdep3)).when(discoveryEntryStoreMock).lookup(any(String[].class),
                                                                                          anyString());

        Promise<Lookup1Deferred> promise = subject.lookup(DOMAINS.clone(), INTERFACE_NAME);

        verify(discoveryEntryStoreMock).lookup(eq(DOMAINS), eq(INTERFACE_NAME));
        Object[] values = checkPromiseSuccess(promise);
        GlobalDiscoveryEntry[] result = (GlobalDiscoveryEntry[]) values[0];
        assertEquals(1, result.length);
        checkDiscoveryEntry(expectedEntry, (GlobalDiscoveryEntry) result[0]);
    }

    @Test
    public void lookupByDomainInterfaceWithGbids_callsStoreAndFiltersBySelectedGbids() throws InterruptedException {
        GlobalDiscoveryEntryPersisted gdep1 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                TOPIC_NAME,
                                                                                validGbids[0]);
        gdep1.setParticipantId(gdep1.getGbid());
        GlobalDiscoveryEntryPersisted gdep2 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                TOPIC_NAME,
                                                                                validGbids[1]);
        gdep2.setParticipantId(gdep2.getGbid());
        GlobalDiscoveryEntryPersisted gdep3 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                TOPIC_NAME,
                                                                                validGbids[2]);
        gdep3.setParticipantId(gdep3.getGbid());
        GlobalDiscoveryEntry expectedEntry1 = new GlobalDiscoveryEntry(gdep1);
        GlobalDiscoveryEntry expectedEntry2 = new GlobalDiscoveryEntry(gdep3);
        doReturn(Arrays.asList(gdep1, gdep2, gdep3)).when(discoveryEntryStoreMock).lookup(any(String[].class),
                                                                                          anyString());

        Promise<Lookup2Deferred> promise = subject.lookup(DOMAINS.clone(),
                                                          INTERFACE_NAME,
                                                          new String[]{ validGbids[2], validGbids[0] });

        verify(discoveryEntryStoreMock).lookup(eq(DOMAINS), eq(INTERFACE_NAME));
        Object[] values = checkPromiseSuccess(promise);
        GlobalDiscoveryEntry[] result = (GlobalDiscoveryEntry[]) values[0];
        assertEquals(2, result.length);
        if (validGbids[0].equals(result[0].getParticipantId())) {
            checkDiscoveryEntry(expectedEntry1, (GlobalDiscoveryEntry) result[0]);
            checkDiscoveryEntry(expectedEntry2, (GlobalDiscoveryEntry) result[1]);
        } else {
            checkDiscoveryEntry(expectedEntry1, (GlobalDiscoveryEntry) result[1]);
            checkDiscoveryEntry(expectedEntry2, (GlobalDiscoveryEntry) result[0]);
        }
    }

    @Test
    public void lookupByDomainInterfaceWithGbids_returnsOnlyOneEntryPerParticipantId() throws InterruptedException {
        String participantId1 = "participantId1";
        String participantId2 = "participantId2";
        GlobalDiscoveryEntryPersisted gdep1_1 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                  TOPIC_NAME,
                                                                                  validGbids[0]);
        gdep1_1.setParticipantId(participantId1);
        gdep1_1.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(gdep1_1.getGbid(), TOPIC_NAME)));
        GlobalDiscoveryEntryPersisted gdep1_2 = new GlobalDiscoveryEntryPersisted(gdep1_1, TOPIC_NAME, validGbids[1]);
        gdep1_2.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(gdep1_2.getGbid(), TOPIC_NAME)));
        GlobalDiscoveryEntryPersisted gdep1_3 = new GlobalDiscoveryEntryPersisted(gdep1_1, TOPIC_NAME, validGbids[2]);
        gdep1_3.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(gdep1_3.getGbid(), TOPIC_NAME)));
        GlobalDiscoveryEntryPersisted gdep2 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                TOPIC_NAME,
                                                                                validGbids[1]);
        gdep2.setParticipantId(participantId2);
        doReturn(Arrays.asList(gdep1_1, gdep1_2, gdep1_3, gdep2)).when(discoveryEntryStoreMock)
                                                                 .lookup(any(String[].class), anyString());

        Promise<Lookup2Deferred> promise = subject.lookup(DOMAINS.clone(),
                                                          INTERFACE_NAME,
                                                          new String[]{ validGbids[0], validGbids[1] });

        verify(discoveryEntryStoreMock).lookup(eq(DOMAINS), eq(INTERFACE_NAME));
        Object[] values = checkPromiseSuccess(promise);
        GlobalDiscoveryEntry[] result = (GlobalDiscoveryEntry[]) values[0];
        assertEquals(2, result.length);
        assertTrue(participantId1.equals(result[0].getParticipantId())
                ^ participantId1.equals(result[1].getParticipantId()));
        assertTrue(participantId2.equals(result[0].getParticipantId())
                ^ participantId2.equals(result[1].getParticipantId()));
    }

    @Test
    public void lookupByDomainInterfaceWithGbids_singleEmptyGbid_filtersByGcdGbid() throws InterruptedException {
        GlobalDiscoveryEntryPersisted gdep1 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                TOPIC_NAME,
                                                                                validGbids[0]);
        gdep1.setParticipantId(gdep1.getGbid());
        GlobalDiscoveryEntryPersisted gdep2 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                TOPIC_NAME,
                                                                                validGbids[1]);
        gdep2.setParticipantId(gdep2.getGbid());
        GlobalDiscoveryEntryPersisted gdep3 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                TOPIC_NAME,
                                                                                validGbids[2]);
        gdep3.setParticipantId(gdep3.getGbid());
        GlobalDiscoveryEntry expectedEntry = new GlobalDiscoveryEntry(gdep3);
        doReturn(Arrays.asList(gdep1, gdep2, gdep3)).when(discoveryEntryStoreMock).lookup(any(String[].class),
                                                                                          anyString());

        Promise<Lookup2Deferred> promise = subject.lookup(DOMAINS.clone(), INTERFACE_NAME, new String[]{ "" });

        verify(discoveryEntryStoreMock).lookup(eq(DOMAINS), eq(INTERFACE_NAME));
        Object[] values = checkPromiseSuccess(promise);
        GlobalDiscoveryEntry[] result = (GlobalDiscoveryEntry[]) values[0];
        assertEquals(1, result.length);
        checkDiscoveryEntry(expectedEntry, (GlobalDiscoveryEntry) result[0]);
    }

    @Test
    public void lookupByDomainInterfaceWithGbids_multipleGbidsWithEmptyGbid_filtersWithEmptyGbidReplaced() throws InterruptedException {
        GlobalDiscoveryEntryPersisted gdep1 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                TOPIC_NAME,
                                                                                validGbids[0]);
        gdep1.setParticipantId(gdep1.getGbid());
        GlobalDiscoveryEntryPersisted gdep2 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                TOPIC_NAME,
                                                                                validGbids[1]);
        gdep2.setParticipantId(gdep2.getGbid());
        GlobalDiscoveryEntryPersisted gdep3 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                TOPIC_NAME,
                                                                                validGbids[2]);
        gdep3.setParticipantId(gdep3.getGbid());
        GlobalDiscoveryEntry expectedEntry2 = new GlobalDiscoveryEntry(gdep2);
        GlobalDiscoveryEntry expectedEntry3 = new GlobalDiscoveryEntry(gdep3);
        doReturn(Arrays.asList(gdep1, gdep2, gdep3)).when(discoveryEntryStoreMock).lookup(any(String[].class),
                                                                                          anyString());

        Promise<Lookup2Deferred> promise = subject.lookup(DOMAINS.clone(),
                                                          INTERFACE_NAME,
                                                          new String[]{ "", validGbids[1] });

        verify(discoveryEntryStoreMock).lookup(eq(DOMAINS), eq(INTERFACE_NAME));
        Object[] values = checkPromiseSuccess(promise);
        GlobalDiscoveryEntry[] result = (GlobalDiscoveryEntry[]) values[0];
        assertEquals(2, result.length);
        if (validGbids[1].equals(result[0].getParticipantId())) {
            checkDiscoveryEntry(expectedEntry2, (GlobalDiscoveryEntry) result[0]);
            checkDiscoveryEntry(expectedEntry3, (GlobalDiscoveryEntry) result[1]);
        } else {
            checkDiscoveryEntry(expectedEntry2, (GlobalDiscoveryEntry) result[1]);
            checkDiscoveryEntry(expectedEntry3, (GlobalDiscoveryEntry) result[0]);
        }
    }

    @Test
    public void lookupByDomainInterfaceWithGbids_emptyAndGcdGbid_filtersWithEmptyGbidReplaced() throws InterruptedException {
        GlobalDiscoveryEntryPersisted gdep1 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                TOPIC_NAME,
                                                                                validGbids[0]);
        gdep1.setParticipantId(gdep1.getGbid());
        GlobalDiscoveryEntryPersisted gdep2 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                TOPIC_NAME,
                                                                                validGbids[1]);
        gdep2.setParticipantId(gdep2.getGbid());
        GlobalDiscoveryEntryPersisted gdep3 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                TOPIC_NAME,
                                                                                validGbids[2]);
        gdep3.setParticipantId(gdep3.getGbid());
        GlobalDiscoveryEntry expectedEntry = new GlobalDiscoveryEntry(gdep3);
        doReturn(Arrays.asList(gdep1, gdep2, gdep3)).when(discoveryEntryStoreMock).lookup(any(String[].class),
                                                                                          anyString());

        Promise<Lookup2Deferred> promise = subject.lookup(DOMAINS.clone(),
                                                          INTERFACE_NAME,
                                                          new String[]{ GCD_GBID, "" });

        verify(discoveryEntryStoreMock).lookup(eq(DOMAINS), eq(INTERFACE_NAME));
        Object[] values = checkPromiseSuccess(promise);
        GlobalDiscoveryEntry[] result = (GlobalDiscoveryEntry[]) values[0];
        assertEquals(1, result.length);
        checkDiscoveryEntry(expectedEntry, (GlobalDiscoveryEntry) result[0]);
    }

    @Test
    public void lookupByParticipantId_callsStoreAndFiltersByOwnGbid() throws InterruptedException {
        GlobalDiscoveryEntryPersisted gdep1_1 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                  TOPIC_NAME,
                                                                                  validGbids[0]);
        gdep1_1.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(gdep1_1.getGbid(), TOPIC_NAME)));
        GlobalDiscoveryEntryPersisted gdep1_2 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                  TOPIC_NAME,
                                                                                  validGbids[1]);
        gdep1_2.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(gdep1_2.getGbid(), TOPIC_NAME)));
        GlobalDiscoveryEntryPersisted gdep1_3 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                  TOPIC_NAME,
                                                                                  validGbids[2]);
        gdep1_3.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(gdep1_3.getGbid(), TOPIC_NAME)));
        GlobalDiscoveryEntry expectedEntry = new GlobalDiscoveryEntry(gdep1_3);
        doReturn(Optional.of(Arrays.asList(gdep1_1, gdep1_2, gdep1_3))).when(discoveryEntryStoreMock)
                                                                       .lookup(anyString());

        Promise<Lookup3Deferred> promise = subject.lookup(PARTICIPANT_ID);

        verify(discoveryEntryStoreMock).lookup(eq(PARTICIPANT_ID));
        Object[] values = checkPromiseSuccess(promise);
        GlobalDiscoveryEntry result = (GlobalDiscoveryEntry) values[0];
        assertEquals(expectedEntry, result);
    }

    @Test
    public void lookupByParticipantIdWithGbids_callsStoreAndFiltersBySelectedGbids() throws InterruptedException {
        GlobalDiscoveryEntryPersisted gdep1_1 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                  TOPIC_NAME,
                                                                                  validGbids[0]);
        gdep1_1.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(gdep1_1.getGbid(), TOPIC_NAME)));
        GlobalDiscoveryEntryPersisted gdep1_2 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                  TOPIC_NAME,
                                                                                  validGbids[1]);
        gdep1_2.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(gdep1_2.getGbid(), TOPIC_NAME)));
        GlobalDiscoveryEntryPersisted gdep1_3 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                  TOPIC_NAME,
                                                                                  validGbids[2]);
        gdep1_3.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(gdep1_3.getGbid(), TOPIC_NAME)));
        doReturn(Optional.of(Arrays.asList(gdep1_1, gdep1_2, gdep1_3))).when(discoveryEntryStoreMock)
                                                                       .lookup(anyString());

        Promise<Lookup4Deferred> promise = subject.lookup(PARTICIPANT_ID, new String[]{ validGbids[0], validGbids[1] });

        verify(discoveryEntryStoreMock).lookup(eq(PARTICIPANT_ID));
        Object[] values = checkPromiseSuccess(promise);
        GlobalDiscoveryEntry result = (GlobalDiscoveryEntry) values[0];
        MqttAddress address = (MqttAddress) CapabilityUtils.getAddressFromGlobalDiscoveryEntry(result);
        assertTrue(validGbids[0].equals(address.getBrokerUri()) ^ validGbids[1].equals(address.getBrokerUri()));
    }

    @Test
    public void lookupByParticipantIdWithGbids_singleEmptyGbid_filtersByGcdGbid() throws InterruptedException {
        GlobalDiscoveryEntryPersisted gdep1_1 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                  TOPIC_NAME,
                                                                                  validGbids[0]);
        gdep1_1.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(gdep1_1.getGbid(), TOPIC_NAME)));
        GlobalDiscoveryEntryPersisted gdep1_2 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                  TOPIC_NAME,
                                                                                  validGbids[1]);
        gdep1_2.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(gdep1_2.getGbid(), TOPIC_NAME)));
        GlobalDiscoveryEntryPersisted gdep1_3 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                  TOPIC_NAME,
                                                                                  validGbids[2]);
        gdep1_3.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(gdep1_3.getGbid(), TOPIC_NAME)));

        GlobalDiscoveryEntry expectedEntry = new GlobalDiscoveryEntry(gdep1_3);
        expectedEntry.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(GCD_GBID, TOPIC_NAME)));

        doReturn(Optional.of(Arrays.asList(gdep1_1, gdep1_2, gdep1_3))).when(discoveryEntryStoreMock)
                                                                       .lookup(anyString());

        Promise<Lookup4Deferred> promise = subject.lookup(PARTICIPANT_ID, new String[]{ "" });

        verify(discoveryEntryStoreMock).lookup(eq(PARTICIPANT_ID));
        Object[] values = checkPromiseSuccess(promise);
        GlobalDiscoveryEntry result = (GlobalDiscoveryEntry) values[0];
        assertEquals(expectedEntry, result);
    }

    @Test
    public void lookupByParticipantIdWithGbids_multipleGbidsWithEmptyGbid_filtersWithEmptyGbidReplaced() throws InterruptedException {
        GlobalDiscoveryEntryPersisted gdep1_1 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                  TOPIC_NAME,
                                                                                  validGbids[0]);
        gdep1_1.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(gdep1_1.getGbid(), TOPIC_NAME)));
        GlobalDiscoveryEntryPersisted gdep1_2 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                  TOPIC_NAME,
                                                                                  validGbids[1]);
        gdep1_2.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(gdep1_2.getGbid(), TOPIC_NAME)));
        GlobalDiscoveryEntryPersisted gdep1_3 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                  TOPIC_NAME,
                                                                                  validGbids[2]);
        gdep1_3.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(gdep1_3.getGbid(), TOPIC_NAME)));
        doReturn(Optional.of(Arrays.asList(gdep1_1, gdep1_2, gdep1_3))).when(discoveryEntryStoreMock)
                                                                       .lookup(anyString());

        Promise<Lookup4Deferred> promise = subject.lookup(PARTICIPANT_ID, new String[]{ "", validGbids[1] });

        verify(discoveryEntryStoreMock).lookup(eq(PARTICIPANT_ID));
        Object[] values = checkPromiseSuccess(promise);
        GlobalDiscoveryEntry result = (GlobalDiscoveryEntry) values[0];
        MqttAddress address = (MqttAddress) CapabilityUtils.getAddressFromGlobalDiscoveryEntry(result);
        assertTrue(GCD_GBID.equals(address.getBrokerUri()) ^ validGbids[1].equals(address.getBrokerUri()));
    }

    @Test
    public void lookupByParticipantIdWithGbids_emptyAndGcdGbid_filtersWithEmptyGbidReplaced() throws InterruptedException {
        GlobalDiscoveryEntryPersisted gdep1_1 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                  TOPIC_NAME,
                                                                                  validGbids[0]);
        gdep1_1.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(gdep1_1.getGbid(), TOPIC_NAME)));
        GlobalDiscoveryEntryPersisted gdep1_2 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                  TOPIC_NAME,
                                                                                  validGbids[1]);
        gdep1_2.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(gdep1_2.getGbid(), TOPIC_NAME)));
        GlobalDiscoveryEntryPersisted gdep1_3 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                                  TOPIC_NAME,
                                                                                  validGbids[2]);
        gdep1_3.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(gdep1_3.getGbid(), TOPIC_NAME)));
        doReturn(Optional.of(Arrays.asList(gdep1_1, gdep1_2, gdep1_3))).when(discoveryEntryStoreMock)
                                                                       .lookup(anyString());

        Promise<Lookup4Deferred> promise = subject.lookup(PARTICIPANT_ID, new String[]{ "", GCD_GBID });

        verify(discoveryEntryStoreMock).lookup(eq(PARTICIPANT_ID));
        Object[] values = checkPromiseSuccess(promise);
        GlobalDiscoveryEntry result = (GlobalDiscoveryEntry) values[0];
        MqttAddress address = (MqttAddress) CapabilityUtils.getAddressFromGlobalDiscoveryEntry(result);
        assertEquals(GCD_GBID, address.getBrokerUri());
    }

    private void testLookupByDomainInterfaceWithGbids_discoveryError(String[] gbids,
                                                                     DiscoveryError expectedError) throws InterruptedException {
        Promise<Lookup2Deferred> promise = subject.lookup(DOMAINS.clone(), INTERFACE_NAME, gbids);
        checkPromiseError(promise, expectedError);
        verify(discoveryEntryStoreMock, times(0)).lookup(any(String[].class), anyString());
    }

    private void testLookupByParticipantIdWithGbids_discoveryError(String[] gbids,
                                                                   DiscoveryError expectedError) throws InterruptedException {
        Promise<Lookup4Deferred> promise = subject.lookup(PARTICIPANT_ID, gbids);
        checkPromiseError(promise, expectedError);
        verify(discoveryEntryStoreMock, times(0)).lookup(anyString());
    }

    @Test
    public void lookupByDomainInterfaceWithGbids_unknownGbid() throws InterruptedException {
        final String[] gbids = new String[]{ "unknown" };
        testLookupByDomainInterfaceWithGbids_discoveryError(gbids, DiscoveryError.UNKNOWN_GBID);
    }

    @Test
    public void lookupByParticipantIdWithGbids_unknownGbid() throws InterruptedException {
        final String[] gbids = new String[]{ "unknown" };
        testLookupByParticipantIdWithGbids_discoveryError(gbids, DiscoveryError.UNKNOWN_GBID);
    }

    @Test
    public void lookupByDomainInterfaceWithGbids_invalidGbid_nullGbid() throws InterruptedException {
        final String[] gbids = new String[]{ null };
        testLookupByDomainInterfaceWithGbids_discoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void lookupByParticipantIdWithGbids_invalidGbid_nullGbid() throws InterruptedException {
        final String[] gbids = new String[]{ null };
        testLookupByParticipantIdWithGbids_discoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void lookupByDomainInterfaceWithGbids_invalidGbid_nullGbidsArray() throws InterruptedException {
        final String[] gbids = null;
        testLookupByDomainInterfaceWithGbids_discoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void lookupByParticipantIdWithGbids_invalidGbid_nullGbidsArray() throws InterruptedException {
        final String[] gbids = null;
        testLookupByParticipantIdWithGbids_discoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void lookupByDomainInterfaceWithGbids_invalidGbid_emptyGbidsArray() throws InterruptedException {
        final String[] gbids = new String[0];
        testLookupByDomainInterfaceWithGbids_discoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void lookupByParticipantIdWithGbids_invalidGbid_emptyGbidsArray() throws InterruptedException {
        final String[] gbids = new String[0];
        testLookupByParticipantIdWithGbids_discoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void lookupByDomainInterfaceWithGbids_invalidGbid_duplicateGbid() throws InterruptedException {
        final String[] gbids = new String[]{ validGbids[0], validGbids[1], validGbids[0] };
        testLookupByDomainInterfaceWithGbids_discoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void lookupByParticipantIdWithGbids_invalidGbid_duplicateGbid() throws InterruptedException {
        final String[] gbids = new String[]{ validGbids[0], validGbids[2], validGbids[2] };
        testLookupByParticipantIdWithGbids_discoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void lookupByDomainInterfaceWithGbids_internalError() throws InterruptedException {
        doThrow(new RuntimeException("error in DiscoveryEntryStore")).when(discoveryEntryStoreMock)
                                                                     .lookup(any(String[].class), anyString());
        Promise<Lookup2Deferred> promise = subject.lookup(DOMAINS.clone(), INTERFACE_NAME, validGbids);
        checkPromiseError(promise, DiscoveryError.INTERNAL_ERROR);
        verify(discoveryEntryStoreMock).lookup(eq(DOMAINS), eq(INTERFACE_NAME));
    }

    @Test
    public void lookupByParticipantIdWithGbids_internalError() throws InterruptedException {
        doThrow(new RuntimeException("error in DiscoveryEntryStore")).when(discoveryEntryStoreMock).lookup(anyString());
        Promise<Lookup4Deferred> promise = subject.lookup(PARTICIPANT_ID, validGbids);
        checkPromiseError(promise, DiscoveryError.INTERNAL_ERROR);
        verify(discoveryEntryStoreMock).lookup(eq(PARTICIPANT_ID));
    }

    @Test
    public void lookupByDomainInterface_internalError() throws InterruptedException {
        doThrow(new RuntimeException("error in DiscoveryEntryStore")).when(discoveryEntryStoreMock)
                                                                     .lookup(any(String[].class), anyString());
        Promise<Lookup1Deferred> promise = subject.lookup(DOMAINS.clone(), INTERFACE_NAME);
        checkPromiseException(promise, ProviderRuntimeException.class, DiscoveryError.INTERNAL_ERROR.name());
        verify(discoveryEntryStoreMock).lookup(eq(DOMAINS), eq(INTERFACE_NAME));
    }

    @Test
    public void lookupByParticipantId_internalError() throws InterruptedException {
        doThrow(new RuntimeException("error in DiscoveryEntryStore")).when(discoveryEntryStoreMock).lookup(anyString());
        Promise<Lookup3Deferred> promise = subject.lookup(PARTICIPANT_ID);
        checkPromiseException(promise, ProviderRuntimeException.class, DiscoveryError.INTERNAL_ERROR.name());
        verify(discoveryEntryStoreMock).lookup(eq(PARTICIPANT_ID));
    }

    @Test
    public void lookupByDomainInterfaceWithGbids_noMatchingEntry() throws InterruptedException {
        doReturn(new ArrayList<>()).when(discoveryEntryStoreMock).lookup(any(String[].class), anyString());

        Promise<Lookup2Deferred> promise = subject.lookup(DOMAINS.clone(),
                                                          INTERFACE_NAME,
                                                          new String[]{ validGbids[1] });

        verify(discoveryEntryStoreMock).lookup(eq(DOMAINS), eq(INTERFACE_NAME));
        Object[] values = checkPromiseSuccess(promise);
        GlobalDiscoveryEntry[] result = (GlobalDiscoveryEntry[]) values[0];
        assertEquals(0, result.length);
    }

    @Test
    public void lookupByDomainInterfaceWithGbids_noEntryForSelectedBackends() throws InterruptedException {
        GlobalDiscoveryEntryPersisted gdep = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                               TOPIC_NAME,
                                                                               validGbids[0]);
        gdep.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(validGbids[0], TOPIC_NAME)));
        doReturn(Arrays.asList(gdep)).when(discoveryEntryStoreMock).lookup(any(String[].class), anyString());

        Promise<Lookup2Deferred> promise = subject.lookup(DOMAINS.clone(),
                                                          INTERFACE_NAME,
                                                          new String[]{ validGbids[1] });

        verify(discoveryEntryStoreMock).lookup(eq(DOMAINS), eq(INTERFACE_NAME));
        checkPromiseError(promise, DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
    }

    @Test
    public void lookupByParticipantIdWithGbids_noEntryForParticipant() throws InterruptedException {
        doReturn(Optional.of(new ArrayList<>())).when(discoveryEntryStoreMock).lookup(anyString());

        Promise<Lookup4Deferred> promise = subject.lookup(PARTICIPANT_ID, new String[]{ validGbids[0], validGbids[1] });

        verify(discoveryEntryStoreMock).lookup(eq(PARTICIPANT_ID));
        checkPromiseError(promise, DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
    }

    @Test
    public void lookupByParticipantIdWithGbids_noEntryForSelectedBackends() throws InterruptedException {
        GlobalDiscoveryEntryPersisted gdep = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                               TOPIC_NAME,
                                                                               validGbids[0]);
        gdep.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(validGbids[0], TOPIC_NAME)));
        doReturn(Optional.of(Arrays.asList(gdep))).when(discoveryEntryStoreMock).lookup(anyString());

        Promise<Lookup4Deferred> promise = subject.lookup(PARTICIPANT_ID, new String[]{ validGbids[1], validGbids[2] });

        verify(discoveryEntryStoreMock).lookup(eq(PARTICIPANT_ID));
        checkPromiseError(promise, DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
    }

    @Test
    public void lookupByDomainInterface_noMatchingEntry() throws InterruptedException {
        doReturn(new ArrayList<>()).when(discoveryEntryStoreMock).lookup(any(String[].class), anyString());

        Promise<Lookup1Deferred> promise = subject.lookup(DOMAINS.clone(), INTERFACE_NAME);

        verify(discoveryEntryStoreMock).lookup(eq(DOMAINS), eq(INTERFACE_NAME));
        Object[] values = checkPromiseSuccess(promise);
        GlobalDiscoveryEntry[] result = (GlobalDiscoveryEntry[]) values[0];
        assertEquals(0, result.length);
    }

    @Test
    public void lookupByDomainInterface_noEntryForSelectedBackends() throws InterruptedException {
        GlobalDiscoveryEntryPersisted gdep = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                               TOPIC_NAME,
                                                                               validGbids[0]);
        gdep.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(validGbids[0], TOPIC_NAME)));
        doReturn(Arrays.asList(gdep)).when(discoveryEntryStoreMock).lookup(any(String[].class), anyString());

        Promise<Lookup1Deferred> promise = subject.lookup(DOMAINS.clone(), INTERFACE_NAME);

        verify(discoveryEntryStoreMock).lookup(eq(DOMAINS), eq(INTERFACE_NAME));
        checkPromiseException(promise,
                              ProviderRuntimeException.class,
                              DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS.name());
    }

    @Test
    public void lookupByParticipantId_noEntryForParticipant() throws InterruptedException {
        doReturn(Optional.of(new ArrayList<>())).when(discoveryEntryStoreMock).lookup(anyString());

        Promise<Lookup3Deferred> promise = subject.lookup(PARTICIPANT_ID);

        verify(discoveryEntryStoreMock).lookup(eq(PARTICIPANT_ID));
        checkPromiseException(promise, ProviderRuntimeException.class, DiscoveryError.NO_ENTRY_FOR_PARTICIPANT.name());
    }

    @Test
    public void lookupByParticipantId_noEntryForSelectedBackends() throws InterruptedException {
        GlobalDiscoveryEntryPersisted gdep = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry,
                                                                               TOPIC_NAME,
                                                                               validGbids[0]);
        gdep.setAddress(CapabilityUtils.serializeAddress(new MqttAddress(validGbids[0], TOPIC_NAME)));
        doReturn(Optional.of(Arrays.asList(gdep))).when(discoveryEntryStoreMock).lookup(anyString());

        Promise<Lookup3Deferred> promise = subject.lookup(PARTICIPANT_ID);

        verify(discoveryEntryStoreMock).lookup(eq(PARTICIPANT_ID));
        checkPromiseException(promise,
                              ProviderRuntimeException.class,
                              DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS.name());
    }

    @Test
    public void touch_callsStore() throws InterruptedException {
        final String ccId = "testCcId-" + System.currentTimeMillis();
        Promise<DeferredVoid> promise = subject.touch(ccId);
        verify(discoveryEntryStoreMock).touch(eq(ccId));
        checkPromiseSuccess(promise);
    }

    @Test
    public void touch_rejectsOnException() throws InterruptedException {
        final String errorMessage = "testException-" + System.currentTimeMillis();
        final RuntimeException testException = new RuntimeException(errorMessage);
        final String ccId = "testCcId-" + System.currentTimeMillis();
        doThrow(testException).when(discoveryEntryStoreMock).touch(anyString());
        Promise<DeferredVoid> promise = subject.touch(ccId);
        verify(discoveryEntryStoreMock).touch(eq(ccId));
        checkPromiseException(promise, ProviderRuntimeException.class, "Touch failed: " + testException);
    }

    @Test
    public void touchWithParticipantIds_callsStore() throws InterruptedException {
        final String ccId = "testCcId-" + System.currentTimeMillis();
        final String[] participantIds = new String[]{ "testParticipant1", "testParticipant2" };
        Promise<DeferredVoid> promise = subject.touch(ccId, participantIds);
        verify(discoveryEntryStoreMock).touch(eq(ccId), eq(participantIds));
        checkPromiseSuccess(promise);
    }

    @Test
    public void touchWithParticipantIds_rejectsOnException() throws InterruptedException {
        final String errorMessage = "testException-" + System.currentTimeMillis();
        final RuntimeException testException = new RuntimeException(errorMessage);
        final String ccId = "testCcId-" + System.currentTimeMillis();
        final String[] participantIds = new String[]{ "testParticipant3", "testParticipant4" };
        doThrow(testException).when(discoveryEntryStoreMock).touch(anyString(), any(String[].class));
        Promise<DeferredVoid> promise = subject.touch(ccId, participantIds);
        verify(discoveryEntryStoreMock).touch(eq(ccId), eq(participantIds));
        checkPromiseException(promise, ProviderRuntimeException.class, "Touch failed: " + testException);
    }

    @Test
    public void removeStale_callsStore() throws InterruptedException {
        final String ccId = "testCcId-" + System.currentTimeMillis();
        final long maxLastSeen = System.currentTimeMillis();
        Promise<DeferredVoid> promise = subject.removeStale(ccId, maxLastSeen);
        verify(discoveryEntryStoreMock).removeStale(eq(ccId), eq(maxLastSeen));
        checkPromiseSuccess(promise);
    }

    @Test
    public void removeStale_rejectsOnException() throws InterruptedException {
        final String errorMessage = "testException-" + System.currentTimeMillis();
        final RuntimeException testException = new RuntimeException(errorMessage);
        final String ccId = "testCcId-" + System.currentTimeMillis();
        final long maxLastSeen = System.currentTimeMillis();
        doThrow(testException).when(discoveryEntryStoreMock).removeStale(anyString(), anyLong());
        Promise<DeferredVoid> promise = subject.removeStale(ccId, maxLastSeen);
        verify(discoveryEntryStoreMock).removeStale(eq(ccId), eq(maxLastSeen));
        checkPromiseException(promise, ProviderRuntimeException.class, "RemoveStale failed: " + testException);
    }

}
