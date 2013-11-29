package io.joynr.integration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.dispatcher.rpc.Callback;
import io.joynr.dispatcher.rpc.RequestStatusCode;
import io.joynr.exceptions.JoynrArbitrationException;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrTimeoutException;
import io.joynr.exceptions.JoynrWaitExpiredException;
import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.integration.util.ServersUtil;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.PropertyLoader;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Properties;
import java.util.UUID;

import joynr.tests.AnotherDerivedStruct;
import joynr.tests.DefaultTestProvider;
import joynr.tests.DerivedStruct;
import joynr.tests.TestEnum;
import joynr.tests.TestProxy;
import joynr.types.GpsFixEnum;
import joynr.types.GpsLocation;
import joynr.types.Trip;

import org.eclipse.jetty.server.Server;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestName;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@RunWith(MockitoJUnitRunner.class)
public class ProviderProxyEnd2EndTest {
    private static final Logger logger = LoggerFactory.getLogger(ProviderProxyEnd2EndTest.class);

    TestProvider provider;
    String domain;

    long timeTookToRegisterProvider;

    private DummyJoynrApplication dummyProviderApplication;

    private DummyJoynrApplication dummyConsumerApplication;

    @Rule
    public TestName name = new TestName();

    private MessagingQos messagingQos;

    private DiscoveryQos discoveryQos;

    @Mock
    Callback<String> callback;

    @Mock
    Callback<Integer> callbackInteger;

    private static Server jettyServer;

    @BeforeClass
    public static void startServer() throws Exception {
        jettyServer = ServersUtil.startServers();
        // keep delays and timeout low for tests
        System.setProperty(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS, "10");
        System.setProperty(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_REQUEST_TIMEOUT, "1000");
    }

    @AfterClass
    public static void stopServer() throws Exception {
        jettyServer.stop();
    }

    @Before
    public void setUp() throws JoynrArbitrationException, JoynrIllegalStateException, Exception {

        // prints the tests name in the log so we know what we are testing
        String methodName = name.getMethodName();
        logger.info(methodName + " setup beginning...");

        // use channelNames = test name
        String channelIdProvider = "JavaTest-" + methodName + UUID.randomUUID().getLeastSignificantBits()
                + "-end2endTestProvider";
        String channelIdConsumer = "JavaTest-" + methodName + UUID.randomUUID().getLeastSignificantBits()
                + "-end2endConsumer";

        Properties joynrConfigProvider = PropertyLoader.loadProperties("testMessaging.properties");
        joynrConfigProvider.put(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL, "localdomain."
                + UUID.randomUUID().toString());
        joynrConfigProvider.put(MessagingPropertyKeys.CHANNELID, channelIdProvider);
        joynrConfigProvider.put(MessagingPropertyKeys.RECEIVERID, UUID.randomUUID().toString());

        dummyProviderApplication = (DummyJoynrApplication) new JoynrInjectorFactory(joynrConfigProvider).createApplication(DummyJoynrApplication.class);

        Properties joynrConfigConsumer = PropertyLoader.loadProperties("testMessaging.properties");
        joynrConfigConsumer.put(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL, "localdomain."
                + UUID.randomUUID().toString());
        joynrConfigConsumer.put(MessagingPropertyKeys.CHANNELID, channelIdConsumer);
        joynrConfigConsumer.put(MessagingPropertyKeys.RECEIVERID, UUID.randomUUID().toString());

        dummyConsumerApplication = (DummyJoynrApplication) new JoynrInjectorFactory(joynrConfigConsumer).createApplication(DummyJoynrApplication.class);

        provider = new TestProvider();
        domain = "ProviderProxyEnd2EndTest." + name.getMethodName() + System.currentTimeMillis();

        // check that registerProvider does not block
        long startTime = System.currentTimeMillis();
        dummyProviderApplication.getRuntime().registerCapability(domain,
                                                                 provider,
                                                                 joynr.tests.TestSync.class,
                                                                 "authToken");
        long endTime = System.currentTimeMillis();
        timeTookToRegisterProvider = endTime - startTime;

        messagingQos = new MessagingQos(5000);
        discoveryQos = new DiscoveryQos(5000, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);

        // this sleep greatly speeds up the tests (400 ms vs 2500 / test) by
        // making sure the channel is created before first messages sent.
        Thread.sleep(100);
        logger.info("setup finished");

    }

