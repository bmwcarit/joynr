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
package io.joynr.test.interlanguage.jee;

import javax.ejb.Stateless;
import javax.inject.Inject;

import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.api.SubscriptionPublisher;
import io.joynr.provider.SubscriptionPublisherInjection;

import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;

import joynr.exceptions.ProviderRuntimeException;
import joynr.exceptions.ApplicationException;
import joynr.interlanguagetest.Enumeration;
import joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray;
import joynr.interlanguagetest.namedTypeCollection2.BaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.BaseStructWithoutElements;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedBaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedEnumerationWithPartlyDefinedValues;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedErrorEnumTc;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedBaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedEnumeration;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedInterfaceEnumerationInTypeCollection;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedStructOfPrimitives;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedTypeCollectionEnumerationInTypeCollection;
import joynr.interlanguagetest.namedTypeCollection2.MapStringString;

import joynr.interlanguagetest.TestInterfaceSync;
import joynr.interlanguagetest.TestInterfaceSubscriptionPublisher;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Sample implementation of the {@link TestInterfaceSyncSync} interface.
 */
@Stateless
@ServiceProvider(serviceInterface = TestInterfaceSync.class)
public class IltProviderBean implements TestInterfaceSync,
        SubscriptionPublisherInjection<TestInterfaceSubscriptionPublisher> {
    private Byte attributeUInt8;
    private Double attributeDouble;
    private Boolean attributeBooleanReadonly;
    private String attributeStringNoSubscriptions;
    private Byte attributeInt8readonlyNoSubscriptions;
    private String[] attributeArrayOfStringImplicit;
    private Enumeration attributeEnumeration;
    private ExtendedEnumerationWithPartlyDefinedValues attributeExtendedEnumerationReadonly;
    private BaseStruct attributeBaseStruct;
    private ExtendedExtendedBaseStruct attributeExtendedExtendedBaseStruct;
    private MapStringString attributeMapStringString;
    private Integer attributeFireAndForget;

    @SuppressWarnings("unused")
    private TestInterfaceSubscriptionPublisher testInterfaceSubscriptionPublisher;

    private static final Logger logger = LoggerFactory.getLogger(IltProviderBean.class);

    @Inject
    public IltProviderBean(@SubscriptionPublisher TestInterfaceSubscriptionPublisher testInterfaceSubscriptionPublisher) {
        attributeFireAndForget = 0;
        this.testInterfaceSubscriptionPublisher = testInterfaceSubscriptionPublisher;
    }

    @Override
    public Byte getAttributeUInt8() {
        return attributeUInt8;
    }

    @Override
    public void setAttributeUInt8(Byte attributeUInt8) {
        this.attributeUInt8 = attributeUInt8;
        // TODO
        //attributeUInt8Changed(attributeUInt8);
    }

    @Override
    public Double getAttributeDouble() {
        return attributeDouble;
    }

    @Override
    public void setAttributeDouble(Double attributeDouble) {
        this.attributeDouble = attributeDouble;
        // TODO
        //attributeDoubleChanged(attributeDouble);
    }

    @Override
    public Boolean getAttributeBooleanReadonly() {
        // since there is no setter, set non-default value here
        attributeBooleanReadonly = true;
        return attributeBooleanReadonly;
    }

    @Override
    public String getAttributeStringNoSubscriptions() {
        return attributeStringNoSubscriptions;
    }

    @Override
    public void setAttributeStringNoSubscriptions(String attributeStringNoSubscriptions) {
        this.attributeStringNoSubscriptions = attributeStringNoSubscriptions;
    }

    @Override
    public Byte getAttributeInt8readonlyNoSubscriptions() {
        attributeInt8readonlyNoSubscriptions = -128;
        return attributeInt8readonlyNoSubscriptions;
    }

    @Override
    public String[] getAttributeArrayOfStringImplicit() {
        return attributeArrayOfStringImplicit;
    }

    @Override
    public void setAttributeArrayOfStringImplicit(String[] attributeArrayOfStringImplicit) {
        this.attributeArrayOfStringImplicit = attributeArrayOfStringImplicit;
        // TODO
        //attributeArrayOfStringImplicitChanged(attributeArrayOfStringImplicit);
    }

    @Override
    public Enumeration getAttributeEnumeration() {
        return attributeEnumeration;
    }

    @Override
    public void setAttributeEnumeration(Enumeration attributeEnumeration) {
        this.attributeEnumeration = attributeEnumeration;
        // TODO
        //attributeEnumerationChanged(attributeEnumeration);
    }

    @Override
    public ExtendedEnumerationWithPartlyDefinedValues getAttributeExtendedEnumerationReadonly() {
        // since there is no setter, hardcode a non-standard value here
        attributeExtendedEnumerationReadonly = ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
        return attributeExtendedEnumerationReadonly;
    }

    @Override
    public BaseStruct getAttributeBaseStruct() {
        return attributeBaseStruct;
    }

    @Override
    public void setAttributeBaseStruct(BaseStruct attributeBaseStruct) {
        this.attributeBaseStruct = attributeBaseStruct;
        // TODO
        //attributeBaseStructChanged(attributeBaseStruct);
    }

    @Override
    public ExtendedExtendedBaseStruct getAttributeExtendedExtendedBaseStruct() {
        return attributeExtendedExtendedBaseStruct;
    }

    @Override
    public void setAttributeExtendedExtendedBaseStruct(ExtendedExtendedBaseStruct attributeExtendedExtendedBaseStruct) {
        this.attributeExtendedExtendedBaseStruct = attributeExtendedExtendedBaseStruct;
        // TODO
        //attributeExtendedExtendedBaseStructChanged(attributeExtendedExtendedBaseStruct);
    }

    @Override
    public Boolean getAttributeWithExceptionFromGetter() {
        throw new ProviderRuntimeException("Exception from getAttributeWithExceptionFromGetter");
    }

    @Override
    public Boolean getAttributeWithExceptionFromSetter() {
        return false;
    }

    @Override
    public void setAttributeWithExceptionFromSetter(Boolean attributeWithExceptionFromSetter) {
        throw new ProviderRuntimeException("Exception from setAttributeWithExceptionFromSetter");
    }

    /*
     * methodWithoutParameters
     *
     * no output possible, but may reject instead
     */
    @Override
    public void methodWithoutParameters() {
        logger.warn("*******************************************");
        logger.warn("* IltProvider.methodWithoutParameters called");
        logger.warn("*******************************************");
    }

    /*
     * methodWithoutInputParameter
     *
     * return fixed output (true) or reject
     */
    @Override
    public Boolean methodWithoutInputParameter() {
        logger.warn("************************************************");
        logger.warn("* IltProvider.methodWithoutInputParameter called");
        logger.warn("************************************************");
        return true;
    }

    /*
     * methodWithoutOutputParameter
     *
     * can only resolve or reject since there is no output parameter
     */
    @Override
    public void methodWithoutOutputParameter(Boolean booleanArg) {
        logger.warn("*************************************************");
        logger.warn("* IltProvider.methodWithoutOutputParameter called");
        logger.warn("*************************************************");

        if (booleanArg) {
            logger.warn("methodWithoutOutputParameter: invalid argument booleanArg");
            throw new ProviderRuntimeException("methodWithoutOutputParameter: received wrong argument");
        }
    }

    /*
     * methodWithSinglePrimitiveParameters
     *
     * returns the integer parameter as string
     */
    @Override
    public String methodWithSinglePrimitiveParameters(Short uInt16Arg) {
        logger.warn("********************************************************");
        logger.warn("* IltProvider.methodWithSinglePrimitiveParameters called");
        logger.warn("********************************************************");

        // check input parameters
        // if (Short.toUnsignedInt(uInt16Arg) != 65535) {
        if (uInt16Arg != 32767) {
            logger.warn("methodWithSinglePrimitiveParameters: invalid argument uInt16Arg");
            throw new ProviderRuntimeException("methodWithSinglePrimitiveParameters: received wrong argument");
        }

        // send back the input converted to a string
        return new Integer(Short.toUnsignedInt(uInt16Arg)).toString();
    }

    /*
     * methodWithMultiplePrimitiveParameters
     *
     * the 'float' of France is delivered as Double here, just return it as 'double'
     * and return the integer argument as string
     */
    @Override
    public MethodWithMultiplePrimitiveParametersReturned methodWithMultiplePrimitiveParameters(Integer int32Arg,
                                                                                               Float floatArg,
                                                                                               Boolean booleanArg) {
        logger.warn("**********************************************************");
        logger.warn("* IltProvider.methodWithMultiplePrimitiveParameters called");
        logger.warn("**********************************************************");

        // check input parameters
        if (int32Arg != 2147483647 || !IltUtil.cmpFloat(floatArg, 47.11f) || booleanArg) {
            logger.warn("methodWithMultiplePrimitiveParameters: invalid argument int32Arg, floatArg or booleanArg");
            throw new ProviderRuntimeException("methodWithMultiplePrimitiveParameters: received wrong argument");
        }

        // prepare output parameters
        Double doubleOut = (double) floatArg;
        String stringOut = int32Arg.toString();
        return new MethodWithMultiplePrimitiveParametersReturned(doubleOut, stringOut);
    }

    /*
     * methodWithSingleArrayParameters
     *
     * Return an array with the stringified double entries
     */
    @Override
    public String[] methodWithSingleArrayParameters(Double[] doubleArrayArg) {
        logger.warn("****************************************************");
        logger.warn("* IltProvider.methodWithSingleArrayParameters called");
        logger.warn("****************************************************");

        // check input parameter
        if (!IltUtil.checkDoubleArray(doubleArrayArg)) {
            logger.warn("methodWithMultiplePrimitiveParameters: invalid argument doubleArrayArg");
            throw new ProviderRuntimeException("methodWithSingleArrayParameters: received wrong argument");
        }

        // prepare output parameter
        String[] stringArrayOut = IltUtil.createStringArray();
        return stringArrayOut;
    }

    /*
     * methodWithMultipleArrayParameters
     *
     * return the byte array as int64array
     * return the string list as list of string arrays with 1 element each, where this element
     * refers to the one from input
     */
    @Override
    public MethodWithMultipleArrayParametersReturned methodWithMultipleArrayParameters(String[] stringArrayArg,
                                                                                       Byte[] int8ArrayArg,
                                                                                       ExtendedInterfaceEnumerationInTypeCollection[] enumArrayArg,
                                                                                       StructWithStringArray[] structWithStringArrayArrayArg) {
        logger.warn("******************************************************");
        logger.warn("* IltProvider.methodWithMultipleArrayParameters called");
        logger.warn("******************************************************");

        if (!IltUtil.checkStringArray(stringArrayArg)) {
            throw new ProviderRuntimeException("methodWithMultipleArrayParameters: invalid stringArrayArg");
        }

        if (!IltUtil.checkByteArray(int8ArrayArg)) {
            throw new ProviderRuntimeException("methodWithMultipleArrayParameters: invalid int8ArrayArg");
        }

        if (!IltUtil.checkExtendedInterfaceEnumerationInTypeCollectionArray(enumArrayArg)) {
            throw new ProviderRuntimeException("methodWithMultipleArrayParameters: invalid enumArrayArg");
        }

        if (!IltUtil.checkStructWithStringArrayArray(structWithStringArrayArrayArg)) {
            throw new ProviderRuntimeException("methodWithMultipleArrayParameters: invalid structWithStringArrayArrayArg");
        }

        Long[] uInt64ArrayOut = IltUtil.createUInt64Array();
        StructWithStringArray[] structWithStringArrayArrayOut = new StructWithStringArray[2];
        structWithStringArrayArrayOut[0] = IltUtil.createStructWithStringArray();
        structWithStringArrayArrayOut[1] = IltUtil.createStructWithStringArray();

        return new MethodWithMultipleArrayParametersReturned(uInt64ArrayOut, structWithStringArrayArrayOut);
    }

    /*
     * methodWithSingleEnumParameters
     *
     * return fixed value for now
     */
    @Override
    public ExtendedTypeCollectionEnumerationInTypeCollection methodWithSingleEnumParameters(ExtendedEnumerationWithPartlyDefinedValues enumerationArg) {
        logger.warn("***************************************************");
        logger.warn("* IltProvider.methodWithSingleEnumParameters called");
        logger.warn("***************************************************");

        // check input parameter
        if (enumerationArg != ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES) {
            logger.warn("methodWithSingleEnumParameters: invalid argument enumerationArg");
            throw new ProviderRuntimeException("methodWithSingleEnumParameters: received wrong argument");
        }

        // prepare output parameter
        ExtendedTypeCollectionEnumerationInTypeCollection enumerationOut = ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
        return enumerationOut;
    }

    /*
     * methodWithMultipleEnumParameters
     *
     * return fixed values for now
     */
    @Override
    public MethodWithMultipleEnumParametersReturned methodWithMultipleEnumParameters(Enumeration enumerationArg,
                                                                                     ExtendedTypeCollectionEnumerationInTypeCollection extendedEnumerationArg) {
        logger.warn("*****************************************************");
        logger.warn("* IltProvider.methodWithMultipleEnumParameters called");
        logger.warn("*****************************************************");

        // check input parameters
        if (enumerationArg != joynr.interlanguagetest.Enumeration.ENUM_0_VALUE_3
                || extendedEnumerationArg != ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
            logger.warn("methodWithMultipleEnumParameters: invalid argument enumerationArg or extendedEnumerationArg");
            throw new ProviderRuntimeException("methodWithMultipleEnumParameters: received wrong argument");
        }

        // prepare output parameters
        ExtendedEnumerationWithPartlyDefinedValues extendedEnumerationOut = ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
        Enumeration enumerationOut = Enumeration.ENUM_0_VALUE_1;
        return new MethodWithMultipleEnumParametersReturned(extendedEnumerationOut, enumerationOut);
    }

    /*
     * methodWithSingleStructParameters
     */
    @Override
    public ExtendedStructOfPrimitives methodWithSingleStructParameters(ExtendedBaseStruct extendedBaseStructArg) {
        logger.warn("*****************************************************");
        logger.warn("* IltProvider.methodWithSingleStructParameters called");
        logger.warn("*****************************************************");

        if (!IltUtil.checkExtendedBaseStruct(extendedBaseStructArg)) {
            logger.error("methodWithSingleStructParameters: invalid parameter extendedBaseStructArg");
            throw new ProviderRuntimeException("methodWithSingleStructParameters: invalid parameter extendedBaseStructArg");
        }

        // prepare output parameters
        ExtendedStructOfPrimitives extendedStructOfPrimitivesOut = IltUtil.createExtendedStructOfPrimitives();
        return extendedStructOfPrimitivesOut;
    }

    /*
     * methodWithMultipleStructParameters
     */
    @Override
    public MethodWithMultipleStructParametersReturned methodWithMultipleStructParameters(ExtendedStructOfPrimitives extendedStructOfPrimitivesArg,
                                                                                         BaseStruct baseStructArg) {
        logger.warn("*******************************************************");
        logger.warn("* IltProvider.methodWithMultipleStructParameters called");
        logger.warn("*******************************************************");

        // check input parameter
        if (!IltUtil.checkExtendedStructOfPrimitives(extendedStructOfPrimitivesArg)) {
            throw new ProviderRuntimeException("methodWithMultipleStructParameters: invalid parameter extendedStructOfPrimitivesArg");
        }

        if (!IltUtil.checkBaseStruct(baseStructArg)) {
            throw new ProviderRuntimeException("methodWithMultipleStructParameters: invalid parameter baseStructArg");
        }

        // set output values
        BaseStructWithoutElements baseStructWithoutElementsOut = IltUtil.createBaseStructWithoutElements();
        ExtendedExtendedBaseStruct extendedExtendedBaseStructOut = IltUtil.createExtendedExtendedBaseStruct();

        return new MethodWithMultipleStructParametersReturned(baseStructWithoutElementsOut,
                                                              extendedExtendedBaseStructOut);
    }

    /*
     * overloadedMethod (1)
     */
    @Override
    public String overloadedMethod() {
        logger.warn("*****************************************");
        logger.warn("* IltProvider.overloadedMethod called (1)");
        logger.warn("*****************************************");
        String stringOut = "TestString 1";
        return stringOut;
    }

    /*
     * overloadedMethod (2)
     */
    @Override
    public String overloadedMethod(Boolean booleanArg) {
        logger.warn("*****************************************");
        logger.warn("* IltProvider.overloadedMethod called (2)");
        logger.warn("*****************************************");
        if (booleanArg) {
            logger.warn("overloadedMethod_2: invalid argument booleanArg");
            throw new ProviderRuntimeException("overloadedMethod_2: invalid parameter baseStructArg");
        }
        String stringOut = "TestString 2";
        return stringOut;
    }

    /*
     * overloadedMethod (3)
     */
    @Override
    public OverloadedMethodOverloadedMethod1Returned overloadedMethod(ExtendedExtendedEnumeration[] enumArrayArg,
                                                                      Long int64Arg,
                                                                      BaseStruct baseStructArg,
                                                                      Boolean booleanArg) {
        logger.warn("*****************************************");
        logger.warn("* IltProvider.overloadedMethod called (3)");
        logger.warn("*****************************************");

        // check input parameter
        if (int64Arg != 1L || booleanArg) {
            logger.warn("overloadedMethod_3: invalid argument int64Arg or booleanArg");
            throw new ProviderRuntimeException("overloadedMethod_3: invalid parameter int64Arg or booleanArg");
        }

        // check enumArrayArg
        if (!IltUtil.checkExtendedExtendedEnumerationArray(enumArrayArg)) {
            throw new ProviderRuntimeException("overloadedMethod_3: invalid parameter enumArrayArg");
        }

        // check baseStructArg
        if (!IltUtil.checkBaseStruct(baseStructArg)) {
            logger.warn("overloadedMethod_3: invalid argument baseStructArg");
            throw new ProviderRuntimeException("overloadedMethod_3: invalid parameter baseStructArg");
        }

        // setup output parameter
        Double doubleOut = 0d;
        String[] stringArrayOut = IltUtil.createStringArray();
        ExtendedBaseStruct extendedBaseStructOut = IltUtil.createExtendedBaseStruct();

        return new OverloadedMethodOverloadedMethod1Returned(doubleOut, stringArrayOut, extendedBaseStructOut);
    }

    /*
     * overloadedMethodWithSelector (1)
     */
    @Override
    public String overloadedMethodWithSelector() {
        logger.warn("*************************************************");
        logger.warn("* IltProvider.overloadedMethodWithSelector called");
        logger.warn("*************************************************");
        String stringOut = "Return value from overloadedMethodWithSelector 1";
        return stringOut;
    }

    /*
     * overloadedMethodWithSelector (2)
     */
    @Override
    public String overloadedMethodWithSelector(Boolean booleanArg) {
        logger.warn("*************************************************");
        logger.warn("* IltProvider.overloadedMethodWithSelector called");
        logger.warn("*************************************************");

        // check input parameter
        if (booleanArg) {
            logger.warn("overloadedMethodWithSelector: invalid argument booleanArg");
            throw new ProviderRuntimeException("overloadedMethodWithSelector: invalid parameter booleanArg");
        }

        // setup output parameter
        String stringOut = "Return value from overloadedMethodWithSelector 2";
        return stringOut;
    }

    /*
     * overloadedMethodWithSelector (3)
     */
    @Override
    public OverloadedMethodWithSelectorOverloadedMethodWithSelector1Returned overloadedMethodWithSelector(ExtendedExtendedEnumeration[] enumArrayArg,
                                                                                                          Long int64Arg,
                                                                                                          BaseStruct baseStructArg,
                                                                                                          Boolean booleanArg) {
        logger.warn("*************************************************");
        logger.warn("* IltProvider.overloadedMethodWithSelector called");
        logger.warn("*************************************************");

        /* check input */
        if (!IltUtil.checkExtendedExtendedEnumerationArray(enumArrayArg)) {
            throw new ProviderRuntimeException("overloadedMethodWithSelector: failed to compare enumArrayArg");
        }

        if (int64Arg != 1L) {
            throw new ProviderRuntimeException("overloadedMethodWithSelector: failed to compare int64Arg");
        }

        if (!IltUtil.checkBaseStruct(baseStructArg)) {
            throw new ProviderRuntimeException("overloadedMethodWithSelector: failed to compare baseStructArg");
        }

        if (booleanArg) {
            throw new ProviderRuntimeException("overloadedMethodWithSelector: failed to compare booleanArg");
        }

        /* prepare output */
        Double doubleOut = 1.1d;

        String[] stringArrayOut = IltUtil.createStringArray();
        ExtendedBaseStruct extendedBaseStructOut = IltUtil.createExtendedBaseStruct();

        return new OverloadedMethodWithSelectorOverloadedMethodWithSelector1Returned(doubleOut,
                                                                                     stringArrayOut,
                                                                                     extendedBaseStructOut);
    }

    /*
     * methodWithStringsAndSpecifiedStringOutLength
     */
    @Override
    public String methodWithStringsAndSpecifiedStringOutLength(String stringArg, Integer int32StringLengthArg) {
        logger.warn("*****************************************************************");
        logger.warn("* IltProvider.methodWithStringsAndSpecifiedStringOutLength called");
        logger.warn("*****************************************************************");
        StringBuilder s = new StringBuilder();
        if (int32StringLengthArg > 1024 * 1024) {
            throw new ProviderRuntimeException("methodWithStringsAndSpecifiedStringOutLength: Maximum length exceeded");
        }
        for (int i = 0; i < int32StringLengthArg; i++) {
            s.append("A");
        }
        return s.toString();
    }

    /*
     * methodWithoutErrorEnum
     */
    @Override
    public void methodWithoutErrorEnum(String wantedExceptionArg) {
        logger.warn("*******************************************");
        logger.warn("* IltProvider.methodWithoutErrorEnum called");
        logger.warn("*******************************************");

        if (wantedExceptionArg.equals("ProviderRuntimeException")) {
            throw new ProviderRuntimeException("Exception from methodWithoutErrorEnum");
        }
    }

    /*
     * methodWithAnonymousErrorEnum
     */
    @Override
    public void methodWithAnonymousErrorEnum(String wantedExceptionArg) throws ApplicationException {
        logger.warn("*************************************************");
        logger.warn("* IltProvider.methodWithAnonymousErrorEnum called");
        logger.warn("*************************************************");

        if (wantedExceptionArg.equals("ProviderRuntimeException")) {
            throw new ProviderRuntimeException("Exception from methodWithAnonymousErrorEnum");
        } else if (wantedExceptionArg.equals("ApplicationException")) {
            throw new ApplicationException(MethodWithAnonymousErrorEnumErrorEnum.ERROR_3_1_NTC);
        }
    }

    /*
     * methodWithExistingErrorEnum
     */
    @Override
    public void methodWithExistingErrorEnum(String wantedExceptionArg) throws ApplicationException {
        logger.warn("************************************************");
        logger.warn("* IltProvider.methodWithExistingErrorEnum called");
        logger.warn("************************************************");

        if (wantedExceptionArg.equals("ProviderRuntimeException")) {
            throw new ProviderRuntimeException("Exception from methodWithExistingErrorEnum");
        } else if (wantedExceptionArg.equals("ApplicationException_1")) {
            throw new ApplicationException(ExtendedErrorEnumTc.ERROR_2_3_TC2);
        } else if (wantedExceptionArg.equals("ApplicationException_2")) {
            throw new ApplicationException(ExtendedErrorEnumTc.ERROR_1_2_TC_2);
        }
    }

    /*
     * methodWithExtendedErrorEnum
     */
    @Override
    public void methodWithExtendedErrorEnum(String wantedExceptionArg) throws ApplicationException {
        logger.warn("************************************************");
        logger.warn("* IltProvider.methodWithExtendedErrorEnum called");
        logger.warn("************************************************");

        if (wantedExceptionArg.equals("ProviderRuntimeException")) {
            throw new ProviderRuntimeException("Exception from methodWithExtendedErrorEnum");
        } else if (wantedExceptionArg.equals("ApplicationException_1")) {
            throw new ApplicationException(MethodWithExtendedErrorEnumErrorEnum.ERROR_3_3_NTC);
        } else if (wantedExceptionArg.equals("ApplicationException_2")) {
            throw new ApplicationException(MethodWithExtendedErrorEnumErrorEnum.ERROR_2_1_TC2);
        }
    }

    /*
     * methodToFireBroadcastWithSinglePrimitiveParameter
     */
    @Override
    public void methodToFireBroadcastWithSinglePrimitiveParameter() {
        logger.warn("**********************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithSinglePrimitiveParameter called");
        logger.warn("**********************************************************************");

        String stringOut = "boom";
        // TODO
        //fireBroadcastWithSinglePrimitiveParameter(stringOut);
    }

    /*
     * methodToFireBroadcastWithMultiplePrimitiveParameters
     */
    @Override
    public void methodToFireBroadcastWithMultiplePrimitiveParameters() {
        logger.warn("*************************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithMultiplePrimitiveParameters called");
        logger.warn("*************************************************************************");

        Double doubleOut = 1.1d;
        String stringOut = "boom";
        // TODO
        //fireBroadcastWithMultiplePrimitiveParameters(doubleOut, stringOut);
    }

    /*
     * methodToFireBroadcastWithSingleArrayParameter
     */
    @Override
    public void methodToFireBroadcastWithSingleArrayParameter() {
        logger.warn("******************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithSingleArrayParameter called");
        logger.warn("******************************************************************");

        String[] stringArrayOut = IltUtil.createStringArray();
        // TODO
        //fireBroadcastWithSingleArrayParameter(stringArrayOut);
    }

    /*
     * methodToFireBroadcastWithMultipleArrayParameters
     */
    @Override
    public void methodToFireBroadcastWithMultipleArrayParameters() {
        logger.warn("*********************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithMultipleArrayParameters called");
        logger.warn("*********************************************************************");

        Long[] uInt64ArrayOut = IltUtil.createUInt64Array();
        StructWithStringArray[] structWithStringArrayArrayOut = IltUtil.createStructWithStringArrayArray();
        // TODO
        //fireBroadcastWithMultipleArrayParameters(uInt64ArrayOut, structWithStringArrayArrayOut);
    }

    /*
     * methodToFireBroadcastWithSingleEnumerationParameter
     */
    @Override
    public void methodToFireBroadcastWithSingleEnumerationParameter() {
        logger.warn("************************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithSingleEnumerationParameter called");
        logger.warn("************************************************************************");

        ExtendedTypeCollectionEnumerationInTypeCollection enumerationOut = ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
        // TODO
        //fireBroadcastWithSingleEnumerationParameter(enumerationOut);
    }

    /*
     * methodToFireBroadcastWithMultipleEnumerationParameters
     */
    @Override
    public void methodToFireBroadcastWithMultipleEnumerationParameters() {
        logger.warn("***************************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithMultipleEnumerationParameters called");
        logger.warn("***************************************************************************");

        ExtendedEnumerationWithPartlyDefinedValues extendedEnumerationOut = ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
        Enumeration enumerationOut = Enumeration.ENUM_0_VALUE_1;
        // TODO
        //fireBroadcastWithMultipleEnumerationParameters(extendedEnumerationOut, enumerationOut);
    }

    /*
     * methodToFireBroadcastWithSingleStructParameter
     */
    @Override
    public void methodToFireBroadcastWithSingleStructParameter() {
        logger.warn("*******************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithSingleStructParameter called");
        logger.warn("*******************************************************************");

        ExtendedStructOfPrimitives extendedStructOfPrimitivesOut = IltUtil.createExtendedStructOfPrimitives();
        // TODO
        //fireBroadcastWithSingleStructParameter(extendedStructOfPrimitivesOut);
    }

    /*
     * methodToFireBroadcastWithMultipleStructParameters
     */
    @Override
    public void methodToFireBroadcastWithMultipleStructParameters() {
        logger.warn("**********************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithMultipleStructParameters called");
        logger.warn("**********************************************************************");

        BaseStructWithoutElements baseStructWithoutElementsOut = IltUtil.createBaseStructWithoutElements();
        ExtendedExtendedBaseStruct extendedExtendedBaseStructOut = IltUtil.createExtendedExtendedBaseStruct();
        // TODO
        //fireBroadcastWithMultipleStructParameters(baseStructWithoutElementsOut, extendedExtendedBaseStructOut);
    }

    /*
     * methodToFireBroadcastWithFiltering
     */
    @Override
    public void methodToFireBroadcastWithFiltering(String stringArg) {
        logger.warn("*******************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithFiltering called");
        logger.warn("*******************************************************");

        // take the stringArg as input for the filtering
        String stringOut = stringArg;
        String[] stringArrayOut = IltUtil.createStringArray();
        ExtendedTypeCollectionEnumerationInTypeCollection enumerationOut = ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
        StructWithStringArray structWithStringArrayOut = IltUtil.createStructWithStringArray();
        StructWithStringArray[] structWithStringArrayArrayOut = IltUtil.createStructWithStringArrayArray();

        // TODO
        //fireBroadcastWithFiltering(stringOut,
        //                           stringArrayOut,
        //                           enumerationOut,
        //                           structWithStringArrayOut,
        //                           structWithStringArrayArrayOut);
    }

    @Override
    public MapStringString getAttributeMapStringString() {
        return attributeMapStringString;
    }

    @Override
    public void setAttributeMapStringString(MapStringString attributeMapStringString) {
        this.attributeMapStringString = attributeMapStringString;
        // TODO
        //attributeMapStringStringChanged(attributeMapStringString);
    }

    @Override
    public MapStringString methodWithSingleMapParameters(MapStringString mapArg) {
        if (mapArg == null) {
            return null;
        }
        MapStringString mapOut = new MapStringString();
        Iterator<Map.Entry<String, String>> iterator = mapArg.entrySet().iterator();
        while (iterator.hasNext()) {
            Map.Entry<String, String> entry = (Entry<String, String>) iterator.next();
            mapOut.put(entry.getValue(), entry.getKey());
        }
        return mapOut;
    }

    @Override
    public Integer getAttributeFireAndForget() {
        return attributeFireAndForget;
    }

    @Override
    public void setAttributeFireAndForget(Integer attributeFireAndForget) {
        this.attributeFireAndForget = attributeFireAndForget;
        // TODO
        //attributeFireAndForgetChanged(attributeFireAndForget);
    }

    @Override
    public void methodFireAndForgetWithoutParameter() {
        setAttributeFireAndForget(attributeFireAndForget + 1);
    }

    @Override
    public void methodFireAndForgetWithInputParameter(Integer int32Arg) {
        setAttributeFireAndForget(int32Arg);
    }

    @Override
    public void setSubscriptionPublisher(TestInterfaceSubscriptionPublisher subscriptionPublisher) {
        // Not supported by JEE integration yet, but necessary to have this method
        // so that the application can be deployed, because the radio.fidl has
        // subscription functionality in it.
    }
}
