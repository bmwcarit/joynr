/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.integration;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.util.List;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Matchers;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import io.joynr.exceptions.DiscoveryException;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import joynr.exceptions.ApplicationException;
import joynr.tests.DefaulttestProvider;
import joynr.tests.testProxy;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;

/**
 * Test that errors from global discovery (add, lookup) are reported correctly to the application.
 * Errors from global remove are not reported and thus cannot be tested here.
 */
@RunWith(MockitoJUnitRunner.class)
public class MqttMultipleBackendDiscoveryErrorTest extends AbstractMqttMultipleBackendTest {

    @Before
    public void setUp() throws InterruptedException {
        super.setUp();
        createJoynrRuntimeWithMockedGcdClient();
    }

    private void testLookupWithDiscoveryError(String[] gbidsForLookup, DiscoveryError expectedError) {
        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(TESTDOMAIN, testProxy.class);
        proxyBuilder.setDiscoveryQos(discoveryQos);
        proxyBuilder.setGbids(gbidsForLookup);
        testProxy proxy = proxyBuilder.build();

        try {
            proxy.voidOperation();
            fail("Should never get this far.");
        } catch (DiscoveryException e) {
            String errorMsg = e.getMessage();
            assertNotNull(errorMsg);
            assertTrue("Error message does not contain \"DiscoveryError\": " + errorMsg,
                       errorMsg.contains("DiscoveryError"));
            assertTrue("Error message does not contain \"" + expectedError + "\": " + errorMsg,
                       errorMsg.contains(expectedError.name()));
        }
    }

