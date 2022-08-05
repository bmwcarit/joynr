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
package io.joynr.integration;

import static io.joynr.util.JoynrUtil.createUuidString;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.Random;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestName;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Module;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.dispatcher.rpc.RequestStatusCode;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrTimeoutException;
import io.joynr.exceptions.JoynrWaitExpiredException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.provider.AbstractJoynrProvider;
import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.proxy.Callback;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.PropertyLoader;
import joynr.ImmutableMessage;
import joynr.MulticastSubscriptionQos;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.test.JoynrTestLoggingRule;
import joynr.tests.DefaulttestProvider;
import joynr.tests.test.MethodWithErrorEnumExtendedErrorEnum;
import joynr.tests.test.MethodWithImplicitErrorEnumErrorEnum;
import joynr.tests.testAsync.MethodWithMultipleOutputParametersCallback;
import joynr.tests.testBroadcastInterface.BroadcastWithMapParametersBroadcastAdapter;
import joynr.tests.testBroadcastInterface.LocationUpdateWithSpeedBroadcastAdapter;
import joynr.tests.testProxy;
import joynr.tests.testSync.MethodWithMultipleOutputParametersReturned;
import joynr.tests.testTypes.AnotherDerivedStruct;
import joynr.tests.testTypes.ComplexTestType;
import joynr.tests.testTypes.ComplexTestType2;
import joynr.tests.testTypes.DerivedStruct;
import joynr.tests.testTypes.ErrorEnumBase;
import joynr.tests.testTypes.TestEnum;
import joynr.tests.testTypes.TestEnumWithoutValues;
import joynr.types.ProviderQos;
import joynr.types.Localisation.GpsFixEnum;
import joynr.types.Localisation.GpsLocation;
import joynr.types.Localisation.Trip;
import joynr.types.TestTypes.TStringKeyMap;

public abstract class AbstractProviderProxyEnd2EndTest extends JoynrEnd2EndTest {
    private static final String MAX_MESSAGE_SIZE = "4000000";

    private static final Logger logger = LoggerFactory.getLogger(AbstractProviderProxyEnd2EndTest.class);
    @Rule
    public JoynrTestLoggingRule joynrTestRule = new JoynrTestLoggingRule(logger);

    // This timeout must be shared by all integration test environments and
    // cannot be too short.
    protected static final long CONST_DEFAULT_TEST_TIMEOUT = 100000L;
    protected static final long CONST_DEFAULT_PROVIDER_REGISTRATION_TIMEOUT = 80000L;

    public static final Semaphore FIRE_AND_FORGET_SEMAPHORE = new Semaphore(1);
    public static final Semaphore FIRE_AND_FORGET_WITH_ENUM_WITH_VALUES_PARAM_SEMAPHORE = new Semaphore(0);
    public static final Semaphore FIRE_AND_FORGET_WITH_ENUM_WITHOUT_VALUES_PARAM_SEMAPHORE = new Semaphore(0);
    public static final Semaphore FIRE_AND_FORGET_WITHOUT_PARAMS_SEMAPHORE = new Semaphore(1);

    TestProvider provider;
    protected ProviderQos testProviderQos = new ProviderQos();
    String domain;
    String domainAsync;

    private static final String OUT_KEY = "outKey";
    private static final String OUT_VALUE = "outValue";
    public static final String TEST_STRING = "Test String";
    public static final Integer TEST_INTEGER = 633536;
    private static final TestEnum TEST_ENUM = TestEnum.TWO;
    public static final GpsLocation TEST_COMPLEXTYPE = new GpsLocation(1.0,
                                                                       2.0,
                                                                       2.5,
                                                                       GpsFixEnum.MODE2D,
                                                                       3.0,
                                                                       4.0,
                                                                       5.0,
                                                                       6.0,
                                                                       7L,
                                                                       89L,
                                                                       Integer.MAX_VALUE);

    private static final Integer FIRE_AND_FORGET_INT_PARAMETER = 1;
    private static final String FIRE_AND_FORGET_STRING_PARAMETER = "test";
    private static final ComplexTestType FIRE_AND_FORGET_COMPLEX_PARAMETER = new ComplexTestType();

    protected List<JoynrRuntime> createdRuntimes = new ArrayList<JoynrRuntime>();
    protected static Properties baseTestConfig;

    long timeTookToRegisterProvider;

    @Rule
    public TestName name = new TestName();

    // The timeouts should not be to small because some test environments are slow
    protected MessagingQos messagingQos = new MessagingQos(15000);
    protected DiscoveryQos discoveryQos = new DiscoveryQos(15000, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);

    @Mock
    Callback<String> callback;

    @Mock
    Callback<Integer> callbackInteger;

    @Mock
    Callback<Void> callbackVoid;

    @Mock
    private CallbackWithModeledError<Void, ErrorEnumBase> callbackWithApplicationExceptionErrorEnumBase;

    @Mock
    private CallbackWithModeledError<Void, MethodWithErrorEnumExtendedErrorEnum> callbackWithApplicationExceptionMethodWithErrorEnumExtendedErrorEnum;

    @Mock
    private CallbackWithModeledError<Void, MethodWithImplicitErrorEnumErrorEnum> callbackWithApplicationExceptionMethodWithImplicitErrorEnumErrorEnum;

    private TestAsyncProviderImpl providerAsync;

    protected JoynrRuntime providerRuntime;
    protected JoynrRuntime consumerRuntime;

    // Overridden by test environment implementations
    protected abstract JoynrRuntime getRuntime(Properties joynrConfig, Module... modules);

    @BeforeClass
    public static void setupBaseConfig() {
        baseTestConfig = new Properties();
        baseTestConfig.put(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS, "10");
        baseTestConfig.put(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_MINIMUM_RETRY_INTERVAL_MS, "200");
    }

