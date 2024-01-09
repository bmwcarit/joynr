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
package io.joynr.test.interlanguage;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.function.BiPredicate;

import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.Callback;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.Future;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.interlanguagetest.Enumeration;
import joynr.interlanguagetest.TestInterface.MethodWithExtendedErrorEnumErrorEnum;
import joynr.interlanguagetest.TestInterfaceAsync.MethodWithMultipleStructParametersCallback;
import joynr.interlanguagetest.TestInterfaceAsync.MethodWithMultipleStructParametersFuture;
import joynr.interlanguagetest.TestInterfaceSync.MethodWithMultipleStructParametersReturned;
import joynr.interlanguagetest.namedTypeCollection2.BaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.BaseStructWithoutElements;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedBaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedStructOfPrimitives;
import joynr.interlanguagetest.namedTypeCollection2.MapStringString;
import joynr.interlanguagetest.typeDefCollection.ArrayTypeDefStruct;

public class IltConsumerAsyncMethodTest extends IltConsumerTest {
    private static final Logger logger = LoggerFactory.getLogger(IltConsumerAsyncMethodTest.class);

    @BeforeClass
    public static void setUp() throws Exception {
        logger.info("setUp: Entering");
        setupConsumerRuntime(false);
        logger.info("setUp: Leaving");
    }

    @AfterClass
    public static void tearDown() throws InterruptedException {
        logger.info("tearDown: Entering");
        generalTearDown();
        logger.info("tearDown: Leaving");
    }

    @Before
    public void resetTestEnvironment() {
        proxyMethodWithParameterCallbackResult = false;
    }

    // variables that are to be changed inside callbacks must be instance variables
    volatile boolean proxyMethodWithParameterCallbackResult = false;

    private <T, U> void callProxyMethodWithParameterAsyncAndAssertResult(String methodName,
                                                                         final T arg,
                                                                         BiPredicate<T, U> expectedResultCheck) {
        try {

            final Semaphore resultAvailable = new Semaphore(0);

            Callback<U> callback = new Callback<U>() {
                @Override
                public void onSuccess(U out) {
                    // check result
                    if (!expectedResultCheck.test(arg, out)) {
                        logger.info(name.getMethodName() + TEST_FAILED_CALLBACK_INVALID_RESULT);
                        proxyMethodWithParameterCallbackResult = false;
                        resultAvailable.release();
                        return;
                    }
                    proxyMethodWithParameterCallbackResult = true;
                    resultAvailable.release();
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    proxyMethodWithParameterCallbackResult = false;
                    if (error instanceof JoynrRuntimeException) {
                        logger.info(name.getMethodName() + TEST_FAILED_CALLBACK_EXCEPTION
                                + ((JoynrRuntimeException) error).getMessage());
                    } else {
                        logger.info(name.getMethodName() + TEST_FAILED_CALLBACK_EXCEPTION);
                    }
                    logger.info(name.getMethodName() + TEST_FAILED);
                    resultAvailable.release();
                }
            };

            Method asyncMethod = testInterfaceProxy.getClass().getMethod(methodName, Callback.class, arg.getClass());
            asyncMethod.invoke(testInterfaceProxy, callback, arg);

            try {
                // wait for callback
                logger.info(name.getMethodName() + TEST_WAIT_FOR_CALLBACK);
                assertTrue(name.getMethodName() + TEST_FAILED_CALLBACK_TIMEOUT,
                           resultAvailable.tryAcquire(10, TimeUnit.SECONDS));

                // check result from callback
                logger.info(name.getMethodName() + TEST_WAIT_FOR_CALLBACK_DONE);
                assertTrue(name.getMethodName() + TEST_FAILED_CALLBACK_ERROR, proxyMethodWithParameterCallbackResult);
            } catch (InterruptedException | JoynrRuntimeException e) {
                fail(name.getMethodName() + TEST_FAILED_EXCEPTION + e.getMessage());
            }
        } catch (Exception e) {
            fail(name.getMethodName() + TEST_FAILED_EXCEPTION + e.getMessage());
        }
    }