    private void checkGcdClientAddLookupNotCalled() {
        verify(gcdClient, times(0)).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                        any(GlobalDiscoveryEntry.class),
                                        anyLong(),
                                        any(String[].class));
        verify(gcdClient,
               times(0)).lookup(Matchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                                any(String.class),
                                anyLong(),
                                any(String[].class));
        verify(gcdClient,
               times(0)).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                any(String[].class),
                                any(String.class),
                                anyLong(),
                                any(String[].class));
    }

    @Test
    public void testLookupWithUnknownGbid_singleGbid() {
        testLookupWithDiscoveryError(new String[]{ "unknownGbid" }, DiscoveryError.UNKNOWN_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testLookupWithUnknownGbid_multipleGbids() {
        testLookupWithDiscoveryError(new String[]{ TESTGBID1, "unknownGbid" }, DiscoveryError.UNKNOWN_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testLookupWithInvalidGbid_singleGbid_null() {
        testLookupWithDiscoveryError(new String[]{ null }, DiscoveryError.INVALID_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testLookupWithInvalidGbid_singleGbid_empty() {
        testLookupWithDiscoveryError(new String[]{ "" }, DiscoveryError.INVALID_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testLookupWithInvalidGbid_multipleGbids_null() {
        testLookupWithDiscoveryError(new String[]{ TESTGBID1, null }, DiscoveryError.INVALID_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testLookupWithInvalidGbid_multipleGbids_empty() {
        testLookupWithDiscoveryError(new String[]{ TESTGBID1, "" }, DiscoveryError.INVALID_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testLookupWithInvalidGbid_multipleGbids_duplicate() {
        testLookupWithDiscoveryError(new String[]{ TESTGBID1, TESTGBID2, TESTGBID1 }, DiscoveryError.INVALID_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    /**
     * Purpose of test is to check whether JDS error is correctly transmitted to the caller that tries to build a proxy.
     */
    private void testLookupWithGlobalDiscoveryError(DiscoveryError expectedError, String[] gbidsForLookup) {
        final String[] expectedGbids = gbidsForLookup.clone();

        doAnswer(new Answer<Void>() {

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError> callback = (CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>) invocation.getArguments()[0];
                callback.onFailure(expectedError);
                return null;
            }
        }).when(gcdClient).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                  any(String[].class),
                                  anyString(),
                                  anyLong(),
                                  any(String[].class));

        testLookupWithDiscoveryError(gbidsForLookup, expectedError);
        verify(gcdClient).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                 eq(new String[]{ TESTDOMAIN }),
                                 eq(testProxy.INTERFACE_NAME),
                                 anyLong(),
                                 eq(expectedGbids));
    }

    /**
     *  A locally valid GBID is invalid for the GCD if the GCD implements additional GBID checks which are more restrictive
     *  than the local (default) checks (null, empty, duplicate).
     */
    @Test
    public void testLookupWithGloballyInvalidGbid() {
        final DiscoveryError expectedError = DiscoveryError.INVALID_GBID;
        final String[] gbidsForLookup = new String[]{ TESTGBID1 };
        testLookupWithGlobalDiscoveryError(expectedError, gbidsForLookup);
    }

    /**
     * This is a test for bad configuration: the cluster controller has configured GBIDs that are not known to the GCD.
     */
    @Test
    public void testLookupWithGloballyUnknownGbid() {
        final DiscoveryError expectedError = DiscoveryError.UNKNOWN_GBID;
        final String[] gbidsForLookup = new String[]{ TESTGBID1 };
        testLookupWithGlobalDiscoveryError(expectedError, gbidsForLookup);
    }

    @Test
    public void testLookupWithGcdInternalError() {
        final DiscoveryError expectedError = DiscoveryError.INTERNAL_ERROR;
        final String[] gbidsForLookup = new String[]{ TESTGBID1 };
        testLookupWithGlobalDiscoveryError(expectedError, gbidsForLookup);
    }

    @Test
    public void testLookupForProviderInDifferentBackend() {
        final DiscoveryError expectedError = DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS;
        final String[] gbidsForLookup = new String[]{ TESTGBID1 };
        testLookupWithGlobalDiscoveryError(expectedError, gbidsForLookup);
    }

    private void testAddWithDiscoveryError(String[] gbidsForAdd, DiscoveryError expectedError) {
        Future<Void> registerFuture = joynrRuntime.getProviderRegistrar(TESTDOMAIN, new DefaulttestProvider())
                                                  .withProviderQos(providerQos)
                                                  .withGbids(gbidsForAdd)
                                                  .awaitGlobalRegistration()
                                                  .register();

        try {
            registerFuture.get(10000);
            fail("Should never get this far.");
        } catch (ApplicationException e) {
            assertEquals(expectedError, e.getError());
        } catch (Exception e) {
            fail("Unexpected exception from registerProvider: " + e);
        }
    }

    private void testAddWithIllegalArgumentException(String[] gbidsForAdd) {
        try {
            joynrRuntime.getProviderRegistrar(TESTDOMAIN, new DefaulttestProvider())
                        .withProviderQos(providerQos)
                        .withGbids(gbidsForAdd)
                        .awaitGlobalRegistration()
                        .register();
            fail("Should never get this far.");
        } catch (IllegalArgumentException e) {
            assertEquals("Provided gbid value(s) must not be null or empty!", e.getMessage());
        } catch (Exception e) {
            fail("Unexpected exception from registerProvider: " + e);
        }
    }

    @Test
    public void testAddWithUnknownGbid_singleGbid() {
        testAddWithDiscoveryError(new String[]{ "unknownGbid" }, DiscoveryError.UNKNOWN_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testAddWithUnknownGbid_multipleGbids() {
        testAddWithDiscoveryError(new String[]{ TESTGBID1, "unknownGbid" }, DiscoveryError.UNKNOWN_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testAddWithInvalidGbid_singleGbid_null() {
        testAddWithIllegalArgumentException(new String[]{ null });
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testAddWithInvalidGbid_singleGbid_empty() {
        testAddWithIllegalArgumentException(new String[]{ "" });
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testAddWithInvalidGbid_multipleGbids_null() {
        testAddWithIllegalArgumentException(new String[]{ TESTGBID1, null });
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testAddWithInvalidGbid_multipleGbids_empty() {
        testAddWithIllegalArgumentException(new String[]{ TESTGBID1, "" });
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testAddWithInvalidGbid_multipleGbids_duplicate() {
        testAddWithDiscoveryError(new String[]{ TESTGBID1, TESTGBID2, TESTGBID1 }, DiscoveryError.INVALID_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    /**
     * Purpose of test is to check whether JDS error is correctly transmitted to the caller that tries to register a provider.
     */
    private void testAddWithGlobalDiscoveryError(DiscoveryError expectedError, String[] gbidsForAdd) {
        final String[] expectedGbids = gbidsForAdd.clone();

        doAnswer(new Answer<Void>() {

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                CallbackWithModeledError<Void, DiscoveryError> callback = (CallbackWithModeledError<Void, DiscoveryError>) invocation.getArguments()[0];
                callback.onFailure(expectedError);
                return null;
            }
        }).when(gcdClient).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                               any(GlobalDiscoveryEntry.class),
                               anyLong(),
                               any(String[].class));

        testAddWithDiscoveryError(gbidsForAdd, expectedError);

        ArgumentCaptor<GlobalDiscoveryEntry> gdeCaptor = ArgumentCaptor.forClass(GlobalDiscoveryEntry.class);
        verify(gcdClient).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                              gdeCaptor.capture(),
                              anyLong(),
                              eq(expectedGbids));
        assertEquals(TESTDOMAIN, gdeCaptor.getValue().getDomain());
        assertEquals(testProxy.INTERFACE_NAME, gdeCaptor.getValue().getInterfaceName());
    }

    /**
     *  A locally valid GBID is invalid for the GCD if the GCD implements additional GBID checks which are more restrictive
     *  than the local (default) checks (null, empty, duplicate).
     */
    @Test
    public void testAddWithGloballyInvalidGbid() {
        final DiscoveryError expectedError = DiscoveryError.INVALID_GBID;
        final String[] gbidsForAdd = new String[]{ TESTGBID1 };
        testAddWithGlobalDiscoveryError(expectedError, gbidsForAdd);
    }

    /**
     * This is a test for bad configuration: the cluster controller has configured GBIDs that are not known to the GCD.
     */
    @Test
    public void testAddWithGloballyUnknownGbid() {
        final DiscoveryError expectedError = DiscoveryError.UNKNOWN_GBID;
        final String[] gbidsForAdd = new String[]{ TESTGBID1 };
        testAddWithGlobalDiscoveryError(expectedError, gbidsForAdd);
    }

    @Test
    public void testAddWithGcdInternalError() {
        final DiscoveryError expectedError = DiscoveryError.INTERNAL_ERROR;
        final String[] gbidsForAdd = new String[]{ TESTGBID1 };
        testAddWithGlobalDiscoveryError(expectedError, gbidsForAdd);
    }

}
