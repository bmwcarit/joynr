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

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.Callback;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.Future;

import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.interlanguagetest.TestInterface.MethodWithExtendedErrorEnumErrorEnum;
import joynr.interlanguagetest.TestInterfaceAsync.MethodWithMultipleStructParametersCallback;
import joynr.interlanguagetest.TestInterfaceAsync.MethodWithMultipleStructParametersFuture;
import joynr.interlanguagetest.TestInterfaceSync.MethodWithMultipleStructParametersReturned;
import joynr.interlanguagetest.namedTypeCollection2.BaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.BaseStructWithoutElements;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedBaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedStructOfPrimitives;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

import static org.junit.Assert.fail;

import edu.umd.cs.findbugs.annotations.SuppressWarnings;

public class IltConsumerAsyncMethodTest extends IltConsumerTest {
    private static final Logger LOG = LoggerFactory.getLogger(IltConsumerAsyncMethodTest.class);

    @BeforeClass
    public static void setUp() throws Exception {
        LOG.info("setUp: Entering");
        setupConsumerRuntime(false);
        LOG.info("setUp: Leaving");
    }

    @AfterClass
    public static void tearDown() throws InterruptedException {
        LOG.info("tearDown: Entering");
        generalTearDown();
        LOG.info("tearDown: Leaving");
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
        LOG.info(name.getMethodName() + "");
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
                        LOG.info(name.getMethodName() + " - callback - invalid baseStructWithoutElementsOut");
                        LOG.info(name.getMethodName() + " - FAILED");
                        return;
                    }

