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
#include "IltAbstractConsumerTest.h"

using namespace ::testing;

class IltConsumerSyncMethodTest : public IltAbstractConsumerTest<::testing::Test>
{
public:
    IltConsumerSyncMethodTest() = default;

    template <typename AttributeType>
    void callProxyMethodWithParameterAndAssertResult(
            std::function<void(joynr::interlanguagetest::TestInterfaceSyncProxy*,
                               AttributeType&,
                               const AttributeType&,
                               boost::optional<joynr::MessagingQos>)> testMethod,
            AttributeType expectedResult);
};

template <typename AttributeType>
void IltConsumerSyncMethodTest::callProxyMethodWithParameterAndAssertResult(
        std::function<void(joynr::interlanguagetest::TestInterfaceSyncProxy*,
                           AttributeType&,
                           const AttributeType&,
                           boost::optional<joynr::MessagingQos>)> testMethod,
        AttributeType expectedResult)
{
    AttributeType result;
    JOYNR_ASSERT_NO_THROW(testMethod(testInterfaceProxy.get(), result, expectedResult, {}));
    ASSERT_EQ(result, expectedResult);
}

// no check possible other than handling exceptions
TEST_F(IltConsumerSyncMethodTest, callMethodWithoutParameters)
{
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->methodWithoutParameters());
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithoutInputParameter)
{
    bool out = false;
    bool expectedOut = true;
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->methodWithoutInputParameter(out));
    ASSERT_EQ(expectedOut, out);
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithoutOutputParameter)
{
    bool arg = false;
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->methodWithoutOutputParameter(arg));
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithSinglePrimitiveParameters)
{
    uint16_t arg = 32767;
    std::string out;
    std::string expectedOut = std::to_string(arg);
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->methodWithSinglePrimitiveParameters(out, arg));
    ASSERT_EQ(expectedOut, out);
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithMultiplePrimitiveParameters)
{
    int32_t arg1 = 2147483647;
    float arg2 = 47.11;
    bool arg3 = false;
    double doubleOut;
    std::string stringOut;
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->methodWithMultiplePrimitiveParameters(
            doubleOut, stringOut, arg1, arg2, arg3));
    ASSERT_TRUE(IltUtil::cmpDouble(doubleOut, arg2));
    ASSERT_EQ(stringOut, std::to_string(arg1));
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithSingleByteBufferParameter)
{
    callProxyMethodWithParameterAndAssertResult<joynr::ByteBuffer>(
            &joynr::interlanguagetest::TestInterfaceProxy::methodWithSingleByteBufferParameter,
            {0x00, 0x64, 0xFF});
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithMultipleByteBufferParameters)
{
    joynr::ByteBuffer byteBufferIn1 = {0x05, 0x7D};
    joynr::ByteBuffer byteBufferIn2 = {0x4E, 0x00};
    joynr::ByteBuffer byteBufferOut;
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->methodWithMultipleByteBufferParameters(
            byteBufferOut, byteBufferIn1, byteBufferIn2));
    ASSERT_EQ(byteBufferOut, IltUtil::concatByteBuffers(byteBufferIn1, byteBufferIn2));
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithInt64TypeDefParameter)
{
    callProxyMethodWithParameterAndAssertResult<std::int64_t>(
            &joynr::interlanguagetest::TestInterfaceProxy::methodWithInt64TypeDefParameter, 1L);
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithStringTypeDefParameter)
{
    callProxyMethodWithParameterAndAssertResult<std::string>(
            &joynr::interlanguagetest::TestInterfaceProxy::methodWithStringTypeDefParameter,
            "TypeDefTestString");
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithStructTypeDefParameter)
{
    callProxyMethodWithParameterAndAssertResult<
            joynr::interlanguagetest::namedTypeCollection2::BaseStruct>(
            &joynr::interlanguagetest::TestInterfaceProxy::methodWithStructTypeDefParameter,
            IltUtil::createBaseStruct());
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithMapTypeDefParameter)
{
    joynr::interlanguagetest::namedTypeCollection2::MapStringString mapTypeDefArg;
    mapTypeDefArg.insert(std::pair<std::string, std::string>("keyString1", "valueString1"));
    mapTypeDefArg.insert(std::pair<std::string, std::string>("keyString2", "valueString2"));
    mapTypeDefArg.insert(std::pair<std::string, std::string>("keyString3", "valueString3"));

    callProxyMethodWithParameterAndAssertResult<
            joynr::interlanguagetest::namedTypeCollection2::MapStringString>(
            &joynr::interlanguagetest::TestInterfaceProxy::methodWithMapTypeDefParameter,
            mapTypeDefArg);
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithEnumTypeDefParameter)
{
    callProxyMethodWithParameterAndAssertResult<joynr::interlanguagetest::Enumeration::Enum>(
            &joynr::interlanguagetest::TestInterfaceProxy::methodWithEnumTypeDefParameter,
            joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_1);
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithByteBufferTypeDefParameter)
{
    callProxyMethodWithParameterAndAssertResult<joynr::ByteBuffer>(
            &joynr::interlanguagetest::TestInterfaceProxy::methodWithByteBufferTypeDefParameter,
            {0x00, 0x64, 0xFF});
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithArrayTypeDefParameter)
{
    std::vector<std::string> stringArray = IltUtil::createStringArray();
    joynr::interlanguagetest::typeDefCollection::ArrayTypeDefStruct arrayTypeDefArg;
    arrayTypeDefArg.setTypeDefStringArray(stringArray);

    callProxyMethodWithParameterAndAssertResult<
            joynr::interlanguagetest::typeDefCollection::ArrayTypeDefStruct>(
            &joynr::interlanguagetest::TestInterfaceProxy::methodWithArrayTypeDefParameter,
            arrayTypeDefArg);
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithSingleArrayParameters)
{
    std::vector<double> arg = IltUtil::createDoubleArray();
    std::vector<std::string> out;
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->methodWithSingleArrayParameters(out, arg));
    ASSERT_TRUE(IltUtil::checkStringArray(out));
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithMultipleArrayParameters)
{
    std::vector<uint64_t> uInt64ArrayOut;
    std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>
            structWithStringArrayArrayOut;
    std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>
            structWithStringArrayArrayArg = IltUtil::createStructWithStringArrayArray();
    std::vector<std::string> stringArrayArg = IltUtil::createStringArray();
    std::vector<int8_t> int8ArrayArg = IltUtil::createByteArray();
    std::vector<joynr::interlanguagetest::namedTypeCollection2::
                        ExtendedInterfaceEnumerationInTypeCollection::Enum>
            enumArrayArg = IltUtil::createExtendedInterfaceEnumerationInTypeCollectionArray();
    std::vector<uint64_t> expectedUInt64ArrayOut =
            IltUtil::convertInt8ArrayToUInt64Array(int8ArrayArg);

    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->methodWithMultipleArrayParameters(
            // output param
            uInt64ArrayOut,
            structWithStringArrayArrayOut,
            // input param
            stringArrayArg,
            int8ArrayArg,
            enumArrayArg,
            structWithStringArrayArrayArg));
    // check output parameter
    // check will only work, if there are no negative values in int8ArrayArg
    ASSERT_THAT(uInt64ArrayOut, testing::ContainerEq(expectedUInt64ArrayOut));
    ASSERT_TRUE(IltUtil::checkStructWithStringArrayArray(structWithStringArrayArrayOut));
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithSingleEnumParameters)
{
    joynr::interlanguagetest::namedTypeCollection2::
            ExtendedTypeCollectionEnumerationInTypeCollection::Enum enumerationOut;
    joynr::interlanguagetest::namedTypeCollection2::ExtendedEnumerationWithPartlyDefinedValues::Enum
            enumerationArg = joynr::interlanguagetest::namedTypeCollection2::
                    ExtendedEnumerationWithPartlyDefinedValues::
                            ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
    JOYNR_ASSERT_NO_THROW(
            testInterfaceProxy->methodWithSingleEnumParameters(enumerationOut, enumerationArg););
    ASSERT_EQ(enumerationOut,
              joynr::interlanguagetest::namedTypeCollection2::
                      ExtendedTypeCollectionEnumerationInTypeCollection::
                              ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION);
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithMultipleEnumParameters)
{
    joynr::interlanguagetest::namedTypeCollection2::ExtendedEnumerationWithPartlyDefinedValues::Enum
            extendedEnumerationOut;
    joynr::interlanguagetest::Enumeration::Enum enumerationOut;
    joynr::interlanguagetest::Enumeration::Enum enumerationArg;
    joynr::interlanguagetest::namedTypeCollection2::
            ExtendedTypeCollectionEnumerationInTypeCollection::Enum extendedEnumerationArg;

    enumerationArg = joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_3;
    extendedEnumerationArg = joynr::interlanguagetest::namedTypeCollection2::
            ExtendedTypeCollectionEnumerationInTypeCollection::
                    ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->methodWithMultipleEnumParameters(
            extendedEnumerationOut, enumerationOut, enumerationArg, extendedEnumerationArg));
    ASSERT_EQ(enumerationOut, joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_1);
    ASSERT_EQ(extendedEnumerationOut,
              joynr::interlanguagetest::namedTypeCollection2::
                      ExtendedEnumerationWithPartlyDefinedValues::
                              ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES);
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithSingleStructParameters)
{
    joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives
            extendedStructOfPrimitivesOut;
    joynr::interlanguagetest::namedTypeCollection2::ExtendedBaseStruct extendedBaseStructArg;

    extendedBaseStructArg = IltUtil::createExtendedBaseStruct();

    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->methodWithSingleStructParameters(
            extendedStructOfPrimitivesOut, extendedBaseStructArg));
    ASSERT_TRUE(IltUtil::checkExtendedStructOfPrimitives(extendedStructOfPrimitivesOut));
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithMultipleStructParameters)
{
    joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements
            baseStructWithoutElementsOut;
    joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct
            extendedExtendedBaseStructOut;
    joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives
            extendedStructOfPrimitivesArg;
    joynr::interlanguagetest::namedTypeCollection2::BaseStruct baseStructArg;

    extendedStructOfPrimitivesArg = IltUtil::createExtendedStructOfPrimitives();
    baseStructArg = IltUtil::createBaseStruct();

    JOYNR_ASSERT_NO_THROW(
            testInterfaceProxy->methodWithMultipleStructParameters(baseStructWithoutElementsOut,
                                                                   extendedExtendedBaseStructOut,
                                                                   extendedStructOfPrimitivesArg,
                                                                   baseStructArg));
    ASSERT_TRUE(IltUtil::checkBaseStructWithoutElements(baseStructWithoutElementsOut));
    ASSERT_TRUE(IltUtil::checkExtendedExtendedBaseStruct(extendedExtendedBaseStructOut));
}

TEST_F(IltConsumerSyncMethodTest, callOverloadedMethod_1)
{
    std::string stringOut;
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->overloadedMethod(stringOut));
    ASSERT_EQ(stringOut, "TestString 1");
}

TEST_F(IltConsumerSyncMethodTest, callOverloadedMethod_2)
{
    std::string stringOut;
    bool booleanArg = false;
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->overloadedMethod(stringOut, booleanArg));
    ASSERT_EQ(stringOut, "TestString 2");
}

TEST_F(IltConsumerSyncMethodTest, callOverloadedMethod_3)
{
    double doubleOut;
    std::vector<std::string> stringArrayOut;
    joynr::interlanguagetest::namedTypeCollection2::ExtendedBaseStruct extendedBaseStructOut;
    std::vector<joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedEnumeration::Enum>
            enumArrayArg;
    int64_t int64Arg;
    joynr::interlanguagetest::namedTypeCollection2::BaseStruct baseStructArg;
    bool booleanArg;

    enumArrayArg = IltUtil::createExtendedExtendedEnumerationArray();
    int64Arg = 1L;
    baseStructArg = IltUtil::createBaseStruct();
    booleanArg = false;

    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->overloadedMethod(doubleOut,
                                                               stringArrayOut,
                                                               extendedBaseStructOut,
                                                               enumArrayArg,
                                                               int64Arg,
                                                               baseStructArg,
                                                               booleanArg));
    ASSERT_EQ(doubleOut, 0);
    ASSERT_TRUE(IltUtil::checkStringArray(stringArrayOut));
    ASSERT_TRUE(IltUtil::checkExtendedBaseStruct(extendedBaseStructOut));
}