    @Before
    public void baseSetup() throws Exception {

        MockitoAnnotations.initMocks(this);

        // prints the tests name in the log so we know what we are testing
        String methodName = name.getMethodName();
        logger.info("{} setup beginning...", methodName);

        domain = "ProviderProxyEnd2EndTest." + name.getMethodName() + System.currentTimeMillis();
        domainAsync = domain + "Async";

        // use channelNames = test name
        String channelIdProvider = "JavaTest-" + methodName + createUuidString() + "-end2endTestProvider";
        String channelIdConsumer = "JavaTest-" + methodName + createUuidString() + "-end2endConsumer";

        Properties joynrConfigProvider = PropertyLoader.loadProperties("testMessaging.properties");
        joynrConfigProvider.put(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL,
                                "localdomain." + createUuidString());
        joynrConfigProvider.put(MessagingPropertyKeys.CHANNELID, channelIdProvider);
        joynrConfigProvider.put(MessagingPropertyKeys.RECEIVERID, createUuidString());
        joynrConfigProvider.put(ConfigurableMessagingSettings.PROPERTY_MAX_MESSAGE_SIZE, MAX_MESSAGE_SIZE);

        providerRuntime = getRuntime(joynrConfigProvider, getSubscriptionPublisherFactoryModule());

        if (providerRuntime != null) {
            createdRuntimes.add(providerRuntime);
        }

        Properties joynrConfigConsumer = PropertyLoader.loadProperties("testMessaging.properties");
        joynrConfigConsumer.put(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL,
                                "localdomain." + createUuidString());
        joynrConfigConsumer.put(MessagingPropertyKeys.CHANNELID, channelIdConsumer);
        joynrConfigConsumer.put(MessagingPropertyKeys.RECEIVERID, createUuidString());
        joynrConfigConsumer.put(ConfigurableMessagingSettings.PROPERTY_MAX_MESSAGE_SIZE, MAX_MESSAGE_SIZE);

        consumerRuntime = getRuntime(joynrConfigConsumer, getSubscriptionPublisherFactoryModule());

        if (consumerRuntime != null) {
            createdRuntimes.add(consumerRuntime);
        }

        provider = new TestProvider();
        testProviderQos.setPriority(System.currentTimeMillis());

        providerAsync = new TestAsyncProviderImpl();

        // check that registerProvider does not block
        long startTime = System.currentTimeMillis();
        providerRuntime.getProviderRegistrar(domain, provider)
                       .withProviderQos(testProviderQos)
                       .awaitGlobalRegistration()
                       .register()
                       .get(CONST_DEFAULT_PROVIDER_REGISTRATION_TIMEOUT);
        long endTime = System.currentTimeMillis();
        timeTookToRegisterProvider = endTime - startTime;
        providerRuntime.getProviderRegistrar(domainAsync, providerAsync)
                       .withProviderQos(testProviderQos)
                       .awaitGlobalRegistration()
                       .register()
                       .get(CONST_DEFAULT_PROVIDER_REGISTRATION_TIMEOUT);

        // this sleep greatly speeds up the tests (400 ms vs 2500 / test) by
        // making sure the channel is created before first messages sent.
        Thread.sleep(100);
        logger.info("Setup finished");

    }

    @After
    public void tearDown() throws InterruptedException {
        if (consumerRuntime != null) {
            consumerRuntime.shutdown(true);
        }

        if (providerRuntime != null) {
            providerRuntime.unregisterProvider(domain, provider);
            providerRuntime.unregisterProvider(domainAsync, provider);
            // wait grace period for the unregister (remove) message to get
            // sent to global discovery
            Thread.sleep(1000);
            providerRuntime.shutdown(true);
        }

        //do not shutdown runtimes twice
        createdRuntimes.remove(providerRuntime);
        createdRuntimes.remove(consumerRuntime);

        for (JoynrRuntime createdRuntime : createdRuntimes) {
            createdRuntime.shutdown(true);
        }
    }

    protected static class TestProvider extends DefaulttestProvider {
        @Override
        public Promise<EchoCallingPrincipalDeferred> echoCallingPrincipal() {
            EchoCallingPrincipalDeferred deferred = new EchoCallingPrincipalDeferred();
            deferred.resolve(AbstractJoynrProvider.getCallContext().getPrincipal());
            return new Promise<EchoCallingPrincipalDeferred>(deferred);
        }

        @Override
        public Promise<Deferred<Integer>> getAttributeWithProviderRuntimeException() {
            Deferred<Integer> deferred = new Deferred<Integer>();
            ProviderRuntimeException error = new ProviderRuntimeException("ProviderRuntimeException");
            deferred.reject(error);
            return new Promise<Deferred<Integer>>(deferred);
        }

        @Override
        public Promise<DeferredVoid> setAttributeWithProviderRuntimeException(Integer attributeWithProviderRuntimeException) {
            throw new IllegalArgumentException("thrownException");
        }

        @Override
        public Promise<MethodWithMultipleOutputParametersDeferred> methodWithMultipleOutputParameters() {
            MethodWithMultipleOutputParametersDeferred deferred = new MethodWithMultipleOutputParametersDeferred();
            String aString = TEST_STRING;
            Integer aNumber = TEST_INTEGER;
            GpsLocation aComplexDataType = TEST_COMPLEXTYPE;
            TestEnum anEnumResult = TEST_ENUM;
            deferred.resolve(aString, aNumber, aComplexDataType, anEnumResult);
            return new Promise<MethodWithMultipleOutputParametersDeferred>(deferred);
        }

        @Override
        public Promise<MethodWithEnumParameterDeferred> methodWithEnumParameter(TestEnum input) {
            MethodWithEnumParameterDeferred deferred = new MethodWithEnumParameterDeferred();
            if (TestEnum.ONE.equals(input)) {
                deferred.resolve(1);
            } else if (TestEnum.TWO.equals(input)) {
                deferred.resolve(2);
            } else if (TestEnum.ZERO.equals(input)) {
                deferred.resolve(0);
            } else {
                deferred.resolve(42);
            }
            return new Promise<MethodWithEnumParameterDeferred>(deferred);
        }

