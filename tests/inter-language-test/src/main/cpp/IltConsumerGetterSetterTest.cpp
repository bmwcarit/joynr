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

bool IltUtil::useRestricted64BitRange = true;
bool IltUtil::useRestrictedUnsignedRange = true;

// joynr::Logger logger("IltAbstractConsumerTest");

class IltConsumerGetterSetterTest : public IltAbstractConsumerTest<::testing::Test>
{
public:
    IltConsumerGetterSetterTest() = default;
};

TEST_F(IltConsumerGetterSetterTest, callSetAttributeUInt8)
{
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->setAttributeUInt8(127));
}

TEST_F(IltConsumerGetterSetterTest, callGetAttributeUInt8)
{
    uint8_t expectedResult = 127;
    uint8_t result = 0;
    JOYNR_ASSERT_NO_THROW({
        testInterfaceProxy->setAttributeUInt8(expectedResult);
        testInterfaceProxy->getAttributeUInt8(result);
    });
    ASSERT_EQ(result, expectedResult);
}

TEST_F(IltConsumerGetterSetterTest, callSetAttributeDouble)
{
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->setAttributeDouble(1.1));
}

TEST_F(IltConsumerGetterSetterTest, callGetAttributeDouble)
{
    double result = 0;
    double expectedResult = 1.1;
    JOYNR_ASSERT_NO_THROW({
        testInterfaceProxy->setAttributeDouble(expectedResult);
        testInterfaceProxy->getAttributeDouble(result);
    });
    ASSERT_TRUE(IltUtil::cmpDouble(result, expectedResult));
}

TEST_F(IltConsumerGetterSetterTest, callGetAttributeBooleanReadonly)
{
    bool result;
    JOYNR_ASSERT_NO_THROW({ testInterfaceProxy->getAttributeBooleanReadonly(result); });
    ASSERT_TRUE(result);
}

TEST_F(IltConsumerGetterSetterTest, callSetAttributeStringNoSubscriptions)
{
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->setAttributeStringNoSubscriptions("Hello world"));
}

TEST_F(IltConsumerGetterSetterTest, callGetAttributeStringNoSubscriptions)
{
    std::string result;
    std::string expectedResult = "Hello world";
    JOYNR_ASSERT_NO_THROW({
        testInterfaceProxy->setAttributeStringNoSubscriptions(expectedResult);
        testInterfaceProxy->getAttributeStringNoSubscriptions(result);
    });
    ASSERT_EQ(result, expectedResult);
}

TEST_F(IltConsumerGetterSetterTest, callGetAttributeInt8readonlyNoSubscriptions)
{
    int8_t result;
    int8_t expectedResult = -128;
    JOYNR_ASSERT_NO_THROW({ testInterfaceProxy->getAttributeInt8readonlyNoSubscriptions(result); });
    ASSERT_EQ(result, expectedResult);
}

TEST_F(IltConsumerGetterSetterTest, callGetAttributeByteBuffer)
{
    joynr::ByteBuffer result;
    joynr::ByteBuffer expectedResult = {0, 100, 255};
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->setAttributeByteBuffer(expectedResult));
    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->getAttributeByteBuffer(result));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(IltConsumerGetterSetterTest, callSetAttributeArrayOfStringImplicit)
{
    std::vector<std::string> stringArrayArg = IltUtil::createStringArray();
    JOYNR_ASSERT_NO_THROW(
            { testInterfaceProxy->setAttributeArrayOfStringImplicit(stringArrayArg); });
}

TEST_F(IltConsumerGetterSetterTest, callGetAttributeArrayOfStringImplicit)
{
    std::vector<std::string> result;
    std::vector<std::string> stringArrayArg = IltUtil::createStringArray();
    JOYNR_ASSERT_NO_THROW({
        testInterfaceProxy->setAttributeArrayOfStringImplicit(stringArrayArg);
        testInterfaceProxy->getAttributeArrayOfStringImplicit(result);
    });
    ASSERT_TRUE(IltUtil::checkStringArray(result));
}

TEST_F(IltConsumerGetterSetterTest, callSetAttributeEnumeration)
{
    JOYNR_ASSERT_NO_THROW({
        joynr::interlanguagetest::Enumeration::Enum enumerationArg =
                joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_2;
        testInterfaceProxy->setAttributeEnumeration(enumerationArg);
    });
}

TEST_F(IltConsumerGetterSetterTest, callGetAttributeEnumeration)
{
    joynr::interlanguagetest::Enumeration::Enum result;
    joynr::interlanguagetest::Enumeration::Enum enumerationArg =
            joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_2;
    JOYNR_ASSERT_NO_THROW({
        testInterfaceProxy->setAttributeEnumeration(enumerationArg);
        testInterfaceProxy->getAttributeEnumeration(result);
    });
    ASSERT_EQ(result, enumerationArg);
}

TEST_F(IltConsumerGetterSetterTest, callGetAttributeExtendedEnumerationReadonly)
{
    joynr::interlanguagetest::namedTypeCollection2::ExtendedEnumerationWithPartlyDefinedValues::Enum
            result;
    JOYNR_ASSERT_NO_THROW({ testInterfaceProxy->getAttributeExtendedEnumerationReadonly(result); });
    ASSERT_EQ(result,
              joynr::interlanguagetest::namedTypeCollection2::
                      ExtendedEnumerationWithPartlyDefinedValues::
                              ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES);
}

TEST_F(IltConsumerGetterSetterTest, callSetAttributeBaseStruct)
{
    joynr::interlanguagetest::namedTypeCollection2::BaseStruct baseStructArg =
            IltUtil::createBaseStruct();
    JOYNR_ASSERT_NO_THROW({ testInterfaceProxy->setAttributeBaseStruct(baseStructArg); });
}

TEST_F(IltConsumerGetterSetterTest, callGetAttributeBaseStruct)
{
    joynr::interlanguagetest::namedTypeCollection2::BaseStruct result;
    joynr::interlanguagetest::namedTypeCollection2::BaseStruct baseStructArg =
            IltUtil::createBaseStruct();
    JOYNR_ASSERT_NO_THROW({
        testInterfaceProxy->setAttributeBaseStruct(baseStructArg);
        testInterfaceProxy->getAttributeBaseStruct(result);
    });
    ASSERT_TRUE(IltUtil::checkBaseStruct(result));
}

TEST_F(IltConsumerGetterSetterTest, callSetAttributeExtendedExtendedBaseStruct)
{
    joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct baseStructArg =
            IltUtil::createExtendedExtendedBaseStruct();
    JOYNR_ASSERT_NO_THROW(
            { testInterfaceProxy->setAttributeExtendedExtendedBaseStruct(baseStructArg); });
}

TEST_F(IltConsumerGetterSetterTest, callGetAttributeExtendedExtendedBaseStruct)
{
    joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct result;
    joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct baseStructArg =
            IltUtil::createExtendedExtendedBaseStruct();
    JOYNR_ASSERT_NO_THROW({
        testInterfaceProxy->setAttributeExtendedExtendedBaseStruct(baseStructArg);
        testInterfaceProxy->getAttributeExtendedExtendedBaseStruct(result);
    });
    ASSERT_TRUE(IltUtil::checkExtendedExtendedBaseStruct(result));
}

TEST_F(IltConsumerGetterSetterTest, callSetAttributeMapStringString)
{
    joynr::interlanguagetest::namedTypeCollection2::MapStringString mapStringStringArg;
    mapStringStringArg.insert(std::pair<std::string, std::string>("keyString1", "valueString1"));
    mapStringStringArg.insert(std::pair<std::string, std::string>("keyString2", "valueString2"));
    mapStringStringArg.insert(std::pair<std::string, std::string>("keyString3", "valueString3"));
    JOYNR_ASSERT_NO_THROW({ testInterfaceProxy->setAttributeMapStringString(mapStringStringArg); });
}

TEST_F(IltConsumerGetterSetterTest, callGetAttributeMapStringString)
{
    // the typedef is required since we run into preprocessor issues otherwise
    typedef std::map<std::string, std::string>::iterator myIterator;
    joynr::interlanguagetest::namedTypeCollection2::MapStringString result;
    joynr::interlanguagetest::namedTypeCollection2::MapStringString mapStringStringArg;
    mapStringStringArg.insert(std::pair<std::string, std::string>("keyString1", "valueString1"));
    mapStringStringArg.insert(std::pair<std::string, std::string>("keyString2", "valueString2"));
    mapStringStringArg.insert(std::pair<std::string, std::string>("keyString3", "valueString3"));
    JOYNR_ASSERT_NO_THROW({
        testInterfaceProxy->setAttributeMapStringString(mapStringStringArg);
        testInterfaceProxy->getAttributeMapStringString(result);
    });
    myIterator it;
    for (int i = 1; i <= 3; i++) {
        it = result.find("keyString" + std::to_string(i));
        ASSERT_NE(it, result.end());
        std::string expected = "valueString" + std::to_string(i);
        ASSERT_EQ(it->second, expected);
    }
}

TEST_F(IltConsumerGetterSetterTest, callSetAttributeWithExceptionFromSetter)
{
    try {
        testInterfaceProxy->setAttributeWithExceptionFromSetter(false);
        FAIL() << "callSetAttributeWithExceptionFromSetter - Unexpected continuation without "
                  "exception";
    } catch (joynr::exceptions::ProviderRuntimeException& e) {
        if (e.getMessage() != "Exception from setAttributeWithExceptionFromSetter") {
            FAIL() << "callSetAttributeWithExceptionFromSetter - invalid exception message";
        }
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        FAIL() << "callSetAttributeWithExceptionFromSetter - unexpected exception type";
    } catch (...) {
        FAIL() << "callSetAttributeWithExceptionFromSetter: unknown exception caught";
    }
}

TEST_F(IltConsumerGetterSetterTest, callGetAttributeWithExceptionFromGetter)
{
    try {
        bool result;
        testInterfaceProxy->getAttributeWithExceptionFromGetter(result);
        FAIL() << "callGetAttributeWithExceptionFromGetter - Unexpected continuation without "
                  "exception";
    } catch (joynr::exceptions::ProviderRuntimeException& e) {
        if (e.getMessage() != "Exception from getAttributeWithExceptionFromGetter") {
            FAIL() << "callGetAttributeWithExceptionFromGetter - invalid exception message";
        }
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        FAIL() << "callGetAttributeWithExceptionFromGetter - unexpected exception type";
    } catch (...) {
        FAIL() << "callGetAttributeWithExceptionFromGetter: unknown exception caught";
    }
}
