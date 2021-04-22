/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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

import static io.joynr.util.JoynrUtil.createUuidString;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.spy;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.ThreadLocalRandom;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.arbitration.ArbitrationStrategyFunction;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.messaging.routing.RoutingEntry;
import io.joynr.proxy.DiscoveryResult;
import io.joynr.proxy.GuidedProxyBuilder;
import io.joynr.proxy.ProxyBuilder;
import joynr.tests.DefaulttestProvider;
import joynr.tests.testProxy;
import joynr.types.DiscoveryEntryWithMetaInfo;

@RunWith(MockitoJUnitRunner.class)
public class RoutingTableStressTest extends AbstractRoutingTableCleanupTest {
    private final String TEST_DOMAIN = "stressTestDomain";
    private final int THREAD_POOL_SIZE = 20;

    private final ExecutorService threadPoolExecutor = new ScheduledThreadPoolExecutor(THREAD_POOL_SIZE);

    private static final long CONST_DEFAULT_TEST_TIMEOUT_MS = 60000;

    private DefaulttestProvider testProvider1;
    private DefaulttestProvider testProvider2;
    private DefaulttestProvider testProvider3;

    private class TestCaseCallable implements Callable<Void> {

        private final int NUMBER_OF_TEST_CASES = 4;

        @Override
        public Void call() throws Exception {
            int numberOfMethod = getRandomNumber(0, NUMBER_OF_TEST_CASES);
            System.out.println("Name of the Thread: " + Thread.currentThread().getName() + ", random number: "
                    + numberOfMethod);
            callTestCase(numberOfMethod);
            return null;
        }

        private int getRandomNumber(int lowerBound, int uppperBound) {
            return ThreadLocalRandom.current().nextInt(lowerBound, uppperBound);
        }

        private void callTestCase(int numberOfTestCase) {
            switch (numberOfTestCase) {
            case 0:
                useProxyBuilder_createProxy_sendMessage_discardProxy();
                break;
            case 1:
                useProxyBuilder_createMultiProxy_sendMessage_discardProxy();
                break;
            case 2:
                useGuidedProxyBuilder_createProxy_sendMessage_discardProxy();
                break;
            case 3:
                useGuidedProxyBuilder_discover_buildNone();
                break;
            default:
                break;
            }
        }
    }

    @Before
    public void setUp() throws InterruptedException, IOException {
        super.setUp();
        testProvider1 = setupGlobalProvider(TESTCUSTOMDOMAIN1);
        testProvider2 = setupGlobalProvider(TESTCUSTOMDOMAIN2);
        testProvider3 = setupGlobalProvider(TESTCUSTOMDOMAIN3);
    }

    @After
    public void tearDown() {
        unregisterGlobal(TESTCUSTOMDOMAIN1, testProvider1);
        unregisterGlobal(TESTCUSTOMDOMAIN2, testProvider2);
        unregisterGlobal(TESTCUSTOMDOMAIN3, testProvider3);
        super.tearDown();
    }

    private DefaulttestProvider setupGlobalProvider(final String domain) {
        DefaulttestProvider testProvider = spy(new DefaulttestProvider());
        registerGlobal(testProvider, domain, providerQosGlobal);
        return testProvider;
    }

    private ArbitrationStrategyFunction customArbitrationStrategyFunction() {
        return new ArbitrationStrategyFunction() {
            @Override
            public Set<DiscoveryEntryWithMetaInfo> select(Map<String, String> parameters,
                                                          Collection<DiscoveryEntryWithMetaInfo> capabilities) {
                if (capabilities.size() >= 3) {
                    DiscoveryEntryWithMetaInfo[] capabilitiesArr = capabilities.toArray(new DiscoveryEntryWithMetaInfo[capabilities.size()]);
                    return new HashSet<>(Arrays.asList(capabilitiesArr[0], capabilitiesArr[1]));
                } else {
                    return new HashSet<>(capabilities);
                }
            }
        };
    }

