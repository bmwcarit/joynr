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
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import io.joynr.exceptions.JoynrRuntimeException;
import joynr.exceptions.ProviderRuntimeException;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.provider.Promise;
import joynr.system.DiscoveryProvider;
import joynr.system.RoutingTypes.Address;
import joynr.types.DiscoveryError;
import joynr.types.DiscoveryQos;
import joynr.types.DiscoveryScope;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@RunWith(MockitoJUnitRunner.class)
public class LocalCapabilitiesDirectoryLookupByDomainInterfaceErrorTest extends AbstractLocalCapabilitiesDirectoryTest {

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectoryLookupByDomainInterfaceErrorTest.class);

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbids_unknownGbids() throws InterruptedException {
        final String[] gbids = new String[]{ "not", "known" };
        testLookupByDomainInterfaceWithDiscoveryError(gbids, DiscoveryError.UNKNOWN_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbids_invalidGbid_emptyGbid() throws InterruptedException {
        final String[] gbids = new String[]{ "" };
        testLookupByDomainInterfaceWithDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbids_invalidGbid_duplicateGbid() throws InterruptedException {
        final String[] gbids = new String[]{ knownGbids[1], knownGbids[0], knownGbids[1] };
        testLookupByDomainInterfaceWithDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbids_invalidGbid_nullGbid() throws InterruptedException {
        final String[] gbids = new String[]{ null };
        testLookupByDomainInterfaceWithDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testLookupByDomainInterfaceWithGbids_invalidGbid_nullGbidArray() throws InterruptedException {
        testLookupByDomainInterfaceWithDiscoveryError(null, DiscoveryError.INVALID_GBID);
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
    public void testLookupByDomainInterfaceWithGbidsIsProperlyRejected_exception() throws InterruptedException {
        domains = new String[]{ DOMAIN_1 };
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        final JoynrRuntimeException exception = new JoynrRuntimeException(MSG_LOOKUP_FAILED);
        final ProviderRuntimeException expectedException = new ProviderRuntimeException(exception.toString());
        mockGcdLookupException(exception, domains);

        final Promise<DiscoveryProvider.Lookup2Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                                                     INTERFACE_NAME,
                                                                                                     discoveryQos,
                                                                                                     knownGbids);

        verify(globalCapabilitiesDirectoryClient).lookup(any(), eq(domains), eq(INTERFACE_NAME), anyLong(), any());
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());

        promiseChecker.checkPromiseException(promise, expectedException);
    }

    private void testLookupByDomainInterfaceIsProperlyRejected(final DiscoveryError expectedError) throws InterruptedException {
        domains = new String[]{ DOMAIN_2 };
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        mockGcdLookupError(expectedError, domains);

        final Promise<DiscoveryProvider.Lookup1Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                                                     INTERFACE_NAME,
                                                                                                     discoveryQos);

        verify(globalCapabilitiesDirectoryClient).lookup(any(), eq(domains), eq(INTERFACE_NAME), anyLong(), any());
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());

        promiseChecker.checkPromiseErrorInProviderRuntimeException(promise, expectedError);
    }

    private void testLookupByDomainInterfaceWithGbidsIsProperlyRejected(final DiscoveryError expectedError) throws InterruptedException {
        domains = new String[]{ DOMAIN_1 };
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        mockGcdLookupError(expectedError, domains);

        final Promise<DiscoveryProvider.Lookup2Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                                                     INTERFACE_NAME,
                                                                                                     discoveryQos,
                                                                                                     knownGbids);

        verify(globalCapabilitiesDirectoryClient).lookup(any(), eq(domains), eq(INTERFACE_NAME), anyLong(), any());
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());

        promiseChecker.checkPromiseError(promise, expectedError);
    }

    private void testLookupByDomainInterfaceWithDiscoveryError(final String[] gbids,
                                                               final DiscoveryError expectedError) throws InterruptedException {
        domains = new String[]{ "domain1", "domain2" };
        discoveryQos = new DiscoveryQos();

        final Promise<DiscoveryProvider.Lookup2Deferred> promise = localCapabilitiesDirectory.lookup(domains,
                                                                                                     INTERFACE_NAME,
                                                                                                     discoveryQos,
                                                                                                     gbids);

        verify(globalCapabilitiesDirectoryClient,
               never()).lookup(any(), any(String[].class), anyString(), anyLong(), any(String[].class));
        verify(routingTable, never()).incrementReferenceCount(any());
        verify(routingTable, never()).put(anyString(), any(Address.class), any(Boolean.class), anyLong());

        promiseChecker.checkPromiseError(promise, expectedError);
    }

    @Override
    protected Logger getLogger() {
        return logger;
    }
}