    @After
    public void tearDown() throws InterruptedException {

        if (dummyProviderApplication != null) {
            dummyProviderApplication.shutdown();
        }

        if (dummyConsumerApplication != null) {
            dummyConsumerApplication.shutdown();
        }

    }

    protected static class TestProvider extends DefaultTestProvider {
        public static final String answer = "Answer to: ";

        public TestProvider() {
        }

        @Override
        public Integer methodWithEnumParameter(TestEnum input) {
            if (TestEnum.ONE.equals(input))
                return 1;
            if (TestEnum.TWO.equals(input))
                return 2;
            if (TestEnum.ZERO.equals(input))
                return 0;

            return 42;
        }

        @Override
        public Integer addNumbers(Integer first, Integer second, Integer third) {
            return first + second + third;
        }

        @Override
        public String waitTooLong(Long ttl_ms) {
            String returnString = "";
            long enteredAt = System.currentTimeMillis();
            try {
                Thread.sleep(ttl_ms + 1);
            } catch (InterruptedException e) {
                returnString += "InterruptedException... ";
            }
            return returnString + "time: " + (System.currentTimeMillis() - enteredAt);
        }

        @Override
        public List<TestEnum> methodWithEnumListReturn(Integer input) {
            return Arrays.asList(new TestEnum[]{ TestEnum.ordinalToEnumValues.get(input) });
        }

        @Override
        public String sayHello() {
            return "Hello";
        }

        @Override
        public String toLowerCase(String inputString) {
            return inputString.toLowerCase();
        }

        @Override
        public Trip optimizeTrip(Trip input) {
            return input;
        }

        @Override
        public String overloadedOperation(DerivedStruct s) {
            return "DerivedStruct";
        }

        @Override
        public String overloadedOperation(AnotherDerivedStruct s) {
            return "AnotherDerivedStruct";
        }

        @Override
        public void voidOperation() {
            // TODO Auto-generated method stub
            super.voidOperation();
        }
    }

    @Test(timeout = 3000)
    public void registerProviderDoesNotBlock() throws InterruptedException {
        Assert.assertTrue("Register Provider should not block. Took " + timeTookToRegisterProvider
                + "ms to register provider", timeTookToRegisterProvider < 100);
        Thread.sleep(1000);
    }

