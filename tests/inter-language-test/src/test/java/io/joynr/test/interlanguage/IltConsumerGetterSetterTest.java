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
import java.util.Objects;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import joynr.exceptions.ProviderRuntimeException;
import joynr.interlanguagetest.Enumeration;
import joynr.interlanguagetest.namedTypeCollection2.BaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedEnumerationWithPartlyDefinedValues;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedBaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.MapStringString;
import joynr.interlanguagetest.typeDefCollection.ArrayTypeDefStruct;

public class IltConsumerGetterSetterTest extends IltConsumerTest {
    private static final Logger logger = LoggerFactory.getLogger(IltConsumerGetterSetterTest.class);

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
    private <T> void genericGetterSetterTestMethod(T arg, String methodName) {
        try {
            Method mSetter = testInterfaceProxy.getClass().getMethod("set" + methodName, arg.getClass());
            Method mGetter = testInterfaceProxy.getClass().getMethod("get" + methodName);

            mSetter.invoke(testInterfaceProxy, arg);
            T result = (T) mGetter.invoke(testInterfaceProxy);

            assertNotNull(name.getMethodName() + TEST_FAILED_NO_RESULT, result);
            assertTrue(name.getMethodName() + TEST_FAILED_INVALID_RESULT, Objects.deepEquals(arg, result));

        } catch (Exception e) {
            fail(name.getMethodName() + TEST_FAILED_EXCEPTION + e.getMessage());
        }
    }

    /*
     * GETTER AND SETTER CALLS
     */