        @Override
        public Promise<AddNumbersDeferred> addNumbers(Integer first, Integer second, Integer third) {
            AddNumbersDeferred deferred = new AddNumbersDeferred();
            deferred.resolve(first + second + third);
            return new Promise<AddNumbersDeferred>(deferred);
        }

        @Override
        public Promise<WaitTooLongDeferred> waitTooLong(Long ttl_ms) {
            WaitTooLongDeferred deferred = new WaitTooLongDeferred();
            String returnString = "";
            long enteredAt = System.currentTimeMillis();
            try {
                Thread.sleep(ttl_ms + 1);
            } catch (InterruptedException e) {
                returnString += "InterruptedException... ";
            }
            deferred.resolve(returnString + "time: " + (System.currentTimeMillis() - enteredAt));
            return new Promise<WaitTooLongDeferred>(deferred);
        }

        @Override
        public Promise<MethodWithEnumListReturnDeferred> methodWithEnumListReturn(Integer input) {
            MethodWithEnumListReturnDeferred deferred = new MethodWithEnumListReturnDeferred();
            deferred.resolve(new TestEnum[]{ TestEnum.getEnumValue(input) });
            return new Promise<MethodWithEnumListReturnDeferred>(deferred);
        }

        @Override
        public Promise<SayHelloDeferred> sayHello() {
            SayHelloDeferred deferred = new SayHelloDeferred();
            deferred.resolve("Hello");
            return new Promise<SayHelloDeferred>(deferred);
        }

        @Override
        public Promise<ToLowerCaseDeferred> toLowerCase(String inputString) {
            ToLowerCaseDeferred deferred = new ToLowerCaseDeferred();
            deferred.resolve(inputString.toLowerCase());
            return new Promise<ToLowerCaseDeferred>(deferred);
        }

        @Override
        public Promise<OptimizeTripDeferred> optimizeTrip(Trip input) {
            OptimizeTripDeferred deferred = new OptimizeTripDeferred();
            deferred.resolve(input);
            return new Promise<OptimizeTripDeferred>(deferred);
        }

        @Override
        public Promise<OverloadedOperation1Deferred> overloadedOperation(DerivedStruct s) {
            OverloadedOperation1Deferred deferred = new OverloadedOperation1Deferred();
            deferred.resolve("DerivedStruct");
            return new Promise<OverloadedOperation1Deferred>(deferred);
        }

        @Override
        public Promise<OverloadedOperation1Deferred> overloadedOperation(AnotherDerivedStruct s) {
            OverloadedOperation1Deferred deferred = new OverloadedOperation1Deferred();
            deferred.resolve("AnotherDerivedStruct");
            return new Promise<OverloadedOperation1Deferred>(deferred);
        }

        @Override
        public Promise<OverloadedOperation2Deferred> overloadedOperation(String input) {
            OverloadedOperation2Deferred deferred = new OverloadedOperation2Deferred();
            int result = Integer.parseInt(input);
            deferred.resolve(new ComplexTestType(result, result));
            return new Promise<OverloadedOperation2Deferred>(deferred);
        }

        @Override
        public Promise<OverloadedOperation3Deferred> overloadedOperation(String input1, String input2) {
            OverloadedOperation3Deferred deferred = new OverloadedOperation3Deferred();
            deferred.resolve(new ComplexTestType2(Integer.parseInt(input1), Integer.parseInt(input2)));
            return new Promise<OverloadedOperation3Deferred>(deferred);
        }

        @Override
        public Promise<MethodWithErrorEnumDeferred> methodWithErrorEnum() {
            MethodWithErrorEnumDeferred deferred = new MethodWithErrorEnumDeferred();
            ErrorEnumBase error = ErrorEnumBase.BASE_ERROR_TYPECOLLECTION;
            deferred.reject(error);
            return new Promise<MethodWithErrorEnumDeferred>(deferred);
        }

        @Override
        public Promise<MethodWithErrorEnumExtendedDeferred> methodWithErrorEnumExtended() {
            MethodWithErrorEnumExtendedDeferred deferred = new MethodWithErrorEnumExtendedDeferred();
            MethodWithErrorEnumExtendedErrorEnum error = MethodWithErrorEnumExtendedErrorEnum.IMPLICIT_ERROR_TYPECOLLECTION;
            deferred.reject(error);
            return new Promise<MethodWithErrorEnumExtendedDeferred>(deferred);
        }

        @Override
        public Promise<MethodWithImplicitErrorEnumDeferred> methodWithImplicitErrorEnum() {
            MethodWithImplicitErrorEnumDeferred deferred = new MethodWithImplicitErrorEnumDeferred();
            MethodWithImplicitErrorEnumErrorEnum error = MethodWithImplicitErrorEnumErrorEnum.IMPLICIT_ERROR;
            deferred.reject(error);
            return new Promise<MethodWithImplicitErrorEnumDeferred>(deferred);
        }

        @Override
        public Promise<DeferredVoid> methodWithProviderRuntimeException() {
            DeferredVoid deferred = new DeferredVoid();
            ProviderRuntimeException error = new ProviderRuntimeException("ProviderRuntimeException");
            deferred.reject(error);
            return new Promise<DeferredVoid>(deferred);
        }

        @Override
        public Promise<DeferredVoid> methodWithThrownException() {
            throw new IllegalArgumentException("thrownException");
        }

        @Override
        public Promise<MethodWithByteBufferDeferred> methodWithByteBuffer(Byte[] input) {
            MethodWithByteBufferDeferred deferred = new MethodWithByteBufferDeferred();
            deferred.resolve(input);
            return new Promise<MethodWithByteBufferDeferred>(deferred);
        }