    private void waitForGarbageCollection(int expectedNumberOfRoutingEntries) throws InterruptedException {
        boolean garbageCollected = false;
        for (int i = 0; i < 100; i++) { // try for 10 seconds
            System.gc();
            if (routingTableHashMap.size() == expectedNumberOfRoutingEntries) {
                System.out.println("Garbage collector has removed the expected number ("
                        + expectedNumberOfRoutingEntries + ") of routing entries!");
                garbageCollected = true;
                break;
            } else {
                Thread.sleep(100);
            }
        }
        if (!garbageCollected) {
            fail("Stress test failed! Garbage collector was not called!");
        }
    }

    private void checkRoutingTables(ConcurrentMap<String, RoutingEntry> desiredRoutingTable,
                                    ConcurrentMap<String, RoutingEntry> actualRoutingTable) {
        assertEquals(desiredRoutingTable.size(), actualRoutingTable.size());

        for (Map.Entry<String, RoutingEntry> desiredEntry : desiredRoutingTable.entrySet()) {
            String desiredParticipantId = desiredEntry.getKey();
            RoutingEntry desiredRoutingEntry = desiredEntry.getValue();
            assertTrue(actualRoutingTable.containsKey(desiredParticipantId));
            RoutingEntry actualRoutingEntry = actualRoutingTable.get(desiredParticipantId);
            assertEquals(desiredRoutingEntry, actualRoutingEntry);
            assertEquals(desiredRoutingEntry.getRefCount(), actualRoutingEntry.getRefCount());
        }
    }

    private ConcurrentMap<String, RoutingEntry> getDeepCopyFor(final ConcurrentMap<String, RoutingEntry> map) {
        ConcurrentMap<String, RoutingEntry> result = new ConcurrentHashMap<>();
        map.forEach((participantId, routingEntry) -> {
            RoutingEntry clone = new RoutingEntry(routingEntry.getAddress(),
                                                  routingEntry.getIsGloballyVisible(),
                                                  routingEntry.getExpiryDateMs(),
                                                  routingEntry.getIsSticky());
            clone.setRefCount(routingEntry.getRefCount());
            result.put(participantId, clone);
        });
        return result;
    }