    @Test(timeout = 3000)
    @Ignore
    public void registerProviderCreateProxyAndCallMethod() throws JoynrArbitrationException,
                                                          JoynrIllegalStateException, InterruptedException {
        int result;
        ProxyBuilder<TestProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     TestProxy.class);
        TestProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        result = proxy.addNumbers(6, 3, 2);
        Assert.assertEquals(11, result);

    }

    @Test(timeout = 3000)
    public void sendObjectsAsArgumentAndReturnValue() throws JoynrArbitrationException, JoynrIllegalStateException,
                                                     InterruptedException {
        ProxyBuilder<TestProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     TestProxy.class);
        TestProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        List<GpsLocation> locationList = new ArrayList<GpsLocation>();
        locationList.add(new GpsLocation(50.1, 20.1, 500.0, GpsFixEnum.MODE3D, 0.0, 0.0, 0.0, 0.0, 0l, 0l, 1000));
        locationList.add(new GpsLocation(50.1, 20.1, 500.0, GpsFixEnum.MODE3D, 0.0, 0.0, 0.0, 0.0, 0l, 0l, 1000));
        locationList.add(new GpsLocation(50.1, 20.1, 500.0, GpsFixEnum.MODE3D, 0.0, 0.0, 0.0, 0.0, 0l, 0l, 1000));
        Trip testObject = new Trip(locationList, "Title");
        Trip result;

        result = proxy.optimizeTrip(testObject);
        Assert.assertEquals(Double.valueOf(500.0), result.getLocations().get(0).getAltitude());

    }

    @Test(timeout = 3000)
    public void asyncMethodCallWithCallback() throws JoynrArbitrationException, JoynrIllegalStateException,
                                             InterruptedException {
        ProxyBuilder<TestProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     TestProxy.class);
        TestProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        Future<String> future = proxy.sayHello(callback);
        String answer = future.getReply(30000);
        Assert.assertEquals(RequestStatusCode.OK, future.getStatus().getCode());
        String expected = "Hello";
        Assert.assertEquals(expected, answer);
        verify(callback).onSuccess(expected);
        verifyNoMoreInteractions(callback);

        @SuppressWarnings("unchecked")
        // needed on jenkins
        Callback<String> callback2 = mock(Callback.class);
        Future<String> future2 = proxy.toLowerCase(callback2, "Argument");
        String answer2 = future2.getReply(30000);
        Assert.assertEquals(RequestStatusCode.OK, future2.getStatus().getCode());
        String expected2 = "argument";

        Assert.assertEquals(expected2, answer2);
        verify(callback2).onSuccess(expected2);
        verifyNoMoreInteractions(callback2);

    }

    @Test(timeout = 3000)
    public void asyncMethodCallWithTtlExpiring() throws JoynrArbitrationException, JoynrIllegalStateException,
                                                InterruptedException {

        ProxyBuilder<TestProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     TestProxy.class);
        long ttl = 2000L;
        MessagingQos testMessagingQos = new MessagingQos(ttl);
        DiscoveryQos testDiscoveryQos = new DiscoveryQos(30000, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);
        TestProxy proxyShortTll = proxyBuilder.setMessagingQos(testMessagingQos)
                                              .setDiscoveryQos(testDiscoveryQos)
                                              .build();

        // the provider waits ttl before responding, causing a ttl
        boolean timeoutExceptionThrown = false;
        // the ttl parameter tells the provider to wait this long before
        // replying, thereby forcing a ttl exception
        Future<String> waitTooLongFuture = proxyShortTll.waitTooLong(callback, ttl * 2);
        try {
            waitTooLongFuture.getReply(10 * ttl); // should never have to wait
            // this long
            // the JoynWaitExpiredException should not be thrown.
        } catch (JoynrWaitExpiredException e) {
            timeoutExceptionThrown = false;
        } catch (JoynrTimeoutException e) {
            timeoutExceptionThrown = true;
        }
        Assert.assertEquals(true, timeoutExceptionThrown);
        Assert.assertEquals(RequestStatusCode.ERROR, waitTooLongFuture.getStatus().getCode());
        verify(callback).onFailure(any(JoynrException.class));
        verifyNoMoreInteractions(callback);

    }

    @Test(timeout = 3000)
    public void testMethodWithEnumInputReturnsResult() throws JoynrArbitrationException, JoynrIllegalStateException,
                                                      InterruptedException {
        ProxyBuilder<TestProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     TestProxy.class);
        TestProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        TestEnum input = TestEnum.TWO;

        int result = proxy.methodWithEnumParameter(input);
        assertEquals(2, result);

    }

    @Test(timeout = 3000)
    public void testVoidOperation() throws JoynrArbitrationException, JoynrIllegalStateException, InterruptedException {
        ProxyBuilder<TestProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     TestProxy.class);
        TestProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        final Future<Boolean> future = new Future<Boolean>();
        proxy.voidOperation(new Callback<Void>() {

            @Override
            public void onSuccess(Void result) {
                future.onSuccess(true);
            }

            @Override
            public void onFailure(JoynrException error) {
                future.onFailure(new JoynrException());
            }

        });
        Boolean reply = future.getReply(8000);

        assertTrue(reply);

    }

    // Currently causes an NPE, see bug 1029
    @Ignore
    @Test(timeout = 3000)
    public void testMethodWithNullEnumInputReturnsSomethingSensible() throws JoynrArbitrationException,
                                                                     JoynrIllegalStateException, InterruptedException {
        TestEnum input = null;
        ProxyBuilder<TestProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     TestProxy.class);
        TestProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        int result = proxy.methodWithEnumParameter(input);
        assertEquals(42, result);
    }

    @Test(timeout = 3000)
    public void sendingANullValueOnceDoesntCrashProvider() throws JoynrArbitrationException,
                                                          JoynrIllegalStateException, InterruptedException {
        ProxyBuilder<TestProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     TestProxy.class);
        TestProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        proxy.methodWithEnumParameter(null);
        TestEnum input = TestEnum.TWO;
        int result = proxy.methodWithEnumParameter(input);
        assertEquals(2, result);
    }

    @Test(timeout = 3000)
    public void testEnumAttribute() throws JoynrArbitrationException, JoynrIllegalStateException, InterruptedException {
        ProxyBuilder<TestProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     TestProxy.class);
        TestProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        proxy.setEnumAttribute(TestEnum.TWO);
        TestEnum result = proxy.getEnumAttribute();
        assertEquals(TestEnum.TWO, result);
    }

    @Ignore
    // methods that return enums are not working at the moment - see JOYN-1027
    @Test(timeout = 3000)
    public void testMethodWithEnumOutput() {
        // TODO write this test, once JOYN-1027 is solved
    }

    @Test(timeout = 3000)
    public void asyncMethodCallWithCallbackAndParameter() throws JoynrArbitrationException, JoynrIllegalStateException,
                                                         InterruptedException {

        ProxyBuilder<TestProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     TestProxy.class);

        TestProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        Future<String> future = proxy.toLowerCase(callback, "Argument");
        String answer = future.getReply(21000);

        Assert.assertEquals(RequestStatusCode.OK, future.getStatus().getCode());
        String expected = "argument";
        Assert.assertEquals(expected, answer);
        verify(callback).onSuccess(expected);
        verifyNoMoreInteractions(callback);

    }

    @Test(timeout = 3000)
    public void asyncMethodCallWithIntegerParametersAndFuture() throws JoynrArbitrationException,
                                                               JoynrIllegalStateException, InterruptedException {
        ProxyBuilder<TestProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     TestProxy.class);
        TestProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        Future<Integer> future = proxy.addNumbers(callbackInteger, 1, 2, 3);
        Integer reply = future.getReply(30000);

        Assert.assertEquals(RequestStatusCode.OK, future.getStatus().getCode());
        Integer expected = 6;
        Assert.assertEquals(expected, reply);
        verify(callbackInteger).onSuccess(expected);
        verifyNoMoreInteractions(callbackInteger);

    }

    @Test(timeout = 3000)
    public void asyncMethodCallWithEnumParametersAndFuture() throws JoynrArbitrationException,
                                                            JoynrIllegalStateException, InterruptedException {
        ProxyBuilder<TestProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     TestProxy.class);
        TestProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        Future<Integer> future = proxy.methodWithEnumParameter(callbackInteger, TestEnum.TWO);
        Integer reply = future.getReply(40000);

        Integer expected = 2;
        Assert.assertEquals(RequestStatusCode.OK, future.getStatus().getCode());
        Assert.assertEquals(expected, reply);
        verify(callbackInteger).onSuccess(expected);
        verifyNoMoreInteractions(callbackInteger);

    }

    @Test(timeout = 3000)
    public void asyncMethodCallWithEnumListReturned() throws JoynrArbitrationException, JoynrIllegalStateException,
                                                     InterruptedException {
        ProxyBuilder<TestProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     TestProxy.class);
        TestProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        List<TestEnum> enumList = proxy.methodWithEnumListReturn(2);
        assertArrayEquals(new TestEnum[]{ TestEnum.TWO }, enumList.toArray());

    }

    @Test(timeout = 3000)
    public void overloadedMethodWithInheritance() throws JoynrArbitrationException, JoynrIllegalStateException,
                                                 InterruptedException {
        ProxyBuilder<TestProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     TestProxy.class);
        TestProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        DerivedStruct derivedStruct = new DerivedStruct();
        AnotherDerivedStruct anotherDerivedStruct = new AnotherDerivedStruct();

        String anotherDerivedResult = proxy.overloadedOperation(anotherDerivedStruct);
        String derivedResult = proxy.overloadedOperation(derivedStruct);
        Assert.assertEquals("DerivedStruct", derivedResult);
        Assert.assertEquals("AnotherDerivedStruct", anotherDerivedResult);

    }

    // @Test(timeout=3000)
    // public void proxyUsesCorrectEndpointAddress() {
    // TODO
    // }

}
