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
import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;
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
import joynr.tests.testBroadcastInterface.LocationUpdateWithSpeedBroadcastAdapter;
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
            if (TestEnum.ONE.equals(input)) {
                return 1;
            }
            if (TestEnum.TWO.equals(input)) {
                return 2;
            }
            if (TestEnum.ZERO.equals(input)) {
                return 0;
            }

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
            return Arrays.asList(new TestEnum[]{ TestEnum.getEnumValue(input) });
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
        public Promise<Deferred<TestEnum>> getEnumAttribute() {
            Deferred<TestEnum> deferred = new Deferred<TestEnum>();
            deferred.resolve(enumAttribute);
            return new Promise<Deferred<TestEnum>>(deferred);
        }

        @Override
        public Promise<DeferredVoid> setEnumAttribute(TestEnum enumAttribute) {
            DeferredVoid deferred = new DeferredVoid();
            this.enumAttribute = enumAttribute;
            deferred.resolve();
            return new Promise<DeferredVoid>(deferred);
        }

        @Override
        public Promise<Deferred<GpsLocation>> getLocation() {
            Deferred<GpsLocation> deferred = new Deferred<GpsLocation>();
            deferred.resolve(location);
            return new Promise<Deferred<GpsLocation>>(deferred);
        }

        @Override
        public Promise<Deferred<Trip>> getMytrip() {
            Deferred<Trip> deferred = new Deferred<Trip>();
            deferred.resolve(myTrip);
            return new Promise<Deferred<Trip>>(deferred);
        }

        @Override
        public Promise<Deferred<GpsLocation>> getYourLocation() {
            Deferred<GpsLocation> deferred = new Deferred<GpsLocation>();
            deferred.resolve(new GpsLocation());
            return new Promise<Deferred<GpsLocation>>(deferred);
        }

        @Override
        public Promise<Deferred<Integer>> getFirstPrime() {
            Deferred<Integer> deferred = new Deferred<Integer>();
            deferred.resolve(10);
            return new Promise<Deferred<Integer>>(deferred);
        }

        @Override
        public Promise<Deferred<List<Integer>>> getListOfInts() {
            Deferred<List<Integer>> deferred = new Deferred<List<Integer>>();
            deferred.resolve(new ArrayList<Integer>());
            return new Promise<Deferred<List<Integer>>>(deferred);
        }

        @Override
        public Promise<Deferred<List<GpsLocation>>> getListOfLocations() {
            Deferred<List<GpsLocation>> deferred = new Deferred<List<GpsLocation>>();
            deferred.resolve(new ArrayList<GpsLocation>());
            return new Promise<Deferred<List<GpsLocation>>>(deferred);
        }

        @Override
        public Promise<Deferred<List<String>>> getListOfStrings() {
            Deferred<List<String>> deferred = new Deferred<List<String>>();
            deferred.resolve(listOfStrings);
            return new Promise<Deferred<List<String>>>(deferred);
        }

        @Override
        public Promise<DeferredVoid> setListOfStrings(List<String> listOfStrings) {
            DeferredVoid deferred = new DeferredVoid();
            this.listOfStrings = listOfStrings;
            deferred.resolve();
            return new Promise<DeferredVoid>(deferred);
        }

        @Override
        public Promise<Deferred<Integer>> getTestAttribute() {
            Deferred<Integer> deferred = new Deferred<Integer>();
            deferred.resolve(testAttribute);
            return new Promise<Deferred<Integer>>(deferred);
        }

        @Override
        public Promise<DeferredVoid> setTestAttribute(Integer testAttribute) {
            DeferredVoid deferred = new DeferredVoid();
            this.testAttribute = testAttribute;
            deferred.resolve();
            return new Promise<DeferredVoid>(deferred);
        }

        @Override
        public Promise<Deferred<GpsLocation>> getComplexTestAttribute() {
            Deferred<GpsLocation> deferred = new Deferred<GpsLocation>();
            deferred.resolve(complexTestAttribute);
            return new Promise<Deferred<GpsLocation>>(deferred);
        }

        @Override
        public Promise<DeferredVoid> setComplexTestAttribute(GpsLocation complexTestAttribute) {
            DeferredVoid deferred = new DeferredVoid();
            this.complexTestAttribute = complexTestAttribute;
            deferred.resolve();
            return new Promise<DeferredVoid>(deferred);
        }

        @Override
        public Promise<Deferred<Integer>> getReadWriteAttribute() {
            return new Promise<Deferred<Integer>>(new Deferred<Integer>());
        }

        @Override
        public Promise<DeferredVoid> setReadWriteAttribute(Integer readWriteAttribute) {
            return new Promise<DeferredVoid>(new DeferredVoid());
        }

        @Override
        public Promise<Deferred<Integer>> getReadOnlyAttribute() {
            return new Promise<Deferred<Integer>>(new Deferred<Integer>());
        }

        @Override
        public Promise<Deferred<Integer>> getWriteOnly() {
            return new Promise<Deferred<Integer>>(new Deferred<Integer>());
        }

        @Override
        public Promise<DeferredVoid> setWriteOnly(Integer writeOnly) {
            return new Promise<DeferredVoid>(new DeferredVoid());
        }

        @Override
        public Promise<Deferred<Integer>> getNotifyWriteOnly() {
            return new Promise<Deferred<Integer>>(new Deferred<Integer>());
        }

        @Override
        public Promise<DeferredVoid> setNotifyWriteOnly(Integer notifyWriteOnly) {
            return new Promise<DeferredVoid>(new DeferredVoid());
        }

        @Override
        public Promise<Deferred<Integer>> getNotifyReadOnly() {
            return new Promise<Deferred<Integer>>(new Deferred<Integer>());
        }

        @Override
        public Promise<Deferred<Integer>> getNotifyReadWrite() {
            return new Promise<Deferred<Integer>>(new Deferred<Integer>());
        }

        @Override
        public Promise<DeferredVoid> setNotifyReadWrite(Integer notifyReadWrite) {
            return new Promise<DeferredVoid>(new DeferredVoid());
        }

        @Override
        public Promise<Deferred<Integer>> getNotify() {
            return new Promise<Deferred<Integer>>(new Deferred<Integer>());
        }

        @Override
        public Promise<DeferredVoid> setNotify(Integer notify) {
            return new Promise<DeferredVoid>(new DeferredVoid());
        }

        @Override
        public Promise<Deferred<Integer>> getATTRIBUTEWITHCAPITALLETTERS() {
            return new Promise<Deferred<Integer>>(new Deferred<Integer>());
        }

        @Override
        public Promise<DeferredVoid> setATTRIBUTEWITHCAPITALLETTERS(Integer aTTRIBUTEWITHCAPITALLETTERS) {
            return new Promise<DeferredVoid>(new DeferredVoid());
        }

        @Override
        public Promise<AddNumbersDeferred> addNumbers(Integer first, Integer second, Integer third) {
            return new Promise<AddNumbersDeferred>(new AddNumbersDeferred());
        }

        @Override
        public Promise<SumIntsDeferred> sumInts(List<Integer> ints) {
            return new Promise<SumIntsDeferred>(new SumIntsDeferred());
        }

        @Override
        public Promise<MethodWithNoInputParametersDeferred> methodWithNoInputParameters() {
            return new Promise<MethodWithNoInputParametersDeferred>(new MethodWithNoInputParametersDeferred());
        }

        @Override
        public Promise<MethodWithEnumParameterDeferred> methodWithEnumParameter(TestEnum input) {
            return new Promise<MethodWithEnumParameterDeferred>(new MethodWithEnumParameterDeferred());
        }

        @Override
        public Promise<MethodWithEnumListParameterDeferred> methodWithEnumListParameter(List<TestEnum> input) {
            return new Promise<MethodWithEnumListParameterDeferred>(new MethodWithEnumListParameterDeferred());
        }

        @Override
        public Promise<MethodWithEnumReturnDeferred> methodWithEnumReturn(Integer input) {
            return new Promise<MethodWithEnumReturnDeferred>(new MethodWithEnumReturnDeferred());
        }

        @Override
        public Promise<MethodWithEnumListReturnDeferred> methodWithEnumListReturn(Integer input) {
            return new Promise<MethodWithEnumListReturnDeferred>(new MethodWithEnumListReturnDeferred());
        }

        @Override
        public Promise<MethodWithByteArrayDeferred> methodWithByteArray(List<Byte> input) {
            return new Promise<MethodWithByteArrayDeferred>(new MethodWithByteArrayDeferred());
        }

        @Override
        public Promise<DeferredVoid> methodEnumDoubleParameters(TestEnum enumParam, Double doubleParam) {
            return new Promise<DeferredVoid>(new DeferredVoid());
        }

        @Override
        public Promise<MethodWithEnumReturnValueDeferred> methodWithEnumReturnValue() {
            MethodWithEnumReturnValueDeferred deferred = new MethodWithEnumReturnValueDeferred();
            deferred.resolve(TestEnum.TWO);
            return new Promise<MethodWithEnumReturnValueDeferred>(deferred);
        }

        @Override
        public Promise<Deferred<TestEnum>> getEnumAttributeReadOnly() {
            Deferred<TestEnum> deferred = new Deferred<TestEnum>();
            deferred.resolve(TestEnum.ONE);
            return new Promise<Deferred<TestEnum>>(deferred);
        }

        @Override
        public Promise<DeferredVoid> methodStringDoubleParameters(String stringParam, Double doubleParam) {
            DeferredVoid deferred = new DeferredVoid();
            deferred.resolve();
            return new Promise<DeferredVoid>(deferred);
        }

        @Override
        public Promise<DeferredVoid> methodCustomCustomParameters(ComplexTestType customParam1,
                                                                  ComplexTestType2 customParam2) {
            return new Promise<DeferredVoid>(new DeferredVoid());
        }

        @Override
        public Promise<DeferredVoid> methodStringDoubleListParameters(String stringParam, List<Double> doubleListParam) {
            return new Promise<DeferredVoid>(new DeferredVoid());
        }

        @Override
        public Promise<DeferredVoid> methodCustomCustomListParameters(ComplexTestType customParam,
                                                                      List<ComplexTestType2> customListParam) {
            return new Promise<DeferredVoid>(new DeferredVoid());
        }

        @Override
        public Promise<DeferredVoid> customTypeAndListParameter(ComplexTestType complexTestType,
                                                                List<BaseStruct> complexArray) {
            return new Promise<DeferredVoid>(new DeferredVoid());
        }

        @Override
        public Promise<DeferredVoid> voidOperation() {
            return new Promise<DeferredVoid>(new DeferredVoid());
        }

        @Override
        public Promise<DeferredVoid> stringAndBoolParameters(String stringParam, Boolean boolParam) {
            return new Promise<DeferredVoid>(new DeferredVoid());
        }

        @Override
        public Promise<ReturnPrimeNumbersDeferred> returnPrimeNumbers(Integer upperBound) {
            return new Promise<ReturnPrimeNumbersDeferred>(new ReturnPrimeNumbersDeferred());
        }

        @Override
        public Promise<OptimizeTripDeferred> optimizeTrip(Trip input) {
            return new Promise<OptimizeTripDeferred>(new OptimizeTripDeferred());
        }

        @Override
        public Promise<OverloadedOperation1Deferred> overloadedOperation(DerivedStruct input) {
            return new Promise<OverloadedOperation1Deferred>(new OverloadedOperation1Deferred());
        }

        @Override
        public Promise<OverloadedOperation1Deferred> overloadedOperation(AnotherDerivedStruct input) {
            return new Promise<OverloadedOperation1Deferred>(new OverloadedOperation1Deferred());
        }

        @Override
        public Promise<OverloadedOperation2Deferred> overloadedOperation(String input) {
            return new Promise<OverloadedOperation2Deferred>(new OverloadedOperation2Deferred());
        }

        @Override
        public Promise<OverloadedOperation3Deferred> overloadedOperation(String input1, String input2) {
            return new Promise<OverloadedOperation3Deferred>(new OverloadedOperation3Deferred());
        }

        @Override
        public Promise<OptimizeLocationsDeferred> optimizeLocations(List<GpsLocation> input) {
            return new Promise<OptimizeLocationsDeferred>(new OptimizeLocationsDeferred());
        }

        @Override
        public Promise<ToLowerCaseDeferred> toLowerCase(String inputString) {
            return new Promise<ToLowerCaseDeferred>(new ToLowerCaseDeferred());
        }

        @Override
        public Promise<WaitTooLongDeferred> waitTooLong(Long ttl_ms) {
            return new Promise<WaitTooLongDeferred>(new WaitTooLongDeferred());
        }

        @Override
        public Promise<SayHelloDeferred> sayHello() {
            return new Promise<SayHelloDeferred>(new SayHelloDeferred());
        }

        @Override
        public Promise<CheckVowelDeferred> checkVowel(Vowel inputVowel) {
            return new Promise<CheckVowelDeferred>(new CheckVowelDeferred());
        }

        @Override
        public Promise<OptimizeLocationListDeferred> optimizeLocationList(List<GpsLocation> inputList) {
            return new Promise<OptimizeLocationListDeferred>(new OptimizeLocationListDeferred());
        }

        @Override
        public Promise<DeferredVoid> setLocation(GpsLocation location) {
            return new Promise<DeferredVoid>(new DeferredVoid());
        }

        @Override
        public Promise<DeferredVoid> setFirstPrime(Integer firstPrime) {
            return new Promise<DeferredVoid>(new DeferredVoid());
        }

        @Override
        public Promise<DeferredVoid> setListOfInts(List<Integer> listOfInts) {
            return new Promise<DeferredVoid>(new DeferredVoid());
        }

        // TODO: remove begin
        @Override
        public void addNumbers(Callback<Integer> callback, Integer first, Integer second, Integer third) {
            // TODO Auto-generated method stub

        }

        @Override
        public void sumInts(Callback<Integer> callback, List<Integer> ints) {
            // TODO Auto-generated method stub

        }

        @Override
        public void methodWithNoInputParameters(Callback<Integer> callback) {
            // TODO Auto-generated method stub

        }

        @Override
        public void methodWithEnumParameter(Callback<Integer> callback, TestEnum input) {
            // TODO Auto-generated method stub

        }

        @Override
        public void methodWithEnumListParameter(Callback<Integer> callback, List<TestEnum> input) {
            // TODO Auto-generated method stub

        }

        @Override
        public void methodWithEnumReturn(Callback<TestEnum> callback, Integer input) {
            // TODO Auto-generated method stub

        }

        @Override
        public void methodWithEnumListReturn(Callback<List<TestEnum>> callback, Integer input) {
            // TODO Auto-generated method stub

        }

        @Override
        public void methodWithByteArray(Callback<List<Byte>> callback, List<Byte> input) {
            // TODO Auto-generated method stub

        }

        @Override
        public void methodEnumDoubleParameters(Callback<Void> callback, TestEnum enumParam, Double doubleParam) {
            // TODO Auto-generated method stub

        }

        @Override
        public void methodStringDoubleParameters(final Callback<Void> callback, String stringParam, Double doubleParam) {
            methodStringDoubleParameters(stringParam, doubleParam).then(new PromiseListener() {

                @Override
                public void onRejection(JoynrException error) {
                    callback.onFailure(error);
                }

                @Override
                public void onFulfillment(Object... values) {
                    callback.onSuccess(null);
                }
            });

        }

        @Override
        public void methodCustomCustomParameters(Callback<Void> callback,
                                                 ComplexTestType customParam1,
                                                 ComplexTestType2 customParam2) {
            // TODO Auto-generated method stub

        }

        @Override
        public void methodStringDoubleListParameters(Callback<Void> callback,
                                                     String stringParam,
                                                     List<Double> doubleListParam) {
            // TODO Auto-generated method stub

        }

        @Override
        public void methodCustomCustomListParameters(Callback<Void> callback,
                                                     ComplexTestType customParam,
                                                     List<ComplexTestType2> customListParam) {
            // TODO Auto-generated method stub

        }

        @Override
        public void customTypeAndListParameter(Callback<Void> callback,
                                               ComplexTestType complexTestType,
                                               List<BaseStruct> complexArray) {
            // TODO Auto-generated method stub

        }

        @Override
        public void voidOperation(Callback<Void> callback) {
            // TODO Auto-generated method stub

        }

        @Override
        public void stringAndBoolParameters(Callback<Void> callback, String stringParam, Boolean boolParam) {
            // TODO Auto-generated method stub

        }

        @Override
        public void returnPrimeNumbers(Callback<List<Integer>> callback, Integer upperBound) {
            // TODO Auto-generated method stub

        }

        @Override
        public void optimizeTrip(Callback<Trip> callback, Trip input) {
            // TODO Auto-generated method stub

        }

        @Override
        public void overloadedOperation(Callback<String> callback, DerivedStruct input) {
            // TODO Auto-generated method stub

        }

        @Override
        public void overloadedOperation(Callback<String> callback, AnotherDerivedStruct input) {
            // TODO Auto-generated method stub

        }

        @Override
        public void overloadedOperation(Callback<ComplexTestType> callback, String input) {
            // TODO Auto-generated method stub

        }

        @Override
        public void overloadedOperation(Callback<ComplexTestType2> callback, String input1, String input2) {
            // TODO Auto-generated method stub

        }

        @Override
        public void optimizeLocations(Callback<List<GpsLocation>> callback, List<GpsLocation> input) {
            // TODO Auto-generated method stub

        }

        @Override
        public void toLowerCase(Callback<String> callback, String inputString) {
            // TODO Auto-generated method stub

        }

        @Override
        public void waitTooLong(Callback<String> callback, Long ttl_ms) {
            // TODO Auto-generated method stub

        }

        @Override
        public void sayHello(Callback<String> callback) {
            // TODO Auto-generated method stub

        }

        @Override
        public void methodWithEnumReturnValue(Callback<TestEnum> callback) {
            // TODO Auto-generated method stub

        }

        @Override
        public void checkVowel(Callback<Boolean> callback, Vowel inputVowel) {
            // TODO Auto-generated method stub

        }

        @Override
        public void optimizeLocationList(Callback<List<GpsLocation>> callback, List<GpsLocation> inputList) {
            // TODO Auto-generated method stub

        }
        // TODO: remove end
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
        proxy.subscribeToLocationUpdateWithSpeedBroadcast(new LocationUpdateWithSpeedBroadcastAdapter() {

            @Override
            public void onReceive(GpsLocation receivedGpsLocation, Double receivedCurrentSpeed) {
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
