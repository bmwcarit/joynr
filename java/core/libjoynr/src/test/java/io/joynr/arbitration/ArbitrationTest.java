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
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
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
import java.util.HashMap;
import java.util.HashSet;
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
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.MultiDomainNoCompatibleProviderFoundException;
import io.joynr.exceptions.NoCompatibleProviderFoundException;
import io.joynr.proxy.Callback;
import io.joynr.runtime.JoynrThreadFactory;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.types.CustomParameter;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.ProviderQos;
import joynr.types.Version;

public class ArbitrationTest {

    private static final long ARBITRATION_TIMEOUT = 1000;
    private static final Long NO_EXPIRY = Long.MAX_VALUE;
    private static String interfaceName = "testInterface";
    private static Version interfaceVersion = new Version(0, 0);

    protected ArrayList<DiscoveryEntryWithMetaInfo> capabilitiesList;

    private final String domain = "testDomain";
    private String publicKeyId = "publicKeyId";
    private String testKeyword = "testKeyword";
    private long testPriority = 42;
    private String expectedParticipantId = "expectedParticipantId";
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

    public interface TestInterface {
        public static final String INTERFACE_NAME = interfaceName;
    }

    @SuppressWarnings({ "unchecked", "rawtypes" })
    @Before
    public void setUp() throws Exception {
        initMocks(this);

        capabilitiesList = new ArrayList<DiscoveryEntryWithMetaInfo>();

        doAnswer(new Answer<Object>() {

            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                Object[] arguments = invocation.getArguments();
                assert (arguments[0] instanceof Callback);
                ((Callback) arguments[0]).resolve((Object) capabilitiesList.toArray(new DiscoveryEntryWithMetaInfo[0]));
                localDiscoveryAggregatorSemaphore.release();
                return null;
            }
        }).when(localDiscoveryAggregator).lookup(Mockito.<Callback> any(),
                                                 eq(new String[]{ domain }),
                                                 eq(interfaceName),
                                                 Mockito.<joynr.types.DiscoveryQos> any());

        Field discoveryEntryVersionFilterField = ArbitratorFactory.class.getDeclaredField("discoveryEntryVersionFilter");
        discoveryEntryVersionFilterField.setAccessible(true);
        discoveryEntryVersionFilterField.set(ArbitratorFactory.class, discoveryEntryVersionFilter);