                    if (!IltUtil.checkExtendedExtendedBaseStruct(extendedExtendedBaseStructOut)) {
                        methodWithMultipleStructParametersAsyncCallbackResult = false;
                        methodWithMultipleStructParametersAsyncCallbackDone = true;
                        LOG.info(name.getMethodName() + " - callback - invalid extendedExtendedBaseStructOut");
                        LOG.info(name.getMethodName() + " - FAILED");
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
                        LOG.info(name.getMethodName() + " - callback - caught exception "
                                + ((JoynrRuntimeException) error).getMessage());
                    } else {
                        LOG.info(name.getMethodName() + " - callback - caught exception");
                    }
                    LOG.info(name.getMethodName() + " - FAILED");
                }
            };

            MethodWithMultipleStructParametersFuture future = testInterfaceProxy.methodWithMultipleStructParameters(callback,
                                                                                                                    extendedStructOfPrimitivesArg,
                                                                                                                    baseStructArg);

            try {
                long timeoutInMilliseconds = 8000;
                LOG.info(name.getMethodName() + " - about to call future.get");
                MethodWithMultipleStructParametersReturned result = future.get(timeoutInMilliseconds);
                if (result == null) {
                    fail(name.getMethodName() + " - FAILED - got no result");
                    return;
                }
                LOG.info(name.getMethodName() + " - returned from future.get");

                // check results from future
                if (!IltUtil.checkBaseStructWithoutElements(result.baseStructWithoutElementsOut)) {
                    fail(name.getMethodName() + " - FAILED - got invalid result - baseStructWithoutElementsOut");
                    return;
                }

                if (!IltUtil.checkExtendedExtendedBaseStruct(result.extendedExtendedBaseStructOut)) {
                    fail(name.getMethodName() + " - FAILED - got invalid result - extendedExtendedBaseStructOut");
                    return;
                }
                LOG.info(name.getMethodName() + " - future result checks OK");

                // check results from callback; expect to be finished within 1 second
                // should have been called ahead anyway
                if (methodWithMultipleStructParametersAsyncCallbackDone == false) {
                    LOG.info(name.getMethodName() + " - about to wait for a second for callback");
                    Thread.sleep(1000);
                    LOG.info(name.getMethodName() + " - wait for callback is over");
                } else {
                    LOG.info(name.getMethodName() + " - callback already done");
                }
                if (methodWithMultipleStructParametersAsyncCallbackDone == false) {
                    fail(name.getMethodName() + " - FAILED - callback NOT done");
                    return;
                }
                if (methodWithMultipleStructParametersAsyncCallbackResult == false) {
                    fail(name.getMethodName() + " - FAILED - callback reported error");
                    return;
                }
                LOG.info(name.getMethodName() + " - callback has set OK flags");
            } catch (InterruptedException | JoynrRuntimeException | ApplicationException e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        LOG.info(name.getMethodName() + " - OK");
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean methodWithSingleArrayParametersAsyncCallbackDone = false;
    volatile boolean methodWithSingleArrayParametersAsyncCallbackResult = false;

    @Test
    public void callMethodWithSingleArrayParametersAsync() {
        LOG.info(name.getMethodName() + "");
        try {
            // setup input parameters
            Double[] doubleArrayArg = IltUtil.createDoubleArray();

            Callback<String[]> callback = new Callback<String[]>() {
                @Override
                public void onSuccess(String[] stringOut) {
                    // check results
                    if (!IltUtil.checkStringArray(stringOut)) {
                        LOG.info(name.getMethodName() + " - invalid stringOut from callback");
                        LOG.info(name.getMethodName() + " - FAILED");
                        methodWithSingleArrayParametersAsyncCallbackResult = false;
                        methodWithSingleArrayParametersAsyncCallbackDone = true;
                        return;
                    }
                    methodWithSingleArrayParametersAsyncCallbackResult = true;
                    methodWithSingleArrayParametersAsyncCallbackDone = true;
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    methodWithSingleArrayParametersAsyncCallbackResult = false;
                    methodWithSingleArrayParametersAsyncCallbackDone = true;
                    if (error instanceof JoynrRuntimeException) {
                        LOG.info(name.getMethodName() + " - callback - caught exception "
                                + ((JoynrRuntimeException) error).getMessage());
                    } else {
                        LOG.info(name.getMethodName() + " - callback - caught exception");
                    }
                    LOG.info(name.getMethodName() + " - FAILED");
                }
            };

            Future<String[]> future = testInterfaceProxy.methodWithSingleArrayParameters(callback, doubleArrayArg);

            try {
                long timeoutInMilliseconds = 8000;
                LOG.info(name.getMethodName() + " - about to call future.get");
                String[] result;

                result = future.get(timeoutInMilliseconds);
                LOG.info(name.getMethodName() + " - returned from future.get");

                // check results from future
                if (result == null) {
                    fail(name.getMethodName() + " - FAILED - got no result");
                    return;
                }
                if (!IltUtil.checkStringArray(result)) {
                    fail(name.getMethodName() + " - FAILED - got invalid result");
                    return;
                }

                LOG.info(name.getMethodName() + " - future result checks OK");

                // check results from callback; expect to be finished within 1 second
                // should have been called ahead anyway
                if (methodWithSingleArrayParametersAsyncCallbackDone == false) {
                    LOG.info(name.getMethodName() + " - about to wait for a second for callback");
                    Thread.sleep(1000);
                    LOG.info(name.getMethodName() + " - wait for callback is over");
                } else {
                    LOG.info(name.getMethodName() + " - callback already done");
                }
                if (methodWithSingleArrayParametersAsyncCallbackDone == false) {
                    fail(name.getMethodName() + " - FAILED - callback NOT done");
                    return;
                }
                if (methodWithSingleArrayParametersAsyncCallbackResult == false) {
                    fail(name.getMethodName() + " - FAILED - callback reported error");
                    return;
                }
                LOG.info(name.getMethodName() + " - callback has set OK flags");
            } catch (InterruptedException | JoynrRuntimeException | ApplicationException e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        LOG.info(name.getMethodName() + " - OK");
        return;
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean methodWithSinglePrimitiveParametersAsyncCallbackDone = false;
    volatile boolean methodWithSinglePrimitiveParametersAsyncCallbackResult = false;

    @Test
    public void callMethodWithSinglePrimitiveParametersAsync() {
        LOG.info(name.getMethodName() + "");
        try {
            // setup input parameters
            // final short arg = (short)65535;
            final short arg = (short) 32767;

            Callback<String> callback = new Callback<String>() {
                @Override
                public void onSuccess(String stringOut) {
                    // check results
                    if (!stringOut.equals(String.valueOf(Short.toUnsignedInt(arg)))) {
                        LOG.info(name.getMethodName() + " - invalid stringOut from callback");
                        LOG.info(name.getMethodName() + " - FAILED");
                        methodWithSinglePrimitiveParametersAsyncCallbackResult = false;
                        methodWithSinglePrimitiveParametersAsyncCallbackDone = true;
                        return;
                    }
                    methodWithSinglePrimitiveParametersAsyncCallbackResult = true;
                    methodWithSinglePrimitiveParametersAsyncCallbackDone = true;
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    methodWithSinglePrimitiveParametersAsyncCallbackResult = false;
                    methodWithSinglePrimitiveParametersAsyncCallbackDone = true;
                    if (error instanceof JoynrRuntimeException) {
                        LOG.info(name.getMethodName() + " - callback - caught exception "
                                + ((JoynrRuntimeException) error).getMessage());
                    } else {
                        LOG.info(name.getMethodName() + " - callback - caught exception");
                    }
                    LOG.info(name.getMethodName() + " - FAILED");
                }
            };

            Future<String> future = testInterfaceProxy.methodWithSinglePrimitiveParameters(callback, arg);

            try {
                long timeoutInMilliseconds = 8000;
                LOG.info(name.getMethodName() + " - about to call future.get");
                String result = future.get(timeoutInMilliseconds);
                LOG.info(name.getMethodName() + " - returned from future.get");

                // check results from future
                if (result == null) {
                    fail(name.getMethodName() + " - FAILED - got no result");
                    return;
                }
                if (!result.equals(String.valueOf(Short.toUnsignedInt(arg)))) {
                    fail(name.getMethodName() + " - FAILED - got invalid result");
                    return;
                }

                LOG.info(name.getMethodName() + " - future result checks OK");

                // check results from callback; expect to be finished within 1 second
                // should have been called ahead anyway
                if (methodWithSinglePrimitiveParametersAsyncCallbackDone == false) {
                    LOG.info(name.getMethodName() + " - about to wait for a second for callback");
                    Thread.sleep(1000);
                    LOG.info(name.getMethodName() + " - wait for callback is over");
                } else {
                    LOG.info(name.getMethodName() + " - callback already done");
                }
                if (methodWithSinglePrimitiveParametersAsyncCallbackDone == false) {
                    fail(name.getMethodName() + " - FAILED - callback NOT done");
                    return;
                }
                if (methodWithSinglePrimitiveParametersAsyncCallbackResult == false) {
                    fail(name.getMethodName() + " - FAILED - callback reported error");
                    return;
                }
                LOG.info(name.getMethodName() + " - callback has set OK flags");
            } catch (InterruptedException | JoynrRuntimeException | ApplicationException e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        LOG.info(name.getMethodName() + " - OK");
        return;
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
        LOG.info(name.getMethodName() + "");
        try {
            // setup input parameters
            String wantedExceptionArg = "ProviderRuntimeException";

            CallbackWithModeledError<Void, MethodWithExtendedErrorEnumErrorEnum> callback = new CallbackWithModeledError<Void, MethodWithExtendedErrorEnumErrorEnum>() {
                @Override
                public void onSuccess(Void result) {
                    LOG.info(name.getMethodName() + " - 1st - unexpected positive return in callback");
                    LOG.info(name.getMethodName() + " - FAILED");
                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                    methodWithExtendedErrorEnumAsyncCallbackDone = true;
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    if (error instanceof ProviderRuntimeException) {
                        if (((ProviderRuntimeException) error).getMessage()
                                                              .endsWith("Exception from methodWithExtendedErrorEnum")) {
                            LOG.info(name.getMethodName() + " - 1st - callback - got expected exception "
                                    + ((JoynrRuntimeException) error).getMessage());
                            methodWithExtendedErrorEnumAsyncCallbackResult = true;
                            methodWithExtendedErrorEnumAsyncCallbackDone = true;
                            return;
                        }
                        LOG.info(name.getMethodName() + " - 1st - callback - caught invalid exception "
                                + ((JoynrRuntimeException) error).getMessage());
                    } else {
                        LOG.info(name.getMethodName() + " - 1st - callback - caught invalid exception "
                                + ((JoynrRuntimeException) error).getMessage());
                    }

                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                    methodWithExtendedErrorEnumAsyncCallbackDone = true;
                    LOG.info(name.getMethodName() + " - FAILED");
                }

                @Override
                public void onFailure(MethodWithExtendedErrorEnumErrorEnum errorEnum) {
                    LOG.info(name.getMethodName() + " - 1st - callback - caught invalid exception ");
                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                    methodWithExtendedErrorEnumAsyncCallbackDone = true;
                    LOG.info(name.getMethodName() + " - FAILED");
                }
            };

            Future<Void> future = testInterfaceProxy.methodWithExtendedErrorEnum(callback, wantedExceptionArg);

            try {
                long timeoutInMilliseconds = 8000;
                LOG.info(name.getMethodName() + " - 1st - about to call future.get");
                future.get(timeoutInMilliseconds);
                fail(name.getMethodName() + " - FAILED - unexpected return from future");
                return;
            } catch (InterruptedException | JoynrRuntimeException | ApplicationException error) {
                if (error instanceof ProviderRuntimeException) {
                    if (((ProviderRuntimeException) error).getMessage()
                                                          .endsWith("Exception from methodWithExtendedErrorEnum")) {
                        LOG.info(name.getMethodName() + " - 1st - caught expected exception "
                                + ((JoynrRuntimeException) error).getMessage());
                        // OK, fallthrough
                    } else {
                        // incorrect message
                        fail(name.getMethodName() + " - FAILED - caught invalid exception: " + error.getMessage());
                        return;
                    }
                } else if (error instanceof JoynrRuntimeException) {
                    // incorrect exception, can output message
                    fail(name.getMethodName() + " - FAILED - caught invalid exception: " + error.getMessage());
                    return;
                } else {
                    // incorrect exception, can not output message
                    fail(name.getMethodName() + " - FAILED - caught unexpected exception");
                    return;
                }

                // check results from callback; expect to be finished within 1 second
                // should have been called ahead anyway
                if (methodWithExtendedErrorEnumAsyncCallbackDone == false) {
                    LOG.info(name.getMethodName() + " - 1st - wait a second for callback");
                    Thread.sleep(1000);
                    LOG.info(name.getMethodName() + " - 1st - wait for callback is over");
                } else {
                    LOG.info(name.getMethodName() + " - 1st - callback already done");
                }
                if (methodWithExtendedErrorEnumAsyncCallbackDone == false) {
                    fail(name.getMethodName() + " - FAILED - callback NOT done");
                    return;
                }
                if (!methodWithExtendedErrorEnumAsyncCallbackResult) {
                    fail(name.getMethodName() + " - FAILED - callback reported failure");
                    return;
                }
                LOG.info(name.getMethodName() + " - 1st - callback caught expected exception");
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        LOG.info(name.getMethodName() + " - ProviderRuntimeException check done");

        // 2nd test
        try {
            // setup input parameters
            String wantedExceptionArg = "ApplicationException_1";

            CallbackWithModeledError<Void, MethodWithExtendedErrorEnumErrorEnum> callback = new CallbackWithModeledError<Void, MethodWithExtendedErrorEnumErrorEnum>() {
                @Override
                public void onSuccess(Void result) {
                    LOG.info(name.getMethodName() + " - 2nd - unexpected positive return in callback");
                    LOG.info(name.getMethodName() + " - FAILED");
                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                    methodWithExtendedErrorEnumAsyncCallbackDone = true;
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    LOG.info(name.getMethodName() + " - 2nd - callback - caught invalid JoynrRuntime like exception "
                            + ((JoynrRuntimeException) error).getMessage());
                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                    methodWithExtendedErrorEnumAsyncCallbackDone = true;
                }

                @Override
                public void onFailure(MethodWithExtendedErrorEnumErrorEnum errorEnum) {
                    if (errorEnum == MethodWithExtendedErrorEnumErrorEnum.ERROR_3_3_NTC) {
                        LOG.info(name.getMethodName()
                                + " - 2nd - callback - caught ApplicationException with expected error enum");
                        methodWithExtendedErrorEnumAsyncCallbackResult = true;
                        methodWithExtendedErrorEnumAsyncCallbackDone = true;
                        return;
                    }
                    LOG.info(name.getMethodName()
                            + " - 2nd - callback - caught invalid ApplicationException with enum " + errorEnum);

                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                    methodWithExtendedErrorEnumAsyncCallbackDone = true;
                }
            };

            Future<Void> future = testInterfaceProxy.methodWithExtendedErrorEnum(callback, wantedExceptionArg);

            try {
                long timeoutInMilliseconds = 8000;
                LOG.info(name.getMethodName() + " - 2nd - about to call future.get");
                future.get(timeoutInMilliseconds);
                fail(name.getMethodName() + " - FAILED - unexpected return from future");
                return;
            } catch (InterruptedException | JoynrRuntimeException | ApplicationException error) {
                if (error instanceof ApplicationException) {
                    if (((ApplicationException) error).getError() == MethodWithExtendedErrorEnumErrorEnum.ERROR_3_3_NTC) {
                        LOG.info(name.getMethodName()
                                + " - 2nd - caught expected ApplicationException with correct error enum");
                    } else {
                        fail(name.getMethodName() + " - FAILED - 2nd - caught invalid ApplicationException with enum: "
                                + ((ApplicationException) error).getError());
                        return;
                    }
                } else if (error instanceof JoynrRuntimeException) {
                    // incorrect exception, can output message
                    fail(name.getMethodName() + " - FAILED - 2nd - caught invalid JoynrRuntimeException: "
                            + error.getMessage());
                    return;
                } else {
                    // incorrect exception, can not output message
                    fail(name.getMethodName() + " - FAILED - caught invalid other exception");
                    return;
                }

                // check results from callback; expect to be finished within 1 second
                // should have been called ahead anyway
                if (methodWithExtendedErrorEnumAsyncCallbackDone == false) {
                    LOG.info(name.getMethodName() + " - 2nd - about to wait for a second for callback");
                    Thread.sleep(1000);
                    LOG.info(name.getMethodName() + " - 2nd - wait for callback is over");
                } else {
                    LOG.info(name.getMethodName() + " - 2nd - callback already done");
                }
                if (methodWithExtendedErrorEnumAsyncCallbackDone == false) {
                    fail(name.getMethodName() + " - FAILED - 2nd - callback NOT done");
                    return;
                }
                if (!methodWithExtendedErrorEnumAsyncCallbackResult) {
                    fail(name.getMethodName() + " - FAILED - 2nd - callback reported success");
                    return;
                }
                LOG.info(name.getMethodName() + " - 2nd - callback has got expected exception");
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        LOG.info(name.getMethodName() + " - ApplicationException check done");
        LOG.info(name.getMethodName() + " - OK");
        return;

        // 3rd test omitted
    }
}
