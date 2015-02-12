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
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
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
import io.joynr.pubsub.publication.AttributeListener;
import io.joynr.pubsub.publication.BroadcastListener;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.PropertyLoader;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Properties;
import java.util.UUID;
import java.util.concurrent.Semaphore;

import joynr.OnChangeSubscriptionQos;
import joynr.tests.AnotherDerivedStruct;
import joynr.tests.BaseStruct;
import joynr.tests.ComplexTestType;
import joynr.tests.ComplexTestType2;
import joynr.tests.DefaulttestProvider;
import joynr.tests.DerivedStruct;
import joynr.tests.TestEnum;
import joynr.tests.testBroadcastInterface.LocationUpdateWithSpeedBroadcastListener;
import joynr.tests.testProviderAsync;
import joynr.tests.testProxy;
import joynr.types.GpsFixEnum;
import joynr.types.GpsLocation;
import joynr.types.ProviderQos;
import joynr.types.Trip;
import joynr.types.Vowel;

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

    private static final int CONST_DEFAULT_TEST_TIMEOUT = 3000;
    TestProvider provider;
    String domain;
    String domainAsync;

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

    private TestAsyncProviderImpl providerAsync;

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

        providerAsync = new TestAsyncProviderImpl();
        domainAsync = domain + "Async";

        // check that registerProvider does not block
        long startTime = System.currentTimeMillis();
        dummyProviderApplication.getRuntime()
                                .registerCapability(domain, provider, joynr.tests.testProvider.class, "authToken")
                                .waitForFullRegistration(CONST_DEFAULT_TEST_TIMEOUT);
        long endTime = System.currentTimeMillis();
        timeTookToRegisterProvider = endTime - startTime;

        dummyProviderApplication.getRuntime()
                                .registerCapability(domainAsync, providerAsync, testProviderAsync.class, "authToken")
                                .waitForFullRegistration(CONST_DEFAULT_TEST_TIMEOUT);

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

    protected static class TestProvider extends DefaulttestProvider {
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
        public ComplexTestType overloadedOperation(String input) {
            int result = Integer.parseInt(input);
            return new ComplexTestType(result, result);
        }

        @Override
        public ComplexTestType2 overloadedOperation(String input1, String input2) {
            return new ComplexTestType2(Integer.parseInt(input1), Integer.parseInt(input2));
        }

        @Override
        public void voidOperation() {
            super.voidOperation();
        }
    }

    protected static class TestAsyncProviderImpl implements testProviderAsync {

        private ProviderQos providerQos = new ProviderQos();
        private TestEnum enumAttribute;
        private GpsLocation location = new GpsLocation();
        private Trip myTrip = new Trip();
        private List<String> listOfStrings;
        private Integer testAttribute;
        private GpsLocation complexTestAttribute;

        @Override
        public ProviderQos getProviderQos() {
            return providerQos;
        }

        @Override
        public void registerAttributeListener(String attributeName, AttributeListener attributeListener) {
        }

        @Override
        public void unregisterAttributeListener(String attributeName, AttributeListener attributeListener) {
        }

        @Override
        public void registerBroadcastListener(String broadcastName, BroadcastListener broadcastListener) {
        }

        @Override
        public void unregisterBroadcastListener(String broadcastName, BroadcastListener broadcastListener) {
        }

        @Override
        public void getEnumAttribute(@JoynrRpcCallback(deserialisationType = TestEnumToken.class) Callback<TestEnum> callback) {
            callback.onSuccess(enumAttribute);
        }

        @Override
        public void setEnumAttribute(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                     @JoynrRpcParam(value = "enumAttribute", deserialisationType = TestEnumToken.class) TestEnum enumAttribute) {
            this.enumAttribute = enumAttribute;
            callback.onSuccess(null);
        }

        @Override
        public void getLocation(@JoynrRpcCallback(deserialisationType = GpsLocationToken.class) Callback<GpsLocation> callback) {
            callback.onSuccess(location);
        }

        @Override
        public void getMytrip(@JoynrRpcCallback(deserialisationType = TripToken.class) Callback<Trip> callback) {
            callback.onSuccess(myTrip);
        }

        @Override
        public void getYourLocation(@JoynrRpcCallback(deserialisationType = GpsLocationToken.class) Callback<GpsLocation> callback) {
            callback.onSuccess(new GpsLocation());
        }

        @Override
        public void getFirstPrime(@JoynrRpcCallback(deserialisationType = IntegerToken.class) Callback<Integer> callback) {
            callback.onSuccess(10);
        }

        @Override
        public void getListOfInts(@JoynrRpcCallback(deserialisationType = ListIntegerToken.class) Callback<List<Integer>> callback) {
            callback.onSuccess(new ArrayList<Integer>());
        }

        @Override
        public void getListOfLocations(@JoynrRpcCallback(deserialisationType = ListGpsLocationToken.class) Callback<List<GpsLocation>> callback) {
            callback.onSuccess(new ArrayList<GpsLocation>());
        }

        @Override
        public void getListOfStrings(@JoynrRpcCallback(deserialisationType = ListStringToken.class) Callback<List<String>> callback) {
            callback.onSuccess(listOfStrings);
        }

        @Override
        public void setListOfStrings(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                     @JoynrRpcParam(value = "listOfStrings", deserialisationType = ListStringToken.class) List<String> listOfStrings) {
            this.listOfStrings = listOfStrings;
            callback.onSuccess(null);
        }

        @Override
        public void getTestAttribute(@JoynrRpcCallback(deserialisationType = IntegerToken.class) Callback<Integer> callback) {
            callback.onSuccess(testAttribute);
        }

        @Override
        public void setTestAttribute(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                     @JoynrRpcParam(value = "testAttribute", deserialisationType = IntegerToken.class) Integer testAttribute) {
            this.testAttribute = testAttribute;
            callback.onSuccess(null);
        }

        @Override
        public void getComplexTestAttribute(@JoynrRpcCallback(deserialisationType = GpsLocationToken.class) Callback<GpsLocation> callback) {
            callback.onSuccess(complexTestAttribute);
        }

        @Override
        public void setComplexTestAttribute(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                            @JoynrRpcParam(value = "complexTestAttribute", deserialisationType = GpsLocationToken.class) GpsLocation complexTestAttribute) {
            this.complexTestAttribute = complexTestAttribute;
            callback.onSuccess(null);
        }

        @Override
        public void getReadWriteAttribute(@JoynrRpcCallback(deserialisationType = IntegerToken.class) Callback<Integer> callback) {
        }

        @Override
        public void setReadWriteAttribute(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                          @JoynrRpcParam(value = "readWriteAttribute", deserialisationType = IntegerToken.class) Integer readWriteAttribute) {
        }

        @Override
        public void getReadOnlyAttribute(@JoynrRpcCallback(deserialisationType = IntegerToken.class) Callback<Integer> callback) {
        }

        @Override
        public void getWriteOnly(@JoynrRpcCallback(deserialisationType = IntegerToken.class) Callback<Integer> callback) {
        }

        @Override
        public void setWriteOnly(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                 @JoynrRpcParam(value = "writeOnly", deserialisationType = IntegerToken.class) Integer writeOnly) {
        }

        @Override
        public void getNotifyWriteOnly(@JoynrRpcCallback(deserialisationType = IntegerToken.class) Callback<Integer> callback) {
        }

        @Override
        public void setNotifyWriteOnly(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                       @JoynrRpcParam(value = "notifyWriteOnly", deserialisationType = IntegerToken.class) Integer notifyWriteOnly) {
        }

        @Override
        public void getNotifyReadOnly(@JoynrRpcCallback(deserialisationType = IntegerToken.class) Callback<Integer> callback) {
        }

        @Override
        public void getNotifyReadWrite(@JoynrRpcCallback(deserialisationType = IntegerToken.class) Callback<Integer> callback) {
        }

        @Override
        public void setNotifyReadWrite(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                       @JoynrRpcParam(value = "notifyReadWrite", deserialisationType = IntegerToken.class) Integer notifyReadWrite) {
        }

        @Override
        public void getNotify(@JoynrRpcCallback(deserialisationType = IntegerToken.class) Callback<Integer> callback) {
        }

        @Override
        public void setNotify(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                              @JoynrRpcParam(value = "notify", deserialisationType = IntegerToken.class) Integer notify) {
        }

        @Override
        public void getATTRIBUTEWITHCAPITALLETTERS(@JoynrRpcCallback(deserialisationType = IntegerToken.class) Callback<Integer> callback) {
        }

        @Override
        public void setATTRIBUTEWITHCAPITALLETTERS(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                                   @JoynrRpcParam(value = "aTTRIBUTEWITHCAPITALLETTERS", deserialisationType = IntegerToken.class) Integer aTTRIBUTEWITHCAPITALLETTERS) {
        }

        @Override
        public void addNumbers(@JoynrRpcCallback(deserialisationType = IntegerToken.class) Callback<Integer> callback,
                               @JoynrRpcParam("first") Integer first,
                               @JoynrRpcParam("second") Integer second,
                               @JoynrRpcParam("third") Integer third) {
        }

        @Override
        public void sumInts(@JoynrRpcCallback(deserialisationType = IntegerToken.class) Callback<Integer> callback,
                            @JoynrRpcParam(value = "ints", deserialisationType = ListIntegerToken.class) List<Integer> ints) {
        }

        @Override
        public void methodWithNoInputParameters(@JoynrRpcCallback(deserialisationType = IntegerToken.class) Callback<Integer> callback) {
        }

        @Override
        public void methodWithEnumParameter(@JoynrRpcCallback(deserialisationType = IntegerToken.class) Callback<Integer> callback,
                                            @JoynrRpcParam("input") TestEnum input) {
        }

        @Override
        public void methodWithEnumListParameter(@JoynrRpcCallback(deserialisationType = IntegerToken.class) Callback<Integer> callback,
                                                @JoynrRpcParam(value = "input", deserialisationType = ListTestEnumToken.class) List<TestEnum> input) {
        }

        @Override
        public void methodWithEnumReturn(@JoynrRpcCallback(deserialisationType = TestEnumToken.class) Callback<TestEnum> callback,
                                         @JoynrRpcParam("input") Integer input) {
        }

        @Override
        public void methodWithEnumListReturn(@JoynrRpcCallback(deserialisationType = ListTestEnumToken.class) Callback<List<TestEnum>> callback,
                                             @JoynrRpcParam("input") Integer input) {
        }

        @Override
        public void methodWithByteArray(@JoynrRpcCallback(deserialisationType = ListByteToken.class) Callback<List<Byte>> callback,
                                        @JoynrRpcParam(value = "input", deserialisationType = ListByteToken.class) List<Byte> input) {
        }

        @Override
        public void methodEnumDoubleParameters(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                               @JoynrRpcParam("enumParam") TestEnum enumParam,
                                               @JoynrRpcParam("doubleParam") Double doubleParam) {
        }

        @Override
        public void methodWithEnumReturnValue(Callback<TestEnum> callback) {
            callback.onSuccess(TestEnum.TWO);
        }

        @Override
        public void getEnumAttributeReadOnly(Callback<TestEnum> callback) {
            callback.onSuccess(TestEnum.ONE);
        }

        @Override
        public void methodStringDoubleParameters(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                                 @JoynrRpcParam("stringParam") String stringParam,
                                                 @JoynrRpcParam("doubleParam") Double doubleParam) {
            callback.onSuccess(null);
        }

        @Override
        public void methodCustomCustomParameters(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                                 @JoynrRpcParam("customParam1") ComplexTestType customParam1,
                                                 @JoynrRpcParam("customParam2") ComplexTestType2 customParam2) {
        }

        @Override
        public void methodStringDoubleListParameters(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                                     @JoynrRpcParam("stringParam") String stringParam,
                                                     @JoynrRpcParam(value = "doubleListParam", deserialisationType = ListDoubleToken.class) List<Double> doubleListParam) {
        }

        @Override
        public void methodCustomCustomListParameters(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                                     @JoynrRpcParam("customParam") ComplexTestType customParam,
                                                     @JoynrRpcParam(value = "customListParam", deserialisationType = ListComplexTestType2Token.class) List<ComplexTestType2> customListParam) {
        }

        @Override
        public void customTypeAndListParameter(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                               @JoynrRpcParam("complexTestType") ComplexTestType complexTestType,
                                               @JoynrRpcParam(value = "complexArray", deserialisationType = ListBaseStructToken.class) List<BaseStruct> complexArray) {
        }

        @Override
        public void voidOperation(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback) {
        }

        @Override
        public void stringAndBoolParameters(@JoynrRpcCallback(deserialisationType = VoidToken.class) Callback<Void> callback,
                                            @JoynrRpcParam("stringParam") String stringParam,
                                            @JoynrRpcParam("boolParam") Boolean boolParam) {
        }

        @Override
        public void returnPrimeNumbers(@JoynrRpcCallback(deserialisationType = ListIntegerToken.class) Callback<List<Integer>> callback,
                                       @JoynrRpcParam("upperBound") Integer upperBound) {
        }

        @Override
        public void optimizeTrip(@JoynrRpcCallback(deserialisationType = TripToken.class) Callback<Trip> callback,
                                 @JoynrRpcParam("input") Trip input) {
        }

        @Override
        public void overloadedOperation(@JoynrRpcCallback(deserialisationType = StringToken.class) Callback<String> callback,
                                        @JoynrRpcParam("input") DerivedStruct input) {
        }

        @Override
        public void overloadedOperation(@JoynrRpcCallback(deserialisationType = StringToken.class) Callback<String> callback,
                                        @JoynrRpcParam("input") AnotherDerivedStruct input) {
        }

        @Override
        public void overloadedOperation(@JoynrRpcCallback(deserialisationType = ComplexTestTypeToken.class) Callback<ComplexTestType> callback,
                                        @JoynrRpcParam("input") String input) {
        }

        @Override
        public void overloadedOperation(@JoynrRpcCallback(deserialisationType = ComplexTestType2Token.class) Callback<ComplexTestType2> callback,
                                        @JoynrRpcParam("input1") String input1,
                                        @JoynrRpcParam("input2") String input2) {
        }

        @Override
        public void optimizeLocations(@JoynrRpcCallback(deserialisationType = ListGpsLocationToken.class) Callback<List<GpsLocation>> callback,
                                      @JoynrRpcParam(value = "input", deserialisationType = ListGpsLocationToken.class) List<GpsLocation> input) {
        }

        @Override
        public void toLowerCase(@JoynrRpcCallback(deserialisationType = StringToken.class) Callback<String> callback,
                                @JoynrRpcParam("inputString") String inputString) {
        }

        @Override
        public void waitTooLong(@JoynrRpcCallback(deserialisationType = StringToken.class) Callback<String> callback,
                                @JoynrRpcParam("ttl_ms") Long ttl_ms) {
        }

        @Override
        public void sayHello(@JoynrRpcCallback(deserialisationType = StringToken.class) Callback<String> callback) {
        }

        @Override
        public void checkVowel(@JoynrRpcCallback(deserialisationType = BooleanToken.class) Callback<Boolean> callback,
                               @JoynrRpcParam("inputVowel") Vowel inputVowel) {
        }

        @Override
        public void optimizeLocationList(@JoynrRpcCallback(deserialisationType = ListGpsLocationToken.class) Callback<List<GpsLocation>> callback,
                                         @JoynrRpcParam(value = "inputList", deserialisationType = ListGpsLocationToken.class) List<GpsLocation> inputList) {
        }

        @Override
        public void setLocation(Callback<Void> callback, GpsLocation location) {
        }

        @Override
        public void setFirstPrime(Callback<Void> callback, Integer firstPrime) {
        }

        @Override
        public void setListOfInts(Callback<Void> callback, List<Integer> listOfInts) {
        }

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    @Ignore
    public void registerProviderCreateProxyAndCallMethod() throws JoynrArbitrationException,
                                                          JoynrIllegalStateException, InterruptedException {
        int result;
        ProxyBuilder<testProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        result = proxy.addNumbers(6, 3, 2);
        Assert.assertEquals(11, result);

    }

    @Ignore
    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void sendObjectsAsArgumentAndReturnValue() throws JoynrArbitrationException, JoynrIllegalStateException,
                                                     InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        List<GpsLocation> locationList = new ArrayList<GpsLocation>();
        locationList.add(new GpsLocation(50.1, 20.1, 500.0, GpsFixEnum.MODE3D, 0.0, 0.0, 0.0, 0.0, 0l, 0l, 1000));
        locationList.add(new GpsLocation(50.1, 20.1, 500.0, GpsFixEnum.MODE3D, 0.0, 0.0, 0.0, 0.0, 0l, 0l, 1000));
        locationList.add(new GpsLocation(50.1, 20.1, 500.0, GpsFixEnum.MODE3D, 0.0, 0.0, 0.0, 0.0, 0l, 0l, 1000));
        Trip testObject = new Trip(locationList, "Title");
        Trip result;

        result = proxy.optimizeTrip(testObject);
        Assert.assertEquals(Double.valueOf(500.0), result.getLocations().get(0).getAltitude());

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncMethodCallWithCallback() throws JoynrArbitrationException, JoynrIllegalStateException,
                                             InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

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

    @Ignore
    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncMethodCallWithTtlExpiring() throws JoynrArbitrationException, JoynrIllegalStateException,
                                                InterruptedException {

        ProxyBuilder<testProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     testProxy.class);
        long ttl = 2000L;
        MessagingQos testMessagingQos = new MessagingQos(ttl);
        DiscoveryQos testDiscoveryQos = new DiscoveryQos(30000, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);
        testProxy proxyShortTll = proxyBuilder.setMessagingQos(testMessagingQos)
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

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testMethodWithEnumInputReturnsResult() throws JoynrArbitrationException, JoynrIllegalStateException,
                                                      InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        TestEnum input = TestEnum.TWO;

        int result = proxy.methodWithEnumParameter(input);
        assertEquals(2, result);

    }

    @Ignore
    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testVoidOperation() throws JoynrArbitrationException, JoynrIllegalStateException, InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

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

    @Test
    public void testAsyncProviderCall() {
        ProxyBuilder<testProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domainAsync,
                                                                                                     testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        proxy.methodStringDoubleParameters("text", 42d);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testMethodWithNullEnumInputReturnsSomethingSensible() throws JoynrArbitrationException,
                                                                     JoynrIllegalStateException, InterruptedException {
        TestEnum input = null;
        ProxyBuilder<testProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        int result = proxy.methodWithEnumParameter(input);
        assertEquals(42, result);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void sendingANullValueOnceDoesntCrashProvider() throws JoynrArbitrationException,
                                                          JoynrIllegalStateException, InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        proxy.methodWithEnumParameter(null);
        TestEnum input = TestEnum.TWO;
        int result = proxy.methodWithEnumParameter(input);
        assertEquals(2, result);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testEnumAttribute() throws JoynrArbitrationException, JoynrIllegalStateException, InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        proxy.setEnumAttribute(TestEnum.TWO);
        TestEnum result = proxy.getEnumAttribute();
        assertEquals(TestEnum.TWO, result);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testSimpleBroadcast() throws JoynrArbitrationException, JoynrIllegalStateException,
                                     InterruptedException {
        final Semaphore broadcastReceived = new Semaphore(0);
        final GpsLocation gpsLocation = new GpsLocation(1.0,
                                                        2.0,
                                                        3.0,
                                                        GpsFixEnum.MODE3D,
                                                        4.0,
                                                        5.0,
                                                        6.0,
                                                        7.0,
                                                        8L,
                                                        9L,
                                                        10);
        final Double currentSpeed = Double.MAX_VALUE;

        ProxyBuilder<testProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        long minInterval_ms = 0;
        long expiryDate = System.currentTimeMillis() + CONST_DEFAULT_TEST_TIMEOUT;
        long publicationTtl_ms = CONST_DEFAULT_TEST_TIMEOUT;
        OnChangeSubscriptionQos subscriptionQos = new OnChangeSubscriptionQos(minInterval_ms,
                                                                              expiryDate,
                                                                              publicationTtl_ms);
        proxy.subscribeToLocationUpdateWithSpeedBroadcast(new LocationUpdateWithSpeedBroadcastListener() {

            @Override
            public void receive(GpsLocation receivedGpsLocation, Double receivedCurrentSpeed) {
                assertEquals(gpsLocation, receivedGpsLocation);
                assertEquals(currentSpeed, receivedCurrentSpeed);
                broadcastReceived.release();

            }
        }, subscriptionQos);

        // wait a little to allow arbitration to finish, and to allow the subscription request to arrive at the provider
        Thread.sleep(300);
        provider.fireBroadcast("locationUpdateWithSpeed", null, gpsLocation, currentSpeed);
        broadcastReceived.acquire();

    }

    @Ignore
    // methods that return enums are not working at the moment - see JOYN-1027
    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testMethodWithEnumOutput() {
        // TODO write this test, once JOYN-1027 is solved
    }

    @Ignore
    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncMethodCallWithCallbackAndParameter() throws JoynrArbitrationException, JoynrIllegalStateException,
                                                         InterruptedException {

        ProxyBuilder<testProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     testProxy.class);

        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        Future<String> future = proxy.toLowerCase(callback, "Argument");
        String answer = future.getReply(21000);

        Assert.assertEquals(RequestStatusCode.OK, future.getStatus().getCode());
        String expected = "argument";
        Assert.assertEquals(expected, answer);
        verify(callback).onSuccess(expected);
        verifyNoMoreInteractions(callback);

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncMethodCallWithIntegerParametersAndFuture() throws JoynrArbitrationException,
                                                               JoynrIllegalStateException, InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        Future<Integer> future = proxy.addNumbers(callbackInteger, 1, 2, 3);
        Integer reply = future.getReply(30000);

        Assert.assertEquals(RequestStatusCode.OK, future.getStatus().getCode());
        Integer expected = 6;
        Assert.assertEquals(expected, reply);
        verify(callbackInteger).onSuccess(expected);
        verifyNoMoreInteractions(callbackInteger);

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncMethodCallWithEnumParametersAndFuture() throws JoynrArbitrationException,
                                                            JoynrIllegalStateException, InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        Future<Integer> future = proxy.methodWithEnumParameter(callbackInteger, TestEnum.TWO);
        Integer reply = future.getReply(40000);

        Integer expected = 2;
        Assert.assertEquals(RequestStatusCode.OK, future.getStatus().getCode());
        Assert.assertEquals(expected, reply);
        verify(callbackInteger).onSuccess(expected);
        verifyNoMoreInteractions(callbackInteger);

    }

    @Ignore
    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncMethodCallWithEnumListReturned() throws JoynrArbitrationException, JoynrIllegalStateException,
                                                     InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        List<TestEnum> enumList = proxy.methodWithEnumListReturn(2);
        assertArrayEquals(new TestEnum[]{ TestEnum.TWO }, enumList.toArray());

    }

    @Ignore
    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void overloadedMethodWithInheritance() throws JoynrArbitrationException, JoynrIllegalStateException,
                                                 InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        DerivedStruct derivedStruct = new DerivedStruct();
        AnotherDerivedStruct anotherDerivedStruct = new AnotherDerivedStruct();

        String anotherDerivedResult = proxy.overloadedOperation(anotherDerivedStruct);
        String derivedResult = proxy.overloadedOperation(derivedStruct);
        Assert.assertEquals("DerivedStruct", derivedResult);
        Assert.assertEquals("AnotherDerivedStruct", anotherDerivedResult);

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void overloadedMethodWithDifferentReturnTypes() throws JoynrArbitrationException,
                                                          JoynrIllegalStateException, InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = dummyConsumerApplication.getRuntime().getProxyBuilder(domain,
                                                                                                     testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        ComplexTestType expectedResult1 = new ComplexTestType(42, 42);
        ComplexTestType2 expectedResult2 = new ComplexTestType2(43, 44);

        ComplexTestType result1 = proxy.overloadedOperation("42");
        ComplexTestType2 result2 = proxy.overloadedOperation("43", "44");

        Assert.assertEquals(expectedResult1, result1);
        Assert.assertEquals(expectedResult2, result2);
    }
}
