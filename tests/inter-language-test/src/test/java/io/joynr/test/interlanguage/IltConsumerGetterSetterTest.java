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

import joynr.exceptions.ProviderRuntimeException;
import joynr.interlanguagetest.Enumeration;
import joynr.interlanguagetest.namedTypeCollection2.BaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedEnumerationWithPartlyDefinedValues;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedBaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.MapStringString;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import org.junit.Assert;
import org.junit.Test;

import static org.junit.Assert.fail;

public class IltConsumerGetterSetterTest extends IltConsumerTest {
    private static final Logger LOG = LoggerFactory.getLogger(IltConsumerTest.class);

    /*
     * GETTER AND SETTER CALLS
     */

    @Test
    public void callGetAttributeUInt8() {
        LOG.info(name.getMethodName() + "");
        try {
            Byte result;

            // must set the value before it can be retrieved again
            byte byteArg = 127;
            testInterfaceProxy.setAttributeUInt8(byteArg);

            result = testInterfaceProxy.getAttributeUInt8();
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (result != 127) {
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
    public void callSetAttributeUInt8() {
        LOG.info(name.getMethodName() + "");
        try {
            byte byteArg = 127;
            testInterfaceProxy.setAttributeUInt8(byteArg);
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callGetAttributeDouble() {
        LOG.info(name.getMethodName() + "");
        try {
            Double result;

            // must set the value before it can be retrieved again
            double doubleArg = 1.1d;
            testInterfaceProxy.setAttributeDouble(doubleArg);

            result = testInterfaceProxy.getAttributeDouble();
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (!IltUtil.cmpDouble(result, 1.1d)) {
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
    public void callSetAttributeDouble() {
        LOG.info(name.getMethodName() + "");
        try {
            double doubleArg = 1.1d;
            testInterfaceProxy.setAttributeDouble(doubleArg);
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callGetAttributeBooleanReadOnly() {
        LOG.info(name.getMethodName() + "");
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

        LOG.info(name.getMethodName() + " - OK");
    }

    // there is no setter for attributeBooleanReadOnly, but this cannot
    // be checked since it would create a compiler error

    @Test
    public void callGetAttributeStringNoSubscriptions() {
        LOG.info(name.getMethodName() + "");
        try {
            String result;

            // must set the value before it can be retrieved again
            String stringArg = "Hello world";
            testInterfaceProxy.setAttributeStringNoSubscriptions(stringArg);

            result = testInterfaceProxy.getAttributeStringNoSubscriptions();
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (!result.equals("Hello world")) {
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
    public void callSetAttributeStringNoSubscriptions() {
        LOG.info(name.getMethodName() + "");
        try {
            String stringArg = "Hello world";
            testInterfaceProxy.setAttributeStringNoSubscriptions(stringArg);
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callGetAttributeInt8readonlyNoSubscriptions() {
        LOG.info(name.getMethodName() + "");
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

        LOG.info(name.getMethodName() + " - OK");
    }

    // no setter for attributeInt8readonlyNoSubscriptions

    @Test
    public void callGetAttributeArrayOfStringImplicit() {
        LOG.info(name.getMethodName() + "");
        try {
            String[] result;

            // must set the value before it can be retrieved again
            String[] stringArrayArg = IltUtil.createStringArray();
            testInterfaceProxy.setAttributeArrayOfStringImplicit(stringArrayArg);

            result = testInterfaceProxy.getAttributeArrayOfStringImplicit();
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (!IltUtil.checkStringArray(result)) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callSetAttributeArrayOfStringImplicit() {
        LOG.info(name.getMethodName() + "");
        try {
            String[] stringArrayArg = IltUtil.createStringArray();
            testInterfaceProxy.setAttributeArrayOfStringImplicit(stringArrayArg);
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callGetAndSetAttributeByteBuffer() {
        LOG.info(name.getMethodName());
        try {
            // must set the value before it can be retrieved again
            Byte[] byteBufferArg = { -128, 0, 127 };
            testInterfaceProxy.setAttributeByteBuffer(byteBufferArg);

            Byte[] result = testInterfaceProxy.getAttributeByteBuffer();
            Assert.assertNotNull(name.getMethodName() + " - FAILED - got no result", result);
            Assert.assertArrayEquals(name.getMethodName() + " - FAILED - got invalid result", byteBufferArg, result);
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }

        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callGetAttributeEnumeration() {
        LOG.info(name.getMethodName() + "");
        try {
            Enumeration result;

            // must set the value before it can be retrieved again
            Enumeration enumerationArg = Enumeration.ENUM_0_VALUE_2;
            testInterfaceProxy.setAttributeEnumeration(enumerationArg);

            result = testInterfaceProxy.getAttributeEnumeration();
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (result != Enumeration.ENUM_0_VALUE_2) {
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
    public void callSetAttributeEnumeration() {
        LOG.info(name.getMethodName() + "");
        try {
            Enumeration enumerationArg = Enumeration.ENUM_0_VALUE_2;
            testInterfaceProxy.setAttributeEnumeration(enumerationArg);
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callGetAttributeExtendedEnumerationReadonly() {
        LOG.info(name.getMethodName() + "");
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

        LOG.info(name.getMethodName() + " - OK");
    }

    // no setter for attributeExtendedEnumerationReadonly

    @Test
    public void callGetAttributeBaseStruct() {
        LOG.info(name.getMethodName() + "");
        try {
            BaseStruct result;

            // must set the value before it can be retrieved again
            BaseStruct baseStructArg = IltUtil.createBaseStruct();
            testInterfaceProxy.setAttributeBaseStruct(baseStructArg);

            result = testInterfaceProxy.getAttributeBaseStruct();
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (!IltUtil.checkBaseStruct(result)) {
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
    public void callSetAttributeBaseStruct() {
        LOG.info(name.getMethodName() + "");
        try {
            BaseStruct baseStructArg = IltUtil.createBaseStruct();
            testInterfaceProxy.setAttributeBaseStruct(baseStructArg);
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callGetAttributeExtendedExtendedBaseStruct() {
        LOG.info(name.getMethodName() + "");
        try {
            ExtendedExtendedBaseStruct result;

            // must set the value before it can be retrieved again
            ExtendedExtendedBaseStruct extendedExtendedBaseStructArg = IltUtil.createExtendedExtendedBaseStruct();
            testInterfaceProxy.setAttributeExtendedExtendedBaseStruct(extendedExtendedBaseStructArg);

            result = testInterfaceProxy.getAttributeExtendedExtendedBaseStruct();
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            if (!IltUtil.checkExtendedExtendedBaseStruct(result)) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callSetAttributeExtendedExtendedBaseStruct() {
        LOG.info(name.getMethodName() + "");
        try {
            ExtendedExtendedBaseStruct extendedExtendedBaseStructArg = IltUtil.createExtendedExtendedBaseStruct();
            testInterfaceProxy.setAttributeExtendedExtendedBaseStruct(extendedExtendedBaseStructArg);
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callSetAttributeMapStringString() {
        LOG.info(name.getMethodName() + "");
        try {
            MapStringString attributeMapStringStringArg = new MapStringString();
            attributeMapStringStringArg.put("keyString1", "valueString1");
            attributeMapStringStringArg.put("keyString2", "valueString2");
            attributeMapStringStringArg.put("keyString3", "valueString3");
            testInterfaceProxy.setAttributeMapStringString(attributeMapStringStringArg);
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }

        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callGetAttributeMapStringString() {
        LOG.info(name.getMethodName() + "");
        try {
            // must set the value before it can be retrieved again
            MapStringString attributeMapStringStringArg = new MapStringString();
            attributeMapStringStringArg.put("keyString1", "valueString1");
            attributeMapStringStringArg.put("keyString2", "valueString2");
            attributeMapStringStringArg.put("keyString3", "valueString3");
            testInterfaceProxy.setAttributeMapStringString(attributeMapStringStringArg);

            MapStringString result = testInterfaceProxy.getAttributeMapStringString();
            if (result == null) {
                fail(name.getMethodName() + " - FAILED - got no result");
                return;
            }
            MapStringString expected = new MapStringString();
            expected.put("keyString1", "valueString1");
            expected.put("keyString2", "valueString2");
            expected.put("keyString3", "valueString3");
            if (!result.equals(expected)) {
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
     * GETTER AND SETTER CALLS WITH EXCEPTION
     */

    @Test
    public void callGetAttributeWithExceptionFromGetter() {
        LOG.info(name.getMethodName() + "");
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

        LOG.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callSetAttributeWithExceptionFromSetter() {
        LOG.info(name.getMethodName() + "");
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

        LOG.info(name.getMethodName() + " - OK");
    }
}