        @Override
        public Promise<MethodWithByteArrayDeferred> methodWithByteArray(Byte[] input) {
            MethodWithByteArrayDeferred deferred = new MethodWithByteArrayDeferred();
            deferred.resolve(input);
            return new Promise<>(deferred);
        }

        @Override
        public void methodFireAndForgetWithoutParams() {
            AbstractProviderProxyEnd2EndTest.FIRE_AND_FORGET_WITHOUT_PARAMS_SEMAPHORE.release();
        }

        @Override
        public void methodFireAndForget(Integer intIn, String stringIn, ComplexTestType complexTestTypeIn) {
            if (intIn != 1) {
                throw new IllegalArgumentException("Didn't receive expected int value: " + FIRE_AND_FORGET_INT_PARAMETER
                        + ", instead: " + intIn);
            }
            if (!FIRE_AND_FORGET_STRING_PARAMETER.equals(stringIn)) {
                throw new IllegalArgumentException("Didn't receive expected string value: "
                        + FIRE_AND_FORGET_STRING_PARAMETER + ", instead: " + stringIn);
            }
            if (!FIRE_AND_FORGET_COMPLEX_PARAMETER.equals(complexTestTypeIn)) {
                throw new IllegalArgumentException("Didn't receive expected complex value: "
                        + FIRE_AND_FORGET_COMPLEX_PARAMETER + ", instead: " + complexTestTypeIn);
            }
            AbstractProviderProxyEnd2EndTest.FIRE_AND_FORGET_SEMAPHORE.release();
        }

        @Override
        public void methodFireAndForgetWithEnumWithValues(TestEnum enumIn) {
            AbstractProviderProxyEnd2EndTest.FIRE_AND_FORGET_WITH_ENUM_WITH_VALUES_PARAM_SEMAPHORE.release();
        }

        @Override
        public void methodFireAndForgetWithEnumWithoutValues(TestEnumWithoutValues enumIn) {
            AbstractProviderProxyEnd2EndTest.FIRE_AND_FORGET_WITH_ENUM_WITHOUT_VALUES_PARAM_SEMAPHORE.release();
        }

        @Override
        public Promise<MethodWithEnumWithoutValuesDeferred> methodWithEnumWithoutValues(TestEnumWithoutValues enumIn) {
            MethodWithEnumWithoutValuesDeferred deferred = new MethodWithEnumWithoutValuesDeferred();
            deferred.resolve(TestEnumWithoutValues.GAMMA);
            return new Promise<MethodWithEnumWithoutValuesDeferred>(deferred);
        }
    }

