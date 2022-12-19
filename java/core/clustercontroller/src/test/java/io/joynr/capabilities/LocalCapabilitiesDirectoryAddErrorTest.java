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
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.capabilities.helpers.GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import joynr.exceptions.ProviderRuntimeException;
import joynr.system.DiscoveryProvider;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@RunWith(MockitoJUnitRunner.class)
public class LocalCapabilitiesDirectoryAddErrorTest extends AbstractLocalCapabilitiesDirectoryTest {

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectoryAddErrorTest.class);

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

    @Test(timeout = TEST_TIMEOUT)
    public void testAddWithGbids_unknownGbid() throws InterruptedException {
        final String[] gbids = new String[]{ knownGbids[1], "unknown" };
        testAddReturnsDiscoveryError(gbids, DiscoveryError.UNKNOWN_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddWithGbids_invalidGbid_emptyGbid() throws InterruptedException {
        final String[] gbids = new String[]{ knownGbids[1], "" };
        testAddReturnsDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddWithGbids_invalidGbid_duplicateGbid() throws InterruptedException {
        final String[] gbids = new String[]{ knownGbids[1], knownGbids[1] };
        testAddReturnsDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddWithGbids_invalidGbid_nullGbid() throws InterruptedException {
        final String[] gbids = new String[]{ knownGbids[1], null };
        testAddReturnsDiscoveryError(gbids, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddWithGbids_invalidGbid_nullGbidArray() throws InterruptedException {
        testAddReturnsDiscoveryError(null, DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddToAllIsProperlyRejected_exception() throws InterruptedException {
        final String[] expectedGbids = knownGbids.clone();
        final JoynrRuntimeException exception = new JoynrRuntimeException("add failed");
        final ProviderRuntimeException expectedException = new ProviderRuntimeException(exception.toString());

        doAnswer(answerCreateHelper.createVoidAnswerWithException(exception)).when(globalCapabilitiesDirectoryClient)
                                                                             .add(any(),
                                                                                  argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                                  anyLong(),
                                                                                  any());

        final Promise<DiscoveryProvider.AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                                        true);

        promiseChecker.checkPromiseException(promise, expectedException);
        verify(globalCapabilitiesDirectoryClient).add(any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                      anyLong(),
                                                      eq(expectedGbids));
        verify(localDiscoveryEntryStoreMock, times(0)).add(any(DiscoveryEntry.class));
        verify(localDiscoveryEntryStoreMock, times(0)).remove(any(String.class));
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddToAllIsProperlyRejected_internalError() throws InterruptedException {
        testAddToAllIsProperlyRejected(DiscoveryError.INTERNAL_ERROR);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddToAllIsProperlyRejected_invalidGbid() throws InterruptedException {
        testAddToAllIsProperlyRejected(DiscoveryError.INVALID_GBID);
    }

    @Test(timeout = TEST_TIMEOUT)
    public void testAddToAllIsProperlyRejected_unknownGbid() throws InterruptedException {
        testAddToAllIsProperlyRejected(DiscoveryError.UNKNOWN_GBID);
    }

    private void testAddToAllIsProperlyRejected(final DiscoveryError expectedError) throws InterruptedException {
        final String[] expectedGbids = knownGbids.clone();
        doAnswer(answerCreateHelper.createVoidAnswerWithDiscoveryError(expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                                      .add(any(),
                                                                                           argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                                           anyLong(),
                                                                                           any());

        final Promise<DiscoveryProvider.AddToAllDeferred> promise = localCapabilitiesDirectory.addToAll(discoveryEntry,
                                                                                                        true);

        promiseChecker.checkPromiseError(promise, expectedError);
        verify(globalCapabilitiesDirectoryClient).add(any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                      anyLong(),
                                                      eq(expectedGbids));
        verify(localDiscoveryEntryStoreMock, times(0)).add(any(DiscoveryEntry.class));
        verify(localDiscoveryEntryStoreMock, times(0)).remove(any(String.class));
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
    }

    private void testAddReturnsDiscoveryError(final String[] gbids,
                                              final DiscoveryError expectedError) throws InterruptedException {
        final Promise<DiscoveryProvider.Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry,
                                                                                               true,
                                                                                               gbids);
        promiseChecker.checkPromiseError(promise, expectedError);
    }

    private void testAddWithGbidsIsProperlyRejected(final DiscoveryError expectedError,
                                                    final boolean awaitGlobalRegistration) throws InterruptedException {
        reset(globalCapabilitiesDirectoryClient, localDiscoveryEntryStoreMock);
        doAnswer(answerCreateHelper.createVoidAnswerWithDiscoveryError(expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                                      .add(any(),
                                                                                           argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                                           anyLong(),
                                                                                           any());

        final String[] gbids = new String[]{ knownGbids[0] };
        final String[] expectedGbids = gbids.clone();
        final Promise<DiscoveryProvider.Add1Deferred> promise = localCapabilitiesDirectory.add(discoveryEntry,
                                                                                               awaitGlobalRegistration,
                                                                                               gbids);
        if (awaitGlobalRegistration) {
            promiseChecker.checkPromiseError(promise, expectedError);
        } else {
            promiseChecker.checkPromiseSuccess(promise, "add without awaitGlobalRegistration failed");
        }

        verify(globalCapabilitiesDirectoryClient,
               timeout(DEFAULT_WAIT_TIME_MS).times(1)).add(any(),
                                                           argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                           anyLong(),
                                                           eq(expectedGbids));
        verify(localDiscoveryEntryStoreMock, times(awaitGlobalRegistration ? 0 : 1)).add(any(DiscoveryEntry.class));
        verify(localDiscoveryEntryStoreMock, times(0)).remove(any(String.class));
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
    }

    private void testAddIsProperlyRejected(final DiscoveryError expectedError) throws InterruptedException {
        reset(globalCapabilitiesDirectoryClient);
        final String[] expectedGbids = knownGbids.clone();
        doAnswer(answerCreateHelper.createVoidAnswerWithDiscoveryError(expectedError)).when(globalCapabilitiesDirectoryClient)
                                                                                      .add(any(),
                                                                                           argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                                                           anyLong(),
                                                                                           any());

        final boolean awaitGlobalRegistration = true;
        final Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        promiseChecker.checkPromiseErrorInProviderRuntimeException(promise, expectedError);

        verify(globalCapabilitiesDirectoryClient).add(any(),
                                                      argThat(new GlobalDiscoveryEntryWithUpdatedLastSeenDateMsMatcher(globalDiscoveryEntry)),
                                                      anyLong(),
                                                      eq(expectedGbids));
        verify(localDiscoveryEntryStoreMock, times(0)).add(any(DiscoveryEntry.class));
        verify(localDiscoveryEntryStoreMock, times(0)).remove(any(String.class));
        verify(globalDiscoveryEntryCacheMock, times(0)).add(any(GlobalDiscoveryEntry.class));
    }

    @Override
    protected Logger getLogger() {
        return logger;
    }
}