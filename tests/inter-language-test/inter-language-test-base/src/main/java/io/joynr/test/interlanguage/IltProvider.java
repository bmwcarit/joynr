/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2018 BMW Car IT GmbH
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

import java.util.Arrays;
import java.util.Iterator;
import java.util.Map.Entry;
import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import joynr.exceptions.ProviderRuntimeException;
import joynr.interlanguagetest.Enumeration;
import joynr.interlanguagetest.TestInterface.MethodWithAnonymousErrorEnumErrorEnum;
import joynr.interlanguagetest.TestInterface.MethodWithExtendedErrorEnumErrorEnum;
import joynr.interlanguagetest.TestInterfaceAbstractProvider;
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
import joynr.interlanguagetest.typeDefCollection.ArrayTypeDefStruct;

public class IltProvider extends TestInterfaceAbstractProvider {
    protected Byte attributeUInt8;
    protected Double attributeDouble;
    protected Boolean attributeBooleanReadonly;
    protected String attributeStringNoSubscriptions;
    protected Byte attributeInt8readonlyNoSubscriptions;
    protected String[] attributeArrayOfStringImplicit;
    protected Byte[] attributeByteBuffer;
    protected Enumeration attributeEnumeration;
    protected ExtendedEnumerationWithPartlyDefinedValues attributeExtendedEnumerationReadonly;
    protected BaseStruct attributeBaseStruct;
    protected ExtendedExtendedBaseStruct attributeExtendedExtendedBaseStruct;
    protected MapStringString attributeMapStringString;
    protected Integer attributeFireAndForget;
    protected Long attributeInt64TypeDef;
    protected String attributeStringTypeDef;
    protected BaseStruct attributeStructTypeDef;
    protected MapStringString attributeMapTypeDef;
    protected Enumeration attributeEnumTypeDef;
    protected Byte[] attributeByteBufferTypeDef;
    protected ArrayTypeDefStruct attributeArrayTypeDef;

    private static final Logger logger = LoggerFactory.getLogger(IltProvider.class);

    public IltProvider() {
        attributeFireAndForget = 0;
    }

    @Override
    public Promise<Deferred<Byte>> getAttributeUInt8() {
        Deferred<Byte> deferred = new Deferred<Byte>();
        deferred.resolve(attributeUInt8);
        return new Promise<Deferred<Byte>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeUInt8(Byte attributeUInt8) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeUInt8 = attributeUInt8;
        attributeUInt8Changed(attributeUInt8);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<Double>> getAttributeDouble() {
        Deferred<Double> deferred = new Deferred<Double>();
        deferred.resolve(attributeDouble);
        return new Promise<Deferred<Double>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeDouble(Double attributeDouble) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeDouble = attributeDouble;
        attributeDoubleChanged(attributeDouble);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<Boolean>> getAttributeBooleanReadonly() {
        Deferred<Boolean> deferred = new Deferred<Boolean>();
        // since there is no setter, set non-default value here
        attributeBooleanReadonly = true;
        deferred.resolve(attributeBooleanReadonly);
        return new Promise<Deferred<Boolean>>(deferred);
    }

    @Override
    public Promise<Deferred<String>> getAttributeStringNoSubscriptions() {
        Deferred<String> deferred = new Deferred<String>();
        deferred.resolve(attributeStringNoSubscriptions);
        return new Promise<Deferred<String>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeStringNoSubscriptions(String attributeStringNoSubscriptions) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeStringNoSubscriptions = attributeStringNoSubscriptions;
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<Byte>> getAttributeInt8readonlyNoSubscriptions() {
        Deferred<Byte> deferred = new Deferred<Byte>();
        attributeInt8readonlyNoSubscriptions = -128;
        deferred.resolve(attributeInt8readonlyNoSubscriptions);
        return new Promise<Deferred<Byte>>(deferred);
    }

    @Override
    public Promise<Deferred<String[]>> getAttributeArrayOfStringImplicit() {
        Deferred<String[]> deferred = new Deferred<String[]>();
        deferred.resolve(attributeArrayOfStringImplicit);
        return new Promise<Deferred<String[]>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeArrayOfStringImplicit(String[] attributeArrayOfStringImplicit) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeArrayOfStringImplicit = attributeArrayOfStringImplicit == null ? null
                : attributeArrayOfStringImplicit.clone();
        attributeArrayOfStringImplicitChanged(attributeArrayOfStringImplicit);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<Byte[]>> getAttributeByteBuffer() {
        Deferred<Byte[]> deferred = new Deferred<Byte[]>();
        deferred.resolve(attributeByteBuffer);
        return new Promise<Deferred<Byte[]>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeByteBuffer(Byte[] attributeByteBuffer) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeByteBuffer = attributeByteBuffer == null ? null : attributeByteBuffer.clone();
        attributeByteBufferChanged(attributeByteBuffer);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<Enumeration>> getAttributeEnumeration() {
        Deferred<Enumeration> deferred = new Deferred<Enumeration>();
        deferred.resolve(attributeEnumeration);
        return new Promise<Deferred<Enumeration>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeEnumeration(Enumeration attributeEnumeration) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeEnumeration = attributeEnumeration;
        attributeEnumerationChanged(attributeEnumeration);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<ExtendedEnumerationWithPartlyDefinedValues>> getAttributeExtendedEnumerationReadonly() {
        Deferred<ExtendedEnumerationWithPartlyDefinedValues> deferred = new Deferred<ExtendedEnumerationWithPartlyDefinedValues>();
        // since there is no setter, hardcode a non-standard value here
        attributeExtendedEnumerationReadonly = ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
        deferred.resolve(attributeExtendedEnumerationReadonly);
        return new Promise<Deferred<ExtendedEnumerationWithPartlyDefinedValues>>(deferred);
    }

    @Override
    public Promise<Deferred<BaseStruct>> getAttributeBaseStruct() {
        Deferred<BaseStruct> deferred = new Deferred<BaseStruct>();
        deferred.resolve(attributeBaseStruct);
        return new Promise<Deferred<BaseStruct>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeBaseStruct(BaseStruct attributeBaseStruct) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeBaseStruct = (attributeBaseStruct != null) ? new BaseStruct(attributeBaseStruct) : null;
        attributeBaseStructChanged(attributeBaseStruct);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<ExtendedExtendedBaseStruct>> getAttributeExtendedExtendedBaseStruct() {
        Deferred<ExtendedExtendedBaseStruct> deferred = new Deferred<ExtendedExtendedBaseStruct>();
        deferred.resolve(attributeExtendedExtendedBaseStruct);
        return new Promise<Deferred<ExtendedExtendedBaseStruct>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeExtendedExtendedBaseStruct(ExtendedExtendedBaseStruct attributeExtendedExtendedBaseStruct) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeExtendedExtendedBaseStruct = (attributeExtendedExtendedBaseStruct != null)
                ? new ExtendedExtendedBaseStruct(attributeExtendedExtendedBaseStruct)
                : null;
        attributeExtendedExtendedBaseStructChanged(attributeExtendedExtendedBaseStruct);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<Boolean>> getAttributeWithExceptionFromGetter() {
        Deferred<Boolean> deferred = new Deferred<Boolean>();
        deferred.reject(new ProviderRuntimeException("Exception from getAttributeWithExceptionFromGetter"));
        return new Promise<Deferred<Boolean>>(deferred);
    }

    @Override
    public Promise<Deferred<Boolean>> getAttributeWithExceptionFromSetter() {
        Deferred<Boolean> deferred = new Deferred<Boolean>();
        deferred.resolve(false);
        return new Promise<Deferred<Boolean>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeWithExceptionFromSetter(Boolean attributeWithExceptionFromSetter) {
        DeferredVoid deferred = new DeferredVoid();
        deferred.reject(new ProviderRuntimeException("Exception from setAttributeWithExceptionFromSetter"));
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<Long>> getAttributeInt64TypeDef() {
        Deferred<Long> deferred = new Deferred<Long>();
        deferred.resolve(attributeInt64TypeDef);
        return new Promise<Deferred<Long>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeInt64TypeDef(Long attributeInt64TypeDef) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeInt64TypeDef = attributeInt64TypeDef;
        attributeInt64TypeDefChanged(attributeInt64TypeDef);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<String>> getAttributeStringTypeDef() {
        Deferred<String> deferred = new Deferred<String>();
        deferred.resolve(attributeStringTypeDef);
        return new Promise<Deferred<String>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeStringTypeDef(String attributeStringTypeDef) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeStringTypeDef = attributeStringTypeDef;
        attributeStringTypeDefChanged(attributeStringTypeDef);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<BaseStruct>> getAttributeStructTypeDef() {
        Deferred<BaseStruct> deferred = new Deferred<BaseStruct>();
        deferred.resolve(attributeStructTypeDef);
        return new Promise<Deferred<BaseStruct>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeStructTypeDef(BaseStruct attributeStructTypeDef) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeStructTypeDef = (attributeStructTypeDef != null) ? new BaseStruct(attributeStructTypeDef) : null;
        attributeStructTypeDefChanged(attributeStructTypeDef);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<MapStringString>> getAttributeMapTypeDef() {
        Deferred<MapStringString> deferred = new Deferred<MapStringString>();
        deferred.resolve(attributeMapTypeDef);
        return new Promise<Deferred<MapStringString>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeMapTypeDef(MapStringString attributeMapTypeDef) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeMapTypeDef = (attributeMapTypeDef != null) ? new MapStringString(attributeMapTypeDef) : null;
        attributeMapTypeDefChanged(attributeMapTypeDef);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<Enumeration>> getAttributeEnumTypeDef() {
        Deferred<Enumeration> deferred = new Deferred<Enumeration>();
        deferred.resolve(attributeEnumTypeDef);
        return new Promise<Deferred<Enumeration>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeEnumTypeDef(Enumeration attributeEnumTypeDef) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeEnumTypeDef = attributeEnumTypeDef;
        attributeEnumTypeDefChanged(attributeEnumTypeDef);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<Byte[]>> getAttributeByteBufferTypeDef() {
        Deferred<Byte[]> deferred = new Deferred<Byte[]>();
        deferred.resolve(attributeByteBufferTypeDef);
        return new Promise<Deferred<Byte[]>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeByteBufferTypeDef(Byte[] attributeByteBufferTypeDef) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeByteBufferTypeDef = attributeByteBufferTypeDef == null ? null
                : attributeByteBufferTypeDef.clone();
        attributeByteBufferTypeDefChanged(attributeByteBufferTypeDef);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<ArrayTypeDefStruct>> getAttributeArrayTypeDef() {
        Deferred<ArrayTypeDefStruct> deferred = new Deferred<ArrayTypeDefStruct>();
        deferred.resolve(attributeArrayTypeDef);
        return new Promise<Deferred<ArrayTypeDefStruct>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeArrayTypeDef(ArrayTypeDefStruct attributeArrayTypeDef) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeArrayTypeDef = (attributeArrayTypeDef != null) ? new ArrayTypeDefStruct(attributeArrayTypeDef)
                : null;
        attributeArrayTypeDefChanged(attributeArrayTypeDef);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodWithoutParameters
     *
     * no output possible, but may reject instead
     */
    @Override
    public Promise<DeferredVoid> methodWithoutParameters() {
        logger.warn("*******************************************");
        logger.warn("* IltProvider.methodWithoutParameters called");
        logger.warn("*******************************************");
        DeferredVoid deferred = new DeferredVoid();
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodWithoutInputParameter
     *
     * return fixed output (true) or reject
     */
    @Override
    public Promise<MethodWithoutInputParameterDeferred> methodWithoutInputParameter() {
        logger.warn("************************************************");
        logger.warn("* IltProvider.methodWithoutInputParameter called");
        logger.warn("************************************************");
        MethodWithoutInputParameterDeferred deferred = new MethodWithoutInputParameterDeferred();
        deferred.resolve(true);
        return new Promise<MethodWithoutInputParameterDeferred>(deferred);
    }

    /*
     * methodWithoutOutputParameter
     *
     * can only resolve or reject since there is no output parameter
     */
    @Override
    public Promise<DeferredVoid> methodWithoutOutputParameter(Boolean booleanArg) {
        logger.warn("*************************************************");
        logger.warn("* IltProvider.methodWithoutOutputParameter called");
        logger.warn("*************************************************");
        DeferredVoid deferred = new DeferredVoid();

        if (booleanArg != false) {
            logger.warn("methodWithoutOutputParameter: invalid argument booleanArg");
            deferred.reject(new ProviderRuntimeException("methodWithoutOutputParameter: received wrong argument"));
            return new Promise<DeferredVoid>(deferred);
        }
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodWithSinglePrimitiveParameters
     *
     * returns the integer parameter as string
     */
    @Override
    public Promise<MethodWithSinglePrimitiveParametersDeferred> methodWithSinglePrimitiveParameters(Short uInt16Arg) {
        logger.warn("********************************************************");
        logger.warn("* IltProvider.methodWithSinglePrimitiveParameters called");
        logger.warn("********************************************************");
        MethodWithSinglePrimitiveParametersDeferred deferred = new MethodWithSinglePrimitiveParametersDeferred();

        // check input parameters
        // if (Short.toUnsignedInt(uInt16Arg) != 65535) {
        if (uInt16Arg != 32767) {
            logger.warn("methodWithSinglePrimitiveParameters: invalid argument uInt16Arg");
            deferred.reject(new ProviderRuntimeException("methodWithSinglePrimitiveParameters: received wrong argument"));
            return new Promise<MethodWithSinglePrimitiveParametersDeferred>(deferred);
        }

        // send back the input converted to a string
        deferred.resolve(Integer.toString(Short.toUnsignedInt(uInt16Arg)));
        return new Promise<MethodWithSinglePrimitiveParametersDeferred>(deferred);
    }

    /*
     * methodWithMultiplePrimitiveParameters
     *
     * the 'float' of France is delivered as Double here, just return it as 'double'
     * and return the integer argument as string
     */
    @Override
    public Promise<MethodWithMultiplePrimitiveParametersDeferred> methodWithMultiplePrimitiveParameters(Integer int32Arg,
                                                                                                        Float floatArg,
                                                                                                        Boolean booleanArg) {
        logger.warn("**********************************************************");
        logger.warn("* IltProvider.methodWithMultiplePrimitiveParameters called");
        logger.warn("**********************************************************");
        MethodWithMultiplePrimitiveParametersDeferred deferred = new MethodWithMultiplePrimitiveParametersDeferred();

        // check input parameters
        if (int32Arg != 2147483647 || !IltUtil.cmpFloat(floatArg, 47.11f) || booleanArg != false) {
            logger.warn("methodWithMultiplePrimitiveParameters: invalid argument int32Arg, floatArg or booleanArg");
            deferred.reject(new ProviderRuntimeException("methodWithMultiplePrimitiveParameters: received wrong argument"));
            return new Promise<MethodWithMultiplePrimitiveParametersDeferred>(deferred);
        }

        // prepare output parameters
        Double doubleOut = (double) floatArg;
        String stringOut = int32Arg.toString();
        deferred.resolve(doubleOut, stringOut);
        return new Promise<MethodWithMultiplePrimitiveParametersDeferred>(deferred);
    }

    /*
     * methodWithSingleArrayParameters
     *
     * Return an array with the stringified double entries
     */
    @Override
    public Promise<MethodWithSingleArrayParametersDeferred> methodWithSingleArrayParameters(Double[] doubleArrayArg) {
        logger.warn("****************************************************");
        logger.warn("* IltProvider.methodWithSingleArrayParameters called");
        logger.warn("****************************************************");
        MethodWithSingleArrayParametersDeferred deferred = new MethodWithSingleArrayParametersDeferred();

        // check input parameter
        if (!IltUtil.checkDoubleArray(doubleArrayArg)) {
            logger.warn("methodWithMultiplePrimitiveParameters: invalid argument doubleArrayArg");
            deferred.reject(new ProviderRuntimeException("methodWithSingleArrayParameters: received wrong argument"));
            return new Promise<MethodWithSingleArrayParametersDeferred>(deferred);
        }

        // prepare output parameter
        String[] stringArrayOut = { "Hello", "World" };
        deferred.resolve(stringArrayOut);
        return new Promise<MethodWithSingleArrayParametersDeferred>(deferred);
    }

    /*
     * methodWithMultipleArrayParameters
     *
     * return the byte array as int64array
     * return the string list as list of string arrays with 1 element each, where this element
     * refers to the one from input
     */
    @Override
    public Promise<MethodWithMultipleArrayParametersDeferred> methodWithMultipleArrayParameters(String[] stringArrayArg,
                                                                                                Byte[] int8ArrayArg,
                                                                                                ExtendedInterfaceEnumerationInTypeCollection[] enumArrayArg,
                                                                                                StructWithStringArray[] structWithStringArrayArrayArg) {
        logger.warn("******************************************************");
        logger.warn("* IltProvider.methodWithMultipleArrayParameters called");
        logger.warn("******************************************************");
        MethodWithMultipleArrayParametersDeferred deferred = new MethodWithMultipleArrayParametersDeferred();

        String[] stringArray = { "Hello", "World" };
        if (!Arrays.equals(stringArray, stringArrayArg)) {
            deferred.reject(new ProviderRuntimeException("methodWithMultipleArrayParameters: invalid stringArrayArg"));
            return new Promise<MethodWithMultipleArrayParametersDeferred>(deferred);
        }

        if (!IltUtil.checkByteArray(int8ArrayArg)) {
            deferred.reject(new ProviderRuntimeException("methodWithMultipleArrayParameters: invalid int8ArrayArg"));
            return new Promise<MethodWithMultipleArrayParametersDeferred>(deferred);
        }

        if (!IltUtil.checkExtendedInterfaceEnumerationInTypeCollectionArray(enumArrayArg)) {
            deferred.reject(new ProviderRuntimeException("methodWithMultipleArrayParameters: invalid enumArrayArg"));
            return new Promise<MethodWithMultipleArrayParametersDeferred>(deferred);
        }

        if (!IltUtil.checkStructWithStringArrayArray(structWithStringArrayArrayArg)) {
            deferred.reject(new ProviderRuntimeException("methodWithMultipleArrayParameters: invalid structWithStringArrayArrayArg"));
            return new Promise<MethodWithMultipleArrayParametersDeferred>(deferred);
        }

        Long[] uInt64ArrayOut = IltUtil.createUInt64Array();
        StructWithStringArray[] structWithStringArrayArrayOut = new StructWithStringArray[2];
        structWithStringArrayArrayOut[0] = IltUtil.createStructWithStringArray();
        structWithStringArrayArrayOut[1] = IltUtil.createStructWithStringArray();

        deferred.resolve(uInt64ArrayOut, structWithStringArrayArrayOut);
        return new Promise<MethodWithMultipleArrayParametersDeferred>(deferred);
    }

    /*
     *methodWithSingleByteBufferParameter
     *
     *Return the same ByteBuffer that was put in as parameter
     */
    @Override
    public Promise<MethodWithSingleByteBufferParameterDeferred> methodWithSingleByteBufferParameter(Byte[] byteBufferArg) {
        logger.info("********************************************************");
        logger.info("* IltProvider.methodWithSingleByteBufferParameter called");
        logger.info("********************************************************");
        MethodWithSingleByteBufferParameterDeferred deferred = new MethodWithSingleByteBufferParameterDeferred();

        // prepare output parameter
        deferred.resolve(byteBufferArg);
        return new Promise<MethodWithSingleByteBufferParameterDeferred>(deferred);
    }

    /*
     *methodWithMultipleByteBufferParameters
     *
     *Return the componentwise sum of the two ByteBuffers
     */
    @Override
    public Promise<MethodWithMultipleByteBufferParametersDeferred> methodWithMultipleByteBufferParameters(Byte[] byteBufferArg1,
                                                                                                          Byte[] byteBufferArg2) {
        logger.info("***********************************************************");
        logger.info("* IltProvider.methodWithMultipleByteBufferParameters called");
        logger.info("***********************************************************");
        MethodWithMultipleByteBufferParametersDeferred deferred = new MethodWithMultipleByteBufferParametersDeferred();

        //calculate result
        Byte[] result = IltUtil.concatenateByteArrays(byteBufferArg1, byteBufferArg2);

        //prepare output parameter
        deferred.resolve(result);
        return new Promise<MethodWithMultipleByteBufferParametersDeferred>(deferred);
    }

    /*
     *methodWithInt64TypeDefParameter
     *
     *Return the same TypeDefForInt64 that was put in as parameter
     */
    @Override
    public Promise<MethodWithInt64TypeDefParameterDeferred> methodWithInt64TypeDefParameter(Long int64TypeDefIn) {
        logger.info("****************************************************");
        logger.info("* IltProvider.methodWithInt64TypeDefParameter called");
        logger.info("****************************************************");
        MethodWithInt64TypeDefParameterDeferred deferred = new MethodWithInt64TypeDefParameterDeferred();

        // prepare output parameter
        deferred.resolve(int64TypeDefIn);
        return new Promise<MethodWithInt64TypeDefParameterDeferred>(deferred);
    }

    /*
     *methodWithStringTypeDefParameter
     *
     *Return the same TypeDefForString that was put in as parameter
     */
    @Override
    public Promise<MethodWithStringTypeDefParameterDeferred> methodWithStringTypeDefParameter(String stringTypeDefIn) {
        logger.info("*****************************************************");
        logger.info("* IltProvider.methodWithStringTypeDefParameter called");
        logger.info("*****************************************************");
        MethodWithStringTypeDefParameterDeferred deferred = new MethodWithStringTypeDefParameterDeferred();

        // prepare output parameter
        deferred.resolve(stringTypeDefIn);
        return new Promise<MethodWithStringTypeDefParameterDeferred>(deferred);
    }

    /*
     *methodWithStructTypeDefParameter
     *
     *Return the same TypeDefForStruct that was put in as parameter
     */
    @Override
    public Promise<MethodWithStructTypeDefParameterDeferred> methodWithStructTypeDefParameter(BaseStruct structTypeDefIn) {
        logger.info("****************************************************");
        logger.info("* IltProvider.methodWithInt64TypeDefParameter called");
        logger.info("****************************************************");
        MethodWithStructTypeDefParameterDeferred deferred = new MethodWithStructTypeDefParameterDeferred();

        // prepare output parameter
        deferred.resolve(structTypeDefIn);
        return new Promise<MethodWithStructTypeDefParameterDeferred>(deferred);
    }

    /*
     *methodWithMapTypeDefParameter
     *
     *Return the same TypeDefForMap that was put in as parameter
     */
    @Override
    public Promise<MethodWithMapTypeDefParameterDeferred> methodWithMapTypeDefParameter(MapStringString mapTypeDefIn) {
        logger.info("****************************************************");
        logger.info("* IltProvider.methodWithMapTypeDefParameter called");
        logger.info("****************************************************");
        MethodWithMapTypeDefParameterDeferred deferred = new MethodWithMapTypeDefParameterDeferred();

        // prepare output parameter
        deferred.resolve(mapTypeDefIn);
        return new Promise<MethodWithMapTypeDefParameterDeferred>(deferred);
    }

    /*
     *methodWithEnumTypeDefParameter
     *
     *Return the same TypeDefForEnum that was put in as parameter
     */
    @Override
    public Promise<MethodWithEnumTypeDefParameterDeferred> methodWithEnumTypeDefParameter(Enumeration enumTypeDefIn) {
        logger.info("***************************************************");
        logger.info("* IltProvider.methodWithEnumTypeDefParameter called");
        logger.info("***************************************************");
        MethodWithEnumTypeDefParameterDeferred deferred = new MethodWithEnumTypeDefParameterDeferred();

        // prepare output parameter
        deferred.resolve(enumTypeDefIn);
        return new Promise<MethodWithEnumTypeDefParameterDeferred>(deferred);
    }

    /*
     *methodWithByteBufferTypeDefParameter
     *
     *Return the same TypeDefForByteBuffer that was put in as parameter
     */
    @Override
    public Promise<MethodWithByteBufferTypeDefParameterDeferred> methodWithByteBufferTypeDefParameter(Byte[] byteBufferTypeDefIn) {
        logger.info("*********************************************************");
        logger.info("* IltProvider.methodWithByteBufferTypeDefParameter called");
        logger.info("*********************************************************");
        MethodWithByteBufferTypeDefParameterDeferred deferred = new MethodWithByteBufferTypeDefParameterDeferred();

        // prepare output parameter
        deferred.resolve(byteBufferTypeDefIn);
        return new Promise<MethodWithByteBufferTypeDefParameterDeferred>(deferred);
    }

    /*
     *methodWithArrayTypeDefParameter
     *
     *Return the same ArrayTypeDefStruct that was put in as parameter
     */
    @Override
    public Promise<MethodWithArrayTypeDefParameterDeferred> methodWithArrayTypeDefParameter(ArrayTypeDefStruct arrayTypeDefIn) {
        logger.info("****************************************************");
        logger.info("* IltProvider.methodWithArrayTypeDefParameter called");
        logger.info("****************************************************");
        MethodWithArrayTypeDefParameterDeferred deferred = new MethodWithArrayTypeDefParameterDeferred();

        // prepare output parameter
        deferred.resolve(arrayTypeDefIn);
        return new Promise<MethodWithArrayTypeDefParameterDeferred>(deferred);
    }

    /*
     * methodWithSingleEnumParameters
     *
     * return fixed value for now
     */
    @Override
    public Promise<MethodWithSingleEnumParametersDeferred> methodWithSingleEnumParameters(ExtendedEnumerationWithPartlyDefinedValues enumerationArg) {
        logger.warn("***************************************************");
        logger.warn("* IltProvider.methodWithSingleEnumParameters called");
        logger.warn("***************************************************");
        MethodWithSingleEnumParametersDeferred deferred = new MethodWithSingleEnumParametersDeferred();

        // check input parameter
        if (enumerationArg != ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES) {
            logger.warn("methodWithSingleEnumParameters: invalid argument enumerationArg");
            deferred.reject(new ProviderRuntimeException("methodWithSingleEnumParameters: received wrong argument"));
            return new Promise<MethodWithSingleEnumParametersDeferred>(deferred);
        }

        // prepare output parameter
        ExtendedTypeCollectionEnumerationInTypeCollection enumerationOut = ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
        deferred.resolve(enumerationOut);
        return new Promise<MethodWithSingleEnumParametersDeferred>(deferred);
    }

    /*
     * methodWithMultipleEnumParameters
     *
     * return fixed values for now
     */
    @Override
    public Promise<MethodWithMultipleEnumParametersDeferred> methodWithMultipleEnumParameters(Enumeration enumerationArg,
                                                                                              ExtendedTypeCollectionEnumerationInTypeCollection extendedEnumerationArg) {
        logger.warn("*****************************************************");
        logger.warn("* IltProvider.methodWithMultipleEnumParameters called");
        logger.warn("*****************************************************");
        MethodWithMultipleEnumParametersDeferred deferred = new MethodWithMultipleEnumParametersDeferred();

        // check input parameters
        if (enumerationArg != joynr.interlanguagetest.Enumeration.ENUM_0_VALUE_3
                || extendedEnumerationArg != ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
            logger.warn("methodWithMultipleEnumParameters: invalid argument enumerationArg or extendedEnumerationArg");
            deferred.reject(new ProviderRuntimeException("methodWithMultipleEnumParameters: received wrong argument"));
            return new Promise<MethodWithMultipleEnumParametersDeferred>(deferred);
        }

        // prepare output parameters
        ExtendedEnumerationWithPartlyDefinedValues extendedEnumerationOut = ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
        Enumeration enumerationOut = Enumeration.ENUM_0_VALUE_1;
        deferred.resolve(extendedEnumerationOut, enumerationOut);
        return new Promise<MethodWithMultipleEnumParametersDeferred>(deferred);
    }

    /*
     * methodWithSingleStructParameters
     */
    @Override
    public Promise<MethodWithSingleStructParametersDeferred> methodWithSingleStructParameters(ExtendedBaseStruct extendedBaseStructArg) {
        logger.warn("*****************************************************");
        logger.warn("* IltProvider.methodWithSingleStructParameters called");
        logger.warn("*****************************************************");
        MethodWithSingleStructParametersDeferred deferred = new MethodWithSingleStructParametersDeferred();

        ExtendedBaseStruct extendedBaseStruct = IltUtil.createExtendedBaseStruct();
        if (!extendedBaseStruct.equals(extendedBaseStructArg)) {
            logger.error("methodWithSingleStructParameters: invalid parameter extendedBaseStructArg");
            deferred.reject(new ProviderRuntimeException("methodWithSingleStructParameters: invalid parameter extendedBaseStructArg"));
            return new Promise<MethodWithSingleStructParametersDeferred>(deferred);
        }

        // prepare output parameters
        ExtendedStructOfPrimitives extendedStructOfPrimitivesOut = IltUtil.createExtendedStructOfPrimitives();
        deferred.resolve(extendedStructOfPrimitivesOut);
        return new Promise<MethodWithSingleStructParametersDeferred>(deferred);
    }

    /*
     * methodWithMultipleStructParameters
     */
    @Override
    public Promise<MethodWithMultipleStructParametersDeferred> methodWithMultipleStructParameters(ExtendedStructOfPrimitives extendedStructOfPrimitivesArg,
                                                                                                  BaseStruct baseStructArg) {
        MethodWithMultipleStructParametersDeferred deferred = new MethodWithMultipleStructParametersDeferred();
        logger.warn("*******************************************************");
        logger.warn("* IltProvider.methodWithMultipleStructParameters called");
        logger.warn("*******************************************************");

        // check input parameter
        if (!IltUtil.checkExtendedStructOfPrimitives(extendedStructOfPrimitivesArg)) {
            deferred.reject(new ProviderRuntimeException("methodWithMultipleStructParameters: invalid parameter extendedStructOfPrimitivesArg"));
            return new Promise<MethodWithMultipleStructParametersDeferred>(deferred);
        }

        if (!baseStructArg.getBaseStructString().equals("Hiya")) {
            deferred.reject(new ProviderRuntimeException("methodWithMultipleStructParameters: invalid parameter baseStructArg"));
            return new Promise<MethodWithMultipleStructParametersDeferred>(deferred);
        }

        // set output values
        BaseStructWithoutElements baseStructWithoutElementsOut = IltUtil.createBaseStructWithoutElements();
        ExtendedExtendedBaseStruct extendedExtendedBaseStructOut = IltUtil.createExtendedExtendedBaseStruct();

        deferred.resolve(baseStructWithoutElementsOut, extendedExtendedBaseStructOut);
        return new Promise<MethodWithMultipleStructParametersDeferred>(deferred);
    }

    /*
     * overloadedMethod (1)
     */
    @Override
    public Promise<OverloadedMethod1Deferred> overloadedMethod() {
        logger.warn("*****************************************");
        logger.warn("* IltProvider.overloadedMethod called (1)");
        logger.warn("*****************************************");
        OverloadedMethod1Deferred deferred = new OverloadedMethod1Deferred();
        String stringOut = "TestString 1";
        deferred.resolve(stringOut);
        return new Promise<OverloadedMethod1Deferred>(deferred);
    }

    /*
     * overloadedMethod (2)
     */
    @Override
    public Promise<OverloadedMethod1Deferred> overloadedMethod(Boolean booleanArg) {
        logger.warn("*****************************************");
        logger.warn("* IltProvider.overloadedMethod called (2)");
        logger.warn("*****************************************");
        OverloadedMethod1Deferred deferred = new OverloadedMethod1Deferred();
        if (booleanArg != false) {
            logger.warn("overloadedMethod_2: invalid argument booleanArg");
            deferred.reject(new ProviderRuntimeException("overloadedMethod_2: invalid parameter baseStructArg"));
            return new Promise<OverloadedMethod1Deferred>(deferred);
        }
        String stringOut = "TestString 2";
        deferred.resolve(stringOut);
        return new Promise<OverloadedMethod1Deferred>(deferred);
    }

    /*
     * overloadedMethod (3)
     */
    @Override
    public Promise<OverloadedMethod2Deferred> overloadedMethod(ExtendedExtendedEnumeration[] enumArrayArg,
                                                               Long int64Arg,
                                                               BaseStruct baseStructArg,
                                                               Boolean booleanArg) {
        logger.warn("*****************************************");
        logger.warn("* IltProvider.overloadedMethod called (3)");
        logger.warn("*****************************************");
        OverloadedMethod2Deferred deferred = new OverloadedMethod2Deferred();

        // check input parameter
        if (int64Arg != 1L || booleanArg != false) {
            logger.warn("overloadedMethod_3: invalid argument int64Arg or booleanArg");
            deferred.reject(new ProviderRuntimeException("overloadedMethod_3: invalid parameter int64Arg or booleanArg"));
            return new Promise<OverloadedMethod2Deferred>(deferred);
        }

        // check enumArrayArg
        if (!IltUtil.checkExtendedExtendedEnumerationArray(enumArrayArg)) {
            deferred.reject(new ProviderRuntimeException("overloadedMethod_3: invalid parameter enumArrayArg"));
            return new Promise<OverloadedMethod2Deferred>(deferred);
        }

        // check baseStructArg
        if (!baseStructArg.getBaseStructString().equals("Hiya")) {
            logger.warn("overloadedMethod_3: invalid argument baseStructArg");
            deferred.reject(new ProviderRuntimeException("overloadedMethod_3: invalid parameter baseStructArg"));
            return new Promise<OverloadedMethod2Deferred>(deferred);
        }

        // setup output parameter
        Double doubleOut = 0d;
        String[] stringArrayOut = { "Hello", "World" };
        ExtendedBaseStruct extendedBaseStructOut = IltUtil.createExtendedBaseStruct();

        deferred.resolve(doubleOut, stringArrayOut, extendedBaseStructOut);
        return new Promise<OverloadedMethod2Deferred>(deferred);
    }

    /*
     * overloadedMethodWithSelector (1)
     */
    @Override
    public Promise<OverloadedMethodWithSelector1Deferred> overloadedMethodWithSelector() {
        logger.warn("*************************************************");
        logger.warn("* IltProvider.overloadedMethodWithSelector called");
        logger.warn("*************************************************");
        OverloadedMethodWithSelector1Deferred deferred = new OverloadedMethodWithSelector1Deferred();
        String stringOut = "Return value from overloadedMethodWithSelector 1";
        deferred.resolve(stringOut);
        return new Promise<OverloadedMethodWithSelector1Deferred>(deferred);
    }

    /*
     * overloadedMethodWithSelector (2)
     */
    @Override
    public Promise<OverloadedMethodWithSelector1Deferred> overloadedMethodWithSelector(Boolean booleanArg) {
        logger.warn("*************************************************");
        logger.warn("* IltProvider.overloadedMethodWithSelector called");
        logger.warn("*************************************************");
        OverloadedMethodWithSelector1Deferred deferred = new OverloadedMethodWithSelector1Deferred();

        // check input parameter
        if (booleanArg != false) {
            logger.warn("overloadedMethodWithSelector: invalid argument booleanArg");
            deferred.reject(new ProviderRuntimeException("overloadedMethodWithSelector: invalid parameter booleanArg"));
            return new Promise<OverloadedMethodWithSelector1Deferred>(deferred);
        }

        // setup output parameter
        String stringOut = "Return value from overloadedMethodWithSelector 2";
        deferred.resolve(stringOut);
        return new Promise<OverloadedMethodWithSelector1Deferred>(deferred);
    }

    /*
     * overloadedMethodWithSelector (3)
     */
    @Override
    public Promise<OverloadedMethodWithSelector2Deferred> overloadedMethodWithSelector(ExtendedExtendedEnumeration[] enumArrayArg,
                                                                                       Long int64Arg,
                                                                                       BaseStruct baseStructArg,
                                                                                       Boolean booleanArg) {
        logger.warn("*************************************************");
        logger.warn("* IltProvider.overloadedMethodWithSelector called");
        logger.warn("*************************************************");
        OverloadedMethodWithSelector2Deferred deferred = new OverloadedMethodWithSelector2Deferred();

        /* check input */
        if (!IltUtil.checkExtendedExtendedEnumerationArray(enumArrayArg)) {
            deferred.reject(new ProviderRuntimeException("overloadedMethodWithSelector: failed to compare enumArrayArg"));
            return new Promise<OverloadedMethodWithSelector2Deferred>(deferred);
        }

        if (int64Arg != 1L) {
            deferred.reject(new ProviderRuntimeException("overloadedMethodWithSelector: failed to compare int64Arg"));
            return new Promise<OverloadedMethodWithSelector2Deferred>(deferred);
        }

        if (!baseStructArg.getBaseStructString().equals("Hiya")) {
            deferred.reject(new ProviderRuntimeException("overloadedMethodWithSelector: failed to compare baseStructArg"));
            return new Promise<OverloadedMethodWithSelector2Deferred>(deferred);
        }

        if (booleanArg != false) {
            deferred.reject(new ProviderRuntimeException("overloadedMethodWithSelector: failed to compare booleanArg"));
            return new Promise<OverloadedMethodWithSelector2Deferred>(deferred);
        }

        /* prepare output */
        Double doubleOut = 1.1d;

        String[] stringArrayOut = { "Hello", "World" };
        ExtendedBaseStruct extendedBaseStructOut = IltUtil.createExtendedBaseStruct();

        deferred.resolve(doubleOut, stringArrayOut, extendedBaseStructOut);
        return new Promise<OverloadedMethodWithSelector2Deferred>(deferred);
    }

    /*
     * methodWithStringsAndSpecifiedStringOutLength
     */
    @Override
    public Promise<MethodWithStringsAndSpecifiedStringOutLengthDeferred> methodWithStringsAndSpecifiedStringOutLength(String stringArg,
                                                                                                                      Integer int32StringLengthArg) {
        logger.warn("*****************************************************************");
        logger.warn("* IltProvider.methodWithStringsAndSpecifiedStringOutLength called");
        logger.warn("*****************************************************************");
        MethodWithStringsAndSpecifiedStringOutLengthDeferred deferred = new MethodWithStringsAndSpecifiedStringOutLengthDeferred();
        StringBuilder s = new StringBuilder();
        if (int32StringLengthArg > 1024 * 1024) {
            deferred.reject(new ProviderRuntimeException("methodWithStringsAndSpecifiedStringOutLength: Maximum length exceeded"));
            return new Promise<MethodWithStringsAndSpecifiedStringOutLengthDeferred>(deferred);
        }
        for (int i = 0; i < int32StringLengthArg; i++) {
            s.append("A");
        }
        deferred.resolve(s.toString());
        return new Promise<MethodWithStringsAndSpecifiedStringOutLengthDeferred>(deferred);
    }

    /*
     * methodWithoutErrorEnum
     */
    @Override
    public Promise<DeferredVoid> methodWithoutErrorEnum(String wantedExceptionArg) {
        logger.warn("*******************************************");
        logger.warn("* IltProvider.methodWithoutErrorEnum called");
        logger.warn("*******************************************");
        DeferredVoid deferred = new DeferredVoid();

        if (wantedExceptionArg.equals("ProviderRuntimeException")) {
            deferred.reject(new ProviderRuntimeException("Exception from methodWithoutErrorEnum"));
        } else {
            deferred.resolve();
        }
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodWithAnonymousErrorEnum
     */
    @Override
    public Promise<MethodWithAnonymousErrorEnumDeferred> methodWithAnonymousErrorEnum(String wantedExceptionArg) {
        logger.warn("*************************************************");
        logger.warn("* IltProvider.methodWithAnonymousErrorEnum called");
        logger.warn("*************************************************");
        MethodWithAnonymousErrorEnumDeferred deferred = new MethodWithAnonymousErrorEnumDeferred();

        if (wantedExceptionArg.equals("ProviderRuntimeException")) {
            deferred.reject(new ProviderRuntimeException("Exception from methodWithAnonymousErrorEnum"));
        } else if (wantedExceptionArg.equals("ApplicationException")) {
            deferred.reject(MethodWithAnonymousErrorEnumErrorEnum.ERROR_3_1_NTC);
        } else {
            deferred.resolve();
        }
        return new Promise<MethodWithAnonymousErrorEnumDeferred>(deferred);
    }

    /*
     * methodWithExistingErrorEnum
     */
    @Override
    public Promise<MethodWithExistingErrorEnumDeferred> methodWithExistingErrorEnum(String wantedExceptionArg) {
        logger.warn("************************************************");
        logger.warn("* IltProvider.methodWithExistingErrorEnum called");
        logger.warn("************************************************");
        MethodWithExistingErrorEnumDeferred deferred = new MethodWithExistingErrorEnumDeferred();
        if (wantedExceptionArg.equals("ProviderRuntimeException")) {
            deferred.reject(new ProviderRuntimeException("Exception from methodWithExistingErrorEnum"));
        } else if (wantedExceptionArg.equals("ApplicationException_1")) {
            deferred.reject(ExtendedErrorEnumTc.ERROR_2_3_TC2);
        } else if (wantedExceptionArg.equals("ApplicationException_2")) {
            deferred.reject(ExtendedErrorEnumTc.ERROR_1_2_TC_2);
        } else {
            deferred.resolve();
        }
        return new Promise<MethodWithExistingErrorEnumDeferred>(deferred);
    }

    /*
     * methodWithExtendedErrorEnum
     */
    @Override
    public Promise<MethodWithExtendedErrorEnumDeferred> methodWithExtendedErrorEnum(String wantedExceptionArg) {
        logger.warn("************************************************");
        logger.warn("* IltProvider.methodWithExtendedErrorEnum called");
        logger.warn("************************************************");
        MethodWithExtendedErrorEnumDeferred deferred = new MethodWithExtendedErrorEnumDeferred();
        if (wantedExceptionArg.equals("ProviderRuntimeException")) {
            deferred.reject(new ProviderRuntimeException("Exception from methodWithExtendedErrorEnum"));
        } else if (wantedExceptionArg.equals("ApplicationException_1")) {
            deferred.reject(MethodWithExtendedErrorEnumErrorEnum.ERROR_3_3_NTC);
        } else if (wantedExceptionArg.equals("ApplicationException_2")) {
            deferred.reject(MethodWithExtendedErrorEnumErrorEnum.ERROR_2_1_TC2);
        } else {
            deferred.resolve();
        }
        return new Promise<MethodWithExtendedErrorEnumDeferred>(deferred);
    }

    /*
     * methodToFireBroadcastWithSinglePrimitiveParameter
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithSinglePrimitiveParameter(String[] partitions) {
        logger.warn("**********************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithSinglePrimitiveParameter called");
        logger.warn("**********************************************************************");
        DeferredVoid deferred = new DeferredVoid();
        String stringOut = "boom";
        fireBroadcastWithSinglePrimitiveParameter(stringOut, partitions);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodToFireBroadcastWithMultiplePrimitiveParameters
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithMultiplePrimitiveParameters(String[] partitions) {
        logger.warn("*************************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithMultiplePrimitiveParameters called");
        logger.warn("*************************************************************************");
        DeferredVoid deferred = new DeferredVoid();
        Double doubleOut = 1.1d;
        String stringOut = "boom";
        fireBroadcastWithMultiplePrimitiveParameters(doubleOut, stringOut, partitions);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodToFireBroadcastWithSingleArrayParameter
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithSingleArrayParameter(String[] partitions) {
        logger.warn("******************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithSingleArrayParameter called");
        logger.warn("******************************************************************");
        DeferredVoid deferred = new DeferredVoid();
        String[] stringArrayOut = { "Hello", "World" };
        fireBroadcastWithSingleArrayParameter(stringArrayOut, partitions);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodToFireBroadcastWithMultipleArrayParameters
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithMultipleArrayParameters(String[] partitions) {
        logger.warn("*********************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithMultipleArrayParameters called");
        logger.warn("*********************************************************************");
        DeferredVoid deferred = new DeferredVoid();
        Long[] uInt64ArrayOut = IltUtil.createUInt64Array();
        StructWithStringArray[] structWithStringArrayArrayOut = IltUtil.createStructWithStringArrayArray();
        fireBroadcastWithMultipleArrayParameters(uInt64ArrayOut, structWithStringArrayArrayOut, partitions);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodToFireBroadcastWithSingleByteBufferParameter
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithSingleByteBufferParameter(Byte[] byteBufferIn,
                                                                                    String[] partitions) {
        logger.info("***********************************************************************");
        logger.info("* IltProvider.methodToFireBroadcastWithSingleByteBufferParameter called");
        logger.info("***********************************************************************");
        DeferredVoid deferred = new DeferredVoid();
        fireBroadcastWithSingleByteBufferParameter(byteBufferIn, partitions);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodToFireBroadcastWithMultipleByteBufferParameters
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithMultipleByteBufferParameters(Byte[] byteBufferIn1,
                                                                                       Byte[] byteBufferIn2,
                                                                                       String[] partitions) {
        logger.info("**************************************************************************");
        logger.info("* IltProvider.methodToFireBroadcastWithMultipleByteBufferParameters called");
        logger.info("**************************************************************************");
        DeferredVoid deferred = new DeferredVoid();
        fireBroadcastWithMultipleByteBufferParameters(byteBufferIn1, byteBufferIn2, partitions);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodToFireBroadcastWithSingleEnumerationParameter
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithSingleEnumerationParameter(String[] partitions) {
        logger.warn("************************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithSingleEnumerationParameter called");
        logger.warn("************************************************************************");
        DeferredVoid deferred = new DeferredVoid();
        ExtendedTypeCollectionEnumerationInTypeCollection enumerationOut = ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
        fireBroadcastWithSingleEnumerationParameter(enumerationOut, partitions);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodToFireBroadcastWithMultipleEnumerationParameters
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithMultipleEnumerationParameters(String[] partitions) {
        logger.warn("***************************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithMultipleEnumerationParameters called");
        logger.warn("***************************************************************************");
        DeferredVoid deferred = new DeferredVoid();
        deferred.resolve();
        ExtendedEnumerationWithPartlyDefinedValues extendedEnumerationOut = ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
        Enumeration enumerationOut = Enumeration.ENUM_0_VALUE_1;
        fireBroadcastWithMultipleEnumerationParameters(extendedEnumerationOut, enumerationOut, partitions);
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodToFireBroadcastWithSingleStructParameter
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithSingleStructParameter(String[] partitions) {
        logger.warn("*******************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithSingleStructParameter called");
        logger.warn("*******************************************************************");
        DeferredVoid deferred = new DeferredVoid();
        ExtendedStructOfPrimitives extendedStructOfPrimitivesOut = IltUtil.createExtendedStructOfPrimitives();
        fireBroadcastWithSingleStructParameter(extendedStructOfPrimitivesOut, partitions);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodToFireBroadcastWithMultipleStructParameters
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithMultipleStructParameters(String[] partitions) {
        logger.warn("**********************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithMultipleStructParameters called");
        logger.warn("**********************************************************************");
        DeferredVoid deferred = new DeferredVoid();
        BaseStructWithoutElements baseStructWithoutElementsOut = IltUtil.createBaseStructWithoutElements();
        ExtendedExtendedBaseStruct extendedExtendedBaseStructOut = IltUtil.createExtendedExtendedBaseStruct();
        fireBroadcastWithMultipleStructParameters(baseStructWithoutElementsOut,
                                                  extendedExtendedBaseStructOut,
                                                  partitions);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodToFireBroadcastWithFiltering
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithFiltering(String stringArg) {
        logger.warn("*******************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithFiltering called");
        logger.warn("*******************************************************");
        DeferredVoid deferred = new DeferredVoid();

        // take the stringArg as input for the filtering
        String stringOut = stringArg;
        String[] stringArrayOut = { "Hello", "World" };
        ExtendedTypeCollectionEnumerationInTypeCollection enumerationOut = ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
        StructWithStringArray structWithStringArrayOut = IltUtil.createStructWithStringArray();
        StructWithStringArray[] structWithStringArrayArrayOut = IltUtil.createStructWithStringArrayArray();

        fireBroadcastWithFiltering(stringOut,
                                   stringArrayOut,
                                   enumerationOut,
                                   structWithStringArrayOut,
                                   structWithStringArrayArrayOut);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<MapStringString>> getAttributeMapStringString() {
        Deferred<MapStringString> deferred = new Deferred<MapStringString>();
        deferred.resolve(attributeMapStringString);
        return new Promise<Deferred<MapStringString>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeMapStringString(MapStringString attributeMapStringString) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeMapStringString = (attributeMapStringString != null)
                ? new MapStringString(attributeMapStringString)
                : null;
        attributeMapStringStringChanged(attributeMapStringString);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<MethodWithSingleMapParametersDeferred> methodWithSingleMapParameters(MapStringString mapArg) {
        MethodWithSingleMapParametersDeferred deferred = new MethodWithSingleMapParametersDeferred();
        if (mapArg == null) {
            deferred.resolve(null);
        } else {
            MapStringString mapOut = new MapStringString();
            Iterator<Map.Entry<String, String>> iterator = mapArg.entrySet().iterator();
            while (iterator.hasNext()) {
                Map.Entry<String, String> entry = (Entry<String, String>) iterator.next();
                mapOut.put(entry.getValue(), entry.getKey());
            }
            deferred.resolve(mapOut);
        }
        return new Promise<MethodWithSingleMapParametersDeferred>(deferred);
    }

    @Override
    public Promise<Deferred<Integer>> getAttributeFireAndForget() {
        Deferred<Integer> deferred = new Deferred<Integer>();
        deferred.resolve(attributeFireAndForget);
        return new Promise<Deferred<Integer>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeFireAndForget(Integer attributeFireAndForget) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeFireAndForget = attributeFireAndForget;
        attributeFireAndForgetChanged(attributeFireAndForget);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public void methodFireAndForgetWithoutParameter() {
        setAttributeFireAndForget(attributeFireAndForget + 1);
    }

    @Override
    public void methodFireAndForgetWithInputParameter(Integer int32Arg) {
        setAttributeFireAndForget(int32Arg);
    }
}
