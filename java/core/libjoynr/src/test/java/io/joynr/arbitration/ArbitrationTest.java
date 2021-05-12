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
package io.joynr.arbitration;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyObject;
import static org.mockito.Matchers.eq;
import static org.mockito.Matchers.isA;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.Semaphore;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.InOrder;
import org.mockito.Matchers;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.exceptions.MultiDomainNoCompatibleProviderFoundException;
import io.joynr.exceptions.NoCompatibleProviderFoundException;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.proxy.Callback;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.Future;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.util.JoynrThreadFactory;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.CustomParameter;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.DiscoveryError;
import joynr.types.ProviderQos;
import joynr.types.Version;

public class ArbitrationTest {

    private static final long ARBITRATION_TIMEOUT = 1000;
    private static final Long NO_EXPIRY = Long.MAX_VALUE;
    private static final MqttAddress testAddress = new MqttAddress("mqtt://testUrl", "testTopic");
    private static String interfaceName = "testInterface";
    private static Version interfaceVersion = new Version(0, 0);

    protected ArrayList<DiscoveryEntryWithMetaInfo> capabilitiesList;

    private final String domain = "testDomain";
    private String publicKeyId = "publicKeyId";
    private String testKeyword = "testKeyword";
    private long testPriority = 42;
    private String expectedParticipantId = "expectedParticipantId";
    private String expectedFixedParticipantId = "fixedParticipantId";

    private Address expectedEndpointAddress;

    private DiscoveryQos discoveryQos;

    private Semaphore localDiscoveryAggregatorSemaphore = new Semaphore(0);

    private ScheduledExecutorService scheduler;

    @Mock
    private ShutdownNotifier shutdownNotifier;
    @Mock
    private LocalDiscoveryAggregator localDiscoveryAggregator;
    @Mock
    private ArbitrationCallback arbitrationCallback;
    @Mock
    private DiscoveryEntryVersionFilter discoveryEntryVersionFilter;
    @Captor
    private ArgumentCaptor<joynr.types.DiscoveryQos> discoveryQosCaptor;
    @Mock
    private MessageRouter messageRouter;

    public interface TestInterface {
        public static final String INTERFACE_NAME = interfaceName;
    }

    @Before
    public void setUp() throws Exception {
        initMocks(this);

        capabilitiesList = new ArrayList<DiscoveryEntryWithMetaInfo>();

        doAnswer(new Answer<Object>() {

            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                Object[] arguments = invocation.getArguments();
                assert (arguments[0] instanceof Callback);
                assert (arguments[0] instanceof CallbackWithModeledError);
                @SuppressWarnings("unchecked")
                CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError> callback = (CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>) arguments[0];
                callback.resolve((Object) capabilitiesList.toArray(new DiscoveryEntryWithMetaInfo[0]));
                localDiscoveryAggregatorSemaphore.release();
                return null;
            }
        }).when(localDiscoveryAggregator)
          .lookup(Mockito.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                  eq(new String[]{ domain }),
                  eq(interfaceName),
                  any(joynr.types.DiscoveryQos.class),
                  Mockito.<String[]> any());

        Field discoveryEntryVersionFilterField = ArbitratorFactory.class.getDeclaredField("discoveryEntryVersionFilter");
        discoveryEntryVersionFilterField.setAccessible(true);
        discoveryEntryVersionFilterField.set(ArbitratorFactory.class, discoveryEntryVersionFilter);

        doAnswer(new Answer<Set<DiscoveryEntry>>() {
            @SuppressWarnings("unchecked")
            @Override
            public Set<DiscoveryEntry> answer(InvocationOnMock invocation) throws Throwable {
                return (Set<DiscoveryEntry>) invocation.getArguments()[1];
            }
        }).when(discoveryEntryVersionFilter)
          .filter(Mockito.<Version> any(),
                  Mockito.<Set<DiscoveryEntryWithMetaInfo>> any(),
                  Mockito.<Map<String, Set<Version>>> any());

        Field schedulerField = ArbitratorFactory.class.getDeclaredField("scheduler");
        schedulerField.setAccessible(true);
        String name = "TEST.joynr.scheduler.arbitration.arbitratorRunnable";
        ThreadFactory joynrThreadFactory = new JoynrThreadFactory(name, true);
        scheduler = Executors.newSingleThreadScheduledExecutor(joynrThreadFactory);
        schedulerField.set(ArbitratorFactory.class, scheduler);

        Field shutdownNotifierField = ArbitratorFactory.class.getDeclaredField("shutdownNotifier");
        shutdownNotifierField.setAccessible(true);
        shutdownNotifierField.set(ArbitratorFactory.class, shutdownNotifier);

        Field messageRouterField = ArbitratorFactory.class.getDeclaredField("messageRouter");
        messageRouterField.setAccessible(true);
        messageRouterField.set(ArbitratorFactory.class, messageRouter);