    @Test
    public void callSetAndGetAttributeUInt8() {
        logger.info(name.getMethodName());
        byte byteArg = 127;
        genericGetterSetterTestMethod(byteArg, "AttributeUInt8");
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callSetAndGetAttributeDouble() {
        logger.info(name.getMethodName());
        double doubleArg = 1.1d;
        genericGetterSetterTestMethod(doubleArg, "AttributeDouble");
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callGetAttributeBooleanReadOnly() {
        logger.info(name.getMethodName() + "");
        try {
            Boolean result;
            // value is hardcoded on provider side since it is readonly
            result = testInterfaceProxy.getAttributeBooleanReadonly();
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (result != true) {
                fail(name.getMethodName() + " - FAILED - got invalid result");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        logger.info(name.getMethodName() + " - OK");
    }

    // there is no setter for attributeBooleanReadOnly, but this cannot
    // be checked since it would create a compiler error

    @Test
    public void callSetAndGetAttributeStringNoSubscriptions() {
        logger.info(name.getMethodName());
        String stringArg = "Hello world";
        genericGetterSetterTestMethod(stringArg, "AttributeStringNoSubscriptions");
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callSetAndGetAttributeInt8readonlyNoSubscriptions() {
        logger.info(name.getMethodName() + "");
        try {
            Byte result;
            // value is hardcoded on provider side since it is readonly
            result = testInterfaceProxy.getAttributeInt8readonlyNoSubscriptions();
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (result != -128) {
                fail(name.getMethodName() + " - FAILED - got invalid result");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        logger.info(name.getMethodName() + " - OK");
    }

    // no setter for attributeInt8readonlyNoSubscriptions

    @Test
    public void callSetAndGetAttributeArrayOfStringImplicit() {
        logger.info(name.getMethodName());
        String[] stringArrayArg = { "Hello", "World" };
        genericGetterSetterTestMethod(stringArrayArg, "AttributeArrayOfStringImplicit");
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callSetAndSetAndGetAttributeByteBuffer() {
        logger.info(name.getMethodName());
        Byte[] byteBufferArg = { -128, 0, 127 };
        genericGetterSetterTestMethod(byteBufferArg, "AttributeByteBuffer");
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callSetAndGetAttributeInt64TypeDef() {
        logger.info(name.getMethodName());
        Long int64TypeDefArg = 1L;
        genericGetterSetterTestMethod(int64TypeDefArg, "AttributeInt64TypeDef");
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callSetAndGetAttributeStringTypeDef() {
        logger.info(name.getMethodName());
        String stringTypeDefArg = "StringTypeDef";
        genericGetterSetterTestMethod(stringTypeDefArg, "AttributeStringTypeDef");
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callSetAndGetAttributeStructTypeDef() {
        logger.info(name.getMethodName());
        BaseStruct structTypeDefArg = IltUtil.createBaseStruct();
        genericGetterSetterTestMethod(structTypeDefArg, "AttributeStructTypeDef");
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callSetAndGetGetAttributeMapTypeDef() {
        logger.info(name.getMethodName());
        MapStringString mapTypeDefArg = new MapStringString();
        mapTypeDefArg.put("keyString1", "valueString1");
        mapTypeDefArg.put("keyString2", "valueString2");
        mapTypeDefArg.put("keyString3", "valueString3");
        genericGetterSetterTestMethod(mapTypeDefArg, "AttributeMapTypeDef");
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callSetAndGetGetAttributeEnumTypeDef() {
        logger.info(name.getMethodName());
        Enumeration enumTypeDefArg = Enumeration.ENUM_0_VALUE_1;
        genericGetterSetterTestMethod(enumTypeDefArg, "AttributeEnumTypeDef");
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callSetAndGetGetAttributeByteBufferTypeDef() {
        logger.info(name.getMethodName());
        Byte[] byteBufferTypeDefArg = { -128, 0, 127 };
        genericGetterSetterTestMethod(byteBufferTypeDefArg, "AttributeByteBufferTypeDef");
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callSetAndGetGetAttributeArrayTypeDef() {
        logger.info(name.getMethodName());
        String[] stringArray = { "Hello", "World" };
        ArrayTypeDefStruct arrayTypeDefArg = new ArrayTypeDefStruct(stringArray);
        genericGetterSetterTestMethod(arrayTypeDefArg, "AttributeArrayTypeDef");
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callSetAndGetGetAttributeEnumeration() {
        logger.info(name.getMethodName());
        Enumeration enumerationArg = Enumeration.ENUM_0_VALUE_2;
        genericGetterSetterTestMethod(enumerationArg, "AttributeEnumeration");
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callGetAttributeExtendedEnumerationReadonly() {
        logger.info(name.getMethodName() + "");
        try {
            ExtendedEnumerationWithPartlyDefinedValues result;

            // value is hardcoded on provider side since it is readonly
            result = testInterfaceProxy.getAttributeExtendedEnumerationReadonly();
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (result != ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES) {
                fail(name.getMethodName() + " - FAILED - got invalid result");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        logger.info(name.getMethodName() + " - OK");
    }

    // no setter for attributeExtendedEnumerationReadonly

    @Test
    public void callSetAndGetGetAttributeBaseStruct() {
        logger.info(name.getMethodName());
        BaseStruct baseStructArg = IltUtil.createBaseStruct();
        genericGetterSetterTestMethod(baseStructArg, "AttributeBaseStruct");
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callSetAndGetAttributeExtendedExtendedBaseStruct() {
        logger.info(name.getMethodName());
        ExtendedExtendedBaseStruct extendedExtendedBaseStructArg = IltUtil.createExtendedExtendedBaseStruct();
        genericGetterSetterTestMethod(extendedExtendedBaseStructArg, "AttributeExtendedExtendedBaseStruct");
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    @Test
    public void callSetAndGetAttributeMapStringString() {
        logger.info(name.getMethodName());
        MapStringString attributeMapStringStringArg = new MapStringString();
        attributeMapStringStringArg.put("keyString1", "valueString1");
        attributeMapStringStringArg.put("keyString2", "valueString2");
        attributeMapStringStringArg.put("keyString3", "valueString3");
        genericGetterSetterTestMethod(attributeMapStringStringArg, "AttributeMapStringString");
        logger.info(name.getMethodName() + TEST_SUCCEEDED);
    }

    /*
     * GETTER AND SETTER CALLS WITH EXCEPTION
     */

    @Test
    public void callGetAttributeWithExceptionFromGetter() {
        logger.info(name.getMethodName() + "");
        try {
            testInterfaceProxy.getAttributeWithExceptionFromGetter();
            fail(name.getMethodName() + " - FAILED - unexpected return without exception");
            return;
        } catch (ProviderRuntimeException e) {
            if (e.getMessage() == null
                    || !e.getMessage().endsWith("Exception from getAttributeWithExceptionFromGetter")) {
                fail(name.getMethodName() + " - FAILED - caught invalid exception: " + e.getMessage());
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callSetAttributeWithExceptionFromSetter() {
        logger.info(name.getMethodName() + "");
        try {
            testInterfaceProxy.setAttributeWithExceptionFromSetter(false);
            fail(name.getMethodName() + " - FAILED - got no result");
            return;
        } catch (ProviderRuntimeException e) {
            if (e.getMessage() == null
                    || !e.getMessage().endsWith("Exception from setAttributeWithExceptionFromSetter")) {
                fail(name.getMethodName() + " - FAILED - caught invalid exception: " + e.getMessage());
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        logger.info(name.getMethodName() + " - OK");
    }
}
