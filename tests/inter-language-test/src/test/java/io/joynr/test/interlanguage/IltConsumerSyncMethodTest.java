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

import java.lang.reflect.Method;
import java.util.Objects;

import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.interlanguagetest.Enumeration;
import joynr.interlanguagetest.TestInterface.MethodWithAnonymousErrorEnumErrorEnum;
import joynr.interlanguagetest.TestInterface.MethodWithExtendedErrorEnumErrorEnum;
import joynr.interlanguagetest.TestInterfaceSync.MethodWithMultipleArrayParametersReturned;
import joynr.interlanguagetest.TestInterfaceSync.MethodWithMultipleEnumParametersReturned;
import joynr.interlanguagetest.TestInterfaceSync.MethodWithMultiplePrimitiveParametersReturned;
import joynr.interlanguagetest.TestInterfaceSync.MethodWithMultipleStructParametersReturned;
import joynr.interlanguagetest.TestInterfaceSync.OverloadedMethodOverloadedMethod1Returned;
import joynr.interlanguagetest.TestInterfaceSync.OverloadedMethodWithSelectorOverloadedMethodWithSelector1Returned;
import joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray;
import joynr.interlanguagetest.namedTypeCollection2.BaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedBaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedEnumerationWithPartlyDefinedValues;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedErrorEnumTc;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedEnumeration;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedInterfaceEnumerationInTypeCollection;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedStructOfPrimitives;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedTypeCollectionEnumerationInTypeCollection;
import joynr.interlanguagetest.namedTypeCollection2.MapStringString;
import joynr.interlanguagetest.typeDefCollection.ArrayTypeDefStruct;

import org.apache.commons.lang.ArrayUtils;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

import org.junit.Assert;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

public class IltConsumerSyncMethodTest extends IltConsumerTest {
    private static final Logger LOG = LoggerFactory.getLogger(IltConsumerSyncMethodTest.class);

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

