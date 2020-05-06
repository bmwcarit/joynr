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

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.function.BiPredicate;

import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.interlanguagetest.Enumeration;
import joynr.interlanguagetest.TestInterfaceSync.MethodWithMultipleArrayParametersReturned;
import joynr.interlanguagetest.TestInterfaceSync.MethodWithMultipleEnumParametersReturned;
import joynr.interlanguagetest.TestInterfaceSync.MethodWithMultiplePrimitiveParametersReturned;
import joynr.interlanguagetest.TestInterfaceSync.MethodWithMultipleStructParametersReturned;
import joynr.interlanguagetest.TestInterfaceSync.OverloadedMethodOverloadedMethod1Returned;
import joynr.interlanguagetest.TestInterfaceSync.OverloadedMethodWithSelectorOverloadedMethodWithSelector1Returned;
import joynr.interlanguagetest.TestInterface.MethodWithAnonymousErrorEnumErrorEnum;
import joynr.interlanguagetest.TestInterface.MethodWithExtendedErrorEnumErrorEnum;
import joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray;
import joynr.interlanguagetest.namedTypeCollection2.BaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedBaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedEnumerationWithPartlyDefinedValues;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedErrorEnumTc;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedBaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedEnumeration;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedInterfaceEnumerationInTypeCollection;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedStructOfPrimitives;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedTypeCollectionEnumerationInTypeCollection;
import joynr.interlanguagetest.namedTypeCollection2.MapStringString;
import joynr.interlanguagetest.typeDefCollection.ArrayTypeDefStruct;

public class IltConsumerSyncMethodTest extends IltConsumerTest {
    private static final Logger logger = LoggerFactory.getLogger(IltConsumerSyncMethodTest.class);

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

    @SuppressWarnings("unchecked")
    private <T, U> void callProxyMethodWithParameterAndAssertResult(String methodName,
                                                                    T arg,
                                                                    BiPredicate<T, U> expectedResultCheck) {
        try {
            Method method = testInterfaceProxy.getClass().getMethod(methodName, arg.getClass());
            U result = (U) method.invoke(testInterfaceProxy, arg);

            assertNotNull(name.getMethodName() + TEST_FAILED_NO_RESULT, result);
            assertTrue(name.getMethodName() + TEST_FAILED_INVALID_RESULT, expectedResultCheck.test(arg, result));
        } catch (Exception e) {
            fail(name.getMethodName() + TEST_FAILED_EXCEPTION + e.getMessage());
        }
    }

    /*
     * SYNCHRONOUS METHOD CALLS
     */