    /*
     * ASYNC METHOD CALLS
     *
     * limit the number of example calls here
     * - one with single out parameters
     * - one with multiple out parameters
     * - one with multiple lists
     */

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean methodWithMultipleStructParametersAsyncCallbackDone = false;
    volatile boolean methodWithMultipleStructParametersAsyncCallbackResult = false;

    @Test
    public void callMethodWithMultipleStructParametersAsync() {
        logger.info(name.getMethodName());
        try {
            // setup input parameters
            ExtendedStructOfPrimitives extendedStructOfPrimitivesArg = IltUtil.createExtendedStructOfPrimitives();
            BaseStruct baseStructArg = IltUtil.createBaseStruct();

            MethodWithMultipleStructParametersCallback callback = new MethodWithMultipleStructParametersCallback() {
                @Override
                public void onSuccess(BaseStructWithoutElements baseStructWithoutElementsOut,
                                      ExtendedExtendedBaseStruct extendedExtendedBaseStructOut) {
                    // check results
                    if (!IltUtil.checkBaseStructWithoutElements(baseStructWithoutElementsOut)) {
                        methodWithMultipleStructParametersAsyncCallbackResult = false;
                        methodWithMultipleStructParametersAsyncCallbackDone = true;
                        logger.info(name.getMethodName() + " - callback - invalid baseStructWithoutElementsOut");
                        logger.info(name.getMethodName() + " - FAILED");
                        return;
                    }

                    ExtendedExtendedBaseStruct extendedExtendedBaseStruct = IltUtil.createExtendedExtendedBaseStruct();
                    if (!extendedExtendedBaseStructOut.equals(extendedExtendedBaseStruct)) {
                        methodWithMultipleStructParametersAsyncCallbackResult = false;
                        methodWithMultipleStructParametersAsyncCallbackDone = true;
                        logger.info(name.getMethodName() + " - callback - invalid extendedExtendedBaseStructOut");
                        logger.info(name.getMethodName() + " - FAILED");
                        return;
                    }
                    methodWithMultipleStructParametersAsyncCallbackResult = true;
                    methodWithMultipleStructParametersAsyncCallbackDone = true;
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    methodWithMultipleStructParametersAsyncCallbackResult = false;
                    methodWithMultipleStructParametersAsyncCallbackDone = true;
                    if (error instanceof JoynrRuntimeException) {
                        logger.info(name.getMethodName() + " - callback - caught exception "
                                + ((JoynrRuntimeException) error).getMessage());
                    } else {
                        logger.info(name.getMethodName() + " - callback - caught exception");
                    }
                    logger.info(name.getMethodName() + " - FAILED");
                }
            };

            MethodWithMultipleStructParametersFuture future = testInterfaceProxy.methodWithMultipleStructParameters(callback,
                                                                                                                    extendedStructOfPrimitivesArg,
                                                                                                                    baseStructArg);

            try {
                long timeoutInMilliseconds = 8000;
                logger.info(name.getMethodName() + " - about to call future.get");
                MethodWithMultipleStructParametersReturned result = future.get(timeoutInMilliseconds);
                assertNotNull(name.getMethodName() + " - FAILED - got no result", result);
                logger.info(name.getMethodName() + " - returned from future.get");

                // check results from future
                assertTrue(name.getMethodName() + " - FAILED - got invalid result - baseStructWithoutElementsOut",
                           IltUtil.checkBaseStructWithoutElements(result.baseStructWithoutElementsOut));

                ExtendedExtendedBaseStruct extendedExtendedBaseStruct = IltUtil.createExtendedExtendedBaseStruct();
                assertEquals(name.getMethodName() + " - FAILED - got invalid result - extendedExtendedBaseStructOut",
                             result.extendedExtendedBaseStructOut,
                             extendedExtendedBaseStruct);
                logger.info(name.getMethodName() + " - future result checks OK");

                // check results from callback; expect to be finished within 1 second
                // should have been called ahead anyway
                if (!methodWithMultipleStructParametersAsyncCallbackDone) {
                    logger.info(name.getMethodName() + " - about to wait for a second for callback");
                    Thread.sleep(1000);
                    logger.info(name.getMethodName() + " - wait for callback is over");
                } else {
                    logger.info(name.getMethodName() + " - callback already done");
                }

                assertTrue(name.getMethodName() + " - FAILED - callback NOT done",
                           methodWithMultipleStructParametersAsyncCallbackDone);
                assertTrue(name.getMethodName() + " - FAILED - callback reported error",
                           methodWithMultipleStructParametersAsyncCallbackResult);
                logger.info(name.getMethodName() + " - callback has set OK flags");
            } catch (InterruptedException | JoynrRuntimeException | ApplicationException e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithSingleArrayParametersAsync() {
        logger.info(name.getMethodName());
        String[] stringArray = { "Hello", "World" };
        Double[] doubleArrayArg = IltUtil.createDoubleArray();
        callProxyMethodWithParameterAsyncAndAssertResult("methodWithSingleArrayParameters",
                                                         doubleArrayArg,
                                                         (Double[] arg, String[] res) -> Arrays.equals(res,
                                                                                                       stringArray));
        logger.info(name.getMethodName() + " - OK");
        return;
    }

    @Test
    public void callMethodWithSinglePrimitiveParametersAsync() {
        logger.info(name.getMethodName());
        short shortArg = (short) 32767;
        callProxyMethodWithParameterAsyncAndAssertResult("methodWithSinglePrimitiveParameters",
                                                         shortArg,
                                                         (Short arg,
                                                          String res) -> res.equals(Integer.valueOf(Short.toUnsignedInt(arg))
                                                                                           .toString()));
        logger.info(name.getMethodName() + " - OK");
        return;
    }

    @Test
    public void callMethodWithSingleByteBufferParameterAsync() {
        logger.info(name.getMethodName());
        final Byte[] byteBufferArg = { -128, 0, 127 };
        callProxyMethodWithParameterAsyncAndAssertResult("methodWithSingleByteBufferParameter",
                                                         byteBufferArg,
                                                         (Byte[] arg, Byte[] res) -> Arrays.equals(arg, res));
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean methodWithMultipleByteBufferParametersAsyncCallbackResult = false;

    @Test
    public void callMethodWithMultipleByteBufferParametersAsync() {
        logger.info(name.getMethodName());

        final Semaphore resultAvailable = new Semaphore(0);
        try {
            // setup input parameter
            final Byte[] byteBufferArg1 = { -5, 125 };
            final Byte[] byteBufferArg2 = { 78, 0 };

            Byte[] concatenatedBuffer = IltUtil.concatenateByteArrays(byteBufferArg1, byteBufferArg2);

            Callback<Byte[]> callback = new Callback<Byte[]>() {
                @Override
                public void onSuccess(Byte[] byteBufferOut) {
                    // check result
                    if (!java.util.Objects.deepEquals(byteBufferOut, concatenatedBuffer)) {
                        logger.info(name.getMethodName() + " - invalid byteBufferOut from callback");
                        logger.info(name.getMethodName() + " - FAILED");
                        methodWithMultipleByteBufferParametersAsyncCallbackResult = false;
                        resultAvailable.release();
                        return;
                    }
                    methodWithMultipleByteBufferParametersAsyncCallbackResult = true;
                    resultAvailable.release();
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    methodWithMultipleByteBufferParametersAsyncCallbackResult = false;
                    if (error instanceof JoynrRuntimeException) {
                        logger.info(name.getMethodName() + " - callback - caught exception "
                                + ((JoynrRuntimeException) error).getMessage());
                    } else {
                        logger.info(name.getMethodName() + " - callback - caught exception");
                    }
                    logger.info(name.getMethodName() + " - FAILED");
                    resultAvailable.release();
                }
            };

            testInterfaceProxy.methodWithMultipleByteBufferParameters(callback, byteBufferArg1, byteBufferArg2);

            try {
                // wait for callback
                logger.info(name.getMethodName() + " - about to wait for callback");
                Assert.assertTrue(name.getMethodName() + " - FAILED - callback not received in time",
                                  resultAvailable.tryAcquire(10, TimeUnit.SECONDS));

                // check result from callback
                logger.info(name.getMethodName() + " - wait for callback is over");
                Assert.assertTrue(name.getMethodName() + " - FAILED - callback reported error",
                                  methodWithMultipleByteBufferParametersAsyncCallbackResult);
            } catch (InterruptedException | JoynrRuntimeException e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithInt64TypeDefParameterAsync() {
        logger.info(name.getMethodName());
        final Long int64TypeDefArg = 1L;
        callProxyMethodWithParameterAsyncAndAssertResult("methodWithInt64TypeDefParameter",
                                                         int64TypeDefArg,
                                                         (Long arg, Long res) -> res.equals(arg));
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithStringTypeDefParameterAsync() {
        logger.info(name.getMethodName());
        final String stringTypeDefArg = "StringTypeDef";
        callProxyMethodWithParameterAsyncAndAssertResult("methodWithStringTypeDefParameter",
                                                         stringTypeDefArg,
                                                         (String arg, String res) -> res.equals(arg));
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithStructTypeDefParameterAsync() {
        logger.info(name.getMethodName());
        final BaseStruct structTypeDefArg = IltUtil.createBaseStruct();
        callProxyMethodWithParameterAsyncAndAssertResult("methodWithStructTypeDefParameter",
                                                         structTypeDefArg,
                                                         (BaseStruct arg, BaseStruct res) -> res.equals(arg));
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithMapTypeDefParameterAsync() {
        logger.info(name.getMethodName());
        final MapStringString mapTypeDefArg = new MapStringString();
        mapTypeDefArg.put("keyString1", "valueString1");
        mapTypeDefArg.put("keyString2", "valueString2");
        mapTypeDefArg.put("keyString3", "valueString3");
        callProxyMethodWithParameterAsyncAndAssertResult("methodWithMapTypeDefParameter",
                                                         mapTypeDefArg,
                                                         (MapStringString arg, MapStringString res) -> res.equals(arg));
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithEnumTypeDefParameterAsync() {
        logger.info(name.getMethodName());
        final Enumeration enumTypeDefArg = Enumeration.ENUM_0_VALUE_1;
        callProxyMethodWithParameterAsyncAndAssertResult("methodWithEnumTypeDefParameter",
                                                         enumTypeDefArg,
                                                         (Enumeration arg, Enumeration res) -> res.equals(arg));
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithByteBufferTypeDefParameterAsync() {
        logger.info(name.getMethodName());
        final Byte[] byteBufferTypeDefArg = { -128, 0, 127 };
        callProxyMethodWithParameterAsyncAndAssertResult("methodWithByteBufferTypeDefParameter",
                                                         byteBufferTypeDefArg,
                                                         (Byte[] arg, Byte[] res) -> Arrays.equals(arg, res));
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithArrayTypeDefParameterAsync() {
        logger.info(name.getMethodName());
        String[] stringArray = { "Hello", "World" };
        final ArrayTypeDefStruct arrayTypeDefArg = new ArrayTypeDefStruct(stringArray);
        callProxyMethodWithParameterAsyncAndAssertResult("methodWithArrayTypeDefParameter",
                                                         arrayTypeDefArg,
                                                         (ArrayTypeDefStruct arg,
                                                          ArrayTypeDefStruct res) -> res.equals(arg));
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    /*
     * ASYNC METHOD CALLS WITH EXCEPTION
     */

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean methodWithExtendedErrorEnumAsyncCallbackDone = false;
    volatile boolean methodWithExtendedErrorEnumAsyncCallbackResult = false;

    @SuppressWarnings("checkstyle:methodlength")
    @Test
    public void callMethodWithExtendedErrorEnumAsync() {
        logger.info(name.getMethodName());
        try {
            // setup input parameters
            String wantedExceptionArg = "ProviderRuntimeException";

            CallbackWithModeledError<Void, MethodWithExtendedErrorEnumErrorEnum> callback = new CallbackWithModeledError<Void, MethodWithExtendedErrorEnumErrorEnum>() {
                @Override
                public void onSuccess(Void result) {
                    logger.info(name.getMethodName() + " - 1st - unexpected positive return in callback");
                    logger.info(name.getMethodName() + " - FAILED");
                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                    methodWithExtendedErrorEnumAsyncCallbackDone = true;
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    if (error instanceof ProviderRuntimeException) {
                        if (((ProviderRuntimeException) error).getMessage()
                                                              .endsWith("Exception from methodWithExtendedErrorEnum")) {
                            logger.info(name.getMethodName() + " - 1st - callback - got expected exception "
                                    + ((JoynrRuntimeException) error).getMessage());
                            methodWithExtendedErrorEnumAsyncCallbackResult = true;
                            methodWithExtendedErrorEnumAsyncCallbackDone = true;
                            return;
                        }
                        logger.info(name.getMethodName() + " - 1st - callback - caught invalid exception "
                                + ((JoynrRuntimeException) error).getMessage());
                    } else {
                        logger.info(name.getMethodName() + " - 1st - callback - caught invalid exception "
                                + ((JoynrRuntimeException) error).getMessage());
                    }

                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                    methodWithExtendedErrorEnumAsyncCallbackDone = true;
                    logger.info(name.getMethodName() + " - FAILED");
                }

                @Override
                public void onFailure(MethodWithExtendedErrorEnumErrorEnum errorEnum) {
                    logger.info(name.getMethodName() + " - 1st - callback - caught invalid exception ");
                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                    methodWithExtendedErrorEnumAsyncCallbackDone = true;
                    logger.info(name.getMethodName() + " - FAILED");
                }
            };

            Future<Void> future = testInterfaceProxy.methodWithExtendedErrorEnum(callback, wantedExceptionArg);

            try {
                long timeoutInMilliseconds = 8000;
                logger.info(name.getMethodName() + " - 1st - about to call future.get");
                future.get(timeoutInMilliseconds);
                fail(name.getMethodName() + " - FAILED - unexpected return from future");
            } catch (InterruptedException | JoynrRuntimeException | ApplicationException error) {
                if (error instanceof ProviderRuntimeException) {
                    if (((ProviderRuntimeException) error).getMessage()
                                                          .endsWith("Exception from methodWithExtendedErrorEnum")) {
                        logger.info(name.getMethodName() + " - 1st - caught expected exception "
                                + ((JoynrRuntimeException) error).getMessage());
                        // OK, fallthrough
                    } else {
                        // incorrect message
                        fail(name.getMethodName() + " - FAILED - caught invalid exception: " + error.getMessage());
                    }
                } else if (error instanceof JoynrRuntimeException) {
                    // incorrect exception, can output message
                    fail(name.getMethodName() + " - FAILED - caught invalid exception: " + error.getMessage());
                } else {
                    // incorrect exception, can not output message
                    fail(name.getMethodName() + " - FAILED - caught unexpected exception");
                }

                // check results from callback; expect to be finished within 1 second
                // should have been called ahead anyway
                if (!methodWithExtendedErrorEnumAsyncCallbackDone) {
                    logger.info(name.getMethodName() + " - 1st - wait a second for callback");
                    Thread.sleep(1000);
                    logger.info(name.getMethodName() + " - 1st - wait for callback is over");
                } else {
                    logger.info(name.getMethodName() + " - 1st - callback already done");
                }

                assertTrue(name.getMethodName() + " - FAILED - callback NOT done",
                           methodWithExtendedErrorEnumAsyncCallbackDone);
                assertTrue(name.getMethodName() + " - FAILED - callback reported failure",
                           methodWithExtendedErrorEnumAsyncCallbackResult);
                logger.info(name.getMethodName() + " - 1st - callback caught expected exception");
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }

        logger.info(name.getMethodName() + " - ProviderRuntimeException check done");

        // 2nd test
        try {
            // setup input parameters
            String wantedExceptionArg = "ApplicationException_1";

            CallbackWithModeledError<Void, MethodWithExtendedErrorEnumErrorEnum> callback = new CallbackWithModeledError<Void, MethodWithExtendedErrorEnumErrorEnum>() {
                @Override
                public void onSuccess(Void result) {
                    logger.info(name.getMethodName() + " - 2nd - unexpected positive return in callback");
                    logger.info(name.getMethodName() + " - FAILED");
                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                    methodWithExtendedErrorEnumAsyncCallbackDone = true;
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    logger.info(name.getMethodName() + " - 2nd - callback - caught invalid JoynrRuntime like exception "
                            + ((JoynrRuntimeException) error).getMessage());
                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                    methodWithExtendedErrorEnumAsyncCallbackDone = true;
                }

                @Override
                public void onFailure(MethodWithExtendedErrorEnumErrorEnum errorEnum) {
                    if (errorEnum == MethodWithExtendedErrorEnumErrorEnum.ERROR_3_3_NTC) {
                        logger.info(name.getMethodName()
                                + " - 2nd - callback - caught ApplicationException with expected error enum");
                        methodWithExtendedErrorEnumAsyncCallbackResult = true;
                        methodWithExtendedErrorEnumAsyncCallbackDone = true;
                        return;
                    }
                    logger.info(name.getMethodName()
                            + " - 2nd - callback - caught invalid ApplicationException with enum " + errorEnum);

                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                    methodWithExtendedErrorEnumAsyncCallbackDone = true;
                }
            };

            Future<Void> future = testInterfaceProxy.methodWithExtendedErrorEnum(callback, wantedExceptionArg);

            try {
                long timeoutInMilliseconds = 8000;
                logger.info(name.getMethodName() + " - 2nd - about to call future.get");
                future.get(timeoutInMilliseconds);
                fail(name.getMethodName() + " - FAILED - unexpected return from future");
            } catch (InterruptedException | JoynrRuntimeException | ApplicationException error) {
                if (error instanceof ApplicationException) {
                    if (((ApplicationException) error).getError() == MethodWithExtendedErrorEnumErrorEnum.ERROR_3_3_NTC) {
                        logger.info(name.getMethodName()
                                + " - 2nd - caught expected ApplicationException with correct error enum");
                    } else {
                        fail(name.getMethodName() + " - FAILED - 2nd - caught invalid ApplicationException with enum: "
                                + ((ApplicationException) error).getError());
                    }
                } else if (error instanceof JoynrRuntimeException) {
                    // incorrect exception, can output message
                    fail(name.getMethodName() + " - FAILED - 2nd - caught invalid JoynrRuntimeException: "
                            + error.getMessage());
                } else {
                    // incorrect exception, can not output message
                    fail(name.getMethodName() + " - FAILED - caught invalid other exception");
                }

                // check results from callback; expect to be finished within 1 second
                // should have been called ahead anyway
                if (!methodWithExtendedErrorEnumAsyncCallbackDone) {
                    logger.info(name.getMethodName() + " - 2nd - about to wait for a second for callback");
                    Thread.sleep(1000);
                    logger.info(name.getMethodName() + " - 2nd - wait for callback is over");
                } else {
                    logger.info(name.getMethodName() + " - 2nd - callback already done");
                }

                assertTrue(name.getMethodName() + " - FAILED - 2nd - callback NOT done",
                           methodWithExtendedErrorEnumAsyncCallbackDone);
                assertTrue(name.getMethodName() + " - FAILED - 2nd - callback reported success",
                           methodWithExtendedErrorEnumAsyncCallbackResult);
                logger.info(name.getMethodName() + " - 2nd - callback has got expected exception");
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
        logger.info(name.getMethodName() + " - ApplicationException check done");
        logger.info(name.getMethodName() + " - OK");

        // 3rd test omitted
    }
}