    @SuppressWarnings("unchecked")
    private <T> void callProxyMethodWithParameterAndAssertResult(String methodName, T arg) {
        try {
            Method method = testInterfaceProxy.getClass().getMethod(methodName, arg.getClass());
            T result = (T) method.invoke(testInterfaceProxy, arg);

            assertNotNull(name.getMethodName() + TEST_FAILED_NO_RESULT, result);
            assertTrue(name.getMethodName() + TEST_FAILED_INVALID_RESULT, Objects.deepEquals(arg, result));
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
        LOG.info(name.getMethodName() + "");
        try {
            testInterfaceProxy.methodWithoutParameters();
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED: " + e.getMessage());
            return;
        }
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithoutInputParameter() {
        LOG.info(name.getMethodName() + "");
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
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithoutOutputParameter() {
        LOG.info(name.getMethodName() + "");
        try {
            boolean arg = false;
            testInterfaceProxy.methodWithoutOutputParameter(arg);
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithSinglePrimitiveParameters() {
        LOG.info(name.getMethodName() + "");
        try {
            // short arg = (short)65535;
            short arg = (short) 32767;
            String result;
            result = testInterfaceProxy.methodWithSinglePrimitiveParameters(arg);
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
            }
            if (!result.equals(new Integer(Short.toUnsignedInt(arg)).toString())) {
                fail(name.getMethodName() + " - FAILED - got invalid result");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callmethodWithSingleMapParameters() {
        LOG.info(name.getMethodName() + "");
        try {
            MapStringString mapArg = new MapStringString();
            mapArg.put("keyString1", "valueString1");
            mapArg.put("keyString2", "valueString2");
            mapArg.put("keyString3", "valueString3");
            MapStringString result;
            result = testInterfaceProxy.methodWithSingleMapParameters(mapArg);
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            MapStringString expected = new MapStringString();
            expected.put("valueString1", "keyString1");
            expected.put("valueString2", "keyString2");
            expected.put("valueString3", "keyString3");
            if (!result.equals(expected)) {
                fail(name.getMethodName() + " - FAILED - got invalid result");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
        LOG.info(name.getMethodName() + " - OK");
    }

    // problems might be to expect wrt. float or double comparison
    @Test
    public void callMethodWithMultiplePrimitiveParameters() {
        LOG.info(name.getMethodName() + "");
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
                LOG.info(name.getMethodName() + " - int32Arg = " + arg1);
                LOG.info(name.getMethodName() + " - input floatArg= " + arg2);
                LOG.info(name.getMethodName() + " - result.doubleOut = " + result.doubleOut);
                LOG.info(name.getMethodName() + " - result.stringOut = " + result.stringOut);
                fail(name.getMethodName() + " - FAILED - got invalid result");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithSingleArrayParameters() {
        LOG.info(name.getMethodName() + "");
        try {
            Double[] arg = IltUtil.createDoubleArray();
            String[] result;

            result = testInterfaceProxy.methodWithSingleArrayParameters(arg);
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (!IltUtil.checkStringArray(result)) {
                fail(name.getMethodName() + " - FAILED - got invalid result");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithMultipleArrayParameters() {
        LOG.info(name.getMethodName() + "");
        try {
            String[] arg1 = IltUtil.createStringArray();
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
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithSingleByteBufferParameter() {
        LOG.info(name.getMethodName());
        Byte[] byteBufferArg = { -128, 0, 127 };
        callProxyMethodWithParameterAndAssertResult("methodWithSingleByteBufferParameter", byteBufferArg);
        LOG.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithMultipleByteBufferParameters() {
        LOG.info(name.getMethodName());
        try {
            Byte[] byteBufferArg1 = { -5, 125 };
            Byte[] byteBufferArg2 = { 78, 0 };

            Byte[] result = testInterfaceProxy.methodWithMultipleByteBufferParameters(byteBufferArg1, byteBufferArg2);

            Assert.assertNotNull(name.getMethodName() + " - FAILED - got no result", result);
            Assert.assertArrayEquals(name.getMethodName() + " - FAILED - got invalid result",
                                     (Byte[]) ArrayUtils.addAll(byteBufferArg1, byteBufferArg2),
                                     result);
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithInt64TypeDefParameter() {
        LOG.info(name.getMethodName());
        Long int64TypeDefArg = 1L;
        callProxyMethodWithParameterAndAssertResult("methodWithInt64TypeDefParameter", int64TypeDefArg);
        LOG.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithStringTypeDefParameter() {
        LOG.info(name.getMethodName());
        String stringTypeDefArg = "StringTypeDef";
        callProxyMethodWithParameterAndAssertResult("methodWithStringTypeDefParameter", stringTypeDefArg);
        LOG.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithStructTypeDefParameter() {
        LOG.info(name.getMethodName());
        BaseStruct structTypeDefArg = IltUtil.createBaseStruct();
        callProxyMethodWithParameterAndAssertResult("methodWithStructTypeDefParameter", structTypeDefArg);
        LOG.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithMapTypeDefParameter() {
        LOG.info(name.getMethodName());
        MapStringString mapTypeDefArg = new MapStringString();
        mapTypeDefArg.put("keyString1", "valueString1");
        mapTypeDefArg.put("keyString2", "valueString2");
        mapTypeDefArg.put("keyString3", "valueString3");
        callProxyMethodWithParameterAndAssertResult("methodWithMapTypeDefParameter", mapTypeDefArg);
        LOG.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithEnumTypeDefParameter() {
        LOG.info(name.getMethodName());
        Enumeration enumTypeDefArg = Enumeration.ENUM_0_VALUE_1;
        callProxyMethodWithParameterAndAssertResult("methodWithEnumTypeDefParameter", enumTypeDefArg);
        LOG.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithByteBufferTypeDefParameter() {
        LOG.info(name.getMethodName());
        Byte[] byteBufferTypeDefArg = { -128, 0, 127 };
        callProxyMethodWithParameterAndAssertResult("methodWithByteBufferTypeDefParameter", byteBufferTypeDefArg);
        LOG.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithArrayTypeDefParameter() {
        LOG.info(name.getMethodName());
        String[] stringArray = IltUtil.createStringArray();
        ArrayTypeDefStruct arrayTypeDefArg = new ArrayTypeDefStruct(stringArray);
        callProxyMethodWithParameterAndAssertResult("methodWithArrayTypeDefParameter", arrayTypeDefArg);
        LOG.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callMethodWithSingleEnumParameters() {
        LOG.info(name.getMethodName() + "");
        try {
            ExtendedEnumerationWithPartlyDefinedValues enumerationArg = ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
            ExtendedTypeCollectionEnumerationInTypeCollection result;

            result = testInterfaceProxy.methodWithSingleEnumParameters(enumerationArg);
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (result != ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
                fail(name.getMethodName() + " - FAILED - got invalid result");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithMultipleEnumParameters() {
        LOG.info(name.getMethodName() + "");
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
                LOG.info(name.getMethodName() + " - FAILED");
                fail(name.getMethodName() + " - FAILED - got invalid result - enumerationOut");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithSingleStructParameters() {
        LOG.info(name.getMethodName() + "");
        try {
            ExtendedBaseStruct arg = IltUtil.createExtendedBaseStruct();
            ExtendedStructOfPrimitives result;

            result = testInterfaceProxy.methodWithSingleStructParameters(arg);
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (!IltUtil.checkExtendedStructOfPrimitives(result)) {
                fail(name.getMethodName() + " - FAILED - got invalid result - extendedStructOfPrimitives");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithMultipleStructParameters() {
        LOG.info(name.getMethodName() + "");
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

            if (!IltUtil.checkExtendedExtendedBaseStruct(result.extendedExtendedBaseStructOut)) {
                fail(name.getMethodName() + " - FAILED - got invalid result - extendedExtendedBaseStructOut");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callOverloadedMethod_1() {
        LOG.info(name.getMethodName() + "");
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
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callOverloadedMethod_2() {
        LOG.info(name.getMethodName() + "");
        try {
            String result;
            Boolean booleanArg = false;
            result = testInterfaceProxy.overloadedMethod(booleanArg);
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (!result.equals("TestString 2")) {
                fail(name.getMethodName() + " - FAILED - got invalid result");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callOverloadedMethod_3() {
        LOG.info(name.getMethodName() + "");
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
            if (result.doubleOut != 0d || !IltUtil.checkStringArray(result.stringArrayOut)) {
                fail(name.getMethodName() + " - FAILED - got invalid result - doubleOut");
                return;
            }
            if (!IltUtil.checkExtendedBaseStruct(result.extendedBaseStructOut)) {
                fail(name.getMethodName() + " - FAILED - got invalid result - extendedBaseStructOut");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callOverloadedMethodWithSelector_1() {
        LOG.info(name.getMethodName() + "");
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
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callOverloadedMethodWithSelector_2() {
        LOG.info(name.getMethodName() + "");
        try {
            String result;
            Boolean booleanArg = false;
            result = testInterfaceProxy.overloadedMethodWithSelector(booleanArg);
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (!result.equals("Return value from overloadedMethodWithSelector 2")) {
                fail(name.getMethodName() + " - FAILED - got invalid result");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callOverloadedMethodWithSelector_3() {
        LOG.info(name.getMethodName() + "");
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
            if (result.doubleOut != 1.1d || !IltUtil.checkExtendedBaseStruct(result.extendedBaseStructOut)
                    || !IltUtil.checkStringArray(result.stringArrayOut)) {
                fail(name.getMethodName() + " - FAILED - got invalid result");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithStringsAndSpecifiedStringOutLength() {
        LOG.info(name.getMethodName() + "");
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
        LOG.info(name.getMethodName() + " - OK");
    }

    /*
     * SYNCHRONOUS METHOD CALLS WITH EXCEPTION HANDLING
     */

    @Test
    public void callMethodWithoutErrorEnum() {
        LOG.info(name.getMethodName() + "");
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
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithAnonymousErrorEnum() {
        LOG.info(name.getMethodName() + "");
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
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithExistingErrorEnum() {
        LOG.info(name.getMethodName() + "");

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
        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithExtendedErrorEnum() {
        LOG.info(name.getMethodName() + "");

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
        LOG.info(name.getMethodName() + " - OK");
    }
}