    // no check possible other than handling exceptions
    @Test
    public void callMethodWithoutParameters() {
        logger.info(name.getMethodName() + "");
        try {
            testInterfaceProxy.methodWithoutParameters();
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithoutInputParameter() {
        logger.info(name.getMethodName() + "");
        try {
            Boolean b;
            b = testInterfaceProxy.methodWithoutInputParameter();
            // expect true to be returned
            if (!b) {
                fail(name.getMethodName() + " - FAILED - got invalid result");
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithoutOutputParameter() {
        logger.info(name.getMethodName() + "");
        try {
            boolean arg = false;
            testInterfaceProxy.methodWithoutOutputParameter(arg);
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithSinglePrimitiveParameters() {
        logger.info(name.getMethodName() + "");
        // short shortArg = (short)65535;
        short shortArg = (short) 32767;
        callProxyMethodWithParameterAndAssertResult("methodWithSinglePrimitiveParameters",
                                                    shortArg,
                                                    (Short arg,
                                                     String res) -> res.equals(new Integer(Short.toUnsignedInt(arg)).toString()));
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callmethodWithSingleMapParameters() {
        logger.info(name.getMethodName() + "");
        MapStringString mapArg = new MapStringString();
        mapArg.put("keyString1", "valueString1");
        mapArg.put("keyString2", "valueString2");
        mapArg.put("keyString3", "valueString3");
        MapStringString expectedMap = new MapStringString();
        expectedMap.put("valueString1", "keyString1");
        expectedMap.put("valueString2", "keyString2");
        expectedMap.put("valueString3", "keyString3");
        callProxyMethodWithParameterAndAssertResult("methodWithSingleMapParameters",
                                                    mapArg,
                                                    (MapStringString arg,
                                                     MapStringString res) -> res.equals(expectedMap));
        logger.info(name.getMethodName() + " - OK");
    }

    // problems might be to expect wrt. float or double comparison
    @Test
    public void callMethodWithMultiplePrimitiveParameters() {
        logger.info(name.getMethodName() + "");
        try {
            int arg1 = 2147483647;
            float arg2 = 47.11f;
            boolean arg3 = false;
            MethodWithMultiplePrimitiveParametersReturned result;
            result = testInterfaceProxy.methodWithMultiplePrimitiveParameters(arg1, arg2, arg3);
            // It might be difficult to compare a float since number representation
            // might be different.
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (!IltUtil.cmpDouble(result.doubleOut, arg2) || !result.stringOut.equals(Integer.toString(arg1))) {
                logger.info(name.getMethodName() + " - int32Arg = " + arg1);
                logger.info(name.getMethodName() + " - input floatArg= " + arg2);
                logger.info(name.getMethodName() + " - result.doubleOut = " + result.doubleOut);
                logger.info(name.getMethodName() + " - result.stringOut = " + result.stringOut);
                fail(name.getMethodName() + " - FAILED - got invalid result");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithSingleArrayParameters() {
        logger.info(name.getMethodName() + "");
        Double[] doubleArrayArg = IltUtil.createDoubleArray();
        String[] stringArray = { "Hello", "World" };
        callProxyMethodWithParameterAndAssertResult("methodWithSingleArrayParameters",
                                                    doubleArrayArg,
                                                    (Double[] arg, String[] res) -> Arrays.equals(res, stringArray));
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithMultipleArrayParameters() {
        logger.info(name.getMethodName() + "");
        try {
            String[] arg1 = { "Hello", "World" };
            Byte[] arg2 = IltUtil.createByteArray();
            ExtendedInterfaceEnumerationInTypeCollection[] arg3 = IltUtil.createExtendedInterfaceEnumerationInTypeCollectionArray();
            StructWithStringArray[] arg4 = IltUtil.createStructWithStringArrayArray();
            MethodWithMultipleArrayParametersReturned result;

            result = testInterfaceProxy.methodWithMultipleArrayParameters(arg1, arg2, arg3, arg4);

            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (!IltUtil.checkUInt64Array(result.uInt64ArrayOut)) {
                fail(name.getMethodName() + " - FAILED - got invalid result - stringArrayArg");
                return;
            }
            if (result.structWithStringArrayArrayOut.length != 2) {
                fail(name.getMethodName() + " - FAILED - got invalid result - structWithStringArrayArrayOut");
                return;
            }
            if (!IltUtil.checkStructWithStringArray(result.structWithStringArrayArrayOut[0])) {
                fail(name.getMethodName() + " - FAILED - got invalid result - structWithStringArrayArrayOut[0]");
                return;
            }
            if (!IltUtil.checkStructWithStringArray(result.structWithStringArrayArrayOut[1])) {
                fail(name.getMethodName() + " - FAILED - got invalid result - structWithStringArrayArrayOut[1]");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithSingleByteBufferParameter() {
        logger.info(name.getMethodName());
        Byte[] byteBufferArg = { -128, 0, 127 };
        callProxyMethodWithParameterAndAssertResult("methodWithSingleByteBufferParameter",
                                                    byteBufferArg,
                                                    (Byte[] arg, Byte[] res) -> Arrays.equals(arg, res));
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithMultipleByteBufferParameters() {
        logger.info(name.getMethodName());
        try {
            Byte[] byteBufferArg1 = { -5, 125 };
            Byte[] byteBufferArg2 = { 78, 0 };

            Byte[] concatenatedBuffer = IltUtil.concatenateByteArrays(byteBufferArg1, byteBufferArg2);

            Byte[] result = testInterfaceProxy.methodWithMultipleByteBufferParameters(byteBufferArg1, byteBufferArg2);

            Assert.assertNotNull(name.getMethodName() + " - FAILED - got no result", result);
            Assert.assertArrayEquals(name.getMethodName() + " - FAILED - got invalid result",
                                     concatenatedBuffer,
                                     result);
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithInt64TypeDefParameter() {
        logger.info(name.getMethodName());
        Long int64TypeDefArg = 1L;
        callProxyMethodWithParameterAndAssertResult("methodWithInt64TypeDefParameter",
                                                    int64TypeDefArg,
                                                    (Long arg, Long res) -> res.equals(arg));
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithStringTypeDefParameter() {
        logger.info(name.getMethodName());
        String stringTypeDefArg = "StringTypeDef";
        callProxyMethodWithParameterAndAssertResult("methodWithStringTypeDefParameter",
                                                    stringTypeDefArg,
                                                    (String arg, String res) -> res.equals(arg));
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithStructTypeDefParameter() {
        logger.info(name.getMethodName());
        BaseStruct structTypeDefArg = IltUtil.createBaseStruct();
        callProxyMethodWithParameterAndAssertResult("methodWithStructTypeDefParameter",
                                                    structTypeDefArg,
                                                    (BaseStruct arg, BaseStruct res) -> res.equals(arg));
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithMapTypeDefParameter() {
        logger.info(name.getMethodName());
        MapStringString mapTypeDefArg = new MapStringString();
        mapTypeDefArg.put("keyString1", "valueString1");
        mapTypeDefArg.put("keyString2", "valueString2");
        mapTypeDefArg.put("keyString3", "valueString3");
        callProxyMethodWithParameterAndAssertResult("methodWithMapTypeDefParameter",
                                                    mapTypeDefArg,
                                                    (MapStringString arg, MapStringString res) -> res.equals(arg));
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithEnumTypeDefParameter() {
        logger.info(name.getMethodName());
        Enumeration enumTypeDefArg = Enumeration.ENUM_0_VALUE_1;
        callProxyMethodWithParameterAndAssertResult("methodWithEnumTypeDefParameter",
                                                    enumTypeDefArg,
                                                    (Enumeration arg, Enumeration res) -> res.equals(arg));
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithByteBufferTypeDefParameter() {
        logger.info(name.getMethodName());
        Byte[] byteBufferTypeDefArg = { -128, 0, 127 };
        callProxyMethodWithParameterAndAssertResult("methodWithByteBufferTypeDefParameter",
                                                    byteBufferTypeDefArg,
                                                    (Byte[] arg, Byte[] res) -> Arrays.equals(arg, res));
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithArrayTypeDefParameter() {
        logger.info(name.getMethodName());
        String[] stringArray = { "Hello", "World" };
        ArrayTypeDefStruct arrayTypeDefArg = new ArrayTypeDefStruct(stringArray);
        callProxyMethodWithParameterAndAssertResult("methodWithArrayTypeDefParameter",
                                                    arrayTypeDefArg,
                                                    (ArrayTypeDefStruct arg,
                                                     ArrayTypeDefStruct res) -> res.equals(arg));
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithSingleEnumParameters() {
        logger.info(name.getMethodName() + "");
        ExtendedEnumerationWithPartlyDefinedValues enumerationArg = ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
        callProxyMethodWithParameterAndAssertResult("methodWithSingleEnumParameters",
                                                    enumerationArg,
                                                    (ExtendedEnumerationWithPartlyDefinedValues arg,
                                                     ExtendedTypeCollectionEnumerationInTypeCollection res) -> res == ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION);
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithMultipleEnumParameters() {
        logger.info(name.getMethodName() + "");
        try {
            joynr.interlanguagetest.Enumeration enumerationArg;
            ExtendedTypeCollectionEnumerationInTypeCollection extendedEnumerationArg;
            MethodWithMultipleEnumParametersReturned result;

            enumerationArg = joynr.interlanguagetest.Enumeration.ENUM_0_VALUE_3;
            extendedEnumerationArg = ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;

            result = testInterfaceProxy.methodWithMultipleEnumParameters(enumerationArg, extendedEnumerationArg);
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (result.enumerationOut != Enumeration.ENUM_0_VALUE_1
                    || result.extendedEnumerationOut != ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES) {
                logger.info(name.getMethodName() + " - FAILED");
                fail(name.getMethodName() + " - FAILED - got invalid result - enumerationOut");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithSingleStructParameters() {
        logger.info(name.getMethodName() + "");
        ExtendedBaseStruct extBaseStructArg = IltUtil.createExtendedBaseStruct();
        callProxyMethodWithParameterAndAssertResult("methodWithSingleStructParameters",
                                                    extBaseStructArg,
                                                    (ExtendedBaseStruct arg,
                                                     ExtendedStructOfPrimitives res) -> IltUtil.checkExtendedStructOfPrimitives(res));
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithMultipleStructParameters() {
        logger.info(name.getMethodName() + "");
        try {
            MethodWithMultipleStructParametersReturned result;

            // setup input parameters
            ExtendedStructOfPrimitives extendedStructOfPrimitivesOut = IltUtil.createExtendedStructOfPrimitives();
            BaseStruct baseStructOut = IltUtil.createBaseStruct();

            result = testInterfaceProxy.methodWithMultipleStructParameters(extendedStructOfPrimitivesOut,
                                                                           baseStructOut);
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (!IltUtil.checkBaseStructWithoutElements(result.baseStructWithoutElementsOut)) {
                fail(name.getMethodName() + " - FAILED - got invalid result - baseStructWithoutElementsOut");
                return;
            }

            ExtendedExtendedBaseStruct extendedExtendedBaseStruct = IltUtil.createExtendedExtendedBaseStruct();
            if (!result.extendedExtendedBaseStructOut.equals(extendedExtendedBaseStruct)) {
                fail(name.getMethodName() + " - FAILED - got invalid result - extendedExtendedBaseStructOut");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callOverloadedMethod_1() {
        logger.info(name.getMethodName() + "");
        try {
            String result;
            result = testInterfaceProxy.overloadedMethod();
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (!result.equals("TestString 1")) {
                fail(name.getMethodName() + " - FAILED - got invalid result");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callOverloadedMethod_2() {
        logger.info(name.getMethodName() + "");
        Boolean booleanArg = false;
        callProxyMethodWithParameterAndAssertResult("overloadedMethod",
                                                    booleanArg,
                                                    (Boolean arg, String res) -> res.equals("TestString 2"));
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callOverloadedMethod_3() {
        logger.info(name.getMethodName() + "");
        try {
            OverloadedMethodOverloadedMethod1Returned result;
            ExtendedExtendedEnumeration[] enumArrayArg = IltUtil.createExtendedExtendedEnumerationArray();
            Long int64Arg = 1L;
            BaseStruct baseStructArg = IltUtil.createBaseStruct();
            Boolean booleanArg = false;

            result = testInterfaceProxy.overloadedMethod(enumArrayArg, int64Arg, baseStructArg, booleanArg);

            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            String[] stringArray = { "Hello", "World" };
            if (result.doubleOut != 0d || (!Arrays.equals(stringArray, result.stringArrayOut))) {
                fail(name.getMethodName() + " - FAILED - got invalid result - doubleOut");
                return;
            }

            ExtendedBaseStruct extendedBaseStruct = IltUtil.createExtendedBaseStruct();
            if (!result.extendedBaseStructOut.equals(extendedBaseStruct)) {
                fail(name.getMethodName() + " - FAILED - got invalid result - extendedBaseStructOut");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callOverloadedMethodWithSelector_1() {
        logger.info(name.getMethodName() + "");
        try {
            String result;
            result = testInterfaceProxy.overloadedMethodWithSelector();
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (!result.equals("Return value from overloadedMethodWithSelector 1")) {
                fail(name.getMethodName() + " - FAILED - got invalid result");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callOverloadedMethodWithSelector_2() {
        logger.info(name.getMethodName() + "");
        Boolean booleanArg = false;
        callProxyMethodWithParameterAndAssertResult("overloadedMethodWithSelector",
                                                    booleanArg,
                                                    (Boolean arg,
                                                     String res) -> res.equals("Return value from overloadedMethodWithSelector 2"));
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callOverloadedMethodWithSelector_3() {
        logger.info(name.getMethodName() + "");
        try {
            OverloadedMethodWithSelectorOverloadedMethodWithSelector1Returned result;
            ExtendedExtendedEnumeration[] enumArrayArg = IltUtil.createExtendedExtendedEnumerationArray();
            Long int64arg = 1L;
            BaseStruct baseStructArg = IltUtil.createBaseStruct();
            Boolean booleanArg = false;
            result = testInterfaceProxy.overloadedMethodWithSelector(enumArrayArg, int64arg, baseStructArg, booleanArg);
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            String[] stringArray = { "Hello", "World" };

            ExtendedBaseStruct extendedBaseStruct = IltUtil.createExtendedBaseStruct();
            if (result.doubleOut != 1.1d || (!result.extendedBaseStructOut.equals(extendedBaseStruct))
                    || (!Arrays.equals(stringArray, result.stringArrayOut))) {
                fail(name.getMethodName() + " - FAILED - got invalid result");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithStringsAndSpecifiedStringOutLength() {
        logger.info(name.getMethodName() + "");
        try {
            String stringArg = "Hello world";
            Integer int32StringLengthArg = 32;
            String result;
            result = testInterfaceProxy.methodWithStringsAndSpecifiedStringOutLength(stringArg, int32StringLengthArg);
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (result.length() != int32StringLengthArg) {
                fail(name.getMethodName() + " - FAILED - got invalid result");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }

    /*
     * SYNCHRONOUS METHOD CALLS WITH EXCEPTION HANDLING
     */

    @Test
    public void callMethodWithoutErrorEnum() {
        logger.info(name.getMethodName() + "");
        try {
            String wantedExceptionArg = "ProviderRuntimeException";
            testInterfaceProxy.methodWithoutErrorEnum(wantedExceptionArg);
            fail(name.getMethodName() + " - FAILED - unexpected return");
            return;
        } catch (ProviderRuntimeException e) {
            if (e.getMessage() == null || !e.getMessage().endsWith("Exception from methodWithoutErrorEnum")) {
                fail(name.getMethodName() + " - FAILED - invalid exception message");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithAnonymousErrorEnum() {
        logger.info(name.getMethodName() + "");
        try {
            String wantedExceptionArg = "ProviderRuntimeException";
            testInterfaceProxy.methodWithAnonymousErrorEnum(wantedExceptionArg);
            fail(name.getMethodName() + " - FAILED - got no result");
            return;
        } catch (ProviderRuntimeException e) {
            if (e.getMessage() == null || !e.getMessage().endsWith("Exception from methodWithAnonymousErrorEnum")) {
                fail(name.getMethodName() + " - FAILED - got invalid ProviderRuntimeException");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        // 2nd test
        try {
            String wantedExceptionArg = "ApplicationException";
            testInterfaceProxy.methodWithAnonymousErrorEnum(wantedExceptionArg);
            fail(name.getMethodName() + " - FAILED - unexpected return of method");
            return;
        } catch (ApplicationException e) {
            if (e.getError() != MethodWithAnonymousErrorEnumErrorEnum.ERROR_3_1_NTC) {
                fail(name.getMethodName() + " - FAILED - got invalid exception error enum value");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithExistingErrorEnum() {
        logger.info(name.getMethodName() + "");

        // 1st test
        try {
            String wantedExceptionArg = "ProviderRuntimeException";
            testInterfaceProxy.methodWithExistingErrorEnum(wantedExceptionArg);
            fail(name.getMethodName() + " - FAILED - 1st - unexpected return without exception");
            return;
        } catch (ProviderRuntimeException e) {
            if (e.getMessage() == null || !e.getMessage().endsWith("Exception from methodWithExistingErrorEnum")) {
                fail(name.getMethodName() + " - FAILED - 1st - got invalid exception content");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - 1st - caught unexpected exception type: " + e.getMessage());
            return;
        }

        // 2nd test
        try {
            String wantedExceptionArg = "ApplicationException_1";
            testInterfaceProxy.methodWithExistingErrorEnum(wantedExceptionArg);
            fail(name.getMethodName() + " - FAILED - 2nd - unexpected return without exception");
            return;
        } catch (ApplicationException e) {
            if (e.getError() != ExtendedErrorEnumTc.ERROR_2_3_TC2) {
                fail(name.getMethodName() + " - FAILED - 2nd - unexpected exception error enum value");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        // 3rd test
        try {
            String wantedExceptionArg = "ApplicationException_2";
            testInterfaceProxy.methodWithExistingErrorEnum(wantedExceptionArg);
            fail(name.getMethodName() + " - FAILED - 3rd - unexpected return without exception");
            return;
        } catch (ApplicationException e) {
            if (e.getError() != ExtendedErrorEnumTc.ERROR_1_2_TC_2) {
                fail(name.getMethodName() + " - FAILED - 3rd - unexpected exception error enum value");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithExtendedErrorEnum() {
        logger.info(name.getMethodName() + "");

        // 1st test
        try {
            String wantedExceptionArg = "ProviderRuntimeException";
            testInterfaceProxy.methodWithExtendedErrorEnum(wantedExceptionArg);
            fail(name.getMethodName() + " - FAILED - 1st - unexpected return without exception");
            return;
        } catch (ProviderRuntimeException e) {
            if (e.getMessage() == null || !e.getMessage().endsWith("Exception from methodWithExtendedErrorEnum")) {
                fail(name.getMethodName() + " - FAILED - 1st - invalid exception message");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        // 2nd test
        try {
            String wantedExceptionArg = "ApplicationException_1";
            testInterfaceProxy.methodWithExtendedErrorEnum(wantedExceptionArg);
            fail(name.getMethodName() + " - FAILED - 2nd - unexpected return without exception");
            return;
        } catch (ApplicationException e) {
            if (e.getError() != MethodWithExtendedErrorEnumErrorEnum.ERROR_3_3_NTC) {
                fail(name.getMethodName() + " - FAILED - 2nd - unexpected exception error enum value");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        // 3rd test
        try {
            String wantedExceptionArg = "ApplicationException_2";
            testInterfaceProxy.methodWithExtendedErrorEnum(wantedExceptionArg);
            fail(name.getMethodName() + " - FAILED - 3rd - unexpected return without exception");
            return;
        } catch (ApplicationException e) {
            if (e.getError() != MethodWithExtendedErrorEnumErrorEnum.ERROR_2_1_TC2) {
                fail(name.getMethodName() + " - FAILED - 3rd - unexpected exception error enum value");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }
}
