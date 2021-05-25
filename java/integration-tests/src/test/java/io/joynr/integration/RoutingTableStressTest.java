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
import static org.mockito.Matchers.any;
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
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.arbitration.ArbitrationStrategyFunction;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.mqtt.IMqttMessagingSkeleton;
import io.joynr.messaging.routing.RoutingEntry;
import io.joynr.provider.Promise;
import io.joynr.proxy.DiscoveryResult;
import io.joynr.proxy.GuidedProxyBuilder;
import io.joynr.proxy.ProxyBuilder;
import joynr.BroadcastFilterParameters;
import joynr.BroadcastSubscriptionRequest;
import joynr.ImmutableMessage;
import joynr.MutableMessage;
import joynr.OnChangeSubscriptionQos;
import joynr.Request;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;
import joynr.tests.DefaulttestProvider;
import joynr.tests.testProxy;
import joynr.types.DiscoveryEntryWithMetaInfo;

@RunWith(MockitoJUnitRunner.class)
public class RoutingTableStressTest extends AbstractRoutingTableCleanupTest {

    private static final Logger logger = LoggerFactory.getLogger(RoutingTableStressTest.class);
    private final String TEST_DOMAIN = "stressTestDomain";
    private final int THREAD_POOL_SIZE = 20;
    private static final String FROM_PARTICIPANTID_PREFIX = "fromParticipantId_";

    private final ExecutorService threadPoolExecutor = new ScheduledThreadPoolExecutor(THREAD_POOL_SIZE);

    private static final long CONST_DEFAULT_TEST_TIMEOUT_MS = 60000;
    private static final int TTL_REPLY_EXPIRED = 512;

    private ConcurrentHashMap<String, CountDownLatch> rpCdlMap;
    private ConcurrentHashMap<String, CountDownLatch> srpCdlMap;
    private ConcurrentHashMap<String, CountDownLatch> pubCdlMap;

    private class MyTestProvider extends DefaulttestProvider {
        @Override
        public Promise<SayHelloDeferred> sayHello() {
            SayHelloDeferred deferred = new SayHelloDeferred();
            Promise<SayHelloDeferred> promise = new Promise<>(deferred);
            new Thread(() -> {
                sleep(TTL_REPLY_EXPIRED);
                deferred.resolve("");
            }).start();
            return promise;
        }
    }

    private MyTestProvider testProvider1;
    private DefaulttestProvider testProvider2;
    private DefaulttestProvider testProvider3;

    private class TestCaseCallable implements Callable<Void> {

        private final int NUMBER_OF_TEST_CASES = 9;

        @Override
        public Void call() throws Exception {
            int numberOfMethod = getRandomNumber(0, NUMBER_OF_TEST_CASES);
            logger.info("Name of the Thread: {}, random number: {}", Thread.currentThread().getName(), numberOfMethod);
            callTestCase(numberOfMethod);
            return null;
        }

        private int getRandomNumber(int lowerBound, int uppperBound) {
            return ThreadLocalRandom.current().nextInt(lowerBound, uppperBound);
        }

        private void callTestCase(int numberOfTestCase) {
            String name = null;
            try {
                switch (numberOfTestCase) {
                case 0:
                    name = "useProxyBuilder_createProxy_sendMessage_discardProxy";
                    useProxyBuilder_createProxy_sendMessage_discardProxy();
                    break;
                case 1:
                    name = "useProxyBuilder_createMultiProxy_sendMessage_discardProxy";
                    useProxyBuilder_createMultiProxy_sendMessage_discardProxy();
                    break;
                case 2:
                    name = "useGuidedProxyBuilder_createProxy_sendMessage_discardProxy";
                    useGuidedProxyBuilder_createProxy_sendMessage_discardProxy();
                    break;
                case 3:
                    name = "useGuidedProxyBuilder_discover_buildNone";
                    useGuidedProxyBuilder_discover_buildNone();
                    break;
                case 4:
                    name = "registerAndUnregisterProviders_local";
                    registerAndUnregisterProviders_local();
                    break;
                case 5:
                    name = "mqttRequestReply_success";
                    mqttRequestReply_success();
                    break;
                case 6:
                    name = "mqttRequestReply_error_replyExpired";
                    mqttRequestReply_error_replyExpired(FIXEDPARTICIPANTID1);
                    break;
                case 7:
                    name = "mqttRequestReply_error_requestExpired";
                    mqttRequestReply_error_requestExpired(FIXEDPARTICIPANTID2);
                    break;
                case 8:
                    name = "mqttSubRequestSubReply_success_stoppedByExpiration";
                    mqttSubRequestSubReply_success_stoppedByExpiration(testProvider3, FIXEDPARTICIPANTID3);
                    break;
                default:
                    break;
                }
                logger.info("Test case {}: {} SUCCEEDED.", numberOfTestCase, name);
            } catch (Throwable e) {
                logger.error("FAILED in {}.", name, e);
                throw e;
            }
        }
    }

