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

#include "IltProvider.h"
#include "IltHelper.h"
#include "IltUtil.h"
#include "joynr/Logger.h"
#include "joynr/exceptions/JoynrException.h"
#include <chrono>
#include <memory>

using namespace joynr;

bool IltUtil::useRestricted64BitRange = true;
bool IltUtil::useRestrictedUnsignedRange = true;

IltProvider::IltProvider() : DefaultTestInterfaceProvider(), mutex(), attributeFireAndForget(0)
{
    // set initial value for attributes here, if any
}

IltProvider::~IltProvider()
{
}

// attribute getter, if any

// attribute setter, if any

// methods to fire broadcasts, if any

// method implementations

void IltProvider::methodWithoutParameters(
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    onSuccess();
}

void IltProvider::methodWithoutInputParameter(
        std::function<void(const bool& booleanOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    onSuccess(true);
}

void IltProvider::methodWithoutOutputParameter(
        const bool& booleanArg,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    onSuccess();
}

void IltProvider::methodWithSinglePrimitiveParameters(
        const uint16_t& uInt16Arg,
        std::function<void(const std::string& stringOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)

{
    JOYNR_LOG_WARN(logger(), "methodWithSinglePrimitiveParameters - START");
    if (uInt16Arg != 32767) {
        JOYNR_LOG_WARN(
                logger(), "methodWithSinglePrimitiveParameters: invalid input parameter detected");
        onError(joynr::exceptions::ProviderRuntimeException(
                "methodWithSinglePrimitiveParameters: received wrong argument"));
        return;
    }
    JOYNR_LOG_WARN(logger(), "methodWithSinglePrimitiveParameters - OK");
    onSuccess(std::to_string(uInt16Arg));
}

void IltProvider::methodWithMultiplePrimitiveParameters(
        const int32_t& int32Arg,
        const float& floatArg,
        const bool& booleanArg,
        std::function<void(const double& doubleOut, const std::string& stringOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    JOYNR_LOG_WARN(logger(), "methodWithMultiplePrimitiveParameters - START");
    if (int32Arg != 2147483647 || !IltUtil::cmpFloat(floatArg, 47.11) || booleanArg != false) {
        JOYNR_LOG_WARN(
                logger(),
                "methodWithMultiplePrimitiveParameters: invalid input parameter(s) detected");
        JOYNR_LOG_WARN(logger(), "int32Arg = {}", int32Arg);
        JOYNR_LOG_WARN(logger(), "floatArg = {}", floatArg);
        JOYNR_LOG_WARN(logger(), "booleanArg = {}", (booleanArg ? "true" : "false"));
        JOYNR_LOG_WARN(logger(), "methodWithMultiplePrimitiveParameters - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "methodWithMultiplePrimitiveParameters: received wrong argument"));
        return;
    }
    double doubleOut = floatArg;
    std::string stringOut = std::to_string(int32Arg);
    JOYNR_LOG_WARN(logger(), "methodWithMultiplePrimitiveParameters - OK");
    onSuccess(doubleOut, stringOut);
}

void IltProvider::methodWithSingleArrayParameters(
        const std::vector<double>& doubleArrayArg,
        std::function<void(const std::vector<std::string>& stringArrayOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    // check input parameter
    JOYNR_LOG_WARN(logger(), "methodWithSingleArrayParameters - START");
    if (!IltUtil::checkDoubleArray(doubleArrayArg)) {
        JOYNR_LOG_WARN(
                logger(), "methodWithSingleArrayParameters: invalid input parameter(s) detected");
        JOYNR_LOG_WARN(logger(), "methodWithSingleArrayParameters - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "methodWithSingleArrayParameters: received wrong argument"));
        return;
    }

    std::vector<std::string> stringArrayOut = IltUtil::createStringArray();
    JOYNR_LOG_WARN(logger(), "methodWithSingleArrayParameters - OK");
    onSuccess(stringArrayOut);
}

void IltProvider::methodWithMultipleArrayParameters(
        const std::vector<std::string>& stringArrayArg,
        const std::vector<int8_t>& int8ArrayArg,
        const std::vector<joynr::interlanguagetest::namedTypeCollection2::
                                  ExtendedInterfaceEnumerationInTypeCollection::Enum>& enumArrayArg,
        const std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>&
                structWithStringArrayArrayArg,
        std::function<
                void(const std::vector<uint64_t>& uInt64ArrayOut,
                     const std::vector<
                             joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>&
                             structWithStringArrayArrayOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    JOYNR_LOG_WARN(logger(), "methodWithMultipleArrayParameters - START");
    if (!IltUtil::checkStringArray(stringArrayArg)) {
        JOYNR_LOG_WARN(logger(),
                       "methodWithMultipleArrayParameters: invalid input parameter stringArrayArg "
                       "detected");
        JOYNR_LOG_WARN(logger(), "methodWithMultipleArrayParameters - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "methodWithMultipleArrayParameters: received wrong argument"));
        return;
    }
    JOYNR_LOG_WARN(logger(), "methodWithMultipleArrayParameters: check OK - stringArrayArg");
    if (!IltUtil::checkByteArray(int8ArrayArg)) {
        JOYNR_LOG_WARN(logger(),
                       "methodWithMultipleArrayParameters: invalid input parameter int8ArrayArg "
                       "detected");
        JOYNR_LOG_WARN(logger(), "methodWithMultipleArrayParameters - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "methodWithMultipleArrayParameters: received wrong argument"));
        return;
    }
    JOYNR_LOG_WARN(logger(), "methodWithMultipleArrayParameters: check OK - int8ArrayArg");
    if (!IltUtil::checkExtendedInterfaceEnumerationInTypeCollectionArray(enumArrayArg)) {
        JOYNR_LOG_WARN(logger(),
                       "methodWithMultipleArrayParameters: invalid input parameter enumArrayArg "
                       "detected");
        JOYNR_LOG_WARN(logger(), "methodWithMultipleArrayParameters - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "methodWithMultipleArrayParameters: received wrong argument"));
        return;
    }
    JOYNR_LOG_WARN(logger(), "methodWithMultipleArrayParameters: check OK - enumArrayArg");

    // TODO: checkStructWithStringArrayarg?

    // setup output parameters
    std::vector<uint64_t> uInt64ArrayOut;
    std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>
            structWithStringArrayArrayOut;

    uInt64ArrayOut = IltUtil::convertInt8ArrayToUInt64Array(int8ArrayArg);

    structWithStringArrayArrayOut = IltUtil::createStructWithStringArrayArray();

    int cnt = 0;
    for (auto it = structWithStringArrayArrayOut.cbegin();
         it != structWithStringArrayArrayOut.cend();
         it++) {
        JOYNR_LOG_WARN(logger(), "methodWithMultipleArrayParameters: Array[{}]: START", cnt);
        for (auto it2 = (*it).getStringArrayElement().cbegin();
             it2 != (*it).getStringArrayElement().cend();
             it2++) {
            JOYNR_LOG_WARN(
                    logger(), "methodWithMultipleArrayParameters: String = {}", (*it2).c_str());
        }
        JOYNR_LOG_WARN(logger(), "methodWithMultipleArrayParameters: Array[{}]: END", cnt);
        cnt++;
    }

    JOYNR_LOG_WARN(logger(), "methodWithMultipleArrayParameters - OK");
    onSuccess(uInt64ArrayOut, structWithStringArrayArrayOut);
}

void IltProvider::methodWithSingleByteBufferParameter(
        const joynr::ByteBuffer& byteBufferIn,
        std::function<void(const joynr::ByteBuffer& byteBufferOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    std::ignore = onError;
    JOYNR_LOG_INFO(logger(), "********************************************************");
    JOYNR_LOG_INFO(logger(), "* IltProvider::methodWithSingleByteBufferParameter called");
    JOYNR_LOG_INFO(logger(), "********************************************************");
    onSuccess(byteBufferIn);
}

void IltProvider::methodWithMultipleByteBufferParameters(
        const joynr::ByteBuffer& byteBufferIn1,
        const joynr::ByteBuffer& byteBufferIn2,
        std::function<void(const joynr::ByteBuffer& byteBufferOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    std::ignore = onError;
    JOYNR_LOG_INFO(logger(), "********************************************************");
    JOYNR_LOG_INFO(logger(), "* IltProvider::methodWithMultipleByteBufferParameters called");
    JOYNR_LOG_INFO(logger(), "********************************************************");
    joynr::ByteBuffer byteBufferOut = IltUtil::concatByteBuffers(byteBufferIn1, byteBufferIn2);
    onSuccess(byteBufferOut);
}

void IltProvider::methodWithInt64TypeDefParameter(
        const joynr::interlanguagetest::typeDefCollection::TypeDefForInt64& int64TypeDefIn,
        std::function<void(const joynr::interlanguagetest::typeDefCollection::TypeDefForInt64&
                                   int64TypeDefOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    JOYNR_LOG_INFO(logger(), "*****************************************************");
    JOYNR_LOG_INFO(logger(), "* IltProvider::methodWithInt64TypeDefParameter called");
    JOYNR_LOG_INFO(logger(), "*****************************************************");
    std::int64_t intReferenceType;
    if (!IltUtil::checkType(intReferenceType, int64TypeDefIn)) {
        JOYNR_LOG_WARN(
                logger(),
                "methodWithInt64TypeDefParameter - FAILED: invalid input parameter int64TypeDefIn");
        onError(joynr::exceptions::ProviderRuntimeException(
                "methodWithInt64TypeDefParameter: received wrong argument"));
        return;
    }
    onSuccess(int64TypeDefIn);
}

void IltProvider::methodWithStringTypeDefParameter(
        const joynr::interlanguagetest::typeDefCollection::TypeDefForString& stringTypeDefIn,
        std::function<void(const joynr::interlanguagetest::typeDefCollection::TypeDefForString&
                                   stringTypeDefOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    JOYNR_LOG_INFO(logger(), "******************************************************");
    JOYNR_LOG_INFO(logger(), "* IltProvider::methodWithStringTypeDefParameter called");
    JOYNR_LOG_INFO(logger(), "******************************************************");
    std::string stringReferenceType;
    if (!IltUtil::checkType(stringReferenceType, stringTypeDefIn)) {
        JOYNR_LOG_WARN(logger(),
                       "methodWithStringTypeDefParameter - FAILED: invalid input "
                       "parameter stringTypeDefIn");
        onError(joynr::exceptions::ProviderRuntimeException(
                "methodWithStringTypeDefParameter: received wrong argument"));
        return;
    }
    onSuccess(stringTypeDefIn);
}

void IltProvider::methodWithStructTypeDefParameter(
        const joynr::interlanguagetest::typeDefCollection::TypeDefForStruct& structTypeDefIn,
        std::function<void(const joynr::interlanguagetest::typeDefCollection::TypeDefForStruct&
                                   structTypeDefOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    JOYNR_LOG_INFO(logger(), "******************************************************");
    JOYNR_LOG_INFO(logger(), "* IltProvider::methodWithStructTypeDefParameter called");
    JOYNR_LOG_INFO(logger(), "******************************************************");
    joynr::interlanguagetest::namedTypeCollection2::BaseStruct structReferenceType;
    if (!IltUtil::checkType(structReferenceType, structTypeDefIn)) {
        JOYNR_LOG_WARN(logger(),
                       "methodWithStructTypeDefParameter - FAILED: invalid input "
                       "parameter structTypeDefIn");
        onError(joynr::exceptions::ProviderRuntimeException(
                "methodWithStructTypeDefParameter: received wrong argument"));
        return;
    }
    onSuccess(structTypeDefIn);
}

void IltProvider::methodWithMapTypeDefParameter(
        const joynr::interlanguagetest::typeDefCollection::TypeDefForMap& mapTypeDefIn,
        std::function<void(const joynr::interlanguagetest::typeDefCollection::TypeDefForMap&
                                   mapTypeDefOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    JOYNR_LOG_INFO(logger(), "***************************************************");
    JOYNR_LOG_INFO(logger(), "* IltProvider::methodWithMapTypeDefParameter called");
    JOYNR_LOG_INFO(logger(), "***************************************************");
    joynr::interlanguagetest::namedTypeCollection2::MapStringString mapReferenceType;
    if (!IltUtil::checkType(mapReferenceType, mapTypeDefIn)) {
        JOYNR_LOG_WARN(logger(),
                       "methodWithMapTypeDefParameter - FAILED: invalid input "
                       "parameter mapTypeDefIn");
        onError(joynr::exceptions::ProviderRuntimeException(
                "methodWithMapTypeDefParameter: received wrong argument"));
        return;
    }
    onSuccess(mapTypeDefIn);
}

void IltProvider::methodWithEnumTypeDefParameter(
        const joynr::interlanguagetest::typeDefCollection::TypeDefForEnum::Enum& enumTypeDefIn,
        std::function<void(const joynr::interlanguagetest::typeDefCollection::TypeDefForEnum::Enum&
                                   enumTypeDefOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    JOYNR_LOG_INFO(logger(), "****************************************************");
    JOYNR_LOG_INFO(logger(), "* IltProvider::methodWithEnumTypeDefParameter called");
    JOYNR_LOG_INFO(logger(), "****************************************************");
    joynr::interlanguagetest::Enumeration::Enum enumReferenceType;
    if (!IltUtil::checkType(enumReferenceType, enumTypeDefIn)) {
        JOYNR_LOG_WARN(logger(),
                       "methodWithEnumTypeDefParameter - FAILED: invalid input "
                       "parameter enumTypeDefIn");
        onError(joynr::exceptions::ProviderRuntimeException(
                "methodWithEnumTypeDefParameter: received wrong argument"));
        return;
    }
    onSuccess(enumTypeDefIn);
}

void IltProvider::methodWithByteBufferTypeDefParameter(
        const joynr::interlanguagetest::typeDefCollection::TypeDefForByteBuffer&
                byteBufferTypeDefIn,
        std::function<void(const joynr::interlanguagetest::typeDefCollection::TypeDefForByteBuffer&
                                   byteBufferTypeDefOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    JOYNR_LOG_INFO(logger(), "**********************************************************");
    JOYNR_LOG_INFO(logger(), "* IltProvider::methodWithByteBufferTypeDefParameter called");
    JOYNR_LOG_INFO(logger(), "**********************************************************");
    joynr::ByteBuffer byteBufferReferenceType;
    if (!IltUtil::checkType(byteBufferReferenceType, byteBufferTypeDefIn)) {
        JOYNR_LOG_WARN(logger(),
                       "methodWithByteBufferTypeDefParameter - FAILED: invalid input "
                       "parameter byteBufferTypeDefIn");
        onError(joynr::exceptions::ProviderRuntimeException(
                "methodWithByteBufferTypeDefParameter: received wrong argument"));
        return;
    }
    onSuccess(byteBufferTypeDefIn);
}

void IltProvider::methodWithArrayTypeDefParameter(
        const interlanguagetest::typeDefCollection::ArrayTypeDefStruct& arrayTypeDefIn,
        std::function<void(const interlanguagetest::typeDefCollection::ArrayTypeDefStruct&)>
                onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    JOYNR_LOG_INFO(logger(), "*****************************************************");
    JOYNR_LOG_INFO(logger(), "* IltProvider::methodWithArrayTypeDefParameter called");
    JOYNR_LOG_INFO(logger(), "*****************************************************");
    joynr::interlanguagetest::typeDefCollection::ArrayTypeDefStruct arrayReferenceType;
    if (!IltUtil::checkType(arrayTypeDefIn, arrayTypeDefIn)) {
        JOYNR_LOG_WARN(logger(),
                       "methodWithArrayTypeDefParameter - FAILED: invalid input "
                       "parameter arrayTypeDefIn");
        onError(joynr::exceptions::ProviderRuntimeException(
                "methodWithArrayTypeDefParameter: received wrong argument"));
        return;
    }
    onSuccess(arrayTypeDefIn);
}

void IltProvider::methodWithSingleEnumParameters(
        const joynr::interlanguagetest::namedTypeCollection2::
                ExtendedEnumerationWithPartlyDefinedValues::Enum& enumerationArg,
        std::function<void(const joynr::interlanguagetest::namedTypeCollection2::
                                   ExtendedTypeCollectionEnumerationInTypeCollection::Enum&
                                           enumerationOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    JOYNR_LOG_WARN(logger(), "methodWithSingleEnumParameters - START");
    if (enumerationArg !=
        joynr::interlanguagetest::namedTypeCollection2::ExtendedEnumerationWithPartlyDefinedValues::
                ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES) {
        JOYNR_LOG_WARN(
                logger(), "methodWithSingleEnumParameters: invalid input parameter enumerationArg");
        JOYNR_LOG_WARN(logger(), "methodWithSingleEnumParameters - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "methodWithSingleEnumParameters: received wrong argument"));
        return;
    }

    joynr::interlanguagetest::namedTypeCollection2::
            ExtendedTypeCollectionEnumerationInTypeCollection::Enum enumerationOut =
                    joynr::interlanguagetest::namedTypeCollection2::
                            ExtendedTypeCollectionEnumerationInTypeCollection::
                                    ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
    JOYNR_LOG_WARN(logger(), "methodWithSingleEnumParameters - OK");
    onSuccess(enumerationOut);
}

void IltProvider::methodWithMultipleEnumParameters(
        const joynr::interlanguagetest::Enumeration::Enum& enumerationArg,
        const joynr::interlanguagetest::namedTypeCollection2::
                ExtendedTypeCollectionEnumerationInTypeCollection::Enum& extendedEnumerationArg,
        std::function<void(
                const joynr::interlanguagetest::namedTypeCollection2::
                        ExtendedEnumerationWithPartlyDefinedValues::Enum& extendedEnumerationOut,
                const joynr::interlanguagetest::Enumeration::Enum& enumerationOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    JOYNR_LOG_WARN(logger(), "methodWithMultipleEnumParameters - START");
    if (enumerationArg != joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_3 ||
        extendedEnumerationArg != joynr::interlanguagetest::namedTypeCollection2::
                                          ExtendedTypeCollectionEnumerationInTypeCollection::
                                                  ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
        JOYNR_LOG_WARN(
                logger(),
                "methodWithMultipleEnumParameters: invalid input parameter enumerationArg or "
                "extendedEnumerationArg");
        JOYNR_LOG_WARN(logger(), "methodWithMultipleEnumParameters - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "methodWithMultipleEnumParameters: received wrong argument"));
        return;
    }

    joynr::interlanguagetest::namedTypeCollection2::ExtendedEnumerationWithPartlyDefinedValues::Enum
            extendedEnumerationOut;
    joynr::interlanguagetest::Enumeration::Enum enumerationOut;
    extendedEnumerationOut = joynr::interlanguagetest::namedTypeCollection2::
            ExtendedEnumerationWithPartlyDefinedValues::
                    ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
    enumerationOut = joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_1;
    JOYNR_LOG_WARN(logger(), "methodWithMultipleEnumParameters - OK");
    onSuccess(extendedEnumerationOut, enumerationOut);
}

void IltProvider::methodWithSingleStructParameters(
        const joynr::interlanguagetest::namedTypeCollection2::ExtendedBaseStruct&
                extendedBaseStructArg,
        std::function<void(
                const joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives&
                        extendedStructOfPrimitivesOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    JOYNR_LOG_WARN(logger(), "methodWithSingleStructParameters - START");
    if (!IltUtil::checkExtendedBaseStruct(extendedBaseStructArg)) {
        JOYNR_LOG_WARN(
                logger(),
                "methodWithSingleStructParameters: invalid input parameter extendedBaseStructArg");
        JOYNR_LOG_WARN(logger(), "methodWithSingleStructParameters - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "methodWithSingleStructParameters: received wrong argument"));
        return;
    }

    joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives
            extendedStructOfPrimitivesOut = IltUtil::createExtendedStructOfPrimitives();

    JOYNR_LOG_WARN(logger(), "methodWithSingleStructParameters - OK");
    onSuccess(extendedStructOfPrimitivesOut);
}

void IltProvider::methodWithMultipleStructParameters(
        const joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives&
                extendedStructOfPrimitivesArg,
        const joynr::interlanguagetest::namedTypeCollection2::BaseStruct& baseStructArg,
        std::function<void(
                const joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements&
                        baseStructWithoutElementsOut,
                const joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct&
                        extendedExtendedBaseStructOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    // check input parameter
    JOYNR_LOG_WARN(logger(), "methodWithMultipleStructParameters - START");
    if (!IltUtil::checkExtendedStructOfPrimitives(extendedStructOfPrimitivesArg)) {
        JOYNR_LOG_WARN(logger(),
                       "methodWithMultipleStructParameters: invalid input parameter "
                       "extendedStructOfPrimitivesArg");
        JOYNR_LOG_WARN(logger(), "methodWithMultipleStructParameters - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "methodWithMultipleStructParameters: received wrong argument"));
        return;
    }
    if (!IltUtil::checkBaseStruct(baseStructArg)) {
        JOYNR_LOG_WARN(logger(),
                       "methodWithMultipleStructParameters: invalid input parameter baseStructArg");
        JOYNR_LOG_WARN(logger(), "methodWithMultipleStructParameters - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "methodWithMultipleStructParameters: received wrong argument"));
        return;
    }

    // set output values
    joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements
            baseStructWithoutElementsOut;
    joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct
            extendedExtendedBaseStructOut;

    baseStructWithoutElementsOut = IltUtil::createBaseStructWithoutElements();
    extendedExtendedBaseStructOut = IltUtil::createExtendedExtendedBaseStruct();
    if (!IltUtil::checkExtendedExtendedBaseStruct(extendedExtendedBaseStructOut)) {
        JOYNR_LOG_WARN(
                logger(),
                "methodWithMultipleStructParameters - check for extendedExtendedBaseStructOut "
                "FAILED");
    } else {
        JOYNR_LOG_WARN(
                logger(),
                "methodWithMultipleStructParameters - check for extendedExtendedBaseStructOut OK");
    }
    JOYNR_LOG_WARN(logger(), "methodWithMultipleStructParameters - OK");
    onSuccess(baseStructWithoutElementsOut, extendedExtendedBaseStructOut);
}

void IltProvider::methodFireAndForgetWithoutParameter()
{
    setAttributeFireAndForget(this->attributeFireAndForget + 1, nullptr, nullptr);
}

void IltProvider::methodFireAndForgetWithInputParameter(const std::int32_t& int32Arg)
{
    setAttributeFireAndForget(int32Arg, nullptr, nullptr);
}

void IltProvider::overloadedMethod(
        std::function<void(const std::string& stringOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    JOYNR_LOG_WARN(logger(), "overloadedMethod_1 - START");
    std::string stringOut = "TestString 1";
    JOYNR_LOG_WARN(logger(), "overloadedMethod_1 - OK");
    onSuccess(stringOut);
}

void IltProvider::overloadedMethod(
        const bool& booleanArg,
        std::function<void(const std::string& stringOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    JOYNR_LOG_WARN(logger(), "overloadedMethod_2 - START");
    if (booleanArg != false) {
        JOYNR_LOG_WARN(logger(), "overloadedMethod_2: invalid input parameter baseStructArg");
        JOYNR_LOG_WARN(logger(), "overloadedMethod_2 - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "overloadedMethod_2: received wrong argument"));
        return;
    }

    std::string stringOut = "TestString 2";
    JOYNR_LOG_WARN(logger(), "overloadedMethod_2 - OK");
    onSuccess(stringOut);
}

void IltProvider::overloadedMethod(
        const std::vector<
                joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedEnumeration::Enum>&
                enumArrayArg,
        const int64_t& int64Arg,
        const joynr::interlanguagetest::namedTypeCollection2::BaseStruct& baseStructArg,
        const bool& booleanArg,
        std::function<void(const double& doubleOut,
                           const std::vector<std::string>& stringArrayOut,
                           const joynr::interlanguagetest::namedTypeCollection2::ExtendedBaseStruct&
                                   extendedBaseStructOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    // check input parameter
    JOYNR_LOG_WARN(logger(), "overloadedMethod_3 - START");
    if (int64Arg != 1L || booleanArg != false) {
        JOYNR_LOG_WARN(
                logger(), "overloadedMethod_3: invalid input parameter int64Arg or booleanArg");
        JOYNR_LOG_WARN(logger(), "overloadedMethod_3 - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "overloadedMethod_3: received wrong argument"));
        return;
    }

    if (!IltUtil::checkExtendedExtendedEnumerationArray(enumArrayArg)) {
        JOYNR_LOG_WARN(logger(), "overloadedMethod_3 - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "overloadedMethod_3: received wrong argument"));
        return;
    }

    if (!IltUtil::checkBaseStruct(baseStructArg)) {
        JOYNR_LOG_WARN(logger(), "overloadedMethod_3 - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "overloadedMethod_3: received wrong argument"));
        return;
    }

    // setup output parameter
    double doubleOut = 0;
    std::vector<std::string> stringArrayOut = IltUtil::createStringArray();
    joynr::interlanguagetest::namedTypeCollection2::ExtendedBaseStruct extendedBaseStructOut =
            IltUtil::createExtendedBaseStruct();
    JOYNR_LOG_WARN(logger(), "overloadedMethod_3 - OK");
    onSuccess(doubleOut, stringArrayOut, extendedBaseStructOut);
}

void IltProvider::overloadedMethodWithSelector(
        std::function<void(const std::string& stringOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    JOYNR_LOG_WARN(logger(), "overloadedMethodWithSelector_1 - START");
    std::string stringOut = "Return value from overloadedMethodWithSelector 1";
    JOYNR_LOG_WARN(logger(), "overloadedMethodWithSelector_1 - OK");
    onSuccess(stringOut);
}

void IltProvider::overloadedMethodWithSelector(
        const bool& booleanArg,
        std::function<void(const std::string& stringOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    JOYNR_LOG_WARN(logger(), "overloadedMethodWithSelector_2 - START");
    if (booleanArg != false) {
        JOYNR_LOG_WARN(
                logger(), "overloadedMethodWithSelector: invalid input parameter booleanArg");
        JOYNR_LOG_WARN(logger(), "overloadedMethodWithSelector_2 - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "overloadedMethodWithSelector_2: received wrong argument"));
        return;
    }

    // setup output parameter
    std::string stringOut = "Return value from overloadedMethodWithSelector 2";
    JOYNR_LOG_WARN(logger(), "overloadedMethodWithSelector_2 - OK");
    onSuccess(stringOut);
}

void IltProvider::overloadedMethodWithSelector(
        const std::vector<
                joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedEnumeration::Enum>&
                enumArrayArg,
        const int64_t& int64Arg,
        const joynr::interlanguagetest::namedTypeCollection2::BaseStruct& baseStructArg,
        const bool& booleanArg,
        std::function<void(const double& doubleOut,
                           const std::vector<std::string>& stringArrayOut,
                           const joynr::interlanguagetest::namedTypeCollection2::ExtendedBaseStruct&
                                   extendedBaseStructOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    // check input
    JOYNR_LOG_WARN(logger(), "overloadedMethodWithSelector_3 - START");
    if (!IltUtil::checkExtendedExtendedEnumerationArray(enumArrayArg)) {
        JOYNR_LOG_WARN(
                logger(), "overloadedMethodWithSelector: invalid input parameter enumArrayArg");
        JOYNR_LOG_WARN(logger(), "overloadedMethodWithSelector_3 - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "overloadedMethodWithSelector_3: received wrong argument"));
        return;
    }
    if (int64Arg != 1) {
        JOYNR_LOG_WARN(logger(), "overloadedMethodWithSelector: invalid input parameter int64Arg");
        JOYNR_LOG_WARN(logger(), "overloadedMethodWithSelector_3 - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "overloadedMethodWithSelector_3: received wrong argument"));
        return;
    }
    if (!IltUtil::checkBaseStruct(baseStructArg)) {
        JOYNR_LOG_WARN(
                logger(), "overloadedMethodWithSelector: invalid input parameter baseStructArg");
        JOYNR_LOG_WARN(logger(), "overloadedMethodWithSelector_3 - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "overloadedMethodWithSelector_3: received wrong argument"));
        return;
    }
    if (booleanArg != false) {
        JOYNR_LOG_WARN(
                logger(), "overloadedMethodWithSelector: invalid input parameter booleanArg");
        JOYNR_LOG_WARN(logger(), "overloadedMethodWithSelector_3 - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException(
                "overloadedMethodWithSelector_3: received wrong argument"));
        return;
    }

    // prepare output
    double doubleOut = 1.1;
    std::vector<std::string> stringArrayOut = IltUtil::createStringArray();
    joynr::interlanguagetest::namedTypeCollection2::ExtendedBaseStruct extendedBaseStructOut =
            IltUtil::createExtendedBaseStruct();

    // DEBUG
    if (!IltUtil::cmpDouble(doubleOut, 1.1)) {
        JOYNR_LOG_WARN(logger(), "overloadedMethodWithSelector_3 - doubleOut comparison failed");
        onError(joynr::exceptions::ProviderRuntimeException(
                "overloadedMethodWithSelector_3: doubleOut comparison failed"));
        return;
    }
    JOYNR_LOG_WARN(logger(), "overloadedMethodWithSelector_3 - doubleOut comparison OK");
    if (!IltUtil::checkStringArray(stringArrayOut)) {
        JOYNR_LOG_WARN(
                logger(), "overloadedMethodWithSelector_3 - checkStringArray comparison failed");
        onError(joynr::exceptions::ProviderRuntimeException(
                "overloadedMethodWithSelector_3: checkStringArray comparison failed"));
        return;
    }
    JOYNR_LOG_WARN(logger(), "overloadedMethodWithSelector_3 - checkStringArray comparison OK");
    if (!IltUtil::checkExtendedBaseStruct(extendedBaseStructOut)) {
        JOYNR_LOG_WARN(
                logger(),
                "overloadedMethodWithSelector_3 - checkExtendedBaseStruct comparison failed");
        onError(joynr::exceptions::ProviderRuntimeException(
                "overloadedMethodWithSelector_3: checkExtendedBaseStruct comparison failed"));
        return;
    }
    JOYNR_LOG_WARN(
            logger(), "overloadedMethodWithSelector_3 - checkExtendedBaseStruct comparison OK");
    // END DEBUG

    JOYNR_LOG_WARN(logger(), "overloadedMethodWithSelector_3 - OK");
    onSuccess(doubleOut, stringArrayOut, extendedBaseStructOut);
}

void IltProvider::methodWithStringsAndSpecifiedStringOutLength(
        const std::string& stringArg,
        const int32_t& int32StringLengthArg,
        std::function<void(const std::string& stringOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    std::string stringOut;

    JOYNR_LOG_WARN(logger(), "methodWithStringsAndSpecifiedStringOutLength - START");
    if (int32StringLengthArg > 1024 * 1024) {
        JOYNR_LOG_WARN(logger(),
                       "methodWithStringsAndSpecifiedStringOutLength: invalid input parameter "
                       "int32StringLengthArg");
        JOYNR_LOG_WARN(logger(), "methodWithStringsAndSpecifiedStringOutLength - FAILED");
        onError(joynr::exceptions::ProviderRuntimeException("methodWithStringsAndSpecifiedStringOut"
                                                            "Length: checkExtendedBaseStruct "
                                                            "comparison failed"));
        return;
    }

    stringOut.append(int32StringLengthArg, 'A');
    JOYNR_LOG_WARN(logger(), "methodWithStringsAndSpecifiedStringOutLength - OK");
    onSuccess(stringOut);
}

// special methods that return exceptions

void IltProvider::methodWithoutErrorEnum(
        const std::string& wantedExceptionArg,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    JOYNR_LOG_WARN(logger(), "*******************************************");
    JOYNR_LOG_WARN(logger(), "* IltProvider::methodWithoutErrorEnum called");
    JOYNR_LOG_WARN(logger(), "*******************************************");
    if (wantedExceptionArg == "ProviderRuntimeException") {
        onError(joynr::exceptions::ProviderRuntimeException(
                "Exception from methodWithoutErrorEnum"));
    } else {
        onSuccess();
    }
}

void IltProvider::methodWithAnonymousErrorEnum(
        const std::string& wantedExceptionArg,
        std::function<void()> onSuccess,
        std::function<void(const joynr::interlanguagetest::TestInterface::
                                   MethodWithAnonymousErrorEnumErrorEnum::Enum& errorEnum)> onError)
{
    JOYNR_LOG_WARN(logger(), "*************************************************");
    JOYNR_LOG_WARN(logger(), "* IltProvider::methodWithAnonymousErrorEnum called");
    JOYNR_LOG_WARN(logger(), "*************************************************");
    if (wantedExceptionArg == "ProviderRuntimeException") {
        throw joynr::exceptions::ProviderRuntimeException(
                "Exception from methodWithAnonymousErrorEnum");
        // not possible in C++ at the moment due to API restrictions
    } else if (wantedExceptionArg == "ApplicationException") {
        onError(joynr::interlanguagetest::TestInterface::MethodWithAnonymousErrorEnumErrorEnum::
                        ERROR_3_1_NTC);
    } else {
        onSuccess();
    }
}

void IltProvider::methodWithExistingErrorEnum(
        const std::string& wantedExceptionArg,
        std::function<void()> onSuccess,
        std::function<void(const joynr::interlanguagetest::namedTypeCollection2::
                                   ExtendedErrorEnumTc::Enum& errorEnum)> onError)
{
    JOYNR_LOG_WARN(logger(), "************************************************");
    JOYNR_LOG_WARN(logger(), "* IltProvider::methodWithExistingErrorEnum called");
    JOYNR_LOG_WARN(logger(), "************************************************");
    if (wantedExceptionArg == "ProviderRuntimeException") {
        throw joynr::exceptions::ProviderRuntimeException(
                "Exception from methodWithExistingErrorEnum");
        // not possible in C++ at the moment due to API restrictions
    } else if (wantedExceptionArg == "ApplicationException_1") {
        onError(joynr::interlanguagetest::namedTypeCollection2::ExtendedErrorEnumTc::ERROR_2_3_TC2);
    } else if (wantedExceptionArg == "ApplicationException_2") {
        onError(joynr::interlanguagetest::namedTypeCollection2::ExtendedErrorEnumTc::
                        ERROR_1_2_TC_2);
    } else {
        onSuccess();
    }
}

void IltProvider::methodWithExtendedErrorEnum(
        const std::string& wantedExceptionArg,
        std::function<void()> onSuccess,
        std::function<void(const joynr::interlanguagetest::TestInterface::
                                   MethodWithExtendedErrorEnumErrorEnum::Enum& errorEnum)> onError)
{
    JOYNR_LOG_WARN(logger(), "************************************************");
    JOYNR_LOG_WARN(logger(), "* IltProvider::methodWithExtendedErrorEnum called");
    JOYNR_LOG_WARN(logger(), "************************************************");
    if (wantedExceptionArg == "ProviderRuntimeException") {
        throw joynr::exceptions::ProviderRuntimeException(
                "Exception from methodWithExtendedErrorEnum");
        // not possible in C++ at the moment due to API restrictions
    } else if (wantedExceptionArg == "ApplicationException_1") {
        onError(joynr::interlanguagetest::TestInterface::MethodWithExtendedErrorEnumErrorEnum::
                        ERROR_3_3_NTC);
    } else if (wantedExceptionArg == "ApplicationException_2") {
        onError(joynr::interlanguagetest::TestInterface::MethodWithExtendedErrorEnumErrorEnum::
                        ERROR_2_1_TC2);
    } else {
        onSuccess();
    }
}

void IltProvider::methodToFireBroadcastWithSinglePrimitiveParameter(
        const std::vector<std::string>& partitions,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    (void)onError;
    JOYNR_LOG_WARN(
            logger(), "**********************************************************************");
    JOYNR_LOG_WARN(
            logger(), "* IltProvider::methodToFireBroadcastWithSinglePrimitiveParameter called");
    JOYNR_LOG_WARN(
            logger(), "**********************************************************************");
    std::string stringOut = "boom";
    fireBroadcastWithSinglePrimitiveParameter(stringOut, partitions);
    onSuccess();
}

void IltProvider::methodToFireBroadcastWithMultiplePrimitiveParameters(
        const std::vector<std::string>& partitions,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    (void)onError;
    JOYNR_LOG_WARN(
            logger(), "*************************************************************************");
    JOYNR_LOG_WARN(
            logger(), "* IltProvider::methodToFireBroadcastWithMultiplePrimitiveParameters called");
    JOYNR_LOG_WARN(
            logger(), "*************************************************************************");
    double doubleOut = 1.1;
    std::string stringOut = "boom";
    fireBroadcastWithMultiplePrimitiveParameters(doubleOut, stringOut, partitions);
    onSuccess();
}

void IltProvider::methodToFireBroadcastWithSingleArrayParameter(
        const std::vector<std::string>& partitions,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    (void)onError;
    JOYNR_LOG_WARN(logger(), "******************************************************************");
    JOYNR_LOG_WARN(logger(), "* IltProvider::methodToFireBroadcastWithSingleArrayParameter called");
    JOYNR_LOG_WARN(logger(), "******************************************************************");
    std::vector<std::string> stringArrayOut = IltUtil::createStringArray();
    fireBroadcastWithSingleArrayParameter(stringArrayOut, partitions);
    onSuccess();
}

void IltProvider::methodToFireBroadcastWithMultipleArrayParameters(
        const std::vector<std::string>& partitions,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    (void)onError;
    JOYNR_LOG_WARN(
            logger(), "*********************************************************************");
    JOYNR_LOG_WARN(
            logger(), "* IltProvider::methodToFireBroadcastWithMultipleArrayParameters called");
    JOYNR_LOG_WARN(
            logger(), "*********************************************************************");
    std::vector<uint64_t> uInt64ArrayOut = IltUtil::createUInt64Array();
    std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>
            structWithStringArrayArrayOut = IltUtil::createStructWithStringArrayArray();
    fireBroadcastWithMultipleArrayParameters(
            uInt64ArrayOut, structWithStringArrayArrayOut, partitions);
    onSuccess();
}

void IltProvider::methodToFireBroadcastWithSingleByteBufferParameter(
        const joynr::ByteBuffer& byteBufferIn,
        const std::vector<std::string>& partitions,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    std::ignore = onError;
    JOYNR_LOG_INFO(
            logger(), "***********************************************************************");
    JOYNR_LOG_INFO(
            logger(), "* IltProvider::methodToFireBroadcastWithSingleByteBufferParameter called");
    JOYNR_LOG_INFO(
            logger(), "***********************************************************************");
    fireBroadcastWithSingleByteBufferParameter(byteBufferIn, partitions);
    onSuccess();
}

void IltProvider::methodToFireBroadcastWithMultipleByteBufferParameters(
        const joynr::ByteBuffer& byteBufferIn1,
        const joynr::ByteBuffer& byteBufferIn2,
        const std::vector<std::string>& partitions,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    std::ignore = onError;
    JOYNR_LOG_INFO(
            logger(), "**************************************************************************");
    JOYNR_LOG_INFO(logger(),
                   "* IltProvider::methodToFireBroadcastWithMultipleByteBufferParameters called");
    JOYNR_LOG_INFO(
            logger(), "**************************************************************************");
    fireBroadcastWithMultipleByteBufferParameters(byteBufferIn1, byteBufferIn2, partitions);
    onSuccess();
}

void IltProvider::methodToFireBroadcastWithSingleEnumerationParameter(
        const std::vector<std::string>& partitions,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    (void)onError;
    JOYNR_LOG_WARN(
            logger(), "************************************************************************");
    JOYNR_LOG_WARN(
            logger(), "* IltProvider::methodToFireBroadcastWithSingleEnumerationParameter called");
    JOYNR_LOG_WARN(
            logger(), "************************************************************************");
    joynr::interlanguagetest::namedTypeCollection2::
            ExtendedTypeCollectionEnumerationInTypeCollection::Enum enumerationOut =
                    joynr::interlanguagetest::namedTypeCollection2::
                            ExtendedTypeCollectionEnumerationInTypeCollection::
                                    ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
    fireBroadcastWithSingleEnumerationParameter(enumerationOut, partitions);
    onSuccess();
}

void IltProvider::methodToFireBroadcastWithMultipleEnumerationParameters(
        const std::vector<std::string>& partitions,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    (void)onError;
    JOYNR_LOG_WARN(logger(),
                   "***************************************************************************");
    JOYNR_LOG_WARN(logger(),
                   "* IltProvider::methodToFireBroadcastWithMultipleEnumerationParameters called");
    JOYNR_LOG_WARN(logger(),
                   "***************************************************************************");

    joynr::interlanguagetest::namedTypeCollection2::ExtendedEnumerationWithPartlyDefinedValues::Enum
            extendedEnumerationOut;
    joynr::interlanguagetest::Enumeration::Enum enumerationOut;
    extendedEnumerationOut = joynr::interlanguagetest::namedTypeCollection2::
            ExtendedEnumerationWithPartlyDefinedValues::
                    ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
    enumerationOut = joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_1;
    fireBroadcastWithMultipleEnumerationParameters(
            extendedEnumerationOut, enumerationOut, partitions);
    onSuccess();
}

void IltProvider::methodToFireBroadcastWithSingleStructParameter(
        const std::vector<std::string>& partitions,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    (void)onError;
    JOYNR_LOG_WARN(logger(), "*******************************************************************");
    JOYNR_LOG_WARN(
            logger(), "* IltProvider::methodToFireBroadcastWithSingleStructParameter called");
    JOYNR_LOG_WARN(logger(), "*******************************************************************");
    joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives
            extendedStructOfPrimitivesOut = IltUtil::createExtendedStructOfPrimitives();
    fireBroadcastWithSingleStructParameter(extendedStructOfPrimitivesOut, partitions);
    onSuccess();
}

void IltProvider::methodToFireBroadcastWithMultipleStructParameters(
        const std::vector<std::string>& partitions,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    (void)onError;
    JOYNR_LOG_WARN(
            logger(), "**********************************************************************");
    JOYNR_LOG_WARN(
            logger(), "* IltProvider::methodToFireBroadcastWithMultipleStructParameters called");
    JOYNR_LOG_WARN(
            logger(), "**********************************************************************");
    joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements
            baseStructWithoutElementsOut;
    joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct
            extendedExtendedBaseStructOut;

    baseStructWithoutElementsOut = IltUtil::createBaseStructWithoutElements();
    extendedExtendedBaseStructOut = IltUtil::createExtendedExtendedBaseStruct();
    fireBroadcastWithMultipleStructParameters(
            baseStructWithoutElementsOut, extendedExtendedBaseStructOut, partitions);
    onSuccess();
}

void IltProvider::methodToFireBroadcastWithFiltering(
        const std::string& stringArg,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    (void)onError;
    JOYNR_LOG_WARN(logger(), "*******************************************************");
    JOYNR_LOG_WARN(logger(), "* IltProvider::methodToFireBroadcastWithFiltering called");
    JOYNR_LOG_WARN(logger(), "*******************************************************");

    std::string stringOut = stringArg;
    std::vector<std::string> stringArrayOut = IltUtil::createStringArray();
    joynr::interlanguagetest::namedTypeCollection2::
            ExtendedTypeCollectionEnumerationInTypeCollection::Enum enumerationOut =
                    joynr::interlanguagetest::namedTypeCollection2::
                            ExtendedTypeCollectionEnumerationInTypeCollection::
                                    ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
    joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray structWithStringArrayOut =
            IltUtil::createStructWithStringArray();
    ;
    std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>
            structWithStringArrayArrayOut = IltUtil::createStructWithStringArrayArray();

    fireBroadcastWithFiltering(stringOut,
                               stringArrayOut,
                               enumerationOut,
                               structWithStringArrayOut,
                               structWithStringArrayArrayOut);
    onSuccess();
}

// use default implementation for standard getter and setter
// which just returns the current value of the attribute

// use special implementation for getter to readonly attributes
// since there is no setter which could be called prior to set value

void IltProvider::getAttributeInt8readonlyNoSubscriptions(
        std::function<void(const int8_t&)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    (void)onError;
    attributeInt8readonlyNoSubscriptions = -128;
    onSuccess(attributeInt8readonlyNoSubscriptions);
}

void IltProvider::getAttributeBooleanReadonly(
        std::function<void(const bool&)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    (void)onError;
    attributeBooleanReadonly = true;
    onSuccess(attributeBooleanReadonly);
}

void IltProvider::getAttributeExtendedEnumerationReadonly(
        std::function<void(const joynr::interlanguagetest::namedTypeCollection2::
                                   ExtendedEnumerationWithPartlyDefinedValues::Enum&)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    (void)onError;
    attributeExtendedEnumerationReadonly = joynr::interlanguagetest::namedTypeCollection2::
            ExtendedEnumerationWithPartlyDefinedValues::
                    ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
    onSuccess(attributeExtendedEnumerationReadonly);
}

void IltProvider::getAttributeFireAndForget(
        std::function<void(const std::int32_t&)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    onSuccess(attributeFireAndForget);
}

void IltProvider::setAttributeFireAndForget(
        const std::int32_t& attributeFireAndForget,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    this->attributeFireAndForget = attributeFireAndForget;
    attributeFireAndForgetChanged(attributeFireAndForget);
    if (onSuccess) {
        onSuccess();
    }
}

// attribute with exception

void IltProvider::getAttributeWithExceptionFromGetter(
        std::function<void(const bool&)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    onError(joynr::exceptions::ProviderRuntimeException(
            "Exception from getAttributeWithExceptionFromGetter"));
}

void IltProvider::getAttributeWithExceptionFromSetter(
        std::function<void(const bool&)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    std::ignore = onError;
    onSuccess(false);
}

void IltProvider::setAttributeWithExceptionFromSetter(
        const bool& attributeWithExceptionFromSetter,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
{
    std::ignore = attributeWithExceptionFromSetter;
    std::ignore = onSuccess;
    onError(joynr::exceptions::ProviderRuntimeException(
            "Exception from setAttributeWithExceptionFromSetter"));
}

void IltProvider::methodWithSingleMapParameters(
        const joynr::interlanguagetest::namedTypeCollection2::MapStringString& mapArg,
        std::function<void(const joynr::interlanguagetest::namedTypeCollection2::MapStringString&
                                   mapOut)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    joynr::interlanguagetest::namedTypeCollection2::MapStringString mapOut;
    for (auto const& entry : mapArg) {
        mapOut.insert(std::pair<std::string, std::string>(entry.second, entry.first));
    }
    onSuccess(mapOut);
}