    protected static class TestAsyncProviderImpl extends DefaulttestProvider {
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
        public Promise<MapParametersDeferred> mapParameters(TStringKeyMap tStringMapIn) {
            MapParametersDeferred deferred = new MapParametersDeferred();
            tStringMapIn.put(OUT_KEY, OUT_VALUE);
            deferred.resolve(tStringMapIn);
            return new Promise<MapParametersDeferred>(deferred);
        }
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void registerProviderCreateProxyAndCallMethod() throws DiscoveryException, JoynrIllegalStateException,
                                                           InterruptedException {
        int result;
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        result = proxy.addNumbers(6, 3, 2);
        assertEquals(11, result);

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void sendObjectsAsArgumentAndReturnValue() throws DiscoveryException, JoynrIllegalStateException,
                                                      InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        List<GpsLocation> locationList = new ArrayList<GpsLocation>();
        locationList.add(new GpsLocation(50.1, 20.1, 500.0, GpsFixEnum.MODE3D, 0.0, 0.0, 0.0, 0.0, 0l, 0l, 1000));
        locationList.add(new GpsLocation(50.1, 20.1, 500.0, GpsFixEnum.MODE3D, 0.0, 0.0, 0.0, 0.0, 0l, 0l, 1000));
        locationList.add(new GpsLocation(50.1, 20.1, 500.0, GpsFixEnum.MODE3D, 0.0, 0.0, 0.0, 0.0, 0l, 0l, 1000));
        Trip testObject = new Trip(locationList.toArray(new GpsLocation[0]), "Title");
        Trip result;

        result = proxy.optimizeTrip(testObject);
        assertEquals(Double.valueOf(500.0), result.getLocations()[0].getAltitude());

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncMethodCallWithCallback() throws DiscoveryException, JoynrIllegalStateException,
                                              InterruptedException, JoynrWaitExpiredException, ApplicationException {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        Future<String> future = proxy.sayHello(callback);
        String answer = future.get(30000);
        assertEquals(RequestStatusCode.OK, future.getStatus().getCode());
        String expected = "Hello";
        assertEquals(expected, answer);
        verify(callback).resolve(expected);
        verifyNoMoreInteractions(callback);

        @SuppressWarnings("unchecked")
        // needed on jenkins
        Callback<String> callback2 = mock(Callback.class);
        Future<String> future2 = proxy.toLowerCase(callback2, "Argument");
        String answer2 = future2.get(30000);
        assertEquals(RequestStatusCode.OK, future2.getStatus().getCode());
        String expected2 = "argument";

        assertEquals(expected2, answer2);
        verify(callback2).resolve(expected2);
        verifyNoMoreInteractions(callback2);

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void calledMethodReturnsMultipleOutputParameters() throws Exception {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        MethodWithMultipleOutputParametersReturned result = proxy.methodWithMultipleOutputParameters();
        assertEquals(TEST_INTEGER, result.aNumber);
        assertEquals(TEST_STRING, result.aString);
        assertEquals(TEST_COMPLEXTYPE, result.aComplexDataType);
        assertEquals(TEST_ENUM, result.anEnumResult);

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void calledMethodReturnsMultipleOutputParametersAsyncCallback() throws Exception {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        final Object untilCallbackFinished = new Object();
        final Map<String, Object> result = new HashMap<String, Object>();

        proxy.methodWithMultipleOutputParameters(new MethodWithMultipleOutputParametersCallback() {

            @Override
            public void onFailure(JoynrRuntimeException error) {
                logger.error("Error in calledMethodReturnsMultipleOutputParametersAsyncCallback", error);
            }

            @Override
            public void onSuccess(String aString,
                                  Integer aNumber,
                                  GpsLocation aComplexDataType,
                                  TestEnum anEnumResult) {
                result.put("receivedString", aString);
                result.put("receivedNumber", aNumber);
                result.put("receivedComplexDataType", aComplexDataType);
                result.put("receivedEnum", anEnumResult);
                synchronized (untilCallbackFinished) {
                    untilCallbackFinished.notify();
                }
            }
        });

        synchronized (untilCallbackFinished) {
            untilCallbackFinished.wait(CONST_DEFAULT_TEST_TIMEOUT);
        }

        assertEquals(TEST_INTEGER, result.get("receivedNumber"));
        assertEquals(TEST_STRING, result.get("receivedString"));
        assertEquals(TEST_COMPLEXTYPE, result.get("receivedComplexDataType"));
        assertEquals(TEST_ENUM, result.get("receivedEnum"));
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void calledMethodReturnsMultipleOutputParametersAsyncFuture() throws Exception {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        Future<MethodWithMultipleOutputParametersReturned> future = proxy.methodWithMultipleOutputParameters(new MethodWithMultipleOutputParametersCallback() {
            @Override
            public void onFailure(JoynrRuntimeException error) {
                logger.error("Error in calledMethodReturnsMultipleOutputParametersAsyncCallback", error);
            }

            @Override
            public void onSuccess(String aString,
                                  Integer aNumber,
                                  GpsLocation aComplexDataType,
                                  TestEnum anEnumResult) {
                assertEquals(TEST_INTEGER, aNumber);
                assertEquals(TEST_STRING, aString);
                assertEquals(TEST_COMPLEXTYPE, aComplexDataType);
                assertEquals(TEST_ENUM, anEnumResult);
            }
        });

        MethodWithMultipleOutputParametersReturned reply = future.get();
        assertEquals(TEST_INTEGER, reply.aNumber);
        assertEquals(TEST_STRING, reply.aString);
        assertEquals(TEST_COMPLEXTYPE, reply.aComplexDataType);
        assertEquals(TEST_ENUM, reply.anEnumResult);

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncMethodCallWithTtlExpiring() throws DiscoveryException, JoynrIllegalStateException,
                                                 InterruptedException, ApplicationException {

        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        long ttl = 2000L;
        MessagingQos testMessagingQos = new MessagingQos(ttl);
        DiscoveryQos testDiscoveryQos = new DiscoveryQos(30000, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);
        testProxy proxyShortTll = proxyBuilder.setMessagingQos(testMessagingQos)
                                              .setDiscoveryQos(testDiscoveryQos)
                                              .build();

        // the provider waits ttl before responding, causing a ttl
        boolean timeoutExceptionThrown = false;
        String timeoutExceptionText = "";
        // the ttl parameter tells the provider to wait this long before
        // replying, thereby forcing a ttl exception
        Future<String> waitTooLongFuture = proxyShortTll.waitTooLong(callback, ttl * 2);
        try {
            waitTooLongFuture.get(10 * ttl); // should never have to wait
            // this long
            // the JoynWaitExpiredException should not be thrown.
        } catch (JoynrWaitExpiredException e) {
            timeoutExceptionThrown = false;
        } catch (JoynrTimeoutException e) {
            timeoutExceptionThrown = true;
            timeoutExceptionText = e.getMessage();
        }
        assertEquals(true, timeoutExceptionThrown);
        assertEquals(true, timeoutExceptionText.contains("ttl for request with requestReplyId"));
        assertEquals(RequestStatusCode.ERROR, waitTooLongFuture.getStatus().getCode());
        verify(callback).onFailure(any(JoynrRuntimeException.class));
        verifyNoMoreInteractions(callback);

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testMethodWithEnumInputReturnsResult() throws DiscoveryException, JoynrIllegalStateException,
                                                       InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        TestEnum input = TestEnum.TWO;

        int result = proxy.methodWithEnumParameter(input);
        assertEquals(2, result);

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testVoidOperation() throws DiscoveryException, JoynrIllegalStateException, InterruptedException,
                                    JoynrWaitExpiredException, ApplicationException {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        final Future<Boolean> future = new Future<Boolean>();
        proxy.voidOperation(new Callback<Void>() {

            @Override
            public void onSuccess(Void result) {
                future.onSuccess(true);
            }

            @Override
            public void onFailure(JoynrRuntimeException error) {
                future.onFailure(error);
            }

        });
        Boolean reply = future.get(8000);

        assertTrue(reply);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testAsyncProviderCall() {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domainAsync, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        proxy.methodStringDoubleParameters("text", 42d);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testMethodWithNullEnumInputReturnsSomethingSensible() throws DiscoveryException,
                                                                      JoynrIllegalStateException, InterruptedException {
        TestEnum input = null;
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        int result = proxy.methodWithEnumParameter(input);
        assertEquals(42, result);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void sendingANullValueOnceDoesntCrashProvider() throws DiscoveryException, JoynrIllegalStateException,
                                                           InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        proxy.methodWithEnumParameter(null);
        TestEnum input = TestEnum.TWO;
        int result = proxy.methodWithEnumParameter(input);
        assertEquals(2, result);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testEnumAttribute() throws DiscoveryException, JoynrIllegalStateException, InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        proxy.setEnumAttribute(TestEnum.TWO);
        TestEnum result = proxy.getEnumAttribute();
        assertEquals(TestEnum.TWO, result);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testByteBufferAttribute() throws DiscoveryException, JoynrIllegalStateException, InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        Byte[] byteArray = { 1, 2, 3 };
        proxy.setByteBufferAttribute(byteArray);
        Byte[] result = proxy.getByteBufferAttribute();
        assertArrayEquals(byteArray, result);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testLargeByteBufferAttribute() throws DiscoveryException, JoynrIllegalStateException,
                                               InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        messagingQos.setTtl_ms(MessagingQos.DEFAULT_TTL);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        byte[] bytes = new byte[1000000];
        new Random().nextBytes(bytes);
        Byte[] byteArray = new Byte[bytes.length];
        int i = 0;
        for (byte nextbyte : bytes) {
            byteArray[i++] = nextbyte;
        }
        proxy.setByteBufferAttribute(byteArray);
        Byte[] result = proxy.getByteBufferAttribute();
        assertArrayEquals(byteArray, result);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testSimpleBroadcast() throws InterruptedException, JoynrWaitExpiredException, JoynrRuntimeException,
                                      ApplicationException {
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
        final Float currentSpeed = Float.MAX_VALUE;

        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        Future<String> subscriptionIdFuture = proxy.subscribeToLocationUpdateWithSpeedBroadcast(new LocationUpdateWithSpeedBroadcastAdapter() {
            @Override
            public void onReceive(GpsLocation receivedGpsLocation, Float receivedCurrentSpeed) {
                assertEquals(gpsLocation, receivedGpsLocation);
                assertEquals(currentSpeed, receivedCurrentSpeed);
                broadcastReceived.release();
            }
        }, new MulticastSubscriptionQos());

        // wait to allow the subscription request to arrive at the provider
        subscriptionIdFuture.get();

        provider.fireLocationUpdateWithSpeed(gpsLocation, currentSpeed);
        broadcastReceived.acquire();
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testBroadcastWithMapParameter() throws DiscoveryException, JoynrIllegalStateException,
                                                InterruptedException {
        final Semaphore broadcastReceived = new Semaphore(0);

        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        final TStringKeyMap mapParam = new TStringKeyMap();
        mapParam.put("key", "value");
        Future<String> subscriptionIdFuture = proxy.subscribeToBroadcastWithMapParametersBroadcast(new BroadcastWithMapParametersBroadcastAdapter() {
            @Override
            public void onReceive(TStringKeyMap receivedMapParam) {
                assertEquals(mapParam, receivedMapParam);
                broadcastReceived.release();
            }
        }, new MulticastSubscriptionQos());

        try {
            subscriptionIdFuture.get();
        } catch (Exception e) {
            fail("Failed to establish subscription.");
        }
        provider.fireBroadcastWithMapParameters(mapParam);
        broadcastReceived.acquire();

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncMethodCallWithCallbackAndParameter() throws DiscoveryException, JoynrIllegalStateException,
                                                          InterruptedException, JoynrWaitExpiredException,
                                                          ApplicationException {

        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);

        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        Future<String> future = proxy.toLowerCase(callback, "Argument");
        String answer = future.get(21000);

        assertEquals(RequestStatusCode.OK, future.getStatus().getCode());
        String expected = "argument";
        assertEquals(expected, answer);
        verify(callback).resolve(expected);
        verifyNoMoreInteractions(callback);

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncMethodCallWithIntegerParametersAndFuture() throws DiscoveryException, JoynrIllegalStateException,
                                                                InterruptedException, JoynrWaitExpiredException,
                                                                ApplicationException {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        Future<Integer> future = proxy.addNumbers(callbackInteger, 1, 2, 3);
        Integer reply = future.get(30000);

        assertEquals(RequestStatusCode.OK, future.getStatus().getCode());
        Integer expected = 6;
        assertEquals(expected, reply);
        verify(callbackInteger).resolve(expected);
        verifyNoMoreInteractions(callbackInteger);

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncMethodCallWithEnumParametersAndFuture() throws DiscoveryException, JoynrIllegalStateException,
                                                             InterruptedException, JoynrWaitExpiredException,
                                                             ApplicationException {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        Future<Integer> future = proxy.methodWithEnumParameter(callbackInteger, TestEnum.TWO);
        Integer reply = future.get(40000);

        Integer expected = 2;
        assertEquals(RequestStatusCode.OK, future.getStatus().getCode());
        assertEquals(expected, reply);
        verify(callbackInteger).resolve(expected);
        verifyNoMoreInteractions(callbackInteger);

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncMethodCallWithEnumListReturned() throws DiscoveryException, JoynrIllegalStateException,
                                                      InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        TestEnum[] enums = proxy.methodWithEnumListReturn(2);
        assertArrayEquals(new TestEnum[]{ TestEnum.TWO }, enums);

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void syncMethodCallReturnsErrorEnum() {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        try {
            proxy.methodWithErrorEnum();
            fail("Should throw ApplicationException");
        } catch (JoynrRuntimeException e) {
            fail(e.toString());
        } catch (ApplicationException e) {
            ApplicationException expected = new ApplicationException(ErrorEnumBase.BASE_ERROR_TYPECOLLECTION);
            assertEquals(expected, e);
            String expectedMessage = "ErrorValue: " + ErrorEnumBase.BASE_ERROR_TYPECOLLECTION;
            assertEquals(expected.getMessage(), expectedMessage);
        }
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncMethodCallReturnsErrorEnum() {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        ApplicationException expected = new ApplicationException(ErrorEnumBase.BASE_ERROR_TYPECOLLECTION);
        String expectedMessage = "ErrorValue: " + ErrorEnumBase.BASE_ERROR_TYPECOLLECTION;
        assertEquals(expected.getMessage(), expectedMessage);
        Future<Void> future = proxy.methodWithErrorEnum(callbackWithApplicationExceptionErrorEnumBase);
        try {
            future.get();
            fail("Should throw ApplicationException");
        } catch (JoynrRuntimeException | InterruptedException e) {
            fail(e.toString());
        } catch (ApplicationException e) {
            assertEquals(expected, e);
        }
        verify(callbackWithApplicationExceptionErrorEnumBase).onFailure((ErrorEnumBase) (expected.getError()));
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void syncMethodCallWithByteBuffer() {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        Byte[] input = { 1, 2, 3 };
        Byte[] result = proxy.methodWithByteBuffer(input);
        assertArrayEquals(input, result);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncMethodCallWithByteBuffer() throws JoynrWaitExpiredException, JoynrRuntimeException,
                                                InterruptedException, ApplicationException {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        Byte[] input = { 1, 2, 3 };
        Future<Byte[]> future = proxy.methodWithByteBuffer(new Callback<Byte[]>() {

            @Override
            public void onFailure(JoynrRuntimeException error) {
                fail("byteBuffer was not returned correctly");

            }

            @Override
            public void onSuccess(Byte[] result) {
            }
        }, input);

        Byte[] result = future.get();
        assertArrayEquals(input, result);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void syncMethodCallReturnsExtendedErrorEnum() {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        try {
            proxy.methodWithErrorEnumExtended();
            fail("Should throw ApplicationException");
        } catch (JoynrRuntimeException e) {
            fail(e.toString());
        } catch (ApplicationException e) {
            ApplicationException expected = new ApplicationException(MethodWithErrorEnumExtendedErrorEnum.IMPLICIT_ERROR_TYPECOLLECTION);
            assertEquals(expected, e);
        }
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncMethodCallReturnsExtendedErrorEnum() {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        ApplicationException expected = new ApplicationException(MethodWithErrorEnumExtendedErrorEnum.IMPLICIT_ERROR_TYPECOLLECTION);
        Future<Void> future = proxy.methodWithErrorEnumExtended(callbackWithApplicationExceptionMethodWithErrorEnumExtendedErrorEnum);
        try {
            future.get();
            fail("Should throw ApplicationException");
        } catch (JoynrRuntimeException | InterruptedException e) {
            fail(e.toString());
        } catch (ApplicationException e) {
            assertEquals(expected, e);
        }
        verify(callbackWithApplicationExceptionMethodWithErrorEnumExtendedErrorEnum).onFailure((MethodWithErrorEnumExtendedErrorEnum) (expected.getError()));
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void syncMethodCallReturnsImplicitErrorEnum() {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        try {
            proxy.methodWithImplicitErrorEnum();
            fail("Should throw ApplicationException");
        } catch (JoynrRuntimeException e) {
            fail(e.toString());
        } catch (ApplicationException e) {
            ApplicationException expected = new ApplicationException(MethodWithImplicitErrorEnumErrorEnum.IMPLICIT_ERROR);
            assertEquals(expected, e);
        }
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncMethodCallReturnsImplicitErrorEnum() {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        ApplicationException expected = new ApplicationException(MethodWithImplicitErrorEnumErrorEnum.IMPLICIT_ERROR);
        Future<Void> future = proxy.methodWithImplicitErrorEnum(callbackWithApplicationExceptionMethodWithImplicitErrorEnumErrorEnum);
        try {
            future.get();
            fail("Should throw ApplicationException");
        } catch (JoynrRuntimeException | InterruptedException e) {
            fail(e.toString());
        } catch (ApplicationException e) {
            assertEquals(expected, e);
        }
        verify(callbackWithApplicationExceptionMethodWithImplicitErrorEnumErrorEnum).onFailure((MethodWithImplicitErrorEnumErrorEnum) (expected.getError()));
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void syncMethodCallReturnsProviderRuntimeException() {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        try {
            proxy.methodWithProviderRuntimeException();
            fail("Should throw ProviderRuntimeException");
        } catch (ProviderRuntimeException e) {
            ProviderRuntimeException expected = new ProviderRuntimeException("ProviderRuntimeException");
            assertEquals(expected, e);
        } catch (Exception e) {
            fail(e.toString());
        }
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void syncMethodCallWithArrayParameter() {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        ComplexTestType customParam = new ComplexTestType(1, 2);
        ComplexTestType2[] customListParam = { new ComplexTestType2(3, 4), new ComplexTestType2(5, 6) };
        proxy.methodCustomCustomListParameters(customParam, customListParam);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncMethodCallReturnsProviderRuntimeException() {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        ProviderRuntimeException expected = new ProviderRuntimeException("ProviderRuntimeException");
        Future<Void> future = proxy.methodWithProviderRuntimeException(callbackVoid);
        try {
            future.get();
            fail("Should throw ProviderRuntimeException");
        } catch (ProviderRuntimeException e) {
            assertEquals(expected, e);
        } catch (Exception e) {
            fail(e.toString());
        }
        verify(callbackVoid).onFailure(expected);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void syncMethodCallReturnsThrownException() {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        try {
            proxy.methodWithThrownException();
            fail("Should throw ProviderRuntimeException");
        } catch (ProviderRuntimeException e) {
            ProviderRuntimeException expected = new ProviderRuntimeException(new IllegalArgumentException("thrownException").toString());
            assertEquals(expected, e);
        } catch (Exception e) {
            fail(e.toString());
        }
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncMethodCallReturnsThrownException() {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        ProviderRuntimeException expected = new ProviderRuntimeException(new IllegalArgumentException("thrownException").toString());
        Future<Void> future = proxy.methodWithThrownException(callbackVoid);
        try {
            future.get();
            fail("Should throw ProviderRuntimeException");
        } catch (ProviderRuntimeException e) {
            assertEquals(expected, e);
        } catch (Exception e) {
            fail(e.toString());
        }
        verify(callbackVoid).onFailure(expected);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void syncGetAttributeWithProviderRuntimeException() {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        try {
            proxy.getAttributeWithProviderRuntimeException();
            fail("Should throw ProviderRuntimeException");
        } catch (ProviderRuntimeException e) {
            ProviderRuntimeException expected = new ProviderRuntimeException("ProviderRuntimeException");
            assertEquals(expected, e);
        } catch (Exception e) {
            fail(e.toString());
        }
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncGetAttributeWithProviderRuntimeException() {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        ProviderRuntimeException expected = new ProviderRuntimeException("ProviderRuntimeException");
        Future<Integer> future = proxy.getAttributeWithProviderRuntimeException(callbackInteger);
        try {
            future.get();
            fail("Should throw ProviderRuntimeException");
        } catch (ProviderRuntimeException e) {
            assertEquals(expected, e);
        } catch (Exception e) {
            fail(e.toString());
        }
        verify(callbackInteger).onFailure(expected);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void syncSetAttributeWithThrownException() {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        try {
            proxy.setAttributeWithProviderRuntimeException(42);
            fail("Should throw ProviderRuntimeException");
        } catch (ProviderRuntimeException e) {
            ProviderRuntimeException expected = new ProviderRuntimeException(new IllegalArgumentException("thrownException").toString());
            assertEquals(expected, e);
        } catch (Exception e) {
            fail(e.toString());
        }
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void asyncSetAttributeWithThrownException() {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        ProviderRuntimeException expected = new ProviderRuntimeException(new IllegalArgumentException("thrownException").toString());
        Future<Void> future = proxy.setAttributeWithProviderRuntimeException(callbackVoid, 42);
        try {
            future.get();
            fail("Should throw ProviderRuntimeException");
        } catch (ProviderRuntimeException e) {
            assertEquals(expected, e);
        } catch (Exception e) {
            fail(e.toString());
        }
        verify(callbackVoid).onFailure(expected);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void overloadedMethodWithInheritance() throws DiscoveryException, JoynrIllegalStateException,
                                                  InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        DerivedStruct derivedStruct = new DerivedStruct();
        AnotherDerivedStruct anotherDerivedStruct = new AnotherDerivedStruct();

        String anotherDerivedResult = proxy.overloadedOperation(anotherDerivedStruct);
        String derivedResult = proxy.overloadedOperation(derivedStruct);
        assertEquals("DerivedStruct", derivedResult);
        assertEquals("AnotherDerivedStruct", anotherDerivedResult);

    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void overloadedMethodWithDifferentReturnTypes() throws DiscoveryException, JoynrIllegalStateException,
                                                           InterruptedException {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        ComplexTestType expectedResult1 = new ComplexTestType(42, 42);
        ComplexTestType2 expectedResult2 = new ComplexTestType2(43, 44);

        ComplexTestType result1 = proxy.overloadedOperation("42");
        ComplexTestType2 result2 = proxy.overloadedOperation("43", "44");

        assertEquals(expectedResult1, result1);
        assertEquals(expectedResult2, result2);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void methodWithMapParameters() throws Exception {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domainAsync, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        TStringKeyMap inMap = new TStringKeyMap();
        String key = "inkey";
        String value = "invalue";
        inMap.put(key, value);
        TStringKeyMap result1 = proxy.mapParameters(inMap);

        assertEquals(value, result1.get(key));
        assertEquals(OUT_VALUE, result1.get(OUT_KEY));
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void methodWithFireAndForget() throws Exception {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        FIRE_AND_FORGET_SEMAPHORE.acquire();
        proxy.methodFireAndForget(FIRE_AND_FORGET_INT_PARAMETER,
                                  FIRE_AND_FORGET_STRING_PARAMETER,
                                  FIRE_AND_FORGET_COMPLEX_PARAMETER);
        assertTrue(FIRE_AND_FORGET_SEMAPHORE.tryAcquire(CONST_DEFAULT_TEST_TIMEOUT, TimeUnit.MILLISECONDS));
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void methodFireAndForgetWithEnumWithValues() throws Exception {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        proxy.methodFireAndForgetWithEnumWithValues(TestEnum.ONE);
        assertTrue(FIRE_AND_FORGET_WITH_ENUM_WITH_VALUES_PARAM_SEMAPHORE.tryAcquire(CONST_DEFAULT_TEST_TIMEOUT,
                                                                                    TimeUnit.MILLISECONDS));
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void methodFireAndForgetWithEnumWithoutValues() throws Exception {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        proxy.methodFireAndForgetWithEnumWithoutValues(TestEnumWithoutValues.BETA);
        assertTrue(FIRE_AND_FORGET_WITH_ENUM_WITHOUT_VALUES_PARAM_SEMAPHORE.tryAcquire(CONST_DEFAULT_TEST_TIMEOUT,
                                                                                       TimeUnit.MILLISECONDS));
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void methodWithEnumWithoutValues() throws Exception {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        TestEnumWithoutValues result = proxy.methodWithEnumWithoutValues(TestEnumWithoutValues.BETA);
        assertEquals(TestEnumWithoutValues.GAMMA, result);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void methodWithFireAndForgetWithoutParams() throws Exception {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        FIRE_AND_FORGET_WITHOUT_PARAMS_SEMAPHORE.acquire();
        proxy.methodFireAndForgetWithoutParams();
        assertTrue(FIRE_AND_FORGET_WITHOUT_PARAMS_SEMAPHORE.tryAcquire(CONST_DEFAULT_TEST_TIMEOUT,
                                                                       TimeUnit.MILLISECONDS));
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testCallingPrincipal() throws Exception {
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        String callingPrincipal = proxy.echoCallingPrincipal();
        assertEquals(ImmutableMessage.DUMMY_CREATOR_USER_ID, callingPrincipal);
    }
}