    @Before
    public void setUp() throws InterruptedException, IOException {
        super.setUp();
        testProvider1 = spy(new MyTestProvider());
        registerGlobal(testProvider1, TESTCUSTOMDOMAIN1, providerQosGlobal);
        testProvider2 = spy(new DefaulttestProvider());
        registerGlobal(testProvider2, TESTCUSTOMDOMAIN2, providerQosGlobal);
        testProvider3 = spy(new DefaulttestProvider());
        registerGlobal(testProvider3, TESTCUSTOMDOMAIN3, providerQosGlobal);

        rpCdlMap = new ConcurrentHashMap<>();
        srpCdlMap = new ConcurrentHashMap<>();
        pubCdlMap = new ConcurrentHashMap<>();
        doAnswer(invocation -> {
            ImmutableMessage msg = (ImmutableMessage) invocation.getArguments()[0];
            SuccessAction action = (SuccessAction) invocation.getArguments()[1];
            action.execute();
            final String to = msg.getRecipient();
            switch (msg.getType()) {
            case VALUE_MESSAGE_TYPE_REPLY:
                rpCdlMap.get(to).countDown();
                break;
            case VALUE_MESSAGE_TYPE_PUBLICATION:
                pubCdlMap.get(to).countDown();
                break;
            case VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY:
                srpCdlMap.get(to).countDown();
                break;
            default:
                break;
            }
            return null;
        }).when(mqttMessagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));
    }

    @After
    public void tearDown() {
        unregisterGlobal(TESTCUSTOMDOMAIN1, testProvider1);
        unregisterGlobal(TESTCUSTOMDOMAIN2, testProvider2);
        unregisterGlobal(TESTCUSTOMDOMAIN3, testProvider3);
        super.tearDown();
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
                logger.info("Garbage collector has removed the expected number ({}) of routing entries!",
                            expectedNumberOfRoutingEntries);
                garbageCollected = true;
                break;
            } else {
                Thread.sleep(100);
            }
        }
        if (!garbageCollected) {
            fail("Stress test failed! Garbage collector was not called! Number of routing entries in table: "
                    + routingTableHashMap.size());
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

    private void registerAndUnregisterProviders_local() {
        // register providers
        String domain1 = TEST_DOMAIN + createUuidString();
        String domain2 = TEST_DOMAIN + createUuidString();
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, domain1, providerQosLocal);
        registerProvider(testProvider, domain2, providerQosLocal);

        // unregister provider
        joynrRuntime.unregisterProvider(domain1, testProvider);
        joynrRuntime.unregisterProvider(domain2, testProvider);
    }

    private void mqttRequestReply_success() {
        final String fromParticipantId = FROM_PARTICIPANTID_PREFIX + "mqttRequestReply_success" + createUuidString();

        CountDownLatch rpCdl = new CountDownLatch(1);
        rpCdlMap.put(fromParticipantId, rpCdl);

        MutableMessage requestMsg = createRequestMsg(fromParticipantId, FIXEDPARTICIPANTID1);
        try {
            fakeIncomingMqttMessage(gbids[1], requestMsg);
        } catch (Exception e) {
            fail("fake incoming request failed: " + e);
        }

        waitFor(rpCdl, 10000);
        rpCdlMap.remove(fromParticipantId);
    }

    private void mqttRequestReply_error_replyExpired(final String providerParticipantId) {
        final String from = FROM_PARTICIPANTID_PREFIX + createUuidString();

        Request request = new Request("sayHello", new Object[0], new Class[0]);
        MessagingQos testMessagingQos = new MessagingQos(defaultMessagingQos);
        testMessagingQos.setTtl_ms(TTL_REPLY_EXPIRED);
        MutableMessage requestMsg = messageFactory.createRequest(from,
                                                                 providerParticipantId,
                                                                 request,
                                                                 testMessagingQos);
        String replyTo = RoutingTypesUtil.toAddressString(new MqttAddress(gbids[1], ""));
        requestMsg.setReplyTo(replyTo);
        try {
            fakeIncomingMqttMessage(gbids[1], requestMsg);
        } catch (Exception e) {
            fail("fake incoming request failed: " + e);
        }

        // delay reply until it is expired
        sleep(testMessagingQos.getRoundTripTtl_ms() * 2);
    }

    private void mqttRequestReply_error_requestExpired(final String providerParticipantId) {
        final String from = FROM_PARTICIPANTID_PREFIX + createUuidString();

        // fake incoming expired request and check refCounts
        Request request = new Request("echoCallingPrincipal", new Object[0], new Class[0]);
        MessagingQos testMessagingQos = new MessagingQos(defaultMessagingQos);
        testMessagingQos.setTtl_ms(0);
        MutableMessage requestMsg = messageFactory.createRequest(from,
                                                                 providerParticipantId,
                                                                 request,
                                                                 testMessagingQos);
        String replyTo = RoutingTypesUtil.toAddressString(new MqttAddress(gbids[1], ""));
        requestMsg.setReplyTo(replyTo);
        CountDownLatch cdl = new CountDownLatch(1);
        try {
            // make sure that the message is expired
            Thread.sleep(1);

            IMqttMessagingSkeleton skeleton = (IMqttMessagingSkeleton) mqttSkeletonFactory.getSkeleton(new MqttAddress(gbids[1],
                                                                                                                       ""));
            skeleton.transmit(requestMsg.getImmutableMessage().getSerializedMessage(), new FailureAction() {
                @Override
                public void execute(Throwable error) {
                    assertTrue(JoynrMessageNotSentException.class.isInstance(error));
                    assertTrue(error.getMessage().contains("expired"));
                    cdl.countDown();
                }
            });
        } catch (Exception e) {
            fail("fake incoming request failed: " + e);
        }

        waitFor(cdl, 10000);
    }

    private synchronized void mqttSubRequestSubReply_success_stoppedByExpiration(DefaulttestProvider testProvider,
                                                                                 final String providerParticipantId) {
        final String from = FROM_PARTICIPANTID_PREFIX + "mqttSubRequestSubReply_stoppedByExpiration"
                + createUuidString();

        CountDownLatch srpCdl = new CountDownLatch(1);
        CountDownLatch pubCdl = new CountDownLatch(2);
        srpCdlMap.put(from, srpCdl);
        pubCdlMap.put(from, pubCdl);

        // fake incoming subscription request and wait for subscription reply
        String subscriptionId = createUuidString();
        final long validityMs = 753;
        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos().setMinIntervalMs(0).setValidityMs(validityMs);
        BroadcastSubscriptionRequest request = new BroadcastSubscriptionRequest(subscriptionId,
                                                                                "intBroadcast",
                                                                                new BroadcastFilterParameters(),
                                                                                qos);
        MutableMessage requestMsg = messageFactory.createSubscriptionRequest(from,
                                                                             providerParticipantId,
                                                                             request,
                                                                             defaultMessagingQos);
        String replyTo = RoutingTypesUtil.toAddressString(new MqttAddress(gbids[1], ""));
        requestMsg.setReplyTo(replyTo);
        try {
            fakeIncomingMqttMessage(gbids[1], requestMsg);
            assertTrue(srpCdl.await(10000, TimeUnit.MILLISECONDS));
        } catch (Exception e) {
            fail(e.toString());
        }

        // sleep some time to make sure that the subscription at the provider's PublicationManager is fully established
        // (The SubscriptionReply is sent before the publicationInformation is stored in PublicationManager
        // .subscriptionId2PublicationInformation. A fired broadcast is dropped if this information is missing because
        // the recipient is unknown (race condition between fire*Broadcast and addSubscriptionRequest).
        sleep(256);
        // trigger publications
        testProvider.fireIntBroadcast(42);
        testProvider.fireIntBroadcast(43);
        // wait for publications
        waitFor(pubCdl, 10000);
        sleep(validityMs + 100);
        srpCdlMap.remove(from);
        pubCdlMap.remove(from);
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
        registerAndUnregisterProviders_local();
        mqttRequestReply_success();
        mqttRequestReply_error_replyExpired(FIXEDPARTICIPANTID1);
        mqttRequestReply_error_requestExpired(FIXEDPARTICIPANTID2);
        mqttSubRequestSubReply_success_stoppedByExpiration(testProvider3, FIXEDPARTICIPANTID3);

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
                fail(e.toString() + Arrays.toString(e.getStackTrace()) + "\n cause: " + e.getCause()
                        + Arrays.toString(e.getCause().getStackTrace()));
            }
        }

        waitForGarbageCollection(startingNumberOfRoutingEntries);

        // Check the number of routing entries
        ConcurrentMap<String, RoutingEntry> endingRoutingTable = getDeepCopyFor(routingTableHashMap);
        checkRoutingTables(startingRoutingTable, endingRoutingTable);
    }
}