        ArbitratorFactory.start();
        verify(shutdownNotifier).registerForShutdown(any(ShutdownListener.class));
    }

    private Set<String> setUpProviderCapabilitiesForMultipleDomains(Set<String> domains) {
        Set<String> participantIds = new HashSet<String>(domains.size());
        ProviderQos qos = new ProviderQos();
        for (String domain : domains) {
            String participantId = "participantId-" + domain;
            participantIds.add(participantId);
            DiscoveryEntryWithMetaInfo entry = new DiscoveryEntryWithMetaInfo(new Version(),
                                                                              domain,
                                                                              TestInterface.INTERFACE_NAME,
                                                                              participantId,
                                                                              qos,
                                                                              System.currentTimeMillis(),
                                                                              NO_EXPIRY,
                                                                              publicKeyId,
                                                                              true);
            capabilitiesList.add(entry);
        }

        doAnswer((invocation) -> {
            Object[] arguments = invocation.getArguments();
            assert (arguments[0] instanceof Callback);
            assert (arguments[0] instanceof CallbackWithModeledError);
            @SuppressWarnings("unchecked")
            CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError> callback = (CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>) arguments[0];
            callback.resolve((Object) capabilitiesList.toArray(new DiscoveryEntryWithMetaInfo[0]));
            localDiscoveryAggregatorSemaphore.release();
            return null;
        }).when(localDiscoveryAggregator)
          .lookup(Mockito.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                  any(String[].class),
                  eq(interfaceName),
                  any(joynr.types.DiscoveryQos.class),
                  Mockito.<String[]> any());

        return participantIds;
    }

    @After
    public void tearDown() {
        ArbitratorFactory.shutdown();
        scheduler.shutdown();
        try {
            assertTrue(scheduler.awaitTermination(2000, TimeUnit.MILLISECONDS));
        } catch (InterruptedException e) {
            fail("InterruptedException in scheduler.awaitTermination: " + e);
        }
    }

    private void createArbitratorWithCallbackAndAwaitArbitration(DiscoveryQos discoveryQos,
                                                                 String... domains) throws InterruptedException {
        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos, null, domains);
    }

    /**
     * @param discoveryQos the DiscoveryQos used to create the arbitrator object
     * @param domains optional list of domains used to create the arbitrator object. If nothing is
     *                passed then the arbitrator is created for the member variable {@link #domain}
     * @throws InterruptedException
     */
    private void createArbitratorWithCallbackAndAwaitArbitration(DiscoveryQos discoveryQos,
                                                                 String[] gbids,
                                                                 String... domains) throws InterruptedException {
        try {
            Set<String> domainsSet;
            if (domains.length == 0) {
                domainsSet = new HashSet<String>(Arrays.asList(domain));
            } else {
                domainsSet = new HashSet<String>(Arrays.asList(domains));
            }

            Arbitrator arbitrator = ArbitratorFactory.create(domainsSet,
                                                             interfaceName,
                                                             interfaceVersion,
                                                             discoveryQos,
                                                             localDiscoveryAggregator,
                                                             gbids);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.scheduleArbitration();

            assertTrue(localDiscoveryAggregatorSemaphore.tryAcquire(ARBITRATION_TIMEOUT, TimeUnit.MILLISECONDS));
        } catch (DiscoveryException e) {
            fail("A Joynr Arbitration Exception has been thrown: " + e.getMessage());
        }
    }

    private ArbitrationResult captureArbitrationResultByArbitrationCallbackOnSuccess() {
        ArgumentCaptor<ArbitrationResult> arbitrationResultCaptor = ArgumentCaptor.forClass(ArbitrationResult.class);
        verify(arbitrationCallback).onSuccess(arbitrationResultCaptor.capture());
        return arbitrationResultCaptor.getValue();
    }

    @Test
    public void callsLocalDiscoveryAggregatorWithCorrectParameters_nullGbidArray() throws InterruptedException {
        final String[] nullGbidArray = null;
        final String[] expectedGbids = new String[]{};

        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(ARBITRATION_TIMEOUT);
        discoveryQos.setRetryIntervalMs(ARBITRATION_TIMEOUT + 1);

        joynr.types.DiscoveryQos expectedDiscoveryQos = convertDiscoveryQos(discoveryQos);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos, nullGbidArray, domain);
        verify(localDiscoveryAggregator, times(1)).lookup(anyObject(),
                                                          eq(new String[]{ domain }),
                                                          eq(interfaceName),
                                                          discoveryQosCaptor.capture(),
                                                          eq(expectedGbids));

        checkDiscoveryQos(expectedDiscoveryQos, discoveryQosCaptor.getValue());
    }

    private joynr.types.DiscoveryQos convertDiscoveryQos(DiscoveryQos discoveryQos) {
        return new joynr.types.DiscoveryQos(discoveryQos.getCacheMaxAgeMs(),
                                            discoveryQos.getDiscoveryTimeoutMs(),
                                            joynr.types.DiscoveryScope.valueOf(discoveryQos.getDiscoveryScope().name()),
                                            discoveryQos.getProviderMustSupportOnChange());
    }

    private void checkDiscoveryQos(joynr.types.DiscoveryQos expected, joynr.types.DiscoveryQos actual) {
        assertTrue("DiscoveryTimeout is larger than expected: " + actual.getDiscoveryTimeout() + ">"
                + expected.getDiscoveryTimeout(), expected.getDiscoveryTimeout() >= actual.getDiscoveryTimeout());

        assertTrue("DiscoveryTimeout (expected-actual): " + expected.getDiscoveryTimeout() + "-"
                + actual.getDiscoveryTimeout() + " > 50 (toleranceMs)",
                   expected.getDiscoveryTimeout() - actual.getDiscoveryTimeout() <= 50);

        assertEquals(expected.getCacheMaxAge(), actual.getCacheMaxAge());
        assertEquals(expected.getDiscoveryScope(), actual.getDiscoveryScope());
        assertEquals(expected.getProviderMustSupportOnChange(), actual.getProviderMustSupportOnChange());
    }

    @Test
    public void callsLocalDiscoveryAggregatorWithCorrectParameters_singleGbid() throws InterruptedException {
        final String[] singleGbid = new String[]{ "joynrdefaultgbid" };
        final String[] expectedGbids = singleGbid.clone();

        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(ARBITRATION_TIMEOUT);
        discoveryQos.setRetryIntervalMs(ARBITRATION_TIMEOUT + 1);

        joynr.types.DiscoveryQos expectedDiscoveryQos = convertDiscoveryQos(discoveryQos);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos, singleGbid, domain);

        verify(localDiscoveryAggregator, times(1)).lookup(anyObject(),
                                                          eq(new String[]{ domain }),
                                                          eq(interfaceName),
                                                          discoveryQosCaptor.capture(),
                                                          eq(expectedGbids));

        checkDiscoveryQos(expectedDiscoveryQos, discoveryQosCaptor.getValue());
    }

    @Test
    public void callsLocalDiscoveryAggregatorWithCorrectParameters_multipleGbids() throws InterruptedException {
        final String[] multipleGbids = { "joynrdefaultgbid", "testGbid2", "testGbid3" };
        final String[] expectedGbids = multipleGbids.clone();

        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(ARBITRATION_TIMEOUT);
        discoveryQos.setRetryIntervalMs(ARBITRATION_TIMEOUT + 1);

        joynr.types.DiscoveryQos expectedDiscoveryQos = convertDiscoveryQos(discoveryQos);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos, multipleGbids, domain);

        verify(localDiscoveryAggregator, times(1)).lookup(anyObject(),
                                                          eq(new String[]{ domain }),
                                                          eq(interfaceName),
                                                          discoveryQosCaptor.capture(),
                                                          eq(expectedGbids));

        checkDiscoveryQos(expectedDiscoveryQos, discoveryQosCaptor.getValue());
    }

    @Test
    public void keywordArbitratorTest() throws InterruptedException {
        ProviderQos providerQos = new ProviderQos();
        CustomParameter[] qosParameters = { new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword) };
        providerQos.setCustomParameters(qosParameters);
        expectedEndpointAddress = testAddress;
        DiscoveryEntryWithMetaInfo expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                           domain,
                                                                                           TestInterface.INTERFACE_NAME,
                                                                                           expectedParticipantId,
                                                                                           providerQos,
                                                                                           System.currentTimeMillis(),
                                                                                           NO_EXPIRY,
                                                                                           publicKeyId,
                                                                                           true);
        capabilitiesList.add(expectedDiscoveryEntry);

        ProviderQos providerQos2 = new ProviderQos();
        CustomParameter[] qosParameters2 = {
                new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, "otherKeyword") };
        providerQos2.setCustomParameters(qosParameters2);

        DiscoveryEntryWithMetaInfo otherDiscoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                        domain,
                                                                                        TestInterface.INTERFACE_NAME,
                                                                                        "wrongParticipantId",
                                                                                        providerQos2,
                                                                                        System.currentTimeMillis(),
                                                                                        NO_EXPIRY,
                                                                                        publicKeyId,
                                                                                        true);
        capabilitiesList.add(otherDiscoveryEntry);

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.Keyword, Long.MAX_VALUE);
        discoveryQos.addCustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);
        ArbitrationResult capturedArbitrationResult = captureArbitrationResultByArbitrationCallbackOnSuccess();

        Set<DiscoveryEntryWithMetaInfo> expectedSelectedDiscoveryEntries = new HashSet<>(Arrays.asList(expectedDiscoveryEntry));
        Set<DiscoveryEntryWithMetaInfo> expectedOtherDiscoveryEntries = new HashSet<>(Arrays.asList(otherDiscoveryEntry));
        ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedSelectedDiscoveryEntries,
                                                                            expectedOtherDiscoveryEntries);
        assertEquals(expectedArbitrationResult, capturedArbitrationResult);
    }

    @Test
    public void keyWordArbitratorMissingKeywordTest() throws InterruptedException {

        ProviderQos providerQos = new ProviderQos();
        CustomParameter[] qosParameters = {
                new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, "wrongkeyword") };
        providerQos.setCustomParameters(qosParameters);

        expectedEndpointAddress = testAddress;
        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            expectedParticipantId,
                                                            providerQos,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));
        ProviderQos providerQos2 = new ProviderQos();
        CustomParameter[] qosParameters2 = {
                new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, "otherKeyword") };
        providerQos2.setCustomParameters(qosParameters2);

        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "wrongParticipantId",
                                                            providerQos2,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));

        int discoveryTimeout = 0; // use minimal timeout to prevent restarting arbitration
        discoveryQos = new DiscoveryQos(discoveryTimeout, ArbitrationStrategy.Keyword, Long.MAX_VALUE);
        discoveryQos.addCustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);

        verify(arbitrationCallback, times(1)).onError(any(Throwable.class));
        verify(arbitrationCallback, never()).onSuccess(any(ArbitrationResult.class));
    }

    // Check that the keyword arbitrator will only consider providers that support onChange subscriptions
    // when this is requested by the DiscoveryQos
    @Test
    public void keywordArbitratorOnChangeSubscriptionsTest() throws InterruptedException {
        ProviderQos providerQos = new ProviderQos();
        CustomParameter[] qosParameters = { new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword) };

        // Create a capability entry for a provider with the correct keyword but that does not support onChange subscriptions
        providerQos.setCustomParameters(qosParameters);
        providerQos.setSupportsOnChangeSubscriptions(false);
        DiscoveryEntryWithMetaInfo otherDiscoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                        domain,
                                                                                        TestInterface.INTERFACE_NAME,
                                                                                        "wrongParticipantId",
                                                                                        providerQos,
                                                                                        System.currentTimeMillis(),
                                                                                        NO_EXPIRY,
                                                                                        publicKeyId,
                                                                                        true);
        capabilitiesList.add(otherDiscoveryEntry);

        // Create a capability entry for a provider with the correct keyword and that also supports onChange subscriptions
        ProviderQos providerQos2 = new ProviderQos();
        CustomParameter[] qosParameters2 = { new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword) };
        providerQos2.setCustomParameters(qosParameters2);
        providerQos2.setSupportsOnChangeSubscriptions(true);

        expectedEndpointAddress = testAddress;
        DiscoveryEntryWithMetaInfo expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                           domain,
                                                                                           TestInterface.INTERFACE_NAME,
                                                                                           "expectedParticipantId",
                                                                                           providerQos2,
                                                                                           System.currentTimeMillis(),
                                                                                           NO_EXPIRY,
                                                                                           publicKeyId,
                                                                                           true);
        capabilitiesList.add(expectedDiscoveryEntry);

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.Keyword, Long.MAX_VALUE);
        discoveryQos.addCustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword);
        discoveryQos.setProviderMustSupportOnChange(true);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);
        ArbitrationResult capturedArbitrationResult = captureArbitrationResultByArbitrationCallbackOnSuccess();

        Set<DiscoveryEntryWithMetaInfo> expectedSelectedDiscoveryEntries = new HashSet<>(Arrays.asList(expectedDiscoveryEntry));
        Set<DiscoveryEntryWithMetaInfo> expectedOtherDiscoveryEntries = new HashSet<>(Arrays.asList(otherDiscoveryEntry));
        ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedSelectedDiscoveryEntries,
                                                                            expectedOtherDiscoveryEntries);
        assertEquals(expectedArbitrationResult, capturedArbitrationResult);
    }

    @Test
    public void testLastSeenArbitrator() throws InterruptedException {
        ProviderQos providerQos = new ProviderQos();

        DiscoveryEntryWithMetaInfo expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                           domain,
                                                                                           TestInterface.INTERFACE_NAME,
                                                                                           expectedParticipantId,
                                                                                           providerQos,
                                                                                           333L,
                                                                                           NO_EXPIRY,
                                                                                           publicKeyId,
                                                                                           true);
        DiscoveryEntryWithMetaInfo otherDiscoveryEntry1 = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                         domain,
                                                                                         TestInterface.INTERFACE_NAME,
                                                                                         "wrongParticipantId1",
                                                                                         providerQos,
                                                                                         111L,
                                                                                         NO_EXPIRY,
                                                                                         publicKeyId,
                                                                                         true);
        DiscoveryEntryWithMetaInfo otherDiscoveryEntry2 = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                         domain,
                                                                                         TestInterface.INTERFACE_NAME,
                                                                                         "wrongParticipantId2",
                                                                                         providerQos,
                                                                                         222L,
                                                                                         NO_EXPIRY,
                                                                                         publicKeyId,
                                                                                         true);

        capabilitiesList.add(expectedDiscoveryEntry);
        capabilitiesList.add(otherDiscoveryEntry1);
        capabilitiesList.add(otherDiscoveryEntry2);

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.LastSeen, Long.MAX_VALUE);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);
        ArbitrationResult capturedArbitrationResult = captureArbitrationResultByArbitrationCallbackOnSuccess();

        Set<DiscoveryEntryWithMetaInfo> expectedSelectedDiscoveryEntries = new HashSet<>(Arrays.asList(expectedDiscoveryEntry));
        Set<DiscoveryEntryWithMetaInfo> expectedOtherDiscoveryEntries = new HashSet<>(Arrays.asList(otherDiscoveryEntry1,
                                                                                                    otherDiscoveryEntry2));
        ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedSelectedDiscoveryEntries,
                                                                            expectedOtherDiscoveryEntries);
        assertEquals(expectedArbitrationResult, capturedArbitrationResult);
    }

    @Test
    public void testPriorityArbitrator() throws InterruptedException {
        ProviderQos providerQos = new ProviderQos();
        providerQos.setPriority(testPriority);

        expectedEndpointAddress = testAddress;
        DiscoveryEntryWithMetaInfo expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                           domain,
                                                                                           TestInterface.INTERFACE_NAME,
                                                                                           expectedParticipantId,
                                                                                           providerQos,
                                                                                           System.currentTimeMillis(),
                                                                                           NO_EXPIRY,
                                                                                           publicKeyId,
                                                                                           true);
        capabilitiesList.add(expectedDiscoveryEntry);

        long lessPrior = 1;
        ProviderQos providerQos2 = new ProviderQos();
        providerQos2.setPriority(lessPrior);

        DiscoveryEntryWithMetaInfo otherDiscoverEntry1 = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                        domain,
                                                                                        TestInterface.INTERFACE_NAME,
                                                                                        "wrongParticipantId",
                                                                                        providerQos2,
                                                                                        System.currentTimeMillis(),
                                                                                        NO_EXPIRY,
                                                                                        publicKeyId,
                                                                                        true);
        capabilitiesList.add(otherDiscoverEntry1);
        long negativePriority = -10;
        ProviderQos providerQos3 = new ProviderQos();
        providerQos3.setPriority(negativePriority);

        Address thirdEndpointAddress = new MqttAddress(testAddress.getBrokerUri(), "topic1");
        ArrayList<Address> thirdEndpointAddresses = new ArrayList<Address>();
        thirdEndpointAddresses.add(thirdEndpointAddress);
        DiscoveryEntryWithMetaInfo otherDiscoverEntry2 = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                        domain,
                                                                                        TestInterface.INTERFACE_NAME,
                                                                                        "thirdParticipantId",
                                                                                        providerQos3,
                                                                                        System.currentTimeMillis(),
                                                                                        NO_EXPIRY,
                                                                                        publicKeyId,
                                                                                        true);
        capabilitiesList.add(otherDiscoverEntry2);

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);
        ArbitrationResult capturedArbitrationResult = captureArbitrationResultByArbitrationCallbackOnSuccess();

        Set<DiscoveryEntryWithMetaInfo> expectedSelectedDiscoveryEntries = new HashSet<>(Arrays.asList(expectedDiscoveryEntry));
        Set<DiscoveryEntryWithMetaInfo> expectedOtherDiscoveryEntries = new HashSet<>(Arrays.asList(otherDiscoverEntry1,
                                                                                                    otherDiscoverEntry2));
        ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedSelectedDiscoveryEntries,
                                                                            expectedOtherDiscoveryEntries);
        assertEquals(expectedArbitrationResult, capturedArbitrationResult);
    }

    @Test
    public void testPriorityArbitratorWithOnlyNegativePriorities() throws InterruptedException {
        ProviderQos providerQos = new ProviderQos();
        providerQos.setPriority(Long.MIN_VALUE);

        expectedEndpointAddress = testAddress;
        ArrayList<Address> expectedEndpointAddresses = new ArrayList<Address>();
        expectedEndpointAddresses.add(expectedEndpointAddress);
        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            expectedParticipantId,
                                                            providerQos,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));
        ProviderQos providerQos2 = new ProviderQos();
        providerQos2.setPriority(Long.MIN_VALUE);

        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "wrongParticipantId",
                                                            providerQos2,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));
        long negativePriority = Long.MIN_VALUE;
        ProviderQos providerQos3 = new ProviderQos();
        providerQos3.setPriority(negativePriority);

        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "thirdParticipantId",
                                                            providerQos3,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);

        verify(arbitrationCallback, times(1)).onError(any(Throwable.class));
        verify(arbitrationCallback, never()).onSuccess(any(ArbitrationResult.class));
    }

    @Test
    public void testPriorityArbitratorOnChangeSubscriptions() throws InterruptedException {
        // Expected provider supports onChangeSubscriptions
        ProviderQos providerQos = new ProviderQos();
        providerQos.setPriority(testPriority);
        providerQos.setSupportsOnChangeSubscriptions(true);

        expectedEndpointAddress = testAddress;
        DiscoveryEntryWithMetaInfo expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                           domain,
                                                                                           TestInterface.INTERFACE_NAME,
                                                                                           expectedParticipantId,
                                                                                           providerQos,
                                                                                           System.currentTimeMillis(),
                                                                                           NO_EXPIRY,
                                                                                           publicKeyId,
                                                                                           true);
        capabilitiesList.add(expectedDiscoveryEntry);

        // A provider with a higher priority that does not support onChangeSubscriptions
        ProviderQos providerQos2 = new ProviderQos();
        providerQos2.setPriority(testPriority + 1);
        providerQos2.setSupportsOnChangeSubscriptions(false);

        Address otherEndpointAddress = new MqttAddress(testAddress.getBrokerUri(), "topic1");
        ArrayList<Address> otherEndpointAddresses = new ArrayList<Address>();
        otherEndpointAddresses.add(otherEndpointAddress);
        DiscoveryEntryWithMetaInfo otherDiscoveryEntry1 = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                         domain,
                                                                                         TestInterface.INTERFACE_NAME,
                                                                                         "wrongParticipantId",
                                                                                         providerQos2,
                                                                                         System.currentTimeMillis(),
                                                                                         NO_EXPIRY,
                                                                                         publicKeyId,
                                                                                         true);
        capabilitiesList.add(otherDiscoveryEntry1);
        // A provider with a higher priority that does not support onChangeSubscriptions
        ProviderQos providerQos3 = new ProviderQos();
        providerQos3.setPriority(testPriority + 2);
        providerQos3.setSupportsOnChangeSubscriptions(false);

        Address thirdEndpointAddress = new MqttAddress(testAddress.getBrokerUri(), "topic2");
        ArrayList<Address> thirdEndpointAddresses = new ArrayList<Address>();
        thirdEndpointAddresses.add(thirdEndpointAddress);
        DiscoveryEntryWithMetaInfo otherDiscoveryEntry2 = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                         domain,
                                                                                         TestInterface.INTERFACE_NAME,
                                                                                         "thirdParticipantId",
                                                                                         providerQos3,
                                                                                         System.currentTimeMillis(),
                                                                                         NO_EXPIRY,
                                                                                         publicKeyId,
                                                                                         true);
        capabilitiesList.add(otherDiscoveryEntry2);

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);
        discoveryQos.setProviderMustSupportOnChange(true);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);
        ArbitrationResult capturedArbitrationResult = captureArbitrationResultByArbitrationCallbackOnSuccess();

        Set<DiscoveryEntryWithMetaInfo> expectedSelectedDiscoveryEntries = new HashSet<>(Arrays.asList(expectedDiscoveryEntry));
        Set<DiscoveryEntryWithMetaInfo> expectedOtherDiscoveryEntries = new HashSet<>(Arrays.asList(otherDiscoveryEntry1,
                                                                                                    otherDiscoveryEntry2));
        ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedSelectedDiscoveryEntries,
                                                                            expectedOtherDiscoveryEntries);
        assertEquals(expectedArbitrationResult, capturedArbitrationResult);
    }

    @Test
    public void testFixedParticipantIdArbitrator() throws InterruptedException {
        ProviderQos providerQos = new ProviderQos();

        // Adding two dummy entries and one entry with "fixedParticipantId"
        DiscoveryEntryWithMetaInfo dummyEntry1 = new DiscoveryEntryWithMetaInfo(interfaceVersion,
                                                                                domain,
                                                                                interfaceName,
                                                                                "dummyParticipantId1",
                                                                                providerQos,
                                                                                System.currentTimeMillis(),
                                                                                NO_EXPIRY,
                                                                                publicKeyId,
                                                                                true);
        DiscoveryEntryWithMetaInfo dummyEntry2 = new DiscoveryEntryWithMetaInfo(interfaceVersion,
                                                                                domain,
                                                                                interfaceName,
                                                                                "dummyParticipantId2",
                                                                                providerQos,
                                                                                System.currentTimeMillis(),
                                                                                NO_EXPIRY,
                                                                                publicKeyId,
                                                                                true);

        DiscoveryEntryWithMetaInfo anotherDiscoveryEntry = new DiscoveryEntryWithMetaInfo(interfaceVersion,
                                                                                          domain,
                                                                                          interfaceName,
                                                                                          expectedFixedParticipantId,
                                                                                          providerQos,
                                                                                          System.currentTimeMillis(),
                                                                                          NO_EXPIRY,
                                                                                          publicKeyId,
                                                                                          true);

        capabilitiesList.add(dummyEntry1);
        capabilitiesList.add(dummyEntry2);
        capabilitiesList.add(anotherDiscoveryEntry);

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.FixedChannel, Long.MAX_VALUE);

        discoveryQos.addCustomParameter(ArbitrationConstants.FIXEDPARTICIPANT_KEYWORD, expectedFixedParticipantId);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);
        ArbitrationResult capturedArbitrationResult = captureArbitrationResultByArbitrationCallbackOnSuccess();

        Set<DiscoveryEntryWithMetaInfo> expectedSelectedDiscoveryEntries = new HashSet<>(Arrays.asList(anotherDiscoveryEntry));
        Set<DiscoveryEntryWithMetaInfo> expectedOtherDiscoveryEntries = new HashSet<>(Arrays.asList(dummyEntry1,
                                                                                                    dummyEntry2));
        ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedSelectedDiscoveryEntries,
                                                                            expectedOtherDiscoveryEntries);
        assertEquals(expectedArbitrationResult, capturedArbitrationResult);
    }

    @Test
    public void testCustomArbitrationFunction() throws InterruptedException {
        // Expected provider supports onChangeSubscriptions
        ProviderQos providerQos = new ProviderQos();

        expectedEndpointAddress = testAddress;
        DiscoveryEntryWithMetaInfo discoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                   domain,
                                                                                   TestInterface.INTERFACE_NAME,
                                                                                   expectedParticipantId,
                                                                                   providerQos,
                                                                                   System.currentTimeMillis(),
                                                                                   NO_EXPIRY,
                                                                                   publicKeyId,
                                                                                   true);
        Set<DiscoveryEntryWithMetaInfo> expectedSelectedDiscoveryEntries = new HashSet<>(Arrays.asList(discoveryEntry));
        ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedSelectedDiscoveryEntries, null);

        capabilitiesList.add(discoveryEntry);

        ArbitrationStrategyFunction arbitrationStrategyFunction = mock(ArbitrationStrategyFunction.class);
        when(arbitrationStrategyFunction.select(Matchers.<Map<String, String>> any(),
                                                Matchers.<Collection<DiscoveryEntryWithMetaInfo>> any())).thenReturn(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(discoveryEntry)));
        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, arbitrationStrategyFunction, Long.MAX_VALUE);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);

        verify(arbitrationStrategyFunction,
               times(1)).select(eq(discoveryQos.getCustomParameters()),
                                eq(new HashSet<DiscoveryEntryWithMetaInfo>(capabilitiesList)));

        verify(arbitrationCallback, times(1)).onSuccess(eq(expectedArbitrationResult));
    }

    @Test
    public void testCustomArbitrationFunctionMultipleMatches() throws InterruptedException {
        // Expected provider supports onChangeSubscriptions
        ProviderQos providerQos = new ProviderQos();

        String publicKeyId = "publicKeyId";

        expectedEndpointAddress = testAddress;
        String[] participantIds = new String[]{ "first-participant", "second-participant" };
        for (int index = 0; index < participantIds.length; index++) {
            DiscoveryEntryWithMetaInfo discoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                       domain,
                                                                                       TestInterface.INTERFACE_NAME,
                                                                                       participantIds[index],
                                                                                       providerQos,
                                                                                       System.currentTimeMillis(),
                                                                                       NO_EXPIRY,
                                                                                       publicKeyId,
                                                                                       true);
            capabilitiesList.add(discoveryEntry);
        }

        ArbitrationStrategyFunction arbitrationStrategyFunction = mock(ArbitrationStrategyFunction.class);
        when(arbitrationStrategyFunction.select(Matchers.<Map<String, String>> any(),
                                                Matchers.<Collection<DiscoveryEntryWithMetaInfo>> any())).thenReturn(new HashSet<DiscoveryEntryWithMetaInfo>(capabilitiesList));
        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, arbitrationStrategyFunction, Long.MAX_VALUE);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);

        verify(arbitrationStrategyFunction,
               times(1)).select(eq(discoveryQos.getCustomParameters()),
                                eq(new HashSet<DiscoveryEntryWithMetaInfo>(capabilitiesList)));

        Set<DiscoveryEntryWithMetaInfo> expectedSelectedDiscoveryEntries = new HashSet<>(capabilitiesList);
        ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedSelectedDiscoveryEntries, null);
        verify(arbitrationCallback, times(1)).onSuccess(eq(expectedArbitrationResult));
    }

    @Test
    public void testVersionFilterUsed() throws InterruptedException {
        ProviderQos providerQos = new ProviderQos();
        String publicKeyId = "publicKeyId";
        expectedEndpointAddress = testAddress;
        String[] participantIds = new String[]{ "first-participant", "second-participant" };
        for (int index = 0; index < participantIds.length; index++) {
            DiscoveryEntryWithMetaInfo discoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(index, index),
                                                                                       domain,
                                                                                       TestInterface.INTERFACE_NAME,
                                                                                       participantIds[index],
                                                                                       providerQos,
                                                                                       System.currentTimeMillis(),
                                                                                       NO_EXPIRY,
                                                                                       publicKeyId,
                                                                                       true);
            capabilitiesList.add(discoveryEntry);
        }

        ArbitrationStrategyFunction arbitrationStrategyFunction = mock(ArbitrationStrategyFunction.class);
        when(arbitrationStrategyFunction.select(Matchers.<Map<String, String>> any(),
                                                Matchers.<Collection<DiscoveryEntryWithMetaInfo>> any())).thenReturn(new HashSet<DiscoveryEntryWithMetaInfo>(capabilitiesList));
        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, arbitrationStrategyFunction, Long.MAX_VALUE);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);

        verify(discoveryEntryVersionFilter).filter(interfaceVersion,
                                                   new HashSet<DiscoveryEntryWithMetaInfo>(capabilitiesList),
                                                   new HashMap<String, Set<Version>>());
    }

    @Test
    public void testArbitrationResultStoresAllNonSelectedEntries() throws InterruptedException {
        ProviderQos providerQos = new ProviderQos();
        providerQos.setSupportsOnChangeSubscriptions(true);
        DiscoveryEntryWithMetaInfo expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(interfaceVersion,
                                                                                           domain,
                                                                                           TestInterface.INTERFACE_NAME,
                                                                                           expectedParticipantId,
                                                                                           providerQos,
                                                                                           333L,
                                                                                           NO_EXPIRY,
                                                                                           publicKeyId,
                                                                                           true);

        // Add two other entries that do not support onChangeSubscription and will be filtered
        ProviderQos providerQos2 = new ProviderQos();
        providerQos2.setSupportsOnChangeSubscriptions(false);
        DiscoveryEntryWithMetaInfo otherDiscoveryEntry1 = new DiscoveryEntryWithMetaInfo(interfaceVersion,
                                                                                         domain,
                                                                                         TestInterface.INTERFACE_NAME,
                                                                                         "otherParticipantId1",
                                                                                         providerQos2,
                                                                                         111L,
                                                                                         NO_EXPIRY,
                                                                                         publicKeyId,
                                                                                         true);
        DiscoveryEntryWithMetaInfo otherDiscoveryEntry2 = new DiscoveryEntryWithMetaInfo(interfaceVersion,
                                                                                         domain,
                                                                                         TestInterface.INTERFACE_NAME,
                                                                                         "otherParticipantId2",
                                                                                         providerQos2,
                                                                                         222L,
                                                                                         NO_EXPIRY,
                                                                                         publicKeyId,
                                                                                         true);

        capabilitiesList.add(expectedDiscoveryEntry);
        capabilitiesList.add(otherDiscoveryEntry1);
        capabilitiesList.add(otherDiscoveryEntry2);

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.LastSeen, Long.MAX_VALUE);
        discoveryQos.setProviderMustSupportOnChange(true);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);
        ArbitrationResult capturedArbitrationResult = captureArbitrationResultByArbitrationCallbackOnSuccess();

        Set<DiscoveryEntryWithMetaInfo> expectedSelectedDiscoveryEntries = new HashSet<>(Arrays.asList(expectedDiscoveryEntry));
        Set<DiscoveryEntryWithMetaInfo> expectedOtherDiscoveryEntries = new HashSet<>(Arrays.asList(otherDiscoveryEntry1,
                                                                                                    otherDiscoveryEntry2));
        ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedSelectedDiscoveryEntries,
                                                                            expectedOtherDiscoveryEntries);
        assertEquals(expectedArbitrationResult, capturedArbitrationResult);
    }

    @Test
    public void testIncompatibleVersionsReported() throws InterruptedException {
        Version incompatibleVersion = new Version(100, 100);
        final Collection<DiscoveryEntryWithMetaInfo> discoveryEntries = Arrays.asList((new DiscoveryEntryWithMetaInfo(incompatibleVersion,
                                                                                                                      domain,
                                                                                                                      interfaceName,
                                                                                                                      "first-participant",
                                                                                                                      new ProviderQos(),
                                                                                                                      System.currentTimeMillis(),
                                                                                                                      NO_EXPIRY,
                                                                                                                      "public-key-1",
                                                                                                                      true)));
        ArbitrationStrategyFunction arbitrationStrategyFunction = mock(ArbitrationStrategyFunction.class);
        when(arbitrationStrategyFunction.select(Mockito.<Map<String, String>> any(),
                                                Mockito.<Collection<DiscoveryEntryWithMetaInfo>> any())).thenReturn(new HashSet<DiscoveryEntryWithMetaInfo>());
        doAnswer(new Answer<Set<DiscoveryEntryWithMetaInfo>>() {
            @SuppressWarnings("unchecked")
            @Override
            public Set<DiscoveryEntryWithMetaInfo> answer(InvocationOnMock invocation) throws Throwable {
                Map<String, Set<Version>> filteredVersions = (Map<String, Set<Version>>) invocation.getArguments()[2];
                Set<DiscoveryEntryWithMetaInfo> discoveryEntries = (Set<DiscoveryEntryWithMetaInfo>) invocation.getArguments()[1];
                filteredVersions.put(domain,
                                     new HashSet<Version>(Arrays.asList(discoveryEntries.iterator()
                                                                                        .next()
                                                                                        .getProviderVersion())));
                discoveryEntries.clear();
                return new HashSet<>();
            }
        }).when(discoveryEntryVersionFilter)
          .filter(Mockito.<Version> any(),
                  Mockito.<Set<DiscoveryEntryWithMetaInfo>> any(),
                  Mockito.<Map<String, Set<Version>>> any());
        DiscoveryQos discoveryQos = new DiscoveryQos(10L, arbitrationStrategyFunction, 0L);
        reset(localDiscoveryAggregator);
        doAnswer(new Answer<Object>() {

            @SuppressWarnings("unchecked")
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                ((Callback<DiscoveryEntryWithMetaInfo[]>) invocation.getArguments()[0]).resolve((Object) discoveryEntries.toArray(new DiscoveryEntryWithMetaInfo[1]));
                localDiscoveryAggregatorSemaphore.release();
                return null;
            }
        }).when(localDiscoveryAggregator)
          .lookup(Mockito.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                  eq(new String[]{ domain }),
                  eq(interfaceName),
                  Mockito.<joynr.types.DiscoveryQos> any(),
                  Mockito.<String[]> any());

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);

        Set<Version> discoveredVersions = new HashSet<>(Arrays.asList(incompatibleVersion));
        ArgumentCaptor<NoCompatibleProviderFoundException> noCompatibleProviderFoundExceptionCaptor = ArgumentCaptor.forClass(NoCompatibleProviderFoundException.class);
        verify(arbitrationCallback).onError(noCompatibleProviderFoundExceptionCaptor.capture());
        assertEquals(discoveredVersions, noCompatibleProviderFoundExceptionCaptor.getValue().getDiscoveredVersions());

    }

    @Test
    public void testMultiDomainIncompatibleVersionsReported() throws InterruptedException {
        final Version incompatibleVersion = new Version(100, 100);
        final String domain1 = "domain1";
        final String domain2 = "domain2";
        final DiscoveryEntryWithMetaInfo discoveryEntry1 = new DiscoveryEntryWithMetaInfo(incompatibleVersion,
                                                                                          domain1,
                                                                                          interfaceName,
                                                                                          "participant1",
                                                                                          new ProviderQos(),
                                                                                          System.currentTimeMillis(),
                                                                                          NO_EXPIRY,
                                                                                          "public-key-1",
                                                                                          true);
        final DiscoveryEntryWithMetaInfo discoveryEntry2 = new DiscoveryEntryWithMetaInfo(incompatibleVersion,
                                                                                          domain2,
                                                                                          interfaceName,
                                                                                          "participant2",
                                                                                          new ProviderQos(),
                                                                                          System.currentTimeMillis(),
                                                                                          NO_EXPIRY,
                                                                                          "public-key-2",
                                                                                          true);
        final Collection<DiscoveryEntryWithMetaInfo> discoveryEntries = Arrays.asList(discoveryEntry1, discoveryEntry2);

        ArbitrationStrategyFunction arbitrationStrategyFunction = mock(ArbitrationStrategyFunction.class);
        when(arbitrationStrategyFunction.select(Mockito.<Map<String, String>> any(),
                                                Mockito.<Collection<DiscoveryEntryWithMetaInfo>> any())).thenReturn(new HashSet<DiscoveryEntryWithMetaInfo>());
        doAnswer(new Answer<Set<DiscoveryEntry>>() {
            @SuppressWarnings("unchecked")
            @Override
            public Set<DiscoveryEntry> answer(InvocationOnMock invocation) throws Throwable {
                Set<DiscoveryEntry> discoveryEntries = (Set<DiscoveryEntry>) invocation.getArguments()[1];
                Map<String, Set<Version>> filteredVersions = (Map<String, Set<Version>>) invocation.getArguments()[2];
                filteredVersions.put(domain1,
                                     new HashSet<Version>(Arrays.asList(discoveryEntry1.getProviderVersion())));
                filteredVersions.put(domain2,
                                     new HashSet<Version>(Arrays.asList(discoveryEntry2.getProviderVersion())));
                discoveryEntries.clear();
                return new HashSet<>();
            }
        }).when(discoveryEntryVersionFilter)
          .filter(Mockito.<Version> any(),
                  Mockito.<Set<DiscoveryEntryWithMetaInfo>> any(),
                  Mockito.<Map<String, Set<Version>>> any());
        DiscoveryQos discoveryQos = new DiscoveryQos(10L, arbitrationStrategyFunction, 0L);
        reset(localDiscoveryAggregator);
        doAnswer(new Answer<Object>() {

            @SuppressWarnings("unchecked")
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                ((Callback<DiscoveryEntryWithMetaInfo[]>) invocation.getArguments()[0]).resolve((Object) discoveryEntries.toArray(new DiscoveryEntryWithMetaInfo[2]));
                localDiscoveryAggregatorSemaphore.release();
                return null;
            }
        }).when(localDiscoveryAggregator)
          .lookup(Mockito.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                  any(String[].class),
                  eq(interfaceName),
                  Mockito.<joynr.types.DiscoveryQos> any(),
                  Mockito.<String[]> any());

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos, domain1, domain2);

        Set<Version> discoveredVersions = new HashSet<>(Arrays.asList(incompatibleVersion));
        ArgumentCaptor<MultiDomainNoCompatibleProviderFoundException> noCompatibleProviderFoundExceptionCaptor = ArgumentCaptor.forClass(MultiDomainNoCompatibleProviderFoundException.class);
        verify(arbitrationCallback).onError(noCompatibleProviderFoundExceptionCaptor.capture());
        assertEquals(discoveredVersions,
                     noCompatibleProviderFoundExceptionCaptor.getValue().getDiscoveredVersionsForDomain(domain2));
        assertEquals(discoveredVersions,
                     noCompatibleProviderFoundExceptionCaptor.getValue().getDiscoveredVersionsForDomain(domain1));

    }

    @Test
    public void useRemainingDiscoveryTimeoutForLookupRetries() throws InterruptedException {
        String[] gbids = null;
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(ARBITRATION_TIMEOUT);
        final long retryInterval = ARBITRATION_TIMEOUT / 3 * 2;
        discoveryQos.setRetryIntervalMs(retryInterval);

        Set<String> domainsSet = new HashSet<String>(Arrays.asList(domain));
        Arbitrator arbitrator = ArbitratorFactory.create(domainsSet,
                                                         interfaceName,
                                                         interfaceVersion,
                                                         discoveryQos,
                                                         localDiscoveryAggregator,
                                                         gbids);
        arbitrator.setArbitrationListener(arbitrationCallback);

        arbitrator.scheduleArbitration();

        assertTrue(localDiscoveryAggregatorSemaphore.tryAcquire(100, TimeUnit.MILLISECONDS));
        assertFalse(localDiscoveryAggregatorSemaphore.tryAcquire(ARBITRATION_TIMEOUT / 2, TimeUnit.MILLISECONDS));
        assertTrue(localDiscoveryAggregatorSemaphore.tryAcquire(ARBITRATION_TIMEOUT, TimeUnit.MILLISECONDS));

        verify(arbitrationCallback, times(1)).onError((isA(DiscoveryException.class)));
        verify(localDiscoveryAggregator,
               times(2)).lookup(Mockito.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                                eq(new String[]{ domain }),
                                eq(interfaceName),
                                discoveryQosCaptor.capture(),
                                Mockito.<String[]> any());

        List<joynr.types.DiscoveryQos> dQosList = discoveryQosCaptor.getAllValues();
        //initial timeout
        assertTrue(dQosList.get(0).getDiscoveryTimeout() > retryInterval);
        assertTrue(dQosList.get(0).getDiscoveryTimeout() <= ARBITRATION_TIMEOUT);
        //remaining timeout
        assertTrue(dQosList.get(1).getDiscoveryTimeout() > 0);
        assertTrue(dQosList.get(1).getDiscoveryTimeout() <= (ARBITRATION_TIMEOUT - retryInterval));
    }

    @Test
    public void doNotRetryLookupIfRetryIntervalIsLargerThanRemainingTimeout() throws InterruptedException {
        String[] gbids = null;
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(ARBITRATION_TIMEOUT);
        final long retryInterval = (ARBITRATION_TIMEOUT / 3) * 2;
        discoveryQos.setRetryIntervalMs(retryInterval);

        Set<String> domainsSet = new HashSet<String>(Arrays.asList(domain));
        Arbitrator arbitrator = ArbitratorFactory.create(domainsSet,
                                                         interfaceName,
                                                         interfaceVersion,
                                                         discoveryQos,
                                                         localDiscoveryAggregator,
                                                         gbids);
        arbitrator.setArbitrationListener(arbitrationCallback);

        Thread.sleep(ARBITRATION_TIMEOUT - retryInterval + 1);
        arbitrator.scheduleArbitration();

        assertTrue(localDiscoveryAggregatorSemaphore.tryAcquire(100, TimeUnit.MILLISECONDS));
        verify(arbitrationCallback, times(1)).onError((isA(DiscoveryException.class)));

        assertFalse(localDiscoveryAggregatorSemaphore.tryAcquire(ARBITRATION_TIMEOUT, TimeUnit.MILLISECONDS));
    }

    private Answer<Future<Void>> createAnswerWithDiscoveryError(DiscoveryError error) {
        return new Answer<Future<Void>>() {

            @Override
            public Future<Void> answer(InvocationOnMock invocation) throws Throwable {
                Object[] args = invocation.getArguments();
                @SuppressWarnings("unchecked")
                CallbackWithModeledError<Void, DiscoveryError> callback = ((CallbackWithModeledError<Void, DiscoveryError>) args[0]);
                callback.onFailure(error);
                localDiscoveryAggregatorSemaphore.release();
                return null;
            }
        };
    }

    private Answer<Future<Void>> createAnswerWithJoynrRuntimeException(JoynrRuntimeException error) {
        return new Answer<Future<Void>>() {

            @Override
            public Future<Void> answer(InvocationOnMock invocation) throws Throwable {
                Object[] args = invocation.getArguments();
                @SuppressWarnings("unchecked")
                CallbackWithModeledError<Void, DiscoveryError> callback = ((CallbackWithModeledError<Void, DiscoveryError>) args[0]);
                callback.onFailure(error);
                localDiscoveryAggregatorSemaphore.release();
                return null;
            }
        };
    }

    @Test
    public void testJoynrRuntimeExceptionFromLocalDiscoveryAggregatorToArbitrationCallbackReported_JoynrShutdownException_WithoutRetry() throws InterruptedException {
        JoynrShutdownException exception = new JoynrShutdownException("");
        JoynrShutdownException expectedException = new JoynrShutdownException("");
        testJoynrExceptionFromLocalDiscoveryAggregatorToArbitrationCallbackReported(exception,
                                                                                    expectedException,
                                                                                    false,
                                                                                    false);
    }

    @Test
    public void testJoynrRuntimeExceptionFromLocalDiscoveryAggregatorToArbitrationCallbackReported_JoynrShutdownException_WithRetry_failsImmediately() throws InterruptedException {
        JoynrShutdownException exception = new JoynrShutdownException("");
        JoynrShutdownException expectedException = new JoynrShutdownException("");
        testJoynrExceptionFromLocalDiscoveryAggregatorToArbitrationCallbackReported(exception,
                                                                                    expectedException,
                                                                                    true,
                                                                                    false);
    }

    @Test
    public void testJoynrRuntimeExceptionFromLocalDiscoveryAggregatorToArbitrationCallbackReported_JoynrRuntimeException_WithoutRetry() throws InterruptedException {
        JoynrRuntimeException exception = new JoynrRuntimeException("");
        JoynrRuntimeException expectedException = new JoynrRuntimeException("");
        testJoynrExceptionFromLocalDiscoveryAggregatorToArbitrationCallbackReported(exception,
                                                                                    expectedException,
                                                                                    false,
                                                                                    false);
    }

    @Test
    public void testJoynrRuntimeExceptionFromLocalDiscoveryAggregatorToArbitrationCallbackReported_JoynrRuntimeException_WithRetry() throws InterruptedException {
        JoynrRuntimeException exception = new JoynrRuntimeException("");
        JoynrRuntimeException expectedException = new JoynrRuntimeException("");
        testJoynrExceptionFromLocalDiscoveryAggregatorToArbitrationCallbackReported(exception,
                                                                                    expectedException,
                                                                                    true,
                                                                                    true);
    }

    @Test
    public void testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported_withoutRetry_NO_ENTRY_FOR_PARTICIPANT() throws InterruptedException {
        String[] gbids = new String[]{ "joynrdefaultgbid" };
        DiscoveryError expectedError = DiscoveryError.NO_ENTRY_FOR_PARTICIPANT;
        testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported(gbids, expectedError, false, false);
    }

    @Test
    public void testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported_withoutRetry_NO_ENTRY_FOR_SELECTED_BACKENDS() throws InterruptedException {
        String[] gbids = new String[]{ "testgbid" };
        DiscoveryError expectedError = DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS;
        testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported(gbids, expectedError, false, false);
    }

    @Test
    public void testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported_withRetry_NO_ENTRY_FOR_PARTICIPANT() throws InterruptedException {
        String[] gbids = new String[]{ "joynrdefaultgbid" };
        DiscoveryError expectedError = DiscoveryError.NO_ENTRY_FOR_PARTICIPANT;
        testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported(gbids, expectedError, true, true);
    }

    @Test
    public void testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported_withRetry_NO_ENTRY_FOR_SELECTED_BACKENDS() throws InterruptedException {
        String[] gbids = new String[]{ "testgbid" };
        DiscoveryError expectedError = DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS;
        testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported(gbids, expectedError, true, true);
    }

    @Test
    public void testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported_withoutRetry_UNKNOWN_GBID() throws InterruptedException {
        String[] gbids = new String[]{ "unknowngbid" };
        DiscoveryError expectedError = DiscoveryError.UNKNOWN_GBID;
        testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported(gbids, expectedError, false, false);
    }

    @Test
    public void testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported_withoutRetry_INVALID_GBID() throws InterruptedException {
        String[] gbids = new String[]{ "invalidGbid" };
        DiscoveryError expectedError = DiscoveryError.INVALID_GBID;
        testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported(gbids, expectedError, false, false);
    }

    @Test
    public void testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported_withoutRetry_INTERNAL_ERROR() throws InterruptedException {
        String[] gbids = new String[]{ "anygbid" };
        DiscoveryError expectedError = DiscoveryError.INTERNAL_ERROR;
        testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported(gbids, expectedError, false, false);
    }

    @Test
    public void testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported_withRetry_failsImmediately_UNKNOWN_GBID() throws InterruptedException {
        String[] gbids = new String[]{ "unknowngbid" };
        DiscoveryError expectedError = DiscoveryError.UNKNOWN_GBID;
        testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported(gbids, expectedError, true, false);
    }

    @Test
    public void testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported_withRetry_failsImmediately_INVALID_GBID() throws InterruptedException {
        String[] gbids = new String[]{ "invalidGbid" };
        DiscoveryError expectedError = DiscoveryError.INVALID_GBID;
        testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported(gbids, expectedError, true, false);
    }

    @Test
    public void testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported_withRetry_failsImmediately_INTERNAL_ERROR() throws InterruptedException {
        String[] gbids = new String[]{ "anygbid" };
        DiscoveryError expectedError = DiscoveryError.INTERNAL_ERROR;
        testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported(gbids, expectedError, true, false);
    }

    @Test
    public void lookupForGuidedProxyBuilder() {
        Set<String> domainsSet = new HashSet<String>(Arrays.asList(domain));
        Arbitrator arbitrator = ArbitratorFactory.create(domainsSet,
                                                         interfaceName,
                                                         interfaceVersion,
                                                         new DiscoveryQos(),
                                                         localDiscoveryAggregator,
                                                         new String[]{});
        arbitrator.setArbitrationListener(arbitrationCallback);
        arbitrator.lookup();
        verify(localDiscoveryAggregator).lookup(Mockito.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                                                any(String[].class),
                                                any(String.class),
                                                any(joynr.types.DiscoveryQos.class),
                                                any(String[].class));
    }

    @Test
    public void lookupForGuidedProxyBuilderWithRetry() throws InterruptedException {
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(ARBITRATION_TIMEOUT);
        final long retryInterval = ARBITRATION_TIMEOUT / 3 * 2;
        discoveryQos.setRetryIntervalMs(retryInterval);

        Set<String> domainsSet = new HashSet<String>(Arrays.asList(domain));
        Arbitrator arbitrator = ArbitratorFactory.create(domainsSet,
                                                         interfaceName,
                                                         interfaceVersion,
                                                         discoveryQos,
                                                         localDiscoveryAggregator,
                                                         new String[]{});
        arbitrator.setArbitrationListener(arbitrationCallback);
        arbitrator.lookup();

        assertTrue(localDiscoveryAggregatorSemaphore.tryAcquire(100, TimeUnit.MILLISECONDS));
        assertFalse(localDiscoveryAggregatorSemaphore.tryAcquire(ARBITRATION_TIMEOUT / 2, TimeUnit.MILLISECONDS));
        assertTrue(localDiscoveryAggregatorSemaphore.tryAcquire(ARBITRATION_TIMEOUT, TimeUnit.MILLISECONDS));

        verify(arbitrationCallback, times(1)).onError((isA(DiscoveryException.class)));

        verify(localDiscoveryAggregator,
               times(2)).lookup(Mockito.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                                any(String[].class),
                                any(String.class),
                                discoveryQosCaptor.capture(),
                                any(String[].class));

        List<joynr.types.DiscoveryQos> dQosList = discoveryQosCaptor.getAllValues();
        //initial timeout
        assertTrue(dQosList.get(0).getDiscoveryTimeout() > retryInterval);
        assertTrue(dQosList.get(0).getDiscoveryTimeout() <= ARBITRATION_TIMEOUT);
        //remaining timeout
        assertTrue(dQosList.get(1).getDiscoveryTimeout() > 0);
        assertTrue(dQosList.get(1).getDiscoveryTimeout() <= (ARBITRATION_TIMEOUT - retryInterval));
    }

    private void testErrorFromLocalDiscoveryAggregatorToArbitrationCallbackReported(String[] gbids,
                                                                                    DiscoveryError expectedError,
                                                                                    boolean withRetry,
                                                                                    boolean expectRetry) throws InterruptedException {
        String[] domains = new String[]{ domain };
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(ARBITRATION_TIMEOUT);
        if (withRetry) {
            discoveryQos.setRetryIntervalMs(ARBITRATION_TIMEOUT / 2 + 50);
        } else {
            discoveryQos.setRetryIntervalMs(ARBITRATION_TIMEOUT + 2);
        }

        doAnswer(createAnswerWithDiscoveryError(expectedError)).when(localDiscoveryAggregator)
                                                               .lookup(Mockito.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                                                                       any(String[].class),
                                                                       any(String.class),
                                                                       any(joynr.types.DiscoveryQos.class),
                                                                       any(String[].class));

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos, gbids.clone(), domains);

        if (expectRetry) {
            // expect 1 retry
            assertTrue(localDiscoveryAggregatorSemaphore.tryAcquire(ARBITRATION_TIMEOUT, TimeUnit.MILLISECONDS));
            verify(localDiscoveryAggregator,
                   times(2)).lookup(Mockito.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                                    eq(domains),
                                    eq(interfaceName),
                                    any(joynr.types.DiscoveryQos.class),
                                    eq(gbids));
        } else {
            verify(localDiscoveryAggregator).lookup(Mockito.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                                                    eq(domains),
                                                    eq(interfaceName),
                                                    any(joynr.types.DiscoveryQos.class),
                                                    eq(gbids));
        }

        ArgumentCaptor<Throwable> throwableCaptor = ArgumentCaptor.forClass(Throwable.class);
        verify(arbitrationCallback, times(1)).onError(throwableCaptor.capture());
        verify(messageRouter, never()).setToKnown(any());
        verify(messageRouter, never()).removeNextHop(any());
        assertTrue(throwableCaptor.getValue() instanceof DiscoveryException);
        assertNotNull(throwableCaptor.getValue().getMessage());
        assertTrue((throwableCaptor.getValue().getMessage()).contains(interfaceName));
        assertTrue((throwableCaptor.getValue().getMessage()).contains(Arrays.toString(gbids)));
        assertTrue((throwableCaptor.getValue().getMessage()).contains(expectedError.toString()));
    }

    private void testJoynrExceptionFromLocalDiscoveryAggregatorToArbitrationCallbackReported(JoynrRuntimeException exception,
                                                                                             JoynrRuntimeException expectedException,
                                                                                             boolean withRetry,
                                                                                             boolean expectRetry) throws InterruptedException {
        String[] domains = new String[]{ domain };
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(ARBITRATION_TIMEOUT);

        if (withRetry) {
            discoveryQos.setRetryIntervalMs(ARBITRATION_TIMEOUT / 2 + 50);
        } else {
            discoveryQos.setRetryIntervalMs(ARBITRATION_TIMEOUT + 2);
        }

        doAnswer(createAnswerWithJoynrRuntimeException(exception)).when(localDiscoveryAggregator)
                                                                  .lookup(Mockito.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                                                                          any(String[].class),
                                                                          any(String.class),
                                                                          any(joynr.types.DiscoveryQos.class),
                                                                          any(String[].class));

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos, domains);

        if (expectRetry) {
            // expect 1 retry
            assertTrue(localDiscoveryAggregatorSemaphore.tryAcquire(ARBITRATION_TIMEOUT, TimeUnit.MILLISECONDS));
            verify(localDiscoveryAggregator,
                   times(2)).lookup(Mockito.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                                    eq(domains),
                                    eq(interfaceName),
                                    any(joynr.types.DiscoveryQos.class),
                                    any(String[].class));
        } else {
            verify(localDiscoveryAggregator,
                   times(1)).lookup(Mockito.<CallbackWithModeledError<DiscoveryEntryWithMetaInfo[], DiscoveryError>> any(),
                                    eq(domains),
                                    eq(interfaceName),
                                    any(joynr.types.DiscoveryQos.class),
                                    any(String[].class));
        }

        ArgumentCaptor<Throwable> throwableCaptor = ArgumentCaptor.forClass(Throwable.class);
        verify(arbitrationCallback, times(1)).onError(throwableCaptor.capture());
        verify(messageRouter, never()).setToKnown(any());
        verify(messageRouter, never()).removeNextHop(any());

        assertNotNull(throwableCaptor.getValue().getMessage());
        assertEquals(throwableCaptor.getValue(), expectedException);
    }

    @Test
    public void testDecRemoteRoutingEntryRefCountNotRequired() throws Exception {
        Set<String> domains = Collections.unmodifiableSet(new HashSet<>(Arrays.asList("a", "b", "c")));
        setUpProviderCapabilitiesForMultipleDomains(domains);
        createArbitratorWithCallbackAndAwaitArbitration(new DiscoveryQos(), domains.toArray(new String[0]));
        verify(arbitrationCallback).onSuccess(any());
        verify(messageRouter, never()).setToKnown(any());
        verify(messageRouter, never()).removeNextHop(any());
    }

    @Test
    public void testDecRemoteRoutingEntryRefCountIfDomainsIncomplete() throws Exception {
        Set<String> domains = Collections.unmodifiableSet(new HashSet<>(Arrays.asList("a", "b", "c")));
        Set<String> incompletDomains = new HashSet<>(domains);
        incompletDomains.remove(domains.iterator().next());
        Set<String> participantIDs = setUpProviderCapabilitiesForMultipleDomains(incompletDomains);
        createArbitratorWithCallbackAndAwaitArbitration(new DiscoveryQos(), domains.toArray(new String[0]));
        verify(arbitrationCallback, never()).onSuccess(any());
        for (String participantId : participantIDs) {
            InOrder incrementBeforeDecrement = Mockito.inOrder(messageRouter);
            incrementBeforeDecrement.verify(messageRouter, times(1)).setToKnown(participantId);
            incrementBeforeDecrement.verify(messageRouter, times(1)).removeNextHop(participantId);
        }
    }

    @Test
    public void testDecRemoteRoutingEntryRefCountIfStrategyNotMet() throws Exception {
        Set<String> domains = Collections.unmodifiableSet(new HashSet<>(Arrays.asList("a", "b", "c")));
        Set<String> participantIDs = setUpProviderCapabilitiesForMultipleDomains(domains);
        ArbitrationStrategyFunction arbitrationStrategyFunction = mock(ArbitrationStrategyFunction.class);
        when(arbitrationStrategyFunction.select(Matchers.<Map<String, String>> any(),
                                                Matchers.<Collection<DiscoveryEntryWithMetaInfo>> any())).thenReturn(null);
        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, arbitrationStrategyFunction, Long.MAX_VALUE);
        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos, domains.toArray(new String[0]));
        verify(arbitrationCallback, never()).onSuccess(any());
        for (String participantId : participantIDs) {
            InOrder incrementBeforeDecrement = Mockito.inOrder(messageRouter);
            incrementBeforeDecrement.verify(messageRouter, times(1)).setToKnown(participantId);
            incrementBeforeDecrement.verify(messageRouter, times(1)).removeNextHop(participantId);
        }
    }

}