    private void useProxyBuilder_createProxy_sendMessage_discardProxy() {
        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2));
        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(domains, testProxy.class);
        testProxy proxy = createProxy(proxyBuilder, defaultMessagingQos, discoveryQosLocal);

        proxy.addNumbers(10, 20, 30);

        proxy = null;
        proxyBuilder = null;
    }

    private void useProxyBuilder_createMultiProxy_sendMessage_discardProxy() {
        // register providers
        String domain1 = TEST_DOMAIN + createUuidString();
        String domain2 = TEST_DOMAIN + createUuidString();
        String domain3 = TEST_DOMAIN + createUuidString();

        // register providers
        DefaulttestProvider testProvider = spy(new DefaulttestProvider());

        registerProvider(testProvider, domain1, providerQosLocal);
        registerProvider(testProvider, domain2, providerQosLocal);
        registerProvider(testProvider, domain3, providerQosLocal);

        // Define custom arbitration strategy function to get multiple providers
        ArbitrationStrategyFunction arbitrationStrategyFunction = customArbitrationStrategyFunction();

        Set<String> domains = new HashSet<>(Arrays.asList(domain1, domain2, domain3));
        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(domains, testProxy.class);

        // create proxy
        DiscoveryQos discoveryQos = new DiscoveryQos(30000,
                                                     arbitrationStrategyFunction,
                                                     Long.MAX_VALUE,
                                                     DiscoveryScope.LOCAL_ONLY);
        testProxy proxy = createProxy(proxyBuilder, defaultMessagingQos, discoveryQos);

        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer((invocation) -> {
            Void result = (Void) invocation.callRealMethod();
            cdl.countDown();
            return result;
        }).when(testProvider).methodFireAndForgetWithoutParams();

        // Perform any proxy operation
        proxy.methodFireAndForgetWithoutParams();
        try {
            assertTrue(cdl.await(10000, TimeUnit.MILLISECONDS));
        } catch (InterruptedException e) {
            fail(e.toString());
        }
        proxy = null;
        proxyBuilder = null;

        // unregister provider
        joynrRuntime.unregisterProvider(domain1, testProvider);
        joynrRuntime.unregisterProvider(domain2, testProvider);
        joynrRuntime.unregisterProvider(domain3, testProvider);
    }

    private void useGuidedProxyBuilder_createProxy_sendMessage_discardProxy() {
        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2, TESTCUSTOMDOMAIN3));
        GuidedProxyBuilder guidedProxyBuilder = joynrRuntime.getGuidedProxyBuilder(domains, testProxy.class);

        // create proxy
        DiscoveryResult discoveryResult = guidedProxyBuilder.setDiscoveryQos(discoveryQosLocal)
                                                            .setMessagingQos(defaultMessagingQos)
                                                            .discover();
        testProxy proxy = guidedProxyBuilder.buildProxy(testProxy.class,
                                                        discoveryResult.getLastSeen().getParticipantId());

        proxy.addNumbers(10, 20, 30);

        proxy = null;
        guidedProxyBuilder = null;
    }

    private void useGuidedProxyBuilder_discover_buildNone() {
        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2, TESTCUSTOMDOMAIN3));
        GuidedProxyBuilder guidedProxyBuilder = joynrRuntime.getGuidedProxyBuilder(domains, testProxy.class);

        // perform discovery
        guidedProxyBuilder.setDiscoveryQos(discoveryQosLocal).setMessagingQos(defaultMessagingQos).discover();
        // build none
        guidedProxyBuilder.buildNone();
    }

    @Test
    public void dummyTest_syncExecution_numberOfRoutingEntriesStaysTheSame() throws InterruptedException {
        // Get number of routing entries before starting the stress test
        ConcurrentMap<String, RoutingEntry> startingRoutingTable = getDeepCopyFor(routingTableHashMap);
        int startingNumberOfRoutingEntries = startingRoutingTable.size();
        assertTrue(startingNumberOfRoutingEntries > 0);

        // Call all test cases
        useProxyBuilder_createProxy_sendMessage_discardProxy();
        useProxyBuilder_createMultiProxy_sendMessage_discardProxy();
        useGuidedProxyBuilder_createProxy_sendMessage_discardProxy();
        useGuidedProxyBuilder_discover_buildNone();

        waitForGarbageCollection(startingNumberOfRoutingEntries);

        ConcurrentMap<String, RoutingEntry> endingRoutingTable = getDeepCopyFor(routingTableHashMap);
        checkRoutingTables(startingRoutingTable, endingRoutingTable);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT_MS)
    public void stressTest_asyncExecution_numberOfRoutingEntriesStaysTheSame() throws InterruptedException {
        // Get number of routing entries before starting the stress test
        ConcurrentMap<String, RoutingEntry> startingRoutingTable = getDeepCopyFor(routingTableHashMap);
        int startingNumberOfRoutingEntries = startingRoutingTable.size();
        assertTrue(startingNumberOfRoutingEntries > 0);

        // Perform stress test
        int numberOfRuns = THREAD_POOL_SIZE * 5;
        List<Future<Void>> list = new ArrayList<Future<Void>>();
        Callable<Void> callable = new TestCaseCallable();
        for (int i = 0; i < numberOfRuns; i++) {
            Future<Void> future = threadPoolExecutor.submit(callable);
            list.add(future);
        }
        for (Future<Void> future : list) {
            try {
                future.get();
            } catch (InterruptedException | ExecutionException e) {
                fail(e.toString());
            }
        }

        waitForGarbageCollection(startingNumberOfRoutingEntries);

        // Check the number of routing entries
        ConcurrentMap<String, RoutingEntry> endingRoutingTable = getDeepCopyFor(routingTableHashMap);
        checkRoutingTables(startingRoutingTable, endingRoutingTable);
    }
}
