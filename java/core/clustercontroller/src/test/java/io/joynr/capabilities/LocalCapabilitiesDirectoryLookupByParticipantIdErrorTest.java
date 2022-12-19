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
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import io.joynr.exceptions.JoynrRuntimeException;
import joynr.exceptions.ProviderRuntimeException;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.provider.Promise;
import joynr.system.DiscoveryProvider.Lookup3Deferred;
import joynr.system.DiscoveryProvider.Lookup4Deferred;
import joynr.system.RoutingTypes.Address;
import joynr.types.DiscoveryError;
import joynr.types.DiscoveryQos;
import joynr.types.DiscoveryScope;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@RunWith(MockitoJUnitRunner.class)
public class LocalCapabilitiesDirectoryLookupByParticipantIdErrorTest extends AbstractLocalCapabilitiesDirectoryTest {

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectoryLookupByParticipantIdErrorTest.class);

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
    public void testLookupByParticipantIdWithGbids_unknownGbids() throws InterruptedException {
        final String[] gbids = new String[]{ "not", "known" };
        testLookupByParticipantIdWithDiscoveryError(gbids, DiscoveryError.UNKNOWN_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbids_invalidGbid_emptyGbid() throws InterruptedException {
        final String[] gbids = new String[]{ "" };
        testLookupByParticipantIdWithDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbids_invalidGbid_duplicateGbid() throws InterruptedException {
        final String[] gbids = new String[]{ knownGbids[1], knownGbids[0], knownGbids[1] };
        testLookupByParticipantIdWithDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbids_invalidGbid_nullGbid() throws InterruptedException {
        final String[] gbids = new String[]{ null };
        testLookupByParticipantIdWithDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbids_invalidGbid_nullGbidArray() throws InterruptedException {
        testLookupByParticipantIdWithDiscoveryError(null, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByParticipantIdWithGbidsIsProperlyRejected_exception() throws InterruptedException {
        final String participantId = "participantId";
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        final JoynrRuntimeException exception = new JoynrRuntimeException(MSG_LOOKUP_FAILED);
        final ProviderRuntimeException expectedException = new ProviderRuntimeException(exception.toString());

        mockGcdLookupException(exception, participantId);

        final Promise<Lookup4Deferred> promise = localCapabilitiesDirectory.lookup(participantId,
                                                                                   discoveryQos,
                                                                                   knownGbids);

        promiseChecker.checkPromiseException(promise, expectedException);
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
    }

    private void testLookupByParticipantIdWithDiscoveryError(final String[] gbids,
                                                             final DiscoveryError expectedError) throws InterruptedException {
        final String participantId = "participantId";
        final Promise<Lookup4Deferred> promise = localCapabilitiesDirectory.lookup(participantId,
                                                                                   new DiscoveryQos(),
                                                                                   gbids);

        verify(globalCapabilitiesDirectoryClient, never()).lookup(any(), anyString(), anyLong(), any(String[].class));
        promiseChecker.checkPromiseError(promise, expectedError);
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
    }

    private void testLookupByParticipantIdIsProperlyRejected(final DiscoveryError expectedError) throws InterruptedException {
        final String participantId = "participantId";

        mockGcdLookupError(expectedError, participantId);

        final Promise<Lookup3Deferred> promise = localCapabilitiesDirectory.lookup(participantId);

        promiseChecker.checkPromiseErrorInProviderRuntimeException(promise, expectedError);
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
    }

    private void testLookupByParticipantIdWithGbidsIsProperlyRejected(final DiscoveryError expectedError) throws InterruptedException {
        final String participantId = "participantId";

        mockGcdLookupError(expectedError, participantId);

        discoveryQos = new DiscoveryQos(10000L, 500L, DiscoveryScope.LOCAL_AND_GLOBAL, false);
        final Promise<Lookup4Deferred> promise = localCapabilitiesDirectory.lookup(participantId,
                                                                                   discoveryQos,
                                                                                   knownGbids);

        promiseChecker.checkPromiseError(promise, expectedError);
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());
    }

    @Override
    protected Logger getLogger() {
        return logger;
    }
}