TEST_F(IltConsumerSyncMethodTest, callOverloadedMethodWithSelector_1)
{
    std::string stringOut;
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->overloadedMethodWithSelector(stringOut));
    ASSERT_EQ(stringOut, "Return value from overloadedMethodWithSelector 1");
}

TEST_F(IltConsumerSyncMethodTest, callOverloadedMethodWithSelector_2)
{
    std::string stringOut;
    bool booleanArg = false;
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->overloadedMethodWithSelector(stringOut, booleanArg));
    ASSERT_EQ(stringOut, "Return value from overloadedMethodWithSelector 2");
}

TEST_F(IltConsumerSyncMethodTest, callOverloadedMethodWithSelector_3)
{
    // output
    double doubleOut;
    std::vector<std::string> stringArrayOut;
    joynr::interlanguagetest::namedTypeCollection2::ExtendedBaseStruct extendedBaseStructOut;
    // input
    const std::vector<
            joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedEnumeration::Enum>
            enumArrayArg = IltUtil::createExtendedExtendedEnumerationArray();
    int64_t int64Arg = 1L;
    joynr::interlanguagetest::namedTypeCollection2::BaseStruct baseStructArg =
            IltUtil::createBaseStruct();
    bool booleanArg = false;

    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->overloadedMethodWithSelector(doubleOut,
                                                                           stringArrayOut,
                                                                           extendedBaseStructOut,
                                                                           enumArrayArg,
                                                                           int64Arg,
                                                                           baseStructArg,
                                                                           booleanArg));
    ASSERT_TRUE(IltUtil::cmpDouble(doubleOut, 1.1));
    ASSERT_TRUE(IltUtil::checkExtendedBaseStruct(extendedBaseStructOut));
    ASSERT_TRUE(IltUtil::checkStringArray(stringArrayOut));
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithStringsAndSpecifiedStringOutLength)
{
    std::string stringOut;
    std::string stringArg = "Hello world";
    int32_t int32StringLengthArg = 32;
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->methodWithStringsAndSpecifiedStringOutLength(
            stringOut, stringArg, int32StringLengthArg));
    ASSERT_EQ(stringOut.length(), 32);
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithoutErrorEnum)
{
    try {
        std::string wantedException = "ProviderRuntimeException";
        testInterfaceProxy->methodWithoutErrorEnum(wantedException);
    } catch (joynr::exceptions::ProviderRuntimeException& e) {
        ASSERT_EQ(e.getMessage(), "Exception from methodWithoutErrorEnum");
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        FAIL() << "callMethodWithoutErrorEnum - unexpected exception type" << e.getMessage();
    } catch (...) {
        FAIL() << "callMethodWithoutErrorEnum: unknown exception caught";
    }
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithAnonymousErrorEnum)
{
#ifdef USE_PROVIDER_RUNTIME_EXCEPTION
    try {
        std::string wantedException = "ProviderRuntimeException";
        testInterfaceProxy->methodWithAnonymousErrorEnum(wantedException);
    } catch (joynr::exceptions::ProviderRuntimeException& e) {
        ASSERT_EQ(e.getMessage(), "Exception from methodWithAnonymousErrorEnum");
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        FAIL() << "callMethodWithAnonymousErrorEnum - 1st - unexpected exception type"
               << e.getMessage();
    } catch (...) {
        FAIL() << "callMethodWithAnonymousErrorEnum: unknown exception caught";
    }
#endif

    // 2nd test
    try {
        std::string wantedException = "ApplicationException";
        testInterfaceProxy->methodWithAnonymousErrorEnum(wantedException);
    } catch (joynr::exceptions::ApplicationException& e) {
        ASSERT_EQ(e.getError<joynr::interlanguagetest::TestInterface::
                                     MethodWithAnonymousErrorEnumErrorEnum::Enum>(),
                  joynr::interlanguagetest::TestInterface::MethodWithAnonymousErrorEnumErrorEnum::
                          ERROR_3_1_NTC);
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        FAIL() << "callMethodWithAnonymousErrorEnum - 2nd - unexpected exception type"
               << e.getMessage();
    } catch (...) {
        FAIL() << "callMethodWithAnonymousErrorEnum: unknown exception caught";
    }
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithExistingErrorEnum)
{
#ifdef USE_PROVIDER_RUNTIME_EXCEPTION
    try {
        std::string wantedException = "ProviderRuntimeException";
        testInterfaceProxy->methodWithExistingErrorEnum(wantedException);
    } catch (joynr::exceptions::ProviderRuntimeException& e) {
        ASSERT_EQ(e.getMessage(), "Exception from methodWithExistingErrorEnum");
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        FAIL() << "callMethodWithExistingErrorEnum - 1st - unexpected exception type"
               << e.getMessage();
    } catch (...) {
        FAIL() << "callMethodWithExistingErrorEnum: unknown exception caught";
    }
#endif

    // 2nd test
    try {
        std::string wantedException = "ApplicationException_1";
        testInterfaceProxy->methodWithExistingErrorEnum(wantedException);
    } catch (joynr::exceptions::ApplicationException& e) {
        ASSERT_EQ(
                e.getError<joynr::interlanguagetest::namedTypeCollection2::ExtendedErrorEnumTc::
                                   Enum>(),
                joynr::interlanguagetest::namedTypeCollection2::ExtendedErrorEnumTc::ERROR_2_3_TC2);
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        FAIL() << "callMethodWithExistingErrorEnum - 2nd - unexpected exception type"
               << e.getMessage();
    } catch (...) {
        FAIL() << "callMethodWithExistingErrorEnum: unknown exception caught";
    }

    // 3rd test
    try {
        std::string wantedException = "ApplicationException_2";
        testInterfaceProxy->methodWithExistingErrorEnum(wantedException);
    } catch (joynr::exceptions::ApplicationException& e) {
        ASSERT_EQ(e.getError<joynr::interlanguagetest::namedTypeCollection2::ExtendedErrorEnumTc::
                                     Enum>(),
                  joynr::interlanguagetest::namedTypeCollection2::ExtendedErrorEnumTc::
                          ERROR_1_2_TC_2);
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        FAIL() << "callMethodWithExistingErrorEnum - 3rd - unexpected exception type"
               << e.getMessage();
    } catch (...) {
        FAIL() << "callMethodWithExistingErrorEnum: unknown exception caught";
    }
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithExtendedErrorEnum)
{
#ifdef USE_PROVIDER_RUNTIME_EXCEPTION
    try {
        std::string wantedException = "ProviderRuntimeException";
        testInterfaceProxy->methodWithExtendedErrorEnum(wantedException);
    } catch (joynr::exceptions::ProviderRuntimeException& e) {
        ASSERT_EQ(e.getMessage(), "Exception from methodWithExtendedErrorEnum");
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        FAIL() << "callMethodWithExtendedErrorEnum - 1st - unexpected exception type"
               << e.getMessage();
    } catch (...) {
        FAIL() << "callMethodWithExtendedErrorEnum: unknown exception caught";
    }
#endif

    // 2nd test
    try {
        std::string wantedException = "ApplicationException_1";
        testInterfaceProxy->methodWithExtendedErrorEnum(wantedException);
    } catch (joynr::exceptions::ApplicationException& e) {
        ASSERT_EQ(e.getError<joynr::interlanguagetest::TestInterface::
                                     MethodWithExtendedErrorEnumErrorEnum::Enum>(),
                  joynr::interlanguagetest::TestInterface::MethodWithExtendedErrorEnumErrorEnum::
                          ERROR_3_3_NTC);
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        FAIL() << "callMethodWithExtendedErrorEnum - 2nd - unexpected exception type"
               << e.getMessage();
    } catch (...) {
        FAIL() << "callMethodWithExtendedErrorEnum: unknown exception caught";
    }

    // 3rd test
    try {
        std::string wantedException = "ApplicationException_2";
        testInterfaceProxy->methodWithExtendedErrorEnum(wantedException);
    } catch (joynr::exceptions::ApplicationException& e) {
        ASSERT_EQ(e.getError<joynr::interlanguagetest::TestInterface::
                                     MethodWithExtendedErrorEnumErrorEnum::Enum>(),
                  joynr::interlanguagetest::TestInterface::MethodWithExtendedErrorEnumErrorEnum::
                          ERROR_2_1_TC2);
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        FAIL() << "callMethodWithExtendedErrorEnum - 3rd - unexpected exception type"
               << e.getMessage();
    } catch (...) {
        FAIL() << "callMethodWithExtendedErrorEnum: unknown exception caught";
    }
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithTypeDefForMapParameter)
{
    using testType = joynr::interlanguagetest::typeDefCollection::TypeDefForMap;
    testType expectedResult;
    expectedResult.insert(std::pair<std::string, std::string>("keyString1", "valueString1"));
    expectedResult.insert(std::pair<std::string, std::string>("keyString2", "valueString2"));
    expectedResult.insert(std::pair<std::string, std::string>("keyString3", "valueString3"));

    callProxyMethodWithParameterAndAssertResult<testType>(
            &joynr::interlanguagetest::TestInterfaceProxy::methodWithMapTypeDefParameter,
            expectedResult);
}

TEST_F(IltConsumerSyncMethodTest, callMethodWithSingleMapParameters)
{
    typedef std::map<std::string, std::string>::iterator myIterator;
    joynr::interlanguagetest::namedTypeCollection2::MapStringString arg;
    arg.insert(std::pair<std::string, std::string>("keyString1", "valueString1"));
    arg.insert(std::pair<std::string, std::string>("keyString2", "valueString2"));
    arg.insert(std::pair<std::string, std::string>("keyString3", "valueString3"));
    joynr::interlanguagetest::namedTypeCollection2::MapStringString result;
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->methodWithSingleMapParameters(result, arg));

    // std::map<std::string, std::string>::iterator it;
    myIterator it;
    for (int i = 1; i <= 3; i++) {
        it = result.find("valueString" + std::to_string(i));
        ASSERT_NE(it, result.end());
        std::string expected = "keyString" + std::to_string(i);
        ASSERT_EQ(it->second, expected);
    }
}