        doAnswer(new Answer<Set<DiscoveryEntry>>() {
            @Override
            public Set<DiscoveryEntry> answer(InvocationOnMock invocation) throws Throwable {
                return (Set<DiscoveryEntry>) invocation.getArguments()[1];
            }
        }).when(discoveryEntryVersionFilter).filter(Mockito.<Version> any(),
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

        ArbitratorFactory.start();
        verify(shutdownNotifier).registerForShutdown(any(ShutdownListener.class));
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

    /**
     * @param discoveryQos the DiscoveryQos used to create the arbitrator object
     * @param domains optional list of domains used to create the arbitrator object. If nothing is
     *                passed then the arbitrator is created for the member variable {@link #domain}
     * @throws InterruptedException
     */
    private void createArbitratorWithCallbackAndAwaitArbitration(DiscoveryQos discoveryQos,
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
                                                             localDiscoveryAggregator);
            arbitrator.setArbitrationListener(arbitrationCallback);
            arbitrator.scheduleArbitration();

            assertTrue(localDiscoveryAggregatorSemaphore.tryAcquire(1000, TimeUnit.MILLISECONDS));
        } catch (DiscoveryException e) {
            fail("A Joyn Arbitration Exception has been thrown");
        }
    }

    @Test
    public void keywordArbitratorTest() throws InterruptedException {
        ProviderQos providerQos = new ProviderQos();
        CustomParameter[] qosParameters = { new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword) };
        providerQos.setCustomParameters(qosParameters);
        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
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

        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "wrongParticipantId",
                                                            providerQos2,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.Keyword, Long.MAX_VALUE);
        discoveryQos.addCustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);

        ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedDiscoveryEntry);
        verify(arbitrationCallback, times(1)).onSuccess(eq(expectedArbitrationResult));
    }

    @Test
    public void keyWordArbitratorMissingKeywordTest() throws InterruptedException {

        ProviderQos providerQos = new ProviderQos();
        CustomParameter[] qosParameters = {
                new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, "wrongkeyword") };
        providerQos.setCustomParameters(qosParameters);

        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
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
        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "wrongParticipantId",
                                                            providerQos,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));

        // Create a capability entry for a provider with the correct keyword and that also supports onChange subscriptions
        ProviderQos providerQos2 = new ProviderQos();
        CustomParameter[] qosParameters2 = { new CustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, testKeyword) };
        providerQos2.setCustomParameters(qosParameters2);
        providerQos2.setSupportsOnChangeSubscriptions(true);

        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
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

        ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedDiscoveryEntry);
        verify(arbitrationCallback, times(1)).onSuccess(eq(expectedArbitrationResult));
    }

    @Test
    public void testLastSeenArbitrator() throws InterruptedException {
        ProviderQos providerQos = new ProviderQos();

        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "wrongParticipantId",
                                                            providerQos,
                                                            222L,
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));

        DiscoveryEntryWithMetaInfo expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                           domain,
                                                                                           TestInterface.INTERFACE_NAME,
                                                                                           expectedParticipantId,
                                                                                           providerQos,
                                                                                           333L,
                                                                                           NO_EXPIRY,
                                                                                           publicKeyId,
                                                                                           true);
        capabilitiesList.add(expectedDiscoveryEntry);

        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "thirdParticipantId",
                                                            providerQos,
                                                            111L,
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));

        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, ArbitrationStrategy.LastSeen, Long.MAX_VALUE);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);

        ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedDiscoveryEntry);
        verify(arbitrationCallback, times(1)).onSuccess(eq(expectedArbitrationResult));
    }

    @Test
    public void testPriorityArbitrator() throws InterruptedException {
        ProviderQos providerQos = new ProviderQos();
        providerQos.setPriority(testPriority);

        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
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

        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "wrongParticipantId",
                                                            providerQos2,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));
        long negativePriority = -10;
        ProviderQos providerQos3 = new ProviderQos();
        providerQos3.setPriority(negativePriority);

        Address thirdEndpointAddress = new ChannelAddress("http://testUrl", "thirdChannelId");
        ArrayList<Address> thirdEndpointAddresses = new ArrayList<Address>();
        thirdEndpointAddresses.add(thirdEndpointAddress);
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

        ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedDiscoveryEntry);
        verify(arbitrationCallback, times(1)).onSuccess(eq(expectedArbitrationResult));
    }

    @Test
    public void testPriorityArbitratorWithOnlyNegativePriorities() throws InterruptedException {
        ProviderQos providerQos = new ProviderQos();
        providerQos.setPriority(Long.MIN_VALUE);

        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
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

        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
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

        Address otherEndpointAddress = new ChannelAddress("http://testUrl", "otherChannelId");
        ArrayList<Address> otherEndpointAddresses = new ArrayList<Address>();
        otherEndpointAddresses.add(otherEndpointAddress);
        capabilitiesList.add(new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                            domain,
                                                            TestInterface.INTERFACE_NAME,
                                                            "wrongParticipantId",
                                                            providerQos2,
                                                            System.currentTimeMillis(),
                                                            NO_EXPIRY,
                                                            publicKeyId,
                                                            true));

        // A provider with a higher priority that does not support onChangeSubscriptions
        ProviderQos providerQos3 = new ProviderQos();
        providerQos3.setPriority(testPriority + 2);
        providerQos3.setSupportsOnChangeSubscriptions(false);

        Address thirdEndpointAddress = new ChannelAddress("http://testUrl", "thirdChannelId");
        ArrayList<Address> thirdEndpointAddresses = new ArrayList<Address>();
        thirdEndpointAddresses.add(thirdEndpointAddress);
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
        discoveryQos.setProviderMustSupportOnChange(true);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);

        ArbitrationResult expectedArbitrationResult = new ArbitrationResult(expectedDiscoveryEntry);
        verify(arbitrationCallback, times(1)).onSuccess(eq(expectedArbitrationResult));
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testCustomArbitrationFunction() throws InterruptedException {
        // Expected provider supports onChangeSubscriptions
        ProviderQos providerQos = new ProviderQos();

        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
        DiscoveryEntryWithMetaInfo discoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                                                   domain,
                                                                                   TestInterface.INTERFACE_NAME,
                                                                                   expectedParticipantId,
                                                                                   providerQos,
                                                                                   System.currentTimeMillis(),
                                                                                   NO_EXPIRY,
                                                                                   publicKeyId,
                                                                                   true);
        capabilitiesList.add(discoveryEntry);

        ArbitrationStrategyFunction arbitrationStrategyFunction = mock(ArbitrationStrategyFunction.class);
        when(arbitrationStrategyFunction.select(any(Map.class),
                                                any(Collection.class))).thenReturn(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(discoveryEntry)));
        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, arbitrationStrategyFunction, Long.MAX_VALUE);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);

        verify(arbitrationStrategyFunction,
               times(1)).select(eq(discoveryQos.getCustomParameters()),
                                eq(new HashSet<DiscoveryEntryWithMetaInfo>(capabilitiesList)));

        ArbitrationResult expectedArbitrationResult = new ArbitrationResult(discoveryEntry);
        verify(arbitrationCallback, times(1)).onSuccess(eq(expectedArbitrationResult));

    }

    @SuppressWarnings("unchecked")
    @Test
    public void testCustomArbitrationFunctionMultipleMatches() throws InterruptedException {
        // Expected provider supports onChangeSubscriptions
        ProviderQos providerQos = new ProviderQos();

        String publicKeyId = "publicKeyId";

        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
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
        when(arbitrationStrategyFunction.select(any(Map.class),
                                                any(Collection.class))).thenReturn(new HashSet<DiscoveryEntryWithMetaInfo>(capabilitiesList));
        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, arbitrationStrategyFunction, Long.MAX_VALUE);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);

        verify(arbitrationStrategyFunction,
               times(1)).select(eq(discoveryQos.getCustomParameters()),
                                eq(new HashSet<DiscoveryEntryWithMetaInfo>(capabilitiesList)));

        DiscoveryEntryWithMetaInfo[] expectedDiscoveryEntries = new DiscoveryEntryWithMetaInfo[capabilitiesList.size()];
        ArbitrationResult expectedArbitrationResult = new ArbitrationResult(capabilitiesList.toArray(expectedDiscoveryEntries));
        verify(arbitrationCallback, times(1)).onSuccess(eq(expectedArbitrationResult));
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testVersionFilterUsed() throws InterruptedException {
        ProviderQos providerQos = new ProviderQos();
        String publicKeyId = "publicKeyId";
        expectedEndpointAddress = new ChannelAddress("http://testUrl", "testChannelId");
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
        when(arbitrationStrategyFunction.select(any(Map.class),
                                                any(Collection.class))).thenReturn(new HashSet<DiscoveryEntryWithMetaInfo>(capabilitiesList));
        discoveryQos = new DiscoveryQos(ARBITRATION_TIMEOUT, arbitrationStrategyFunction, Long.MAX_VALUE);

        createArbitratorWithCallbackAndAwaitArbitration(discoveryQos);

        verify(discoveryEntryVersionFilter).filter(interfaceVersion,
                                                   new HashSet<DiscoveryEntryWithMetaInfo>(capabilitiesList),
                                                   new HashMap<String, Set<Version>>());
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
        }).when(discoveryEntryVersionFilter).filter(Mockito.<Version> any(),
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
        }).when(localDiscoveryAggregator).lookup(Mockito.<Callback<DiscoveryEntryWithMetaInfo[]>> any(),
                                                 eq(new String[]{ domain }),
                                                 eq(interfaceName),
                                                 Mockito.<joynr.types.DiscoveryQos> any());

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
        }).when(discoveryEntryVersionFilter).filter(Mockito.<Version> any(),
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
        }).when(localDiscoveryAggregator).lookup(Mockito.<Callback<DiscoveryEntryWithMetaInfo[]>> any(),
                                                 any(String[].class),
                                                 eq(interfaceName),
                                                 Mockito.<joynr.types.DiscoveryQos> any());

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
    public void testLookupForGuidedProxyBuilder() {
        Set<String> domainsSet = new HashSet<String>(Arrays.asList(domain));
        Arbitrator arbitrator = ArbitratorFactory.create(domainsSet,
                                                         interfaceName,
                                                         interfaceVersion,
                                                         new DiscoveryQos(),
                                                         localDiscoveryAggregator);
        arbitrator.setArbitrationListener(arbitrationCallback);
        arbitrator.lookup();
        verify(localDiscoveryAggregator).lookup(Mockito.<Callback<DiscoveryEntryWithMetaInfo[]>> any(),
                                                any(String[].class),
                                                any(String.class),
                                                any(joynr.types.DiscoveryQos.class));
    }

}
