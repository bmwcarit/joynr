/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include <string>
#include <cstdint>
#include <unistd.h>
#include <memory>

#include "IltHelper.h"
#include "IltUtil.h"
#include "joynr/interlanguagetest/TestInterfaceProxy.h"
#include "joynr/interlanguagetest/namedTypeCollection2/ExtendedErrorEnumTc.h"
#include "joynr/interlanguagetest/TestInterface/MethodWithAnonymousErrorEnumErrorEnum.h"
#include "joynr/interlanguagetest/TestInterface/MethodWithExtendedErrorEnumErrorEnum.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/SubscriptionListener.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include <cassert>
#include <limits>
#include "joynr/JsonSerializer.h"
#include "joynr/Logger.h"

using namespace joynr;

bool IltUtil::useRestricted64BitRange = true;
bool IltUtil::useRestrictedUnsignedRange = true;

joynr::Logger logger("IltConsumerCPP");

class TestResult
{
private:
    std::string name;
    bool result;

public:
    TestResult(std::string name, bool result) : name(name), result(result)
    {
    }

    std::string getName()
    {
        return this->name;
    }

    bool getResult()
    {
        return this->result;
    }
};

std::vector<TestResult> tests;

void reportTest(std::string name, bool result)
{
    tests.push_back(TestResult(name, result));
}

int evaluateAndPrintResults()
{
    int exitCode;
    int cntFailed = 0;
    int cntOk = 0;
    int cols = 75;

    std::string horizontalRuler = std::string(cols, '=');

    std::string output = "\n" + horizontalRuler + "\n";
    output += "INTERLANGUAGE TEST SUMMARY (C++ CONSUMER):\n";
    output += horizontalRuler + "\n";
    for (auto it = tests.cbegin(); it != tests.cend(); it++) {
        TestResult t = (*it);
        std::string result = (t.getResult() ? "OK" : "FAILED");
        int length = cols - t.getName().length() - result.length();
        length = (length > 0) ? length : 1;
        std::string filler = std::string(length, '.');
        output += t.getName() + filler + result + "\n";
        if (t.getResult()) {
            cntOk++;
        } else {
            cntFailed++;
        }
    }
    output += horizontalRuler + "\n";
    output += "Tests executed: " + std::to_string(cntOk + cntFailed) + ", Success: " +
              std::to_string(cntOk) + ", Failures: " + std::to_string(cntFailed) + "\n";
    output += horizontalRuler + "\n";
    if (cntFailed > 0) {
        output += "Final result: FAILED\n";
        exitCode = 1;
    } else {
        output += "Final result: SUCCESS\n";
        exitCode = 0;
    }
    output += horizontalRuler + "\n";
    JOYNR_LOG_INFO(logger, output);
    return exitCode;
}

void waitForChange(volatile bool& value, int timeout)
{
    useconds_t remaining = timeout * 1000;
    useconds_t interval = 100000; // 0.1 seconds

    while (remaining > 0 && !value) {
        usleep(interval);
        remaining -= interval;
    }
}

// no check possible other than handling exceptions
bool callMethodWithoutParameters(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithoutParameters");
    try {
        testInterfaceProxy->methodWithoutParameters();
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callMethodWithoutParameters: exception: " << e.getMessage() << std::endl;
        JOYNR_LOG_INFO(logger, "callMethodWithoutParameters - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callMethodWithoutParameters - OK");
    return true;
}

bool callMethodWithoutInputParameter(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithoutInputParameter");
    try {
        bool b;
        testInterfaceProxy->methodWithoutInputParameter(b);
        // expect true to be returned
        if (!b) {
            JOYNR_LOG_INFO(logger, "callMethodWithoutInputParameter - result has invalid content");
            JOYNR_LOG_INFO(logger, "callMethodWithoutInputParameter - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callMethodWithoutInputParameters: exception: " << e.getMessage() << std::endl;
        JOYNR_LOG_INFO(logger, "callMethodWithoutInputParameter - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callMethodWithoutInputParameter - OK");
    return true;
}

bool callMethodWithoutOutputParameter(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithoutOutputParameter");
    try {
        bool arg = false;
        testInterfaceProxy->methodWithoutOutputParameter(arg);
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callMethodWithoutOutputParameters: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callMethodWithoutOutputParameter - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callMethodWithoutOutputParameter - OK");
    return true;
}

bool callMethodWithSinglePrimitiveParameters(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithSinglePrimitiveParameters");
    try {
        uint16_t arg = 32767;
        std::string result;
        testInterfaceProxy->methodWithSinglePrimitiveParameters(result, arg);
        if (result != std::to_string(arg)) {
            JOYNR_LOG_INFO(
                    logger, "callMethodWithSinglePrimitiveParameters - result has invalid content");
            JOYNR_LOG_INFO(logger, "callMethodWithSinglePrimitiveParameters - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callMethodWithSinglePrimitiveParameters: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callMethodWithSinglePrimitiveParameters - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callMethodWithSinglePrimitiveParameters - OK");
    return true;
}

bool callMethodWithMultiplePrimitiveParameters(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithMultiplePrimitiveParameters");
    try {
        int32_t arg1 = 2147483647;
        float arg2 = 47.11;
        bool arg3 = false;
        double doubleOut;
        std::string stringOut;
        testInterfaceProxy->methodWithMultiplePrimitiveParameters(
                doubleOut, stringOut, arg1, arg2, arg3);
        if (!IltUtil::cmpDouble(doubleOut, arg2) || stringOut != std::to_string(arg1)) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithMultiplePrimitiveParameters - doubleOut has invalid content");
            JOYNR_LOG_INFO(logger, "callMethodWithMultiplePrimitiveParameters - FAILED");
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callMethodWithSinglePrimitiveParameters: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callMethodWithMultiplePrimitiveParameters - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callMethodWithMultiplePrimitiveParameters - OK");
    return true;
}

bool callMethodWithSingleArrayParameters(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithSingleArrayParameters");
    try {
        std::vector<double> arg = IltUtil::createDoubleArray();
        std::vector<std::string> result;

        testInterfaceProxy->methodWithSingleArrayParameters(result, arg);
        // check output parameter
        if (!IltUtil::checkStringArray(result)) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithSingleArrayParameters - result parameter has invalid content");
            JOYNR_LOG_INFO(logger, "callMethodWithSingleArrayParameters - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callMethodWithSingleArrayParameters: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callMethodWithSingleArrayParameters - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callMethodWithSingleArrayParameters - OK");
    return true;
}

bool callMethodWithMultipleArrayParameters(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithMultipleArrayParameters");
    try {
        std::vector<uint64_t> uInt64ArrayOut;
        std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>
                structWithStringArrayArrayOut;
        std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>
                structWithStringArrayArrayArg = IltUtil::createStructWithStringArrayArray();
        std::vector<std::string> stringArrayArg = IltUtil::createStringArray();
        std::vector<int8_t> int8ArrayArg = IltUtil::createByteArray();
        std::vector<joynr::interlanguagetest::namedTypeCollection2::
                            ExtendedInterfaceEnumerationInTypeCollection::Enum> enumArrayArg =
                IltUtil::createExtendedInterfaceEnumerationInTypeCollectionArray();

        testInterfaceProxy->methodWithMultipleArrayParameters(
                // output param
                uInt64ArrayOut,
                structWithStringArrayArrayOut,
                // input param
                stringArrayArg,
                int8ArrayArg,
                enumArrayArg,
                structWithStringArrayArrayArg);
        // check output parameter
        // check will only work, if there are no negative values in int8ArrayArg
        if (uInt64ArrayOut.size() != int8ArrayArg.size()) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithMultipleArrayParameters - uInt64ArrayOut has invalid size");
            JOYNR_LOG_INFO(logger, "callMethodWithMultipleArrayParameters - FAILED");
            return false;
        }
        for (int i = 0; i < uInt64ArrayOut.size(); i++) {
            if (uInt64ArrayOut.at(i) != int8ArrayArg.at(i)) {
                JOYNR_LOG_INFO(logger,
                               "callMethodWithMultipleArrayParameters - uInt64ArrayOut has "
                               "invalid content");
                JOYNR_LOG_INFO(logger, "callMethodWithMultipleArrayParameters - FAILED");
                return false;
            }
        }

        if (!IltUtil::checkStructWithStringArrayArray(structWithStringArrayArrayOut)) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithMultipleArrayParameters - structWithStringArrayArrayOut has "
                    "invalid content");
            JOYNR_LOG_INFO(logger, "callMethodWithMultipleArrayParameters - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callMethodWithMultipleArrayParameters: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callMethodWithMultipleArrayParameters - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callMethodWithMultipleArrayParameters - OK");
    return true;
}

bool callMethodWithSingleEnumParameters(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithSingleEnumParameters");
    try {
        joynr::interlanguagetest::namedTypeCollection2::
                ExtendedTypeCollectionEnumerationInTypeCollection::Enum enumerationOut;
        joynr::interlanguagetest::namedTypeCollection2::ExtendedEnumerationWithPartlyDefinedValues::
                Enum enumerationArg = joynr::interlanguagetest::namedTypeCollection2::
                        ExtendedEnumerationWithPartlyDefinedValues::
                                ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;

        testInterfaceProxy->methodWithSingleEnumParameters(enumerationOut, enumerationArg);
        if (enumerationOut != joynr::interlanguagetest::namedTypeCollection2::
                                      ExtendedTypeCollectionEnumerationInTypeCollection::
                                              ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithSingleEnumParameters - enumerationOut has invalid content");
            JOYNR_LOG_INFO(logger, "callMethodWithSingleEnumParameters - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callMethodWithSingleEnumParameters: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callMethodWithSingleEnumParameters - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callMethodWithSingleEnumParameters - OK");
    return true;
}

bool callMethodWithMultipleEnumParameters(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithMultipleEnumParameters");
    try {
        joynr::interlanguagetest::namedTypeCollection2::ExtendedEnumerationWithPartlyDefinedValues::
                Enum extendedEnumerationOut;
        joynr::interlanguagetest::Enumeration::Enum enumerationOut;
        joynr::interlanguagetest::Enumeration::Enum enumerationArg;
        joynr::interlanguagetest::namedTypeCollection2::
                ExtendedTypeCollectionEnumerationInTypeCollection::Enum extendedEnumerationArg;

        enumerationArg = joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_3;
        extendedEnumerationArg = joynr::interlanguagetest::namedTypeCollection2::
                ExtendedTypeCollectionEnumerationInTypeCollection::
                        ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;

        testInterfaceProxy->methodWithMultipleEnumParameters(
                extendedEnumerationOut, enumerationOut, enumerationArg, extendedEnumerationArg);

        if (enumerationOut != joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_1 ||
            extendedEnumerationOut !=
                    joynr::interlanguagetest::namedTypeCollection2::
                            ExtendedEnumerationWithPartlyDefinedValues::
                                    ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES) {
            JOYNR_LOG_INFO(logger,
                           "callMethodWithMultipleEnumParameters - enumerationOut or "
                           "extendedEnumerationOut "
                           "hava invalid content");
            JOYNR_LOG_INFO(logger, "callMethodWithMultipleEnumParameters - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callMethodWithMultipleEnumParameters: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callMethodWithMultipleEnumParameters - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callMethodWithMultipleEnumParameters - OK");
    return true;
}

bool callMethodWithSingleStructParameters(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithSingleStructParameters");
    try {
        joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives
                extendedStructOfPrimitivesOut;
        joynr::interlanguagetest::namedTypeCollection2::ExtendedBaseStruct extendedBaseStructArg;

        extendedBaseStructArg = IltUtil::createExtendedBaseStruct();

        testInterfaceProxy->methodWithSingleStructParameters(
                extendedStructOfPrimitivesOut, extendedBaseStructArg);
        if (!IltUtil::checkExtendedStructOfPrimitives(extendedStructOfPrimitivesOut)) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithSingleStructParameters - extendedStructOfPrimitivesOut has "
                    "invalid content");
            JOYNR_LOG_INFO(logger, "callMethodWithSingleStructParameters - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callMethodWithSingleStructParameters: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callMethodWithSingleStructParameters - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callMethodWithSingleStructParameters - OK");
    return true;
}

bool callMethodWithMultipleStructParameters(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithMultipleStructParameters");
    try {
        joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements
                baseStructWithoutElementsOut;
        joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct
                extendedExtendedBaseStructOut;
        joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives
                extendedStructOfPrimitivesArg;
        joynr::interlanguagetest::namedTypeCollection2::BaseStruct baseStructArg;

        extendedStructOfPrimitivesArg = IltUtil::createExtendedStructOfPrimitives();
        baseStructArg = IltUtil::createBaseStruct();

        testInterfaceProxy->methodWithMultipleStructParameters(baseStructWithoutElementsOut,
                                                               extendedExtendedBaseStructOut,
                                                               extendedStructOfPrimitivesArg,
                                                               baseStructArg);
        if (!IltUtil::checkBaseStructWithoutElements(baseStructWithoutElementsOut)) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithMultipleStructParameters - baseStructWithoutElementsOut has "
                    "invalid content");
            JOYNR_LOG_INFO(logger, "callMethodWithMultipleStructParameters - FAILED");
            return false;
        }
        if (!IltUtil::checkExtendedExtendedBaseStruct(extendedExtendedBaseStructOut)) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithMultipleStructParameters - extendedExtendedBaseStructOut has "
                    "invalid content");
            JOYNR_LOG_INFO(logger, "callMethodWithMultipleStructParameters - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callMethodWithMultipleStructParameters: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callMethodWithMultipleStructParameters - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callMethodWithMultipleStructParameters - OK");
    return true;
}

bool callOverloadedMethod_1(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callOverloadedMethod_1");
    try {
        std::string stringOut;
        testInterfaceProxy->overloadedMethod(stringOut);
        if (stringOut != "TestString 1") {
            JOYNR_LOG_INFO(logger, "callOverloadedMethod_1 - stringOut has invalid content");
            JOYNR_LOG_INFO(logger, "callOverloadedMethod_1 - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callOverloadedMethod_1: exception: " << e.getMessage() << std::endl;
        JOYNR_LOG_INFO(logger, "callOverloadedMethod_1 - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callOverloadedMethod_1 - OK");
    return true;
}

bool callOverloadedMethod_2(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callOverloadedMethod_2");
    try {
        std::string stringOut;
        bool booleanArg = false;
        testInterfaceProxy->overloadedMethod(stringOut, booleanArg);
        if (stringOut != "TestString 2") {
            JOYNR_LOG_INFO(logger, "callOverloadedMethod_2 - stringOut has invalid content");
            JOYNR_LOG_INFO(logger, "callOverloadedMethod_2 - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callOverloadedMethod_2: exception: " << e.getMessage() << std::endl;
        JOYNR_LOG_INFO(logger, "callOverloadedMethod_2 - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callOverloadedMethod_2 - OK");
    return true;
}

bool callOverloadedMethod_3(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callOverloadedMethod_3");
    try {
        double doubleOut;
        std::vector<std::string> stringArrayOut;
        joynr::interlanguagetest::namedTypeCollection2::ExtendedBaseStruct extendedBaseStructOut;
        std::vector<
                joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedEnumeration::Enum>
                enumArrayArg;
        int64_t int64Arg;
        joynr::interlanguagetest::namedTypeCollection2::BaseStruct baseStructArg;
        bool booleanArg;

        enumArrayArg = IltUtil::createExtendedExtendedEnumerationArray();
        int64Arg = 1L;
        baseStructArg = IltUtil::createBaseStruct();
        booleanArg = false;

        testInterfaceProxy->overloadedMethod(doubleOut,
                                             stringArrayOut,
                                             extendedBaseStructOut,
                                             enumArrayArg,
                                             int64Arg,
                                             baseStructArg,
                                             booleanArg);
        if (doubleOut != 0) {
            JOYNR_LOG_INFO(logger, "callOverloadedMethod_3 - doubleOut has invalid content");
            JOYNR_LOG_INFO(logger, "callOverloadedMethod_3 - FAILED");
            return false;
        }
        if (!IltUtil::checkStringArray(stringArrayOut)) {
            JOYNR_LOG_INFO(logger, "callOverloadedMethod_3 - stringArrayOut has invalid content");
            JOYNR_LOG_INFO(logger, "callOverloadedMethod_3 - FAILED");
            return false;
        }
        if (!IltUtil::checkExtendedBaseStruct(extendedBaseStructOut)) {
            JOYNR_LOG_INFO(
                    logger, "callOverloadedMethod_3 - extendedBaseStructOut has invalid content");
            JOYNR_LOG_INFO(logger, "callOverloadedMethod_3 - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callOverloadedMethod_3: exception: " << e.getMessage() << std::endl;
        JOYNR_LOG_INFO(logger, "callOverloadedMethod_3 - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callOverloadedMethod_3 - OK");
    return true;
}

bool callOverloadedMethodWithSelector_1(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callOverloadedMethodWithSelector_1");
    try {
        std::string stringOut;
        testInterfaceProxy->overloadedMethodWithSelector(stringOut);
        if (stringOut != "Return value from overloadedMethodWithSelector 1") {
            JOYNR_LOG_INFO(
                    logger, "callOverloadedMethodWithSelector_1 - stringOut has invalid content");
            JOYNR_LOG_INFO(logger, "callOverloadedMethodWithSelector_1 - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callOverloadedMethodWithSelector_1: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callOverloadedMethodWithSelector_1 - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callOverloadedMethodWithSelector_1 - OK");
    return true;
}

bool callOverloadedMethodWithSelector_2(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callOverloadedMethodWithSelector_2");
    try {
        std::string stringOut;
        bool booleanArg = false;
        testInterfaceProxy->overloadedMethodWithSelector(stringOut, booleanArg);
        if (stringOut != "Return value from overloadedMethodWithSelector 2") {
            JOYNR_LOG_INFO(
                    logger, "callOverloadedMethodWithSelector_2 - stringOut has invalid content");
            JOYNR_LOG_INFO(logger, "callOverloadedMethodWithSelector_2 - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callOverloadedMethodWithSelector_2: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callOverloadedMethodWithSelector_2 - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callOverloadedMethodWithSelector_2 - OK");
    return true;
}

bool callOverloadedMethodWithSelector_3(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callOverloadedMethodWithSelector_3");
    try {
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

        testInterfaceProxy->overloadedMethodWithSelector(doubleOut,
                                                         stringArrayOut,
                                                         extendedBaseStructOut,
                                                         enumArrayArg,
                                                         int64Arg,
                                                         baseStructArg,
                                                         booleanArg);
        if (!IltUtil::cmpDouble(doubleOut, 1.1)) {
            JOYNR_LOG_INFO(
                    logger, "callOverloadedMethodWithSelector_3 - doubleOut has invalid content");
            JOYNR_LOG_INFO(logger, "callOverloadedMethodWithSelector_3 - FAILED");
            return false;
        }
        if (!IltUtil::checkExtendedBaseStruct(extendedBaseStructOut)) {
            JOYNR_LOG_INFO(logger,
                           "callOverloadedMethodWithSelector_3 - extendedBaseStructOut has "
                           "invalid content");
            JOYNR_LOG_INFO(logger, "callOverloadedMethodWithSelector_3 - FAILED");
            return false;
        }
        if (!IltUtil::checkStringArray(stringArrayOut)) {
            JOYNR_LOG_INFO(
                    logger,
                    "callOverloadedMethodWithSelector_3 - stringArrayOut has invalid content");
            JOYNR_LOG_INFO(logger, "callOverloadedMethodWithSelector_3 - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callOverloadedMethodWithSelector_3: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callOverloadedMethodWithSelector_3 - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callOverloadedMethodWithSelector_3 - OK");
    return true;
}

bool callMethodWithStringsAndSpecifiedStringOutLength(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithStringsAndSpecifiedStringOutLength");
    try {
        std::string stringOut;
        std::string stringArg = "Hello world";
        int32_t int32StringLengthArg = 32;
        testInterfaceProxy->methodWithStringsAndSpecifiedStringOutLength(
                stringOut, stringArg, int32StringLengthArg);
        if (stringOut.length() != 32) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithStringsAndSpecifiedStringOutLength - stringOut has invalid "
                    "length");
            JOYNR_LOG_INFO(logger, "callMethodWithStringsAndSpecifiedStringOutLength - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callMethodWithStringsAndSpecifiedStringOutLength: exception: "
                  << e.getMessage() << std::endl;
        JOYNR_LOG_INFO(logger, "callMethodWithStringsAndSpecifiedStringOutLength - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callMethodWithStringsAndSpecifiedStringOutLength - OK");
    return true;
}

bool callMethodWithoutErrorEnum(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithoutErrorEnum");
    try {
        std::string wantedException = "ProviderRuntimeException";
        testInterfaceProxy->methodWithoutErrorEnum(wantedException);
    } catch (joynr::exceptions::ProviderRuntimeException& e) {
        if (e.getMessage() != "Exception from methodWithoutErrorEnum") {
            JOYNR_LOG_INFO(logger, "callMethodWithoutErrorEnum - invalid exception message");
            JOYNR_LOG_INFO(logger, "callMethodWithoutErrorEnum - FAILED");
            return false;
        }
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(logger, "callMethodWithoutErrorEnum - unexpected exception type");
        JOYNR_LOG_INFO(logger, "callMethodWithoutErrorEnum - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callMethodWithoutErrorEnum - OK");
    return true;
}

bool callMethodWithAnonymousErrorEnum(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithAnonymousErrorEnum");
#ifdef USE_PROVIDER_RUNTIME_EXCEPTION
    try {
        std::string wantedException = "ProviderRuntimeException";
        testInterfaceProxy->methodWithAnonymousErrorEnum(wantedException);
    } catch (joynr::exceptions::ProviderRuntimeException& e) {
        if (e.getMessage() != "Exception from methodWithAnonymousErrorEnum") {
            JOYNR_LOG_INFO(
                    logger, "callMethodWithAnonymousErrorEnum - 1st - invalid exception message");
            JOYNR_LOG_INFO(logger, "callMethodWithAnonymousErrorEnum - FAILED");
            return false;
        }
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(
                logger, "callMethodWithAnonymousErrorEnum - 1st - unexpected exception type");
        JOYNR_LOG_INFO(logger, "callMethodWithAnonymousErrorEnum - FAILED");
        return false;
    }
#endif

    // 2nd test
    try {
        std::string wantedException = "ApplicationException";
        testInterfaceProxy->methodWithAnonymousErrorEnum(wantedException);
    } catch (joynr::exceptions::ApplicationException& e) {
        if (e.getError<joynr::interlanguagetest::TestInterface::
                               MethodWithAnonymousErrorEnumErrorEnum::Enum>() !=
            joynr::interlanguagetest::TestInterface::MethodWithAnonymousErrorEnumErrorEnum::
                    ERROR_3_1_NTC) {
            JOYNR_LOG_INFO(logger,
                           "callMethodWithAnonymousErrorEnum - 2nd - unexpected exception "
                           "error enum value");
            JOYNR_LOG_INFO(logger, "callMethodWithAnonymousErrorEnum - FAILED");
            return false;
        }
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(
                logger, "callMethodWithAnonymousErrorEnum - 2nd - unexpected exception type");
        JOYNR_LOG_INFO(logger, "callMethodWithAnonymousErrorEnum - FAILED");
        return false;
    }

    JOYNR_LOG_INFO(logger, "callMethodWithAnonymousErrorEnum - OK");
    return true;
}

bool callMethodWithExistingErrorEnum(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithExistingErrorEnum");
#ifdef USE_PROVIDER_RUNTIME_EXCEPTION
    try {
        std::string wantedException = "ProviderRuntimeException";
        testInterfaceProxy->methodWithExistingErrorEnum(wantedException);
    } catch (joynr::exceptions::ProviderRuntimeException& e) {
        if (e.getMessage() != "Exception from methodWithExistingErrorEnum") {
            JOYNR_LOG_INFO(
                    logger, "callMethodWithExistingErrorEnum - 1st - invalid exception message");
            JOYNR_LOG_INFO(logger, "callMethodWithExistingErrorEnum - FAILED");
            return false;
        }
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(logger, "callMethodWithExistingErrorEnum - 1st - unexpected exception type");
        JOYNR_LOG_INFO(logger, "callMethodWithExistingErrorEnum - FAILED");
        return false;
    }
#endif

    // 2nd test
    try {
        std::string wantedException = "ApplicationException_1";
        testInterfaceProxy->methodWithExistingErrorEnum(wantedException);
    } catch (joynr::exceptions::ApplicationException& e) {
        if (e.getError<
                    joynr::interlanguagetest::namedTypeCollection2::ExtendedErrorEnumTc::Enum>() !=
            joynr::interlanguagetest::namedTypeCollection2::ExtendedErrorEnumTc::ERROR_2_3_TC2) {
            JOYNR_LOG_INFO(logger,
                           "callMethodWithExistingErrorEnum - 2nd - unexpected exception "
                           "error enum value");
            JOYNR_LOG_INFO(logger, "callMethodWithExistingErrorEnum - FAILED");
            return false;
        }
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(logger, "callMethodWithExistingErrorEnum - 2nd - unexpected exception type");
        JOYNR_LOG_INFO(logger, "callMethodWithExistingErrorEnum - FAILED");
        return false;
    }

    // 3rd test
    try {
        std::string wantedException = "ApplicationException_2";
        testInterfaceProxy->methodWithExistingErrorEnum(wantedException);
    } catch (joynr::exceptions::ApplicationException& e) {
        if (e.getError<
                    joynr::interlanguagetest::namedTypeCollection2::ExtendedErrorEnumTc::Enum>() !=
            joynr::interlanguagetest::namedTypeCollection2::ExtendedErrorEnumTc::ERROR_1_2_TC_2) {
            JOYNR_LOG_INFO(logger,
                           "callMethodWithExistingErrorEnum - 3rd - unexpected exception "
                           "error enum value");
            JOYNR_LOG_INFO(logger, "callMethodWithExistingErrorEnum - FAILED");
            return false;
        }
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(logger, "callMethodWithExistingErrorEnum - 3rd - unexpected exception type");
        JOYNR_LOG_INFO(logger, "callMethodWithExistingErrorEnum - FAILED");
        return false;
    }

    JOYNR_LOG_INFO(logger, "callMethodWithExistingErrorEnum - OK");
    return true;
}

bool callMethodWithExtendedErrorEnum(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnum");
#ifdef USE_PROVIDER_RUNTIME_EXCEPTION
    try {
        std::string wantedException = "ProviderRuntimeException";
        testInterfaceProxy->methodWithExtendedErrorEnum(wantedException);
    } catch (joynr::exceptions::ProviderRuntimeException& e) {
        if (e.getMessage() != "Exception from methodWithExtendedErrorEnum") {
            JOYNR_LOG_INFO(
                    logger, "callMethodWithExtendedErrorEnum - 1st - invalid exception message");
            JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnum - FAILED");
            return false;
        }
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnum - 1st - unexpected exception type");
        JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnum - FAILED");
        return false;
    }
#endif

    // 2nd test
    try {
        std::string wantedException = "ApplicationException_1";
        testInterfaceProxy->methodWithExtendedErrorEnum(wantedException);
    } catch (joynr::exceptions::ApplicationException& e) {
        if (e.getError<joynr::interlanguagetest::TestInterface::
                               MethodWithExtendedErrorEnumErrorEnum::Enum>() !=
            joynr::interlanguagetest::TestInterface::MethodWithExtendedErrorEnumErrorEnum::
                    ERROR_3_3_NTC) {
            JOYNR_LOG_INFO(logger,
                           "callMethodWithExtendedErrorEnum - 2nd - unexpected exception "
                           "error enum value");
            JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnum - FAILED");
            return false;
        }
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnum - 2nd - unexpected exception type");
        JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnum - FAILED");
        return false;
    }

    // 3rd test
    try {
        std::string wantedException = "ApplicationException_2";
        testInterfaceProxy->methodWithExtendedErrorEnum(wantedException);
    } catch (joynr::exceptions::ApplicationException& e) {
        if (e.getError<joynr::interlanguagetest::TestInterface::
                               MethodWithExtendedErrorEnumErrorEnum::Enum>() !=
            joynr::interlanguagetest::TestInterface::MethodWithExtendedErrorEnumErrorEnum::
                    ERROR_2_1_TC2) {
            JOYNR_LOG_INFO(logger,
                           "callMethodWithExtendedErrorEnum - 3rd - unexpected exception "
                           "error enum value");
            JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnum - FAILED");
            return false;
        }
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnum - 3rd - unexpected exception type");
        JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnum - FAILED");
        return false;
    }

    JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnum - OK");
    return true;
}

// variables that are to be changed inside callbacks must be declared global
volatile bool methodWithMultipleStructParametersAsyncCallbackDone = false;
volatile bool methodWithMultipleStructParametersAsyncCallbackResult = false;

bool callMethodWithMultipleStructParametersAsync(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithMultipleStructParametersAsync");
    try {
        // setup input parameters
        joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives
                extendedStructOfPrimitivesArg = IltUtil::createExtendedStructOfPrimitives();
        joynr::interlanguagetest::namedTypeCollection2::BaseStruct baseStructArg =
                IltUtil::createBaseStruct();

        std::function<void(
                const joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements&,
                const joynr::interlanguagetest::namedTypeCollection2::
                        ExtendedExtendedBaseStruct&)> onSuccess =
                [](const joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements&
                           baseStructWithoutElementsOut,
                   const joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct&
                           extendedExtendedBaseStructOut) {
            // check results
            if (!IltUtil::checkBaseStructWithoutElements(baseStructWithoutElementsOut)) {
                methodWithMultipleStructParametersAsyncCallbackResult = false;
                methodWithMultipleStructParametersAsyncCallbackDone = true;
                JOYNR_LOG_INFO(logger,
                               "callMethodWithMultipleStructParametersAsync - callback - invalid "
                               "baseStructWithoutElementsOut");
                JOYNR_LOG_INFO(logger, "callMethodWithMultipleStructParametersAsync - FAILED");
                return;
            }

            if (!IltUtil::checkExtendedExtendedBaseStruct(extendedExtendedBaseStructOut)) {
                methodWithMultipleStructParametersAsyncCallbackResult = false;
                methodWithMultipleStructParametersAsyncCallbackDone = true;
                JOYNR_LOG_INFO(logger,
                               "callMethodWithMultipleStructParametersAsync - callback - invalid "
                               "extendedExtendedBaseStructOut");
                JOYNR_LOG_INFO(logger, "callMethodWithMultipleStructParametersAsync - FAILED");
                return;
            }
            methodWithMultipleStructParametersAsyncCallbackResult = true;
            methodWithMultipleStructParametersAsyncCallbackDone = true;
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithMultipleStructParametersAsync - callback - got correct values");
        };

        std::function<void(const joynr::exceptions::JoynrException&)> onError =
                [](const joynr::exceptions::JoynrException& error) {
            methodWithMultipleStructParametersAsyncCallbackResult = false;
            methodWithMultipleStructParametersAsyncCallbackDone = true;
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithMultipleStructParametersAsync - callback - caught exception");
            JOYNR_LOG_INFO(logger, error.getTypeName());
            JOYNR_LOG_INFO(logger, error.getMessage());
        };

        std::shared_ptr<joynr::Future<
                joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements,
                joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct>>
                future = testInterfaceProxy->methodWithMultipleStructParametersAsync(
                        extendedStructOfPrimitivesArg, baseStructArg, onSuccess, onError);

        try {
            long timeoutInMilliseconds = 8000;
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithMultipleStructParametersAsync - about to call future.getReply");
            future->wait(timeoutInMilliseconds);
            JOYNR_LOG_INFO(logger,
                           "callMethodWithMultipleStructParametersAsync - returned from "
                           "future.waitForFinished");
            if (!future->isOk()) {
                JOYNR_LOG_INFO(
                        logger,
                        "callMethodWithMultipleStructParametersAsync - negative requestStatus");
                JOYNR_LOG_INFO(logger, "callMethodWithMultipleStructParametersAsync - FAILED");
                return false;
            }

            // TODO: It is unclear here whether we have a result or whether the timeout
            // has expired. We need a future->get(... , timeout)

            joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements
                    baseStructWithoutElementsOut;
            joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct
                    extendedExtendedBaseStructOut;

            // the following call would throw an exception, in case the Requeststatus
            // is negative, however we have already returned here
            future->get(baseStructWithoutElementsOut, extendedExtendedBaseStructOut);

            // check results from future
            if (!IltUtil::checkBaseStructWithoutElements(baseStructWithoutElementsOut)) {
                JOYNR_LOG_INFO(logger,
                               "callMethodWithMultipleStructParametersAsync - invalid "
                               "baseStructWithoutElementsOut");
                JOYNR_LOG_INFO(logger, "callMethodWithMultipleStructParametersAsync - FAILED");
                return false;
            }

            if (!IltUtil::checkExtendedExtendedBaseStruct(extendedExtendedBaseStructOut)) {
                JOYNR_LOG_INFO(logger,
                               "callMethodWithMultipleStructParametersAsync - invalid "
                               "extendedExtendedBaseStructOut");
                JOYNR_LOG_INFO(logger, "callMethodWithMultipleStructParametersAsync - FAILED");
                return false;
            }
            JOYNR_LOG_INFO(logger,
                           "callMethodWithMultipleStructParametersAsync - future result checks OK");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            waitForChange(methodWithMultipleStructParametersAsyncCallbackDone, 1000);
            if (methodWithMultipleStructParametersAsyncCallbackDone == false) {
                JOYNR_LOG_INFO(
                        logger, "callMethodWithMultipleStructParametersAsync - callback NOT done");
                JOYNR_LOG_INFO(logger, "callMethodWithMultipleStructParametersAsync - FAILED");
                return false;
            }
            if (methodWithMultipleStructParametersAsyncCallbackResult == false) {
                JOYNR_LOG_INFO(
                        logger,
                        "callMethodWithMultipleStructParametersAsync - callback reported error");
                JOYNR_LOG_INFO(logger, "callMethodWithMultipleStructParametersAsync - FAILED");
                return false;
            }
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithMultipleStructParametersAsync - callback has set OK flags");
        } catch (joynr::exceptions::ApplicationException& e) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithMultipleStructParametersAsync - caught ApplicationException");
            JOYNR_LOG_INFO(logger, "callMethodWithMultipleStructParametersAsync - FAILED");
            return false;
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithMultipleStructParametersAsync - caught JoynrRuntimeException");
            JOYNR_LOG_INFO(logger, "callMethodWithMultipleStructParametersAsync - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(
                logger, "callMethodWithMultipleStructParametersAsync - caught other exception");
        JOYNR_LOG_INFO(logger, "callMethodWithMultipleStructParametersAsync - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callMethodWithMultipleStructParametersAsync - OK");
    return true;
}

// variables that are to be changed inside callbacks must be declared global
volatile bool methodWithSingleArrayParametersAsyncCallbackDone = false;
volatile bool methodWithSingleArrayParametersAsyncCallbackResult = false;

bool callMethodWithSingleArrayParametersAsync(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithSingleArrayParametersAsync");
    try {
        // setup input parameters
        std::vector<double> arg = IltUtil::createDoubleArray();

        std::function<void(const std::vector<std::string>& result)> onSuccess =
                [](const std::vector<std::string>& result) {
            // check results
            if (!IltUtil::checkStringArray(result)) {
                methodWithSingleArrayParametersAsyncCallbackResult = false;
                methodWithSingleArrayParametersAsyncCallbackDone = true;
                JOYNR_LOG_INFO(logger,
                               "callMethodWithSingleArrayParametersAsync - callback - invalid "
                               "baseStructWithoutElementsOut");
                JOYNR_LOG_INFO(logger, "callMethodWithSingleArrayParametersAsync - FAILED");
                return;
            }

            methodWithSingleArrayParametersAsyncCallbackResult = true;
            methodWithSingleArrayParametersAsyncCallbackDone = true;
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithSingleArrayParametersAsync - callback - got correct values");
        };

        std::function<void(const joynr::exceptions::JoynrException&)> onError =
                [](const joynr::exceptions::JoynrException& error) {
            methodWithSingleArrayParametersAsyncCallbackResult = false;
            methodWithSingleArrayParametersAsyncCallbackDone = true;
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithSingleArrayParametersAsync - callback - caught exception");
            JOYNR_LOG_INFO(logger, error.getTypeName());
            JOYNR_LOG_INFO(logger, error.getMessage());
        };

        std::shared_ptr<joynr::Future<std::vector<std::string>>> future =
                testInterfaceProxy->methodWithSingleArrayParametersAsync(arg, onSuccess, onError);

        try {
            long timeoutInMilliseconds = 8000;
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithSingleArrayParametersAsync - about to call future.getReply");
            future->wait(timeoutInMilliseconds);
            JOYNR_LOG_INFO(logger,
                           "callMethodWithSingleArrayParametersAsync - returned from "
                           "future.wait");
            if (!future->isOk()) {
                JOYNR_LOG_INFO(logger,
                               "callMethodWithSingleArrayParametersAsync - negative requestStatus");
                JOYNR_LOG_INFO(logger, "callMethodWithSingleArrayParametersAsync - FAILED");
                return false;
            }

            std::vector<std::string> result;

            // the following call would throw an exception, in case the Requeststatus
            // is negative, however we have already returned here
            // future->getValues(result);
            future->get(result);

            // check results from future
            if (!IltUtil::checkStringArray(result)) {
                JOYNR_LOG_INFO(logger,
                               "callMethodWithSingleArrayParametersAsync - invalid "
                               "baseStructWithoutElementsOut");
                JOYNR_LOG_INFO(logger, "callMethodWithSingleArrayParametersAsync - FAILED");
                return false;
            }

            JOYNR_LOG_INFO(
                    logger, "callMethodWithSingleArrayParametersAsync - future result checks OK");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            waitForChange(methodWithSingleArrayParametersAsyncCallbackDone, 1000);
            if (methodWithSingleArrayParametersAsyncCallbackDone == false) {
                JOYNR_LOG_INFO(
                        logger, "callMethodWithSingleArrayParametersAsync - callback NOT done");
                JOYNR_LOG_INFO(logger, "callMethodWithSingleArrayParametersAsync - FAILED");
                return false;
            }
            if (methodWithSingleArrayParametersAsyncCallbackResult == false) {
                JOYNR_LOG_INFO(
                        logger,
                        "callMethodWithSingleArrayParametersAsync - callback reported error");
                JOYNR_LOG_INFO(logger, "callMethodWithSingleArrayParametersAsync - FAILED");
                return false;
            }
            JOYNR_LOG_INFO(
                    logger, "callMethodWithSingleArrayParametersAsync - callback has set OK flags");
        } catch (joynr::exceptions::ApplicationException& e) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithSingleArrayParametersAsync - caught ApplicationException");
            JOYNR_LOG_INFO(logger, "callMethodWithSingleArrayParametersAsync - FAILED");
            return false;
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithSingleArrayParametersAsync - caught JoynrRuntimeException");
            JOYNR_LOG_INFO(logger, "callMethodWithSingleArrayParametersAsync - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(logger, "callMethodWithSingleArrayParametersAsync - caught other exception");
        JOYNR_LOG_INFO(logger, "callMethodWithSingleArrayParametersAsync - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callMethodWithSingleArrayParametersAsync - OK");
    return true;
}

// variables that are to be changed inside callbacks must be declared global
volatile bool methodWithSinglePrimitiveParametersAsyncCallbackDone = false;
volatile bool methodWithSinglePrimitiveParametersAsyncCallbackResult = false;

bool callMethodWithSinglePrimitiveParametersAsync(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithSinglePrimitiveParametersAsync");
    try {
        // setup input parameters
        uint16_t arg = 32767;

        std::function<void(const std::string&)> onSuccess = [arg](const std::string& result) {
            // check results
            if (result != std::to_string(arg)) {
                methodWithSinglePrimitiveParametersAsyncCallbackResult = false;
                methodWithSinglePrimitiveParametersAsyncCallbackDone = true;
                JOYNR_LOG_INFO(logger,
                               "callMethodWithSinglePrimitiveParametersAsync - callback - invalid "
                               "baseStructWithoutElementsOut");
                JOYNR_LOG_INFO(logger, "callMethodWithSinglePrimitiveParametersAsync - FAILED");
                return;
            }

            methodWithSinglePrimitiveParametersAsyncCallbackResult = true;
            methodWithSinglePrimitiveParametersAsyncCallbackDone = true;
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithSinglePrimitiveParametersAsync - callback - got correct values");
        };

        std::function<void(const joynr::exceptions::JoynrException&)> onError =
                [](const joynr::exceptions::JoynrException& error) {
            methodWithSinglePrimitiveParametersAsyncCallbackResult = false;
            methodWithSinglePrimitiveParametersAsyncCallbackDone = true;
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithSinglePrimitiveParametersAsync - callback - caught exception");
            JOYNR_LOG_INFO(logger, error.getTypeName());
            JOYNR_LOG_INFO(logger, error.getMessage());
        };

        std::shared_ptr<joynr::Future<std::string>> future =
                testInterfaceProxy->methodWithSinglePrimitiveParametersAsync(
                        arg, onSuccess, onError);

        try {
            long timeoutInMilliseconds = 8000;
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithSinglePrimitiveParametersAsync - about to call future.getReply");
            future->wait(timeoutInMilliseconds);
            JOYNR_LOG_INFO(logger,
                           "callMethodWithSinglePrimitiveParametersAsync - returned from "
                           "future.wait");
            if (!future->isOk()) {
                JOYNR_LOG_INFO(
                        logger,
                        "callMethodWithSinglePrimitiveParametersAsync - negative requestStatus");
                JOYNR_LOG_INFO(logger, "callMethodWithSinglePrimitiveParametersAsync - FAILED");
                return false;
            }

            std::string result;

            // the following call would throw an exception, in case the Requeststatus
            // is negative, however we have already returned here
            future->get(result);

            // check results from future
            if (result != std::to_string(arg)) {
                JOYNR_LOG_INFO(logger,
                               "callMethodWithSinglePrimitiveParametersAsync - invalid "
                               "baseStructWithoutElementsOut");
                JOYNR_LOG_INFO(logger, "callMethodWithSinglePrimitiveParametersAsync - FAILED");
                return false;
            }

            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithSinglePrimitiveParametersAsync - future result checks OK");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            waitForChange(methodWithSinglePrimitiveParametersAsyncCallbackDone, 1000);
            if (methodWithSinglePrimitiveParametersAsyncCallbackDone == false) {
                JOYNR_LOG_INFO(
                        logger, "callMethodWithSinglePrimitiveParametersAsync - callback NOT done");
                JOYNR_LOG_INFO(logger, "callMethodWithSinglePrimitiveParametersAsync - FAILED");
                return false;
            }
            if (methodWithSinglePrimitiveParametersAsyncCallbackResult == false) {
                JOYNR_LOG_INFO(
                        logger,
                        "callMethodWithSinglePrimitiveParametersAsync - callback reported error");
                JOYNR_LOG_INFO(logger, "callMethodWithSinglePrimitiveParametersAsync - FAILED");
                return false;
            }
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithSinglePrimitiveParametersAsync - callback has set OK flags");
        } catch (joynr::exceptions::ApplicationException& e) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithSinglePrimitiveParametersAsync - caught ApplicationException");
            JOYNR_LOG_INFO(logger, "callMethodWithSinglePrimitiveParametersAsync - FAILED");
            return false;
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithSinglePrimitiveParametersAsync - caught JoynrRuntimeException");
            JOYNR_LOG_INFO(logger, "callMethodWithSinglePrimitiveParametersAsync - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(
                logger, "callMethodWithSinglePrimitiveParametersAsync - caught other exception");
        JOYNR_LOG_INFO(logger, "callMethodWithSinglePrimitiveParametersAsync - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callMethodWithSinglePrimitiveParametersAsync - OK");
    return true;
}

// variables that are to be changed inside callbacks must be declared global
volatile bool methodWithExtendedErrorEnumAsyncCallbackDone = false;
volatile bool methodWithExtendedErrorEnumAsyncCallbackResult = false;

bool callMethodWithExtendedErrorEnumAsync(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync");
#ifdef USE_PROVIDER_RUNTIME_EXCEPTION
    try {
        // setup input parameters
        std::string arg = "ProviderRuntimeException";

        std::function<void()> onSuccess = []() {
            // check results
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithExtendedErrorEnumAsync - 1st - callback - unexpected call to "
                    "onSuccess");
            methodWithExtendedErrorEnumAsyncCallbackResult = false;
            methodWithExtendedErrorEnumAsyncCallbackDone = true;
        };

        auto onRuntimeError = [](const joynr::exceptions::JoynrRuntimeException& error) {
            if (error.getTypeName() == "joynr.exceptions.ProviderRuntimeException") {
                if (error.getMessage() == "Exception from methodWithExtendedErrorEnum") {
                    JOYNR_LOG_INFO(
                            logger,
                            "callMethodWithExtendedErrorEnumAsync - 1st - callback - got expected "
                            "exception");
                    methodWithExtendedErrorEnumAsyncCallbackResult = true;
                } else {
                    JOYNR_LOG_INFO(logger,
                                   "callMethodWithExtendedErrorEnumAsync - 1st - callback - got "
                                   "ProviderRuntimeException with wrong message");
                    JOYNR_LOG_INFO(logger, error.getMessage());
                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                }
            } else {
                JOYNR_LOG_INFO(
                        logger,
                        "callMethodWithExtendedErrorEnumAsync - 1st - callback - got invalid "
                        "exception "
                        "type");
                JOYNR_LOG_INFO(logger, error.getTypeName());
                JOYNR_LOG_INFO(logger, error.getMessage());
                methodWithExtendedErrorEnumAsyncCallbackResult = false;
            }
            methodWithExtendedErrorEnumAsyncCallbackDone = true;
        };

        std::shared_ptr<joynr::Future<void>> future =
                testInterfaceProxy->methodWithExtendedErrorEnumAsync(
                        arg, onSuccess, nullptr, onRuntimeError);

        try {
            long timeoutInMilliseconds = 8000;
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithExtendedErrorEnumAsync - 1st - about to call future.getReply");
            // future->wait(timeoutInMilliseconds);
            future->get();
            JOYNR_LOG_INFO(logger,
                           "callMethodWithExtendedErrorEnumAsync - 1st - returned from "
                           "future->get()");
            if (future->isOk()) {
                JOYNR_LOG_INFO(logger,
                               "callMethodWithExtendedErrorEnumAsync - 1st - unexpected success "
                               "requestStatus");
                JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
                return false;
            }
            JOYNR_LOG_INFO(logger,
                           "callMethodWithExtendedErrorEnumAsync - 1st - call flow unexpectedly "
                           "passed future->wait call, expected it to throw exception");
            JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
            return false;
        } catch (joynr::exceptions::ProviderRuntimeException& error) {
            // expected case
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithExtendedErrorEnumAsync - 1st - caught ProviderRuntimeException");
            if (error.getTypeName() != "joynr.exceptions.ProviderRuntimeException") {
                JOYNR_LOG_INFO(logger,
                               "callMethodWithExtendedErrorEnumAsync - 1st - got invalid "
                               "exception "
                               "type");
                JOYNR_LOG_INFO(logger, error.getTypeName());
                JOYNR_LOG_INFO(logger, error.getMessage());
                JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
                return false;
            }

            if (error.getMessage() != "Exception from methodWithExtendedErrorEnum") {
                JOYNR_LOG_INFO(logger,
                               "callMethodWithExtendedErrorEnumAsync - 1st - got "
                               "ProviderRuntimeException with wrong message");
                JOYNR_LOG_INFO(logger, error.getMessage());
                JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
                return false;
            }

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            waitForChange(methodWithExtendedErrorEnumAsyncCallbackDone, 1000);
            if (methodWithExtendedErrorEnumAsyncCallbackDone == false) {
                JOYNR_LOG_INFO(
                        logger, "callMethodWithExtendedErrorEnumAsync - 1st - callback NOT done");
                JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
                return false;
            }
            if (methodWithExtendedErrorEnumAsyncCallbackResult == false) {
                JOYNR_LOG_INFO(
                        logger,
                        "callMethodWithExtendedErrorEnumAsync - 1st - callback reported error");
                JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
                return false;
            }
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithExtendedErrorEnumAsync - 1st - callback has set OK flags");
            // fallthrough
        } catch (joynr::exceptions::ApplicationException& e) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithExtendedErrorEnumAsync - 1st - caught ApplicationException");
            JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
            return false;
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithExtendedErrorEnumAsync - 1st - caught JoynrRuntimeException");
            JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(
                logger,
                "callMethodWithExtendedErrorEnumAsync - 1st - caught unexpected exception type");
        JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
        return false;
    }
#endif

    // Checks with ApplicationException

    methodWithExtendedErrorEnumAsyncCallbackResult = false;
    methodWithExtendedErrorEnumAsyncCallbackDone = false;
    try {
        // setup input parameters
        std::string arg = "ApplicationException_1";

        std::function<void()> onSuccess = []() {
            // check results
            methodWithExtendedErrorEnumAsyncCallbackResult = false;
            methodWithExtendedErrorEnumAsyncCallbackDone = true;
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithExtendedErrorEnumAsync - 2nd - callback - unexpected call to "
                    "onSuccess");
        };

        using joynr::interlanguagetest::TestInterface::MethodWithExtendedErrorEnumErrorEnum;
        auto onApplicationError = [](const MethodWithExtendedErrorEnumErrorEnum::Enum& errorEnum) {
            if (errorEnum != MethodWithExtendedErrorEnumErrorEnum::ERROR_3_3_NTC) {
                JOYNR_LOG_INFO(logger,
                               "callMethodWithExtendedErrorEnumAsync - 2nd - callback - got "
                               "ApplicationException with wrong enumeration");
                methodWithExtendedErrorEnumAsyncCallbackResult = false;
            } else {
                JOYNR_LOG_INFO(logger,
                               "callMethodWithExtendedErrorEnumAsync - 2nd - callback - got "
                               "expected ApplicationException");
                methodWithExtendedErrorEnumAsyncCallbackResult = true;
            }
            methodWithExtendedErrorEnumAsyncCallbackDone = true;
        };
        auto onRuntimeError = [](const exceptions::JoynrRuntimeException& error) {
            JOYNR_LOG_INFO(logger,
                           "callMethodWithExtendedErrorEnumAsync - 2nd - callback - got invalid "
                           "exception "
                           "type");
            JOYNR_LOG_INFO(logger, error.getTypeName());
            JOYNR_LOG_INFO(logger, error.getMessage());
            methodWithExtendedErrorEnumAsyncCallbackResult = false;
            methodWithExtendedErrorEnumAsyncCallbackDone = true;
        };

        std::shared_ptr<joynr::Future<void>> future =
                testInterfaceProxy->methodWithExtendedErrorEnumAsync(
                        arg, onSuccess, onApplicationError, onRuntimeError);

        try {
            long timeoutInMilliseconds = 8000;
            JOYNR_LOG_INFO(logger,
                           "callMethodWithExtendedErrorEnumAsync - 2nd - about to call "
                           "future.waitForFinished()");
            // future->wait(timeoutInMilliseconds);
            future->get();
            JOYNR_LOG_INFO(logger,
                           "callMethodWithExtendedErrorEnumAsync - 2nd - returned from "
                           "future.get()");
            if (future->isOk()) {
                JOYNR_LOG_INFO(logger,
                               "callMethodWithExtendedErrorEnumAsync - 2nd - unexpected success "
                               "requestStatus");
                JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
                return false;
            }
            JOYNR_LOG_INFO(logger,
                           "callMethodWithExtendedErrorEnumAsync - 2nd - call flow unexpectedly "
                           "passed future->wait call, expected it to throw exception");
            JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
            return false;
        } catch (joynr::exceptions::ProviderRuntimeException& error) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithExtendedErrorEnumAsync - 2nd - caught ProviderRuntimeException");
            JOYNR_LOG_INFO(logger, error.getTypeName());
            JOYNR_LOG_INFO(logger, error.getMessage());
            JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
            return false;
        } catch (joynr::exceptions::ApplicationException& error) {
            // expected case
            if (error.getTypeName() == "joynr.exceptions.ApplicationException") {
                try {
                    const joynr::exceptions::ApplicationException& appError =
                            dynamic_cast<const joynr::exceptions::ApplicationException&>(error);
                    // check enum value
                    if (appError.getError<joynr::interlanguagetest::TestInterface::
                                                  MethodWithExtendedErrorEnumErrorEnum::Enum>() !=
                        joynr::interlanguagetest::TestInterface::
                                MethodWithExtendedErrorEnumErrorEnum::ERROR_3_3_NTC) {
                        JOYNR_LOG_INFO(logger,
                                       "callMethodWithExtendedErrorEnumAsync - 2nd - got "
                                       "ApplicationException with wrong enumeration");
                        JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
                        return false;
                    } else {
                        JOYNR_LOG_INFO(logger,
                                       "callMethodWithExtendedErrorEnumAsync - 2nd - got "
                                       "expected ApplicationException");
                        // fallthrough
                    }
                } catch (std::bad_cast& bc) {
                    JOYNR_LOG_INFO(logger,
                                   "callMethodWithExtendedErrorEnumAsync - 2nd - "
                                   "cast to ApplicationException failed");
                    JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
                    return false;
                }
            } else {
                JOYNR_LOG_INFO(logger,
                               "callMethodWithExtendedErrorEnumAsync - 2nd - got invalid "
                               "exception "
                               "type");
                JOYNR_LOG_INFO(logger, error.getTypeName());
                JOYNR_LOG_INFO(logger, error.getMessage());
                JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
                return false;
            }

            JOYNR_LOG_INFO(
                    logger, "callMethodWithExtendedErrorEnumAsync - 2nd - catch error checks OK");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            waitForChange(methodWithExtendedErrorEnumAsyncCallbackDone, 1000);
            if (methodWithExtendedErrorEnumAsyncCallbackDone == false) {
                JOYNR_LOG_INFO(
                        logger, "callMethodWithExtendedErrorEnumAsync - 2nd - callback NOT done");
                JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
                return false;
            }
            if (methodWithExtendedErrorEnumAsyncCallbackResult == false) {
                JOYNR_LOG_INFO(
                        logger,
                        "callMethodWithExtendedErrorEnumAsync - 2nd - callback reported error");
                JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
                return false;
            }
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithExtendedErrorEnumAsync - 2nd - callback has set OK flags");
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithExtendedErrorEnumAsync - 2nd - caught JoynrRuntimeException");
            JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(
                logger, "callMethodWithExtendedErrorEnumAsync - 2nd - caught other exception");
        JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
        return false;
    }

    // further checks

    methodWithExtendedErrorEnumAsyncCallbackResult = false;
    methodWithExtendedErrorEnumAsyncCallbackDone = false;
    try {
        // setup input parameters
        std::string arg = "ApplicationException_2";

        std::function<void()> onSuccess = []() {
            // check results
            methodWithExtendedErrorEnumAsyncCallbackResult = false;
            methodWithExtendedErrorEnumAsyncCallbackDone = true;
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithExtendedErrorEnumAsync - 3rd - callback - unexpected call to "
                    "onSuccess");
        };

        using joynr::interlanguagetest::TestInterface::MethodWithExtendedErrorEnumErrorEnum;
        auto onApplicationError = [](const MethodWithExtendedErrorEnumErrorEnum::Enum& errorEnum) {
            if (errorEnum != MethodWithExtendedErrorEnumErrorEnum::ERROR_2_1_TC2) {
                JOYNR_LOG_INFO(logger,
                               "callMethodWithExtendedErrorEnumAsync - 3rd - callback - got "
                               "ApplicationException with wrong enumeration");
                methodWithExtendedErrorEnumAsyncCallbackResult = false;
            } else {
                JOYNR_LOG_INFO(logger,
                               "callMethodWithExtendedErrorEnumAsync - 3rd - callback - got "
                               "expected ApplicationException");
                methodWithExtendedErrorEnumAsyncCallbackResult = true;
            }
            methodWithExtendedErrorEnumAsyncCallbackDone = true;
        };

        auto onRuntimeError = [](const exceptions::JoynrRuntimeException& error) {
            JOYNR_LOG_INFO(logger,
                           "callMethodWithExtendedErrorEnumAsync - 3rd - callback - got invalid "
                           "exception "
                           "type");
            JOYNR_LOG_INFO(logger, error.getTypeName());
            JOYNR_LOG_INFO(logger, error.getMessage());
            methodWithExtendedErrorEnumAsyncCallbackResult = false;
            methodWithExtendedErrorEnumAsyncCallbackDone = true;
        };

        // the method call is not expected to throw
        std::shared_ptr<joynr::Future<void>> future =
                testInterfaceProxy->methodWithExtendedErrorEnumAsync(
                        arg, onSuccess, onApplicationError, onRuntimeError);

        try {
            long timeoutInMilliseconds = 8000;
            JOYNR_LOG_INFO(logger,
                           "callMethodWithExtendedErrorEnumAsync - 3rd - about to call "
                           "future->waitForFinished()");
            // future->wait(timeoutInMilliseconds);
            future->get();
            JOYNR_LOG_INFO(logger,
                           "callMethodWithExtendedErrorEnumAsync - 3rd - returned from "
                           "future->get()");
            if (future->isOk()) {
                JOYNR_LOG_INFO(logger,
                               "callMethodWithExtendedErrorEnumAsync - 3rd - unexpected success "
                               "requestStatus");
                JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
                return false;
            }
            JOYNR_LOG_INFO(logger,
                           "callMethodWithExtendedErrorEnumAsync - 2nd - call flow unexpectedly "
                           "passed future->wait call, expected it to throw exception");
            JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
            return false;
        } catch (joynr::exceptions::ProviderRuntimeException& error) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithExtendedErrorEnumAsync - 3rd - caught ProviderRuntimeException");
            JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
            return false;
        } catch (joynr::exceptions::ApplicationException& error) {
            if (error.getTypeName() == "joynr.exceptions.ApplicationException") {
                try {
                    const joynr::exceptions::ApplicationException& appError =
                            dynamic_cast<const joynr::exceptions::ApplicationException&>(error);
                    // check enum value
                    if (appError.getError<joynr::interlanguagetest::TestInterface::
                                                  MethodWithExtendedErrorEnumErrorEnum::Enum>() !=
                        joynr::interlanguagetest::TestInterface::
                                MethodWithExtendedErrorEnumErrorEnum::ERROR_2_1_TC2) {
                        JOYNR_LOG_INFO(logger,
                                       "callMethodWithExtendedErrorEnumAsync - 3rd - got "
                                       "ApplicationException with wrong enumeration");
                        JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
                        return false;
                    } else {
                        JOYNR_LOG_INFO(logger,
                                       "callMethodWithExtendedErrorEnumAsync - 3rd - got "
                                       "expected ApplicationException");
                        // fallthrough
                    }
                } catch (std::bad_cast& bc) {
                    JOYNR_LOG_INFO(logger,
                                   "callMethodWithExtendedErrorEnumAsync - 3rd - "
                                   "cast to ApplicationException failed");
                    JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
                    return false;
                }
            } else {
                JOYNR_LOG_INFO(
                        logger,
                        "callMethodWithExtendedErrorEnumAsync - 3rd - callback - got invalid "
                        "exception "
                        "type");
                JOYNR_LOG_INFO(logger, error.getTypeName());
                JOYNR_LOG_INFO(logger, error.getMessage());
                JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
                return false;
            }

            JOYNR_LOG_INFO(
                    logger, "callMethodWithExtendedErrorEnumAsync - 3rd - catch error checks OK");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            waitForChange(methodWithExtendedErrorEnumAsyncCallbackDone, 1000);
            if (methodWithExtendedErrorEnumAsyncCallbackDone == false) {
                JOYNR_LOG_INFO(
                        logger, "callMethodWithExtendedErrorEnumAsync - 3rd - callback NOT done");
                JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
                return false;
            }
            if (methodWithExtendedErrorEnumAsyncCallbackResult == false) {
                JOYNR_LOG_INFO(
                        logger,
                        "callMethodWithExtendedErrorEnumAsync - 3rd - callback reported error");
                JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
                return false;
            }
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithExtendedErrorEnumAsync - 3rd - callback has set OK flags");
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_INFO(
                    logger,
                    "callMethodWithExtendedErrorEnumAsync - 3rd - caught JoynrRuntimeException");
            JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(
                logger, "callMethodWithExtendedErrorEnumAsync - 3rd - caught other exception");
        JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - FAILED");
        return false;
    }

    JOYNR_LOG_INFO(logger, "callMethodWithExtendedErrorEnumAsync - OK");
    return true;
}

// getter

bool callSetAttributeUInt8(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callSetAttributeUInt8");
    try {
        testInterfaceProxy->setAttributeUInt8(127);
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callSetAttributeUInt8: exception: " << e.getMessage() << std::endl;
        JOYNR_LOG_INFO(logger, "callSetAttributeUInt8 - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callSetAttributeUInt8 - OK");
    return true;
}

bool callGetAttributeUInt8(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callGetAttributeUInt8");
    try {
        uint8_t result;
        testInterfaceProxy->getAttributeUInt8(result);
        if (result != 127) {
            JOYNR_LOG_INFO(logger, "callGetAttributeUInt8 - unexpected content");
            JOYNR_LOG_INFO(logger, "callGetAttributeUInt8 - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callGetAttributeUInt8: exception: " << e.getMessage() << std::endl;
        JOYNR_LOG_INFO(logger, "callGetAttributeUInt8 - FAILED");
        return false;
    }

    JOYNR_LOG_INFO(logger, "callGetAttributeUInt8 - OK");
    return true;
}

bool callSetAttributeDouble(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callSetAttributeDouble");
    try {
        testInterfaceProxy->setAttributeDouble(1.1);
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callSetAttributeDouble: exception: " << e.getMessage() << std::endl;
        JOYNR_LOG_INFO(logger, "callSetAttributeDouble - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callSetAttributeDouble - OK");
    return true;
}

bool callGetAttributeDouble(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callGetAttributeDouble");
    try {
        double result;
        testInterfaceProxy->getAttributeDouble(result);
        if (!IltUtil::cmpDouble(result, 1.1)) {
            JOYNR_LOG_INFO(logger, "callGetAttributeDouble - unexpected content");
            JOYNR_LOG_INFO(logger, "callGetAttributeDouble - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callGetAttributeDouble: exception: " << e.getMessage() << std::endl;
        JOYNR_LOG_INFO(logger, "callGetAttributeDouble - FAILED");
        return false;
    }

    JOYNR_LOG_INFO(logger, "callGetAttributeDouble - OK");
    return true;
}

bool callGetAttributeBooleanReadonly(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callGetAttributeBooleanReadonly");
    try {
        bool result;
        testInterfaceProxy->getAttributeBooleanReadonly(result);
        if (result != true) {
            JOYNR_LOG_INFO(logger, "callGetAttributeDoubleReadonly - unexpected content");
            JOYNR_LOG_INFO(logger, "callGetAttributeDoubleReadonly - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callGetAttributeBoolean: exception: " << e.getMessage() << std::endl;
        JOYNR_LOG_INFO(logger, "callGetAttributeBooleanReadonly - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callGetAttributeBooleanReadonly - OK");
    return true;
}

bool callSetAttributeStringNoSubscriptions(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callSetAttributeStringNoSubscriptions");
    try {
        testInterfaceProxy->setAttributeStringNoSubscriptions("Hello world");
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callSetAttributeStringNoSubscriptions: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callSetAttributeStringNoSubscriptions - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callSetAttributeStringNoSubscriptions - OK");
    return true;
}

bool callGetAttributeStringNoSubscriptions(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callGetAttributeStringNoSubscriptions");
    try {
        std::string result;
        testInterfaceProxy->getAttributeStringNoSubscriptions(result);
        if (result != "Hello world") {
            JOYNR_LOG_INFO(logger, "callGetAttributeDoubleNoSubscriptions - unexpected content");
            JOYNR_LOG_INFO(logger, "callGetAttributeDoubleNoSubscriptions - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callGetAttributeStringNoSubscriptions: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callGetAttributeStringNoSubscriptions - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callGetAttributeStringNoSubscriptions - OK");
    return true;
}

bool callGetAttributeInt8readonlyNoSubscriptions(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callGetAttributeInt8readonlyNoSubscriptions");
    try {
        int8_t result;
        testInterfaceProxy->getAttributeInt8readonlyNoSubscriptions(result);
        if (result != -128) {
            JOYNR_LOG_INFO(
                    logger, "callGetAttributeInt8readonlyNoSubscriptions - unexpected content");
            JOYNR_LOG_INFO(logger, "callGetAttributeInt8readonlyNoSubscriptions - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callGetAttributeInt8readonlyNoSubscriptions: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callGetAttributeInt8readonlyNoSubscriptions - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callGetAttributeInt8readonlyNoSubscriptions - OK");
    return true;
}

bool callSetAttributeArrayOfStringImplicit(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callSetAttributeArrayOfStringImplicit");
    try {
        std::vector<std::string> stringArrayArg = IltUtil::createStringArray();
        testInterfaceProxy->setAttributeArrayOfStringImplicit(stringArrayArg);
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callSetAttributeArrayOfStringImplicit: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callSetAttributeArrayOfStringImplicit - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callSetAttributeArrayOfStringImplicit - OK");
    return true;
}

bool callGetAttributeArrayOfStringImplicit(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callGetAttributeArrayOfStringImplicit");
    try {
        std::vector<std::string> result;
        testInterfaceProxy->getAttributeArrayOfStringImplicit(result);
        if (!IltUtil::checkStringArray(result)) {
            JOYNR_LOG_INFO(logger, "callGetAttributeDouble - unexpected content");
            JOYNR_LOG_INFO(logger, "callGetAttributeDouble - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callGetAttributeArrayOfStringImplicit: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callGetAttributeArrayOfStringImplicit - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callGetAttributeArrayOfStringImplicit - OK");
    return true;
}

bool callSetAttributeEnumeration(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callSetAttributeEnumeration");
    try {
        joynr::interlanguagetest::Enumeration::Enum enumerationArg =
                joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_2;
        testInterfaceProxy->setAttributeEnumeration(enumerationArg);
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callSetAttributeEnumeration: exception: " << e.getMessage() << std::endl;
        JOYNR_LOG_INFO(logger, "callSetAttributeEnumeration - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callSetAttributeEnumeration - OK");
    return true;
}

bool callGetAttributeEnumeration(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callGetAttributeEnumeration");
    try {
        joynr::interlanguagetest::Enumeration::Enum result;
        testInterfaceProxy->getAttributeEnumeration(result);
        if (result != joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_2) {
            JOYNR_LOG_INFO(logger, "callGetAttributeDouble - unexpected content");
            JOYNR_LOG_INFO(logger, "callGetAttributeDouble - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callGetAttributeEnumeration: exception: " << e.getMessage() << std::endl;
        JOYNR_LOG_INFO(logger, "callGetAttributeEnumeration - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callGetAttributeEnumeration - OK");
    return true;
}

bool callGetAttributeExtendedEnumerationReadonly(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callGetAttributeExtendedEnumerationReadonly");
    try {
        joynr::interlanguagetest::namedTypeCollection2::ExtendedEnumerationWithPartlyDefinedValues::
                Enum result;
        testInterfaceProxy->getAttributeExtendedEnumerationReadonly(result);
        if (result != joynr::interlanguagetest::namedTypeCollection2::
                              ExtendedEnumerationWithPartlyDefinedValues::
                                      ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES) {
            JOYNR_LOG_INFO(
                    logger, "callGetAttributeExtendedEnumerationReadonly - unexpected content");
            JOYNR_LOG_INFO(logger, "callGetAttributeExtendedEnumerationReadonly - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callGetAttributeExtendedEnumerationReadonly: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callGetAttributeExtendedEnumerationReadonly - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callGetAttributeExtendedEnumerationReadonly - OK");
    return true;
}

bool callSetAttributeBaseStruct(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callSetAttributeBaseStruct");
    try {
        joynr::interlanguagetest::namedTypeCollection2::BaseStruct baseStructArg =
                IltUtil::createBaseStruct();
        testInterfaceProxy->setAttributeBaseStruct(baseStructArg);
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callSetAttributeBaseStruct: exception: " << e.getMessage() << std::endl;
        JOYNR_LOG_INFO(logger, "callSetAttributeBaseStruct - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callSetAttributeBaseStruct - OK");
    return true;
}

bool callGetAttributeBaseStruct(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callGetAttributeBaseStruct");
    try {
        joynr::interlanguagetest::namedTypeCollection2::BaseStruct result;
        testInterfaceProxy->getAttributeBaseStruct(result);
        if (!IltUtil::checkBaseStruct(result)) {
            JOYNR_LOG_INFO(logger, "callGetAttributeBaseStruct - unexpected content");
            JOYNR_LOG_INFO(logger, "callGetAttributeBaseStruct - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callGetAttributeBaseStruct: exception: " << e.getMessage() << std::endl;
        JOYNR_LOG_INFO(logger, "callGetAttributeBaseStruct - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callGetAttributeBaseStruct - OK");
    return true;
}

bool callSetAttributeExtendedExtendedBaseStruct(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callSetAttributeExtendedExtendedBaseStruct");
    try {
        joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct baseStructArg =
                IltUtil::createExtendedExtendedBaseStruct();
        testInterfaceProxy->setAttributeExtendedExtendedBaseStruct(baseStructArg);
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callSetAttributeExtendedExtendedBaseStruct: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callSetAttributeExtendedExtendedBaseStruct - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callSetAttributeExtendedExtendedBaseStruct - OK");
    return true;
}

bool callGetAttributeExtendedExtendedBaseStruct(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callGetAttributeExtendedExtendedBaseStruct");
    try {
        joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct result;
        testInterfaceProxy->getAttributeExtendedExtendedBaseStruct(result);
        if (!IltUtil::checkExtendedExtendedBaseStruct(result)) {
            JOYNR_LOG_INFO(
                    logger, "callGetAttributeExtendedExtendedBaseStruct - unexpected content");
            JOYNR_LOG_INFO(logger, "callGetAttributeExtendedExtendedBaseStruct - FAILED");
            return false;
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callGetAttributeExtendedExtendedBaseStruct: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callGetAttributeExtendedExtendedBaseStruct - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callGetAttributeExtendedExtendedBaseStruct - OK");
    return true;
}

bool callSetAttributeMapStringString(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callSetAttributeMapStringString");
    try {
        joynr::interlanguagetest::namedTypeCollection2::MapStringString mapStringStringArg;
        mapStringStringArg.insert(
                std::pair<std::string, std::string>("keyString1", "valueString1"));
        mapStringStringArg.insert(
                std::pair<std::string, std::string>("keyString2", "valueString2"));
        mapStringStringArg.insert(
                std::pair<std::string, std::string>("keyString3", "valueString3"));
        testInterfaceProxy->setAttributeMapStringString(mapStringStringArg);
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callSetAttributeMapStringString: exception: " << e.getMessage() << std::endl;
        JOYNR_LOG_INFO(logger, "callSetAttributeMapStringString - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callSetAttributeMapStringString - OK");
    return true;
}

bool callGetAttributeMapStringString(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callGetAttributeMapStringString");
    try {
        joynr::interlanguagetest::namedTypeCollection2::MapStringString result;
        testInterfaceProxy->getAttributeMapStringString(result);
        std::map<std::string, std::string>::iterator it;
        for (int i = 1; i <= 3; i++) {
            it = result.find("keyString" + std::to_string(i));
            if (it == result.end()) {
                JOYNR_LOG_INFO(
                        logger, "callGetAttributeMapStringString - unexpected content (end)");
                JOYNR_LOG_INFO(logger, "callGetAttributeMapStringString - FAILED");
                return false;
            }
            std::string expected = "valueString" + std::to_string(i);
            if (it->second != expected) {
                JOYNR_LOG_INFO(
                        logger,
                        "callGetAttributeMapStringString - unexpected content: " + it->second);
                JOYNR_LOG_INFO(logger, "callGetAttributeMapStringString - FAILED");
                return false;
            }
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callGetAttributeMapStringString: exception: " << e.getMessage() << std::endl;
        JOYNR_LOG_INFO(logger, "callGetAttributeMapStringString - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callGetAttributeMapStringString - OK");
    return true;
}

bool callMethodWithSingleMapParameters(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callMethodWithSingleMapParameters");
    try {
        joynr::interlanguagetest::namedTypeCollection2::MapStringString arg;
        arg.insert(std::pair<std::string, std::string>("keyString1", "valueString1"));
        arg.insert(std::pair<std::string, std::string>("keyString2", "valueString2"));
        arg.insert(std::pair<std::string, std::string>("keyString3", "valueString3"));
        joynr::interlanguagetest::namedTypeCollection2::MapStringString result;
        testInterfaceProxy->methodWithSingleMapParameters(result, arg);

        std::map<std::string, std::string>::iterator it;
        for (int i = 1; i <= 3; i++) {
            it = result.find("valueString" + std::to_string(i));
            if (it == result.end()) {
                JOYNR_LOG_INFO(
                        logger, "callMethodWithSingleMapParameters - unexpected content (end)");
                JOYNR_LOG_INFO(logger, "callMethodWithSingleMapParameters - FAILED");
                return false;
            }
            std::string expected = "keyString" + std::to_string(i);
            if (it->second != expected) {
                JOYNR_LOG_INFO(
                        logger,
                        "callMethodWithSingleMapParameters - unexpected content: " + it->second);
                JOYNR_LOG_INFO(logger, "callMethodWithSingleMapParameters - FAILED");
                return false;
            }
        }
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        std::cout << "callMethodWithSingleMapParameters: exception: " << e.getMessage()
                  << std::endl;
        JOYNR_LOG_INFO(logger, "callMethodWithSingleMapParameters - FAILED");
        return false;
    }
    JOYNR_LOG_INFO(logger, "callMethodWithSingleMapParameters - OK");
    return true;
}

bool callSetAttributeWithExceptionFromSetter(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callSetAttributeWithExceptionFromSetter");
    try {
        testInterfaceProxy->setAttributeWithExceptionFromSetter(false);
        JOYNR_LOG_INFO(logger,
                       "callSetAttributeWithExceptionFromSetter - Unexpected continuation "
                       "without exception");
        JOYNR_LOG_INFO(logger, "callSetAttributeWithExceptionFromSetter - FAILED");
        return false;
    } catch (joynr::exceptions::ProviderRuntimeException& e) {
        if (e.getMessage() != "Exception from setAttributeWithExceptionFromSetter") {
            JOYNR_LOG_INFO(
                    logger, "callSetAttributeWithExceptionFromSetter - invalid exception message");
            JOYNR_LOG_INFO(logger, "callSetAttributeWithExceptionFromSetter - FAILED");
            return false;
        }
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(
                logger, "callSetAttributeWithExceptionFromSetter - unexpected exception type");
        JOYNR_LOG_INFO(logger, "callSetAttributeWithExceptionFromSetter - FAILED");
        return false;
    }

    JOYNR_LOG_INFO(logger, "callSetAttributeWithExceptionFromSetter - OK");
    return true;
}

bool callGetAttributeWithExceptionFromGetter(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    JOYNR_LOG_INFO(logger, "callGetAttributeWithExceptionFromGetter");
    try {
        bool result;
        testInterfaceProxy->getAttributeWithExceptionFromGetter(result);
        JOYNR_LOG_INFO(logger,
                       "callGetAttributeWithExceptionFromGetter - Unexpected continuation "
                       "without exception");
        JOYNR_LOG_INFO(logger, "callGetAttributeWithExceptionFromGetter - FAILED");
        return false;
    } catch (joynr::exceptions::ProviderRuntimeException& e) {
        if (e.getMessage() != "Exception from getAttributeWithExceptionFromGetter") {
            JOYNR_LOG_INFO(
                    logger, "callGetAttributeWithExceptionFromGetter - invalid exception message");
            JOYNR_LOG_INFO(logger, "callGetAttributeWithExceptionFromGetter - FAILED");
            return false;
        }
        // OK
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(
                logger, "callGetAttributeWithExceptionFromGetter - unexpected exception type");
        JOYNR_LOG_INFO(logger, "callGetAttributeWithExceptionFromGetter - FAILED");
        return false;
    }

    JOYNR_LOG_INFO(logger, "callGetAttributeWithExceptionFromGetter - OK");
    return true;
}

// variables that are to be changed inside callbacks must be declared global
volatile bool subscribeBroadcastWithSinglePrimitiveParameterCallbackDone = false;
volatile bool subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = false;

class BroadcastWithSinglePrimitiveParameterBroadcastListener
        : public SubscriptionListener<std::string>
{
public:
    BroadcastWithSinglePrimitiveParameterBroadcastListener()
    {
    }

    ~BroadcastWithSinglePrimitiveParameterBroadcastListener()
    {
    }

    void onReceive(const std::string& stringOut)
    {
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithSinglePrimitiveParameter - callback - got broadcast");
        if (stringOut != "boom") {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithSinglePrimitiveParameter - callback - invalid "
                    "content");
            subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithSinglePrimitiveParameter - callback - content OK");
            subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = true;
        }
        subscribeBroadcastWithSinglePrimitiveParameterCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_INFO(logger,
                       "callSubscribeBroadcastWithSinglePrimitiveParameter - callback - got error");
        subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = false;
        subscribeBroadcastWithSinglePrimitiveParameterCallbackDone = true;
    }
};

bool callSubscribeBroadcastWithSinglePrimitiveParameter(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    joynr::OnChangeSubscriptionQos subscriptionQos(validity, minInterval_ms);
    bool result;
    JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSinglePrimitiveParameter");
    try {
        std::shared_ptr<ISubscriptionListener<std::string>> listener(
                new BroadcastWithSinglePrimitiveParameterBroadcastListener());
        subscriptionId =
                testInterfaceProxy->subscribeToBroadcastWithSinglePrimitiveParameterBroadcast(
                        listener, subscriptionQos);
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithSinglePrimitiveParameter - subscription successful");
        JOYNR_LOG_INFO(
                logger, "callSubscribeBroadcastWithSinglePrimitiveParameter - Waiting one second");
        usleep(1000000);
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithSinglePrimitiveParameter - Wait done, invoking fire "
                "method");
        testInterfaceProxy->methodToFireBroadcastWithSinglePrimitiveParameter();
        JOYNR_LOG_INFO(
                logger, "callSubscribeBroadcastWithSinglePrimitiveParameter - fire method invoked");
        waitForChange(subscribeBroadcastWithSinglePrimitiveParameterCallbackDone, 1000);
        if (!subscribeBroadcastWithSinglePrimitiveParameterCallbackDone) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithSinglePrimitiveParameter - callback did not get "
                    "called in time");
            result = false;
        } else if (subscribeBroadcastWithSinglePrimitiveParameterCallbackResult) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithSinglePrimitiveParameter - callback got called and "
                    "received expected publication");
            result = true;
        } else {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithSinglePrimitiveParameter - callback got called but "
                    "received unexpected error or publication content");
            result = false;
        }

        // try to unsubscribe in any case
        try {
            testInterfaceProxy->unsubscribeFromBroadcastWithSinglePrimitiveParameterBroadcast(
                    subscriptionId);
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithSinglePrimitiveParameter - unsubscribe successful");
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithSinglePrimitiveParameter - caught unexpected "
                           "exception on unsubscribe");
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSinglePrimitiveParameter - FAILED");
            result = false;
        }

        if (!result) {
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSinglePrimitiveParameter - FAILED");
        } else {
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSinglePrimitiveParameter - OK");
        }
        return result;
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithSinglePrimitiveParameter - caught unexpected exception");
        JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSinglePrimitiveParameter - FAILED");
        return false;
    }
}

// variables that are to be changed inside callbacks must be declared global
volatile bool subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone = false;
volatile bool subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = false;

class BroadcastWithMultiplePrimitiveParametersBroadcastListener
        : public SubscriptionListener<double, std::string>
{
public:
    BroadcastWithMultiplePrimitiveParametersBroadcastListener()
    {
    }

    ~BroadcastWithMultiplePrimitiveParametersBroadcastListener()
    {
    }

    void onReceive(const double& doubleOut, const std::string& stringOut)
    {
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithMultiplePrimitiveParameters - callback - got broadcast");
        if (!IltUtil::cmpDouble(doubleOut, 1.1)) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultiplePrimitiveParameters - callback - invalid "
                    "doubleOut content");
            subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = false;
        } else if (stringOut != "boom") {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultiplePrimitiveParameters - callback - invalid "
                    "content");
            subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithMultiplePrimitiveParameters - "
                           "callback - content OK");
            subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = true;
        }
        subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithMultiplePrimitiveParameters - callback - got error");
        subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = false;
        subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone = true;
    }
};

bool callSubscribeBroadcastWithMultiplePrimitiveParameters(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    joynr::OnChangeSubscriptionQos subscriptionQos(validity, minInterval_ms);
    bool result;
    JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithMultiplePrimitiveParameters");
    try {
        std::shared_ptr<ISubscriptionListener<double, std::string>> listener(
                new BroadcastWithMultiplePrimitiveParametersBroadcastListener());
        subscriptionId =
                testInterfaceProxy->subscribeToBroadcastWithMultiplePrimitiveParametersBroadcast(
                        listener, subscriptionQos);
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithMultiplePrimitiveParameters - subscription successful");
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithMultiplePrimitiveParameters - Waiting one second");
        usleep(1000000);
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithMultiplePrimitiveParameters - Wait done, invoking fire "
                "method");
        testInterfaceProxy->methodToFireBroadcastWithMultiplePrimitiveParameters();
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithMultiplePrimitiveParameters - fire method invoked");
        waitForChange(subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone, 1000);
        if (!subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultiplePrimitiveParameters - callback did not get "
                    "called in time");
            result = false;
        } else if (subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithMultiplePrimitiveParameters - callback got "
                           "called and "
                           "received expected publication");
            result = true;
        } else {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithMultiplePrimitiveParameters - callback got "
                           "called but "
                           "received unexpected error or publication content");
            result = false;
        }

        // try to unsubscribe in any case
        try {
            testInterfaceProxy->unsubscribeFromBroadcastWithMultiplePrimitiveParametersBroadcast(
                    subscriptionId);
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithMultiplePrimitiveParameters - "
                           "unsubscribe successful");
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultiplePrimitiveParameters - caught unexpected "
                    "exception on unsubscribe");
            JOYNR_LOG_INFO(
                    logger, "callSubscribeBroadcastWithMultiplePrimitiveParameters - FAILED");
            result = false;
        }

        if (!result) {
            JOYNR_LOG_INFO(
                    logger, "callSubscribeBroadcastWithMultiplePrimitiveParameters - FAILED");
        } else {
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithMultiplePrimitiveParameters - OK");
        }
        return result;
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(logger,
                       "callSubscribeBroadcastWithMultiplePrimitiveParameters - caught unexpected "
                       "exception");
        JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithMultiplePrimitiveParameters - FAILED");
        return false;
    }
}

// variables that are to be changed inside callbacks must be declared global
volatile bool subscribeBroadcastWithSingleArrayParameterCallbackDone = false;
volatile bool subscribeBroadcastWithSingleArrayParameterCallbackResult = false;

class BroadcastWithSingleArrayParameterBroadcastListener
        : public SubscriptionListener<std::vector<std::string>>
{
public:
    BroadcastWithSingleArrayParameterBroadcastListener()
    {
    }

    ~BroadcastWithSingleArrayParameterBroadcastListener()
    {
    }

    void onReceive(const std::vector<std::string>& stringArrayOut)
    {
        JOYNR_LOG_INFO(logger,
                       "callSubscribeBroadcastWithSingleArrayParameter - callback - got broadcast");
        if (!IltUtil::checkStringArray(stringArrayOut)) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithSingleArrayParameter - callback - invalid "
                           "content");
            subscribeBroadcastWithSingleArrayParameterCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithSingleArrayParameter - callback - content OK");
            subscribeBroadcastWithSingleArrayParameterCallbackResult = true;
        }
        subscribeBroadcastWithSingleArrayParameterCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_INFO(
                logger, "callSubscribeBroadcastWithSingleArrayParameter - callback - got error");
        subscribeBroadcastWithSingleArrayParameterCallbackResult = false;
        subscribeBroadcastWithSingleArrayParameterCallbackDone = true;
    }
};

bool callSubscribeBroadcastWithSingleArrayParameter(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    joynr::OnChangeSubscriptionQos subscriptionQos(validity, minInterval_ms);
    bool result;
    JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSingleArrayParameter");
    try {
        std::shared_ptr<ISubscriptionListener<std::vector<std::string>>> listener(
                new BroadcastWithSingleArrayParameterBroadcastListener());
        subscriptionId = testInterfaceProxy->subscribeToBroadcastWithSingleArrayParameterBroadcast(
                listener, subscriptionQos);
        JOYNR_LOG_INFO(
                logger, "callSubscribeBroadcastWithSingleArrayParameter - subscription successful");
        JOYNR_LOG_INFO(
                logger, "callSubscribeBroadcastWithSingleArrayParameter - Waiting one second");
        usleep(1000000);
        JOYNR_LOG_INFO(logger,
                       "callSubscribeBroadcastWithSingleArrayParameter - Wait done, invoking fire "
                       "method");
        testInterfaceProxy->methodToFireBroadcastWithSingleArrayParameter();
        JOYNR_LOG_INFO(
                logger, "callSubscribeBroadcastWithSingleArrayParameter - fire method invoked");
        waitForChange(subscribeBroadcastWithSingleArrayParameterCallbackDone, 1000);
        if (!subscribeBroadcastWithSingleArrayParameterCallbackDone) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithSingleArrayParameter - callback did not get "
                           "called in time");
            result = false;
        } else if (subscribeBroadcastWithSingleArrayParameterCallbackResult) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithSingleArrayParameter - callback got called and "
                    "received expected publication");
            result = true;
        } else {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithSingleArrayParameter - callback got called but "
                    "received unexpected error or publication content");
            result = false;
        }

        // try to unsubscribe in any case
        try {
            testInterfaceProxy->unsubscribeFromBroadcastWithSingleArrayParameterBroadcast(
                    subscriptionId);
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithSingleArrayParameter - unsubscribe successful");
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithSingleArrayParameter - caught unexpected "
                           "exception on unsubscribe");
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSingleArrayParameter - FAILED");
            result = false;
        }

        if (!result) {
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSingleArrayParameter - FAILED");
        } else {
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSingleArrayParameter - OK");
        }
        return result;
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithSingleArrayParameter - caught unexpected exception");
        JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSingleArrayParameter - FAILED");
        return false;
    }
}

// variables that are to be changed inside callbacks must be declared global
volatile bool subscribeBroadcastWithMultipleArrayParametersCallbackDone = false;
volatile bool subscribeBroadcastWithMultipleArrayParametersCallbackResult = false;

class BroadcastWithMultipleArrayParametersBroadcastListener
        : public SubscriptionListener<
                  std::vector<uint64_t>,
                  std::vector<
                          joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>>
{
public:
    BroadcastWithMultipleArrayParametersBroadcastListener()
    {
    }

    ~BroadcastWithMultipleArrayParametersBroadcastListener()
    {
    }

    void onReceive(const std::vector<uint64_t>& uInt64ArrayOut,
                   const std::vector<
                           joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>&
                           structWithStringArrayArrayOut)
    {
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithMultipleArrayParameters - callback - got broadcast");
        if (!IltUtil::checkUInt64Array(uInt64ArrayOut)) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithMultipleArrayParameters - callback - invalid "
                           "doubleOut content");
            subscribeBroadcastWithMultipleArrayParametersCallbackResult = false;
        } else if (!IltUtil::checkStructWithStringArrayArray(structWithStringArrayArrayOut)) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithMultipleArrayParameters - callback - invalid "
                           "content");
            subscribeBroadcastWithMultipleArrayParametersCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultipleArrayParameters - callback - content OK");
            subscribeBroadcastWithMultipleArrayParametersCallbackResult = true;
        }
        subscribeBroadcastWithMultipleArrayParametersCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_INFO(
                logger, "callSubscribeBroadcastWithMultipleArrayParameters - callback - got error");
        subscribeBroadcastWithMultipleArrayParametersCallbackResult = false;
        subscribeBroadcastWithMultipleArrayParametersCallbackDone = true;
    }
};

bool callSubscribeBroadcastWithMultipleArrayParameters(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    joynr::OnChangeSubscriptionQos subscriptionQos(validity, minInterval_ms);
    bool result;
    JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithMultipleArrayParameters");
    try {
        std::shared_ptr<ISubscriptionListener<
                std::vector<uint64_t>,
                std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>>>
                listener(new BroadcastWithMultipleArrayParametersBroadcastListener());
        subscriptionId =
                testInterfaceProxy->subscribeToBroadcastWithMultipleArrayParametersBroadcast(
                        listener, subscriptionQos);
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithMultipleArrayParameters - subscription successful");
        JOYNR_LOG_INFO(
                logger, "callSubscribeBroadcastWithMultipleArrayParameters - Waiting one second");
        usleep(1000000);
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithMultipleArrayParameters - Wait done, invoking fire "
                "method");
        testInterfaceProxy->methodToFireBroadcastWithMultipleArrayParameters();
        JOYNR_LOG_INFO(
                logger, "callSubscribeBroadcastWithMultipleArrayParameters - fire method invoked");
        waitForChange(subscribeBroadcastWithMultipleArrayParametersCallbackDone, 1000);
        if (!subscribeBroadcastWithMultipleArrayParametersCallbackDone) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultipleArrayParameters - callback did not get "
                    "called in time");
            result = false;
        } else if (subscribeBroadcastWithMultipleArrayParametersCallbackResult) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultipleArrayParameters - callback got called and "
                    "received expected publication");
            result = true;
        } else {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultipleArrayParameters - callback got called but "
                    "received unexpected error or publication content");
            result = false;
        }

        // try to unsubscribe in any case
        try {
            testInterfaceProxy->unsubscribeFromBroadcastWithMultipleArrayParametersBroadcast(
                    subscriptionId);
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultipleArrayParameters - unsubscribe successful");
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithMultipleArrayParameters - caught unexpected "
                           "exception on unsubscribe");
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithMultipleArrayParameters - FAILED");
            result = false;
        }

        if (!result) {
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithMultipleArrayParameters - FAILED");
        } else {
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithMultipleArrayParameters - OK");
        }
        return result;
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(logger,
                       "callSubscribeBroadcastWithMultipleArrayParameters - caught unexpected "
                       "exception");
        JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithMultipleArrayParameters - FAILED");
        return false;
    }
}

// variables that are to be changed inside callbacks must be declared global
volatile bool subscribeBroadcastWithSingleEnumerationParameterCallbackDone = false;
volatile bool subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;

class BroadcastWithSingleEnumerationParameterBroadcastListener
        : public SubscriptionListener<
                  joynr::interlanguagetest::namedTypeCollection2::
                          ExtendedTypeCollectionEnumerationInTypeCollection::Enum>
{
public:
    BroadcastWithSingleEnumerationParameterBroadcastListener()
    {
    }

    ~BroadcastWithSingleEnumerationParameterBroadcastListener()
    {
    }

    void onReceive(const joynr::interlanguagetest::namedTypeCollection2::
                           ExtendedTypeCollectionEnumerationInTypeCollection::Enum& enumerationOut)
    {
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithSingleEnumerationParameter - callback - got broadcast");
        if (enumerationOut != joynr::interlanguagetest::namedTypeCollection2::
                                      ExtendedTypeCollectionEnumerationInTypeCollection::
                                              ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithSingleEnumerationParameter - callback - invalid "
                    "content");
            subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithSingleEnumerationParameter - callback - content OK");
            subscribeBroadcastWithSingleEnumerationParameterCallbackResult = true;
        }
        subscribeBroadcastWithSingleEnumerationParameterCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithSingleEnumerationParameter - callback - got error");
        subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;
        subscribeBroadcastWithSingleEnumerationParameterCallbackDone = true;
    }
};

bool callSubscribeBroadcastWithSingleEnumerationParameter(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    joynr::OnChangeSubscriptionQos subscriptionQos(validity, minInterval_ms);
    bool result;
    JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSingleEnumerationParameter");
    try {
        std::shared_ptr<ISubscriptionListener<
                joynr::interlanguagetest::namedTypeCollection2::
                        ExtendedTypeCollectionEnumerationInTypeCollection::Enum>>
                listener(new BroadcastWithSingleEnumerationParameterBroadcastListener());
        subscriptionId =
                testInterfaceProxy->subscribeToBroadcastWithSingleEnumerationParameterBroadcast(
                        listener, subscriptionQos);
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithSingleEnumerationParameter - subscription successful");
        JOYNR_LOG_INFO(logger,
                       "callSubscribeBroadcastWithSingleEnumerationParameter - Waiting one second");
        usleep(1000000);
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithSingleEnumerationParameter - Wait done, invoking fire "
                "method");
        testInterfaceProxy->methodToFireBroadcastWithSingleEnumerationParameter();
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithSingleEnumerationParameter - fire method invoked");
        waitForChange(subscribeBroadcastWithSingleEnumerationParameterCallbackDone, 1000);
        if (!subscribeBroadcastWithSingleEnumerationParameterCallbackDone) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithSingleEnumerationParameter - callback did not get "
                    "called in time");
            result = false;
        } else if (subscribeBroadcastWithSingleEnumerationParameterCallbackResult) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithSingleEnumerationParameter - callback got "
                           "called and "
                           "received expected publication");
            result = true;
        } else {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithSingleEnumerationParameter - callback got "
                           "called but "
                           "received unexpected error or publication content");
            result = false;
        }

        // try to unsubscribe in any case
        try {
            testInterfaceProxy->unsubscribeFromBroadcastWithSingleEnumerationParameterBroadcast(
                    subscriptionId);
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithSingleEnumerationParameter - "
                           "unsubscribe successful");
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithSingleEnumerationParameter - caught unexpected "
                    "exception on unsubscribe");
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSingleEnumerationParameter - FAILED");
            result = false;
        }

        if (!result) {
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSingleEnumerationParameter - FAILED");
        } else {
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSingleEnumerationParameter - OK");
        }
        return result;
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(logger,
                       "callSubscribeBroadcastWithSingleEnumerationParameter - caught "
                       "unexpected exception");
        JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSingleEnumerationParameter - FAILED");
        return false;
    }
}

// variables that are to be changed inside callbacks must be declared global
volatile bool subscribeBroadcastWithMultipleEnumerationParametersCallbackDone = false;
volatile bool subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = false;

class BroadcastWithMultipleEnumerationParametersBroadcastListener
        : public SubscriptionListener<joynr::interlanguagetest::namedTypeCollection2::
                                              ExtendedEnumerationWithPartlyDefinedValues::Enum,
                                      joynr::interlanguagetest::Enumeration::Enum>
{
public:
    BroadcastWithMultipleEnumerationParametersBroadcastListener()
    {
    }

    ~BroadcastWithMultipleEnumerationParametersBroadcastListener()
    {
    }

    void onReceive(const joynr::interlanguagetest::namedTypeCollection2::
                           ExtendedEnumerationWithPartlyDefinedValues::Enum& extendedEnumerationOut,
                   const joynr::interlanguagetest::Enumeration::Enum& enumerationOut)
    {
        JOYNR_LOG_INFO(logger,
                       "callSubscribeBroadcastWithMultipleEnumerationParameters - callback "
                       "- got broadcast");
        if (extendedEnumerationOut !=
            joynr::interlanguagetest::namedTypeCollection2::
                    ExtendedEnumerationWithPartlyDefinedValues::
                            ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultipleEnumerationParameters - callback - invalid "
                    "extendedEnumerationOut content");
            subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = false;
        } else if (enumerationOut != joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_1) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultipleEnumerationParameters - callback - invalid "
                    "enumerationOut content");
            subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultipleEnumerationParameters - callback - content "
                    "OK");
            subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = true;
        }
        subscribeBroadcastWithMultipleEnumerationParametersCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithMultipleEnumerationParameters - callback - got error");
        subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = false;
        subscribeBroadcastWithMultipleEnumerationParametersCallbackDone = true;
    }
};

bool callSubscribeBroadcastWithMultipleEnumerationParameters(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    joynr::OnChangeSubscriptionQos subscriptionQos(validity, minInterval_ms);
    bool result;
    JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithMultipleEnumerationParameters");
    try {
        std::shared_ptr<
                ISubscriptionListener<joynr::interlanguagetest::namedTypeCollection2::
                                              ExtendedEnumerationWithPartlyDefinedValues::Enum,
                                      joynr::interlanguagetest::Enumeration::Enum>>
                listener(new BroadcastWithMultipleEnumerationParametersBroadcastListener());
        subscriptionId =
                testInterfaceProxy->subscribeToBroadcastWithMultipleEnumerationParametersBroadcast(
                        listener, subscriptionQos);
        JOYNR_LOG_INFO(logger,
                       "callSubscribeBroadcastWithMultipleEnumerationParameters - "
                       "subscription successful");
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithMultipleEnumerationParameters - Waiting one second");
        usleep(1000000);
        JOYNR_LOG_INFO(logger,
                       "callSubscribeBroadcastWithMultipleEnumerationParameters - Wait done, "
                       "invoking fire "
                       "method");
        testInterfaceProxy->methodToFireBroadcastWithMultipleEnumerationParameters();
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithMultipleEnumerationParameters - fire method invoked");
        waitForChange(subscribeBroadcastWithMultipleEnumerationParametersCallbackDone, 1000);
        if (!subscribeBroadcastWithMultipleEnumerationParametersCallbackDone) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithMultipleEnumerationParameters - callback did "
                           "not get "
                           "called in time");
            result = false;
        } else if (subscribeBroadcastWithMultipleEnumerationParametersCallbackResult) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultipleEnumerationParameters - callback got called "
                    "and "
                    "received expected publication");
            result = true;
        } else {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultipleEnumerationParameters - callback got called "
                    "but "
                    "received unexpected error or publication content");
            result = false;
        }

        // try to unsubscribe in any case
        try {
            testInterfaceProxy->unsubscribeFromBroadcastWithMultipleEnumerationParametersBroadcast(
                    subscriptionId);
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithMultipleEnumerationParameters - unsubscribe "
                           "successful");
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultipleEnumerationParameters - caught unexpected "
                    "exception on unsubscribe");
            JOYNR_LOG_INFO(
                    logger, "callSubscribeBroadcastWithMultipleEnumerationParameters - FAILED");
            result = false;
        }

        if (!result) {
            JOYNR_LOG_INFO(
                    logger, "callSubscribeBroadcastWithMultipleEnumerationParameters - FAILED");
        } else {
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithMultipleEnumerationParameters - OK");
        }
        return result;
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithMultipleEnumerationParameters - caught unexpected "
                "exception");
        JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithMultipleEnumerationParameters - FAILED");
        return false;
    }
}

// variables that are to be changed inside callbacks must be declared global
volatile bool subscribeBroadcastWithSingleStructParameterCallbackDone = false;
volatile bool subscribeBroadcastWithSingleStructParameterCallbackResult = false;

class BroadcastWithSingleStructParameterBroadcastListener
        : public SubscriptionListener<
                  joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives>
{
public:
    BroadcastWithSingleStructParameterBroadcastListener()
    {
    }

    ~BroadcastWithSingleStructParameterBroadcastListener()
    {
    }

    void onReceive(const joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives&
                           extendedStructOfPrimitivesOut)
    {
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithSingleStructParameter - callback - got broadcast");
        if (!IltUtil::checkExtendedStructOfPrimitives(extendedStructOfPrimitivesOut)) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithSingleStructParameter - callback - invalid "
                           "content");
            subscribeBroadcastWithSingleStructParameterCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithSingleStructParameter - callback - content OK");
            subscribeBroadcastWithSingleStructParameterCallbackResult = true;
        }
        subscribeBroadcastWithSingleStructParameterCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_INFO(
                logger, "callSubscribeBroadcastWithSingleStructParameter - callback - got error");
        subscribeBroadcastWithSingleStructParameterCallbackResult = false;
        subscribeBroadcastWithSingleStructParameterCallbackDone = true;
    }
};

bool callSubscribeBroadcastWithSingleStructParameter(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    joynr::OnChangeSubscriptionQos subscriptionQos(validity, minInterval_ms);
    bool result;
    JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSingleStructParameter");
    try {
        std::shared_ptr<ISubscriptionListener<
                joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives>>
                listener(new BroadcastWithSingleStructParameterBroadcastListener());
        subscriptionId = testInterfaceProxy->subscribeToBroadcastWithSingleStructParameterBroadcast(
                listener, subscriptionQos);
        JOYNR_LOG_INFO(logger,
                       "callSubscribeBroadcastWithSingleStructParameter - subscription successful");
        JOYNR_LOG_INFO(
                logger, "callSubscribeBroadcastWithSingleStructParameter - Waiting one second");
        usleep(1000000);
        JOYNR_LOG_INFO(logger,
                       "callSubscribeBroadcastWithSingleStructParameter - Wait done, invoking fire "
                       "method");
        testInterfaceProxy->methodToFireBroadcastWithSingleStructParameter();
        JOYNR_LOG_INFO(
                logger, "callSubscribeBroadcastWithSingleStructParameter - fire method invoked");
        waitForChange(subscribeBroadcastWithSingleStructParameterCallbackDone, 1000);
        if (!subscribeBroadcastWithSingleStructParameterCallbackDone) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithSingleStructParameter - callback did not get "
                           "called in time");
            result = false;
        } else if (subscribeBroadcastWithSingleStructParameterCallbackResult) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithSingleStructParameter - callback got called and "
                    "received expected publication");
            result = true;
        } else {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithSingleStructParameter - callback got called but "
                    "received unexpected error or publication content");
            result = false;
        }

        // try to unsubscribe in any case
        try {
            testInterfaceProxy->unsubscribeFromBroadcastWithSingleStructParameterBroadcast(
                    subscriptionId);
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithSingleStructParameter - unsubscribe successful");
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithSingleStructParameter - caught unexpected "
                           "exception on unsubscribe");
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSingleStructParameter - FAILED");
            result = false;
        }

        if (!result) {
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSingleStructParameter - FAILED");
        } else {
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSingleStructParameter - OK");
        }
        return result;
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithSingleStructParameter - caught unexpected exception");
        JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithSingleStructParameter - FAILED");
        return false;
    }
}

// variables that are to be changed inside callbacks must be declared global
volatile bool subscribeBroadcastWithMultipleStructParametersCallbackDone = false;
volatile bool subscribeBroadcastWithMultipleStructParametersCallbackResult = false;

class BroadcastWithMultipleStructParametersBroadcastListener
        : public SubscriptionListener<
                  joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements,
                  joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct>
{
public:
    BroadcastWithMultipleStructParametersBroadcastListener()
    {
    }

    ~BroadcastWithMultipleStructParametersBroadcastListener()
    {
    }

    void onReceive(const joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements&
                           baseStructWithoutElementsOut,
                   const joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct&
                           extendedExtendedBaseStructOut)
    {
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithMultipleStructParameters - callback - got broadcast");
        if (!IltUtil::checkBaseStructWithoutElements(baseStructWithoutElementsOut)) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultipleStructParameters - callback - invalid "
                    "baseStructWithoutElementsOut content");
            subscribeBroadcastWithMultipleStructParametersCallbackResult = false;
        } else if (!IltUtil::checkExtendedExtendedBaseStruct(extendedExtendedBaseStructOut)) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultipleStructParameters - callback - invalid "
                    "extendedExtendedBaseStructOut content");
            subscribeBroadcastWithMultipleStructParametersCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultipleStructParameters - callback - content "
                    "OK");
            subscribeBroadcastWithMultipleStructParametersCallbackResult = true;
        }
        subscribeBroadcastWithMultipleStructParametersCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_INFO(logger,
                       "callSubscribeBroadcastWithMultipleStructParameters - callback - got error");
        subscribeBroadcastWithMultipleStructParametersCallbackResult = false;
        subscribeBroadcastWithMultipleStructParametersCallbackDone = true;
    }
};

bool callSubscribeBroadcastWithMultipleStructParameters(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    joynr::OnChangeSubscriptionQos subscriptionQos(validity, minInterval_ms);
    bool result;
    JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithMultipleStructParameters");
    try {
        std::shared_ptr<ISubscriptionListener<
                joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements,
                joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct>>
                listener(new BroadcastWithMultipleStructParametersBroadcastListener());
        subscriptionId =
                testInterfaceProxy->subscribeToBroadcastWithMultipleStructParametersBroadcast(
                        listener, subscriptionQos);
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithMultipleStructParameters - subscription successful");
        JOYNR_LOG_INFO(
                logger, "callSubscribeBroadcastWithMultipleStructParameters - Waiting one second");
        usleep(1000000);
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeBroadcastWithMultipleStructParameters - Wait done, invoking fire "
                "method");
        testInterfaceProxy->methodToFireBroadcastWithMultipleStructParameters();
        JOYNR_LOG_INFO(
                logger, "callSubscribeBroadcastWithMultipleStructParameters - fire method invoked");
        waitForChange(subscribeBroadcastWithMultipleStructParametersCallbackDone, 1000);
        if (!subscribeBroadcastWithMultipleStructParametersCallbackDone) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultipleStructParameters - callback did not get "
                    "called in time");
            result = false;
        } else if (subscribeBroadcastWithMultipleStructParametersCallbackResult) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultipleStructParameters - callback got called "
                    "and "
                    "received expected publication");
            result = true;
        } else {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeBroadcastWithMultipleStructParameters - callback got called "
                    "but "
                    "received unexpected error or publication content");
            result = false;
        }

        // try to unsubscribe in any case
        try {
            testInterfaceProxy->unsubscribeFromBroadcastWithMultipleStructParametersBroadcast(
                    subscriptionId);
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithMultipleStructParameters - unsubscribe "
                           "successful");
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithMultipleStructParameters - caught unexpected "
                           "exception on unsubscribe");
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithMultipleStructParameters - FAILED");
            result = false;
        }

        if (!result) {
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithMultipleStructParameters - FAILED");
        } else {
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithMultipleStructParameters - OK");
        }
        return result;
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(logger,
                       "callSubscribeBroadcastWithMultipleStructParameters - caught unexpected "
                       "exception");
        JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithMultipleStructParameters - FAILED");
        return false;
    }
}

// variables that are to be changed inside callbacks must be declared global
volatile bool subscribeBroadcastWithFilteringCallbackDone = false;
volatile bool subscribeBroadcastWithFilteringCallbackResult = false;

class BroadcastWithFilteringBroadcastListener
        : public SubscriptionListener<
                  std::string,
                  std::vector<std::string>,
                  joynr::interlanguagetest::namedTypeCollection2::
                          ExtendedTypeCollectionEnumerationInTypeCollection::Enum,
                  joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray,
                  std::vector<
                          joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>>
{
public:
    BroadcastWithFilteringBroadcastListener()
    {
    }

    ~BroadcastWithFilteringBroadcastListener()
    {
    }

    void onReceive(const std::string& stringOut,
                   const std::vector<std::string>& stringArrayOut,
                   const joynr::interlanguagetest::namedTypeCollection2::
                           ExtendedTypeCollectionEnumerationInTypeCollection::Enum& enumerationOut,
                   const joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray&
                           structWithStringArrayOut,
                   const std::vector<
                           joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>&
                           structWithStringArrayArrayOut)
    {
        JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithFiltering - callback - got broadcast");
        if (!IltUtil::checkStringArray(stringArrayOut)) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithFiltering - callback - invalid "
                           "stringArrayOut content");
            subscribeBroadcastWithFilteringCallbackResult = false;
        } else if (enumerationOut != joynr::interlanguagetest::namedTypeCollection2::
                                             ExtendedTypeCollectionEnumerationInTypeCollection::
                                                     ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithFiltering - callback - invalid "
                           "enumerationOut content");
            subscribeBroadcastWithFilteringCallbackResult = false;
        } else if (!IltUtil::checkStructWithStringArray(structWithStringArrayOut)) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithFiltering - callback - invalid "
                           "structWithStringArrayOut content");
            subscribeBroadcastWithFilteringCallbackResult = false;
        } else if (!IltUtil::checkStructWithStringArrayArray(structWithStringArrayArrayOut)) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithFiltering - callback - invalid "
                           "structWithStringArrayArrayOut content");
            subscribeBroadcastWithFilteringCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithFiltering - callback - content "
                           "OK");
            subscribeBroadcastWithFilteringCallbackResult = true;
        }
        subscribeBroadcastWithFilteringCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithFiltering - callback - got error");
        subscribeBroadcastWithFilteringCallbackResult = false;
        subscribeBroadcastWithFilteringCallbackDone = true;
    }
};

bool callSubscribeBroadcastWithFiltering(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    joynr::OnChangeSubscriptionQos subscriptionQos(validity, minInterval_ms);
    bool result;
    JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithFiltering");
    try {
        std::shared_ptr<ISubscriptionListener<
                std::string,
                std::vector<std::string>,
                joynr::interlanguagetest::namedTypeCollection2::
                        ExtendedTypeCollectionEnumerationInTypeCollection::Enum,
                joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray,
                std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>>>
                listener(new BroadcastWithFilteringBroadcastListener());
        joynr::interlanguagetest::TestInterfaceBroadcastWithFilteringBroadcastFilterParameters
                filterParameters;

        std::string filterStringOfInterest = "fireBroadcast";
        std::vector<std::string> filterStringArrayOfInterest = IltUtil::createStringArray();
        std::string filterStringArrayOfInterestJson(
                JsonSerializer::serialize(filterStringArrayOfInterest));

        joynr::interlanguagetest::namedTypeCollection2::
                ExtendedTypeCollectionEnumerationInTypeCollection::Enum filterEnumOfInterest =
                        joynr::interlanguagetest::namedTypeCollection2::
                                ExtendedTypeCollectionEnumerationInTypeCollection::
                                        ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
        std::string filterEnumOfInterestJson(
                "\"" + joynr::interlanguagetest::namedTypeCollection2::
                               ExtendedTypeCollectionEnumerationInTypeCollection::getLiteral(
                                       filterEnumOfInterest) +
                "\"");

        joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray
                filterStructWithStringArray = IltUtil::createStructWithStringArray();
        std::string filterStructWithStringArrayJson(
                JsonSerializer::serialize(filterStructWithStringArray));

        std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>
                filterStructWithStringArrayArray = IltUtil::createStructWithStringArrayArray();
        std::string filterStructWithStringArrayArrayJson(
                JsonSerializer::serialize(filterStructWithStringArrayArray));

        filterParameters.setStringOfInterest(filterStringOfInterest);
        filterParameters.setStringArrayOfInterest(filterStringArrayOfInterestJson);
        filterParameters.setEnumerationOfInterest(filterEnumOfInterestJson);
        filterParameters.setStructWithStringArrayOfInterest(filterStructWithStringArrayJson);
        filterParameters.setStructWithStringArrayArrayOfInterest(
                filterStructWithStringArrayArrayJson);

        subscriptionId = testInterfaceProxy->subscribeToBroadcastWithFilteringBroadcast(
                filterParameters, listener, subscriptionQos);
        JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithFiltering - subscription successful");
        JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithFiltering - Waiting one second");
        usleep(1000000);
        JOYNR_LOG_INFO(logger,
                       "callSubscribeBroadcastWithFiltering - Wait done, invoking fire "
                       "method");
        std::string stringArg = "fireBroadcast";
        testInterfaceProxy->methodToFireBroadcastWithFiltering(stringArg);
        JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithFiltering - fire method invoked");
        waitForChange(subscribeBroadcastWithFilteringCallbackDone, 1000);
        if (!subscribeBroadcastWithFilteringCallbackDone) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithFiltering - callback did not get "
                           "called in time");
            result = false;
        } else if (subscribeBroadcastWithFilteringCallbackResult) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithFiltering - callback got called "
                           "and "
                           "received expected publication");
            result = true;
        } else {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithFiltering - callback got called "
                           "but "
                           "received unexpected error or publication content");
            result = false;
        }

        // get out, if first test run failed
        if (result == false) {
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithFiltering - FAILED");
            return false;
        }

        // reset counter for 2nd test
        subscribeBroadcastWithFilteringCallbackDone = false;
        subscribeBroadcastWithFilteringCallbackResult = false;

        JOYNR_LOG_INFO(logger,
                       "callSubscribeBroadcastWithFiltering - Wait done, invoking fire "
                       "method with wrong stringArg");
        stringArg = "doNotFireBroadcast";
        testInterfaceProxy->methodToFireBroadcastWithFiltering(stringArg);
        JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithFiltering - fire method invoked");

        waitForChange(subscribeBroadcastWithFilteringCallbackDone, 1000);
        if (!subscribeBroadcastWithFilteringCallbackDone) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithFiltering - callback did not get "
                           "called in time");
            result = true;
        } else {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithFiltering - callback got called "
                           "unexpectedly");
            result = false;
        }

        // try to unsubscribe in any case
        try {
            testInterfaceProxy->unsubscribeFromBroadcastWithFilteringBroadcast(subscriptionId);
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithFiltering - unsubscribe "
                           "successful");
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeBroadcastWithFiltering - caught unexpected "
                           "exception on unsubscribe");
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithFiltering - FAILED");
            result = false;
        }

        if (!result) {
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithFiltering - FAILED");
        } else {
            JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithFiltering - OK");
        }
        return result;
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(logger,
                       "callSubscribeBroadcastWithFiltering - caught unexpected "
                       "exception");
        JOYNR_LOG_INFO(logger, "callSubscribeBroadcastWithFiltering - FAILED");
        return false;
    }
}

// SUBSCRIBE ATTRIBUTE ENUMERATION

// variables that are to be changed inside callbacks must be declared global
volatile bool subscribeAttributeEnumerationCallbackDone = false;
volatile bool subscribeAttributeEnumerationCallbackResult = false;

class AttributeEnumerationListener
        : public SubscriptionListener<joynr::interlanguagetest::Enumeration::Enum>
{
public:
    AttributeEnumerationListener() = default;

    ~AttributeEnumerationListener() override = default;

    void onReceive(const joynr::interlanguagetest::Enumeration::Enum& enumerationOut) override
    {
        JOYNR_LOG_INFO(logger, "callSubscribeAttributeEnumeration - callback - got broadcast");
        if (enumerationOut != joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_2) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeAttributeEnumeration - callback - invalid "
                           "content");
            subscribeAttributeEnumerationCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(logger, "callSubscribeAttributeEnumeration - callback - content OK");
            subscribeAttributeEnumerationCallbackResult = true;
        }
        subscribeAttributeEnumerationCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error) override
    {
        JOYNR_LOG_INFO(logger, "callSubscribeAttributeEnumeration - callback - got error");
        subscribeAttributeEnumerationCallbackResult = false;
        subscribeAttributeEnumerationCallbackDone = true;
    }
};

bool callSubscribeAttributeEnumeration(interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    joynr::OnChangeSubscriptionQos subscriptionQos(validity, minInterval_ms);
    bool result;
    JOYNR_LOG_INFO(logger, "callSubscribeAttributeEnumeration");
    try {
        std::shared_ptr<ISubscriptionListener<joynr::interlanguagetest::Enumeration::Enum>>
                listener(new AttributeEnumerationListener());
        subscriptionId =
                testInterfaceProxy->subscribeToAttributeEnumeration(listener, subscriptionQos);
        JOYNR_LOG_INFO(logger, "callSubscribeAttributeEnumeration - subscription successful");
        JOYNR_LOG_INFO(logger, "callSubscribeAttributeEnumeration - Waiting one second");
        usleep(1000000);
        waitForChange(subscribeAttributeEnumerationCallbackDone, 1000);
        if (!subscribeAttributeEnumerationCallbackDone) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeAttributeEnumeration - callback did not get "
                           "called in time");
            result = false;
        } else if (subscribeAttributeEnumerationCallbackResult) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeAttributeEnumeration - callback got called and "
                           "received expected publication");
            result = true;
        } else {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeAttributeEnumeration - callback got called but "
                           "received unexpected error or publication content");
            result = false;
        }

        // try to unsubscribe in any case
        try {
            testInterfaceProxy->unsubscribeFromAttributeEnumeration(subscriptionId);
            JOYNR_LOG_INFO(logger, "callSubscribeAttributeEnumeration - unsubscribe successful");
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeAttributeEnumeration - caught unexpected "
                           "exception on unsubscribe");
            JOYNR_LOG_INFO(logger, "callSubscribeAttributeEnumeration - FAILED");
            result = false;
        }

        if (!result) {
            JOYNR_LOG_INFO(logger, "callSubscribeAttributeEnumeration - FAILED");
        } else {
            JOYNR_LOG_INFO(logger, "callSubscribeAttributeEnumeration - OK");
        }
        return result;
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(logger, "callSubscribeAttributeEnumeration - caught unexpected exception");
        JOYNR_LOG_INFO(logger, "callSubscribeAttributeEnumeration - FAILED");
        return false;
    }
}

// variables that are to be changed inside callbacks must be declared global
volatile bool subscribeAttributeWithExceptionFromGetterCallbackDone = false;
volatile bool subscribeAttributeWithExceptionFromGetterCallbackResult = false;

class AttributeWithExceptionFromGetterListener : public SubscriptionListener<bool>
{
public:
    AttributeWithExceptionFromGetterListener() = default;

    ~AttributeWithExceptionFromGetterListener() override = default;

    void onReceive(const bool& value) override
    {
        JOYNR_LOG_INFO(logger,
                       "callSubscribeAttributeWithExceptionFromGetter - callback - "
                       "unexpectedly got broadcast");
        subscribeAttributeWithExceptionFromGetterCallbackResult = false;
        subscribeAttributeWithExceptionFromGetterCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error) override
    {
        if (error.getTypeName() == "joynr.exceptions.ProviderRuntimeException") {
            if (error.getMessage() == "Exception from getAttributeWithExceptionFromGetter") {
                JOYNR_LOG_INFO(
                        logger,
                        "callSubscribeAttributeWithExceptionFromGetter - callback - got expected "
                        "exception");
                subscribeAttributeWithExceptionFromGetterCallbackResult = true;
            } else {
                JOYNR_LOG_INFO(logger,
                               "callSubscribeAttributeWithExceptionFromGetter - callback - got "
                               "ProviderRuntimeException with wrong message");
                JOYNR_LOG_INFO(logger, error.getMessage());
                subscribeAttributeWithExceptionFromGetterCallbackResult = false;
            }
        } else {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeAttributeWithExceptionFromGetter - callback - got invalid "
                           "exception "
                           "type");
            JOYNR_LOG_INFO(logger, error.getTypeName());
            JOYNR_LOG_INFO(logger, error.getMessage());
            subscribeAttributeWithExceptionFromGetterCallbackResult = false;
        }
        subscribeAttributeWithExceptionFromGetterCallbackDone = true;
    }
};

bool callSubscribeAttributeWithExceptionFromGetter(
        interlanguagetest::TestInterfaceProxy* testInterfaceProxy)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    joynr::OnChangeSubscriptionQos subscriptionQos(validity, minInterval_ms);
    bool result;
    JOYNR_LOG_INFO(logger, "callSubscribeAttributeWithExceptionFromGetter");
    try {
        std::shared_ptr<ISubscriptionListener<bool>> listener(
                new AttributeWithExceptionFromGetterListener());
        subscriptionId = testInterfaceProxy->subscribeToAttributeWithExceptionFromGetter(
                listener, subscriptionQos);
        JOYNR_LOG_INFO(
                logger, "callSubscribeAttributeWithExceptionFromGetter - subscription successful");
        JOYNR_LOG_INFO(
                logger, "callSubscribeAttributeWithExceptionFromGetter - Waiting one second");
        waitForChange(subscribeAttributeWithExceptionFromGetterCallbackDone, 1000);
        if (!subscribeAttributeWithExceptionFromGetterCallbackDone) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeAttributeWithExceptionFromGetter - callback did not get "
                           "called in time");
            result = false;
        } else if (subscribeAttributeWithExceptionFromGetterCallbackResult) {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeAttributeWithExceptionFromGetter - callback got called and "
                    "received expected exception");
            result = true;
        } else {
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeAttributeWithExceptionFromGetter - callback got called but "
                    "received unexpected error or unexpected publication");
            result = false;
        }

        // try to unsubscribe in any case
        try {
            testInterfaceProxy->unsubscribeFromAttributeWithExceptionFromGetter(subscriptionId);
            JOYNR_LOG_INFO(
                    logger,
                    "callSubscribeAttributeWithExceptionFromGetter - unsubscribe successful");
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_INFO(logger,
                           "callSubscribeAttributeWithExceptionFromGetter - caught unexpected "
                           "exception on unsubscribe");
            JOYNR_LOG_INFO(logger, "callSubscribeAttributeWithExceptionFromGetter - FAILED");
            result = false;
        }

        if (!result) {
            JOYNR_LOG_INFO(logger, "callSubscribeAttributeWithExceptionFromGetter - FAILED");
        } else {
            JOYNR_LOG_INFO(logger, "callSubscribeAttributeWithExceptionFromGetter - OK");
        }
        return result;
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_INFO(
                logger,
                "callSubscribeAttributeWithExceptionFromGetter - caught unexpected exception");
        JOYNR_LOG_INFO(logger, "callSubscribeAttributeWithExceptionFromGetter - FAILED");
        return false;
    }
}

//------- Main entry point -------------------------------------------------------

int main(int argc, char* argv[])
{
    int exitCode;

    // Check the usage
    std::string programName(argv[0]);
    if (argc != 2) {
        JOYNR_LOG_ERROR(logger, "USAGE: {} <provider-domain>", programName);
        return 1;
    }

    // Get the provider domain
    std::string providerDomain(argv[1]);
    JOYNR_LOG_INFO(logger, "Creating proxy for provider on domain {}", providerDomain);

    // Get the current program directory
    std::string dir(IltHelper::getAbsolutePathToExecutable(programName));

    // Initialise the joynr runtime
    std::string pathToMessagingSettings(dir + "/resources/test-app-consumer.settings");
    // not used
    // std::string pathToLibJoynrSettings(dir.toStdString() +
    // "/resources/test-app-consumer.libjoynr.settings");
    JoynrRuntime* runtime = JoynrRuntime::createRuntime(pathToMessagingSettings);

    // Create proxy builder
    ProxyBuilder<interlanguagetest::TestInterfaceProxy>* proxyBuilder =
            runtime->createProxyBuilder<interlanguagetest::TestInterfaceProxy>(providerDomain);

    // Messaging Quality of service
    std::int64_t qosMsgTtl = 30000;                // Time to live is 30 secs in one direction
    std::int64_t qosCacheDataFreshnessMs = 400000; // Only consider data cached for < 400 secs

    // Find the provider with the highest priority set in ProviderQos
    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(40000);
    discoveryQos.setCacheMaxAgeMs(std::numeric_limits<std::int64_t>::max());
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);

    // run the tests
    int cntFailed = 0;
    interlanguagetest::TestInterfaceProxy* proxy;

    try {
        // Build a proxy
        JOYNR_LOG_INFO(logger, "About to call proxyBuilder");
        proxy = proxyBuilder->setMessagingQos(MessagingQos(qosMsgTtl))
                        ->setCached(false)
                        ->setDiscoveryQos(discoveryQos)
                        ->build();
        JOYNR_LOG_INFO(logger, "call to Proxybuilder successfully completed");

        if (proxy == nullptr) {
            std::cout << "proxybuilder returned proxy == nullptr" << std::endl;
            reportTest("proxy built", false);
            throw new joynr::exceptions::JoynrRuntimeException("received proxy == nullptr");
        }

        reportTest("proxy built", true);

        // Synchronous method calls

        // tests without some parameters
        reportTest("callMethodWithoutParameters", callMethodWithoutParameters(proxy));
        reportTest("callMethodWithoutInputParameter", callMethodWithoutInputParameter(proxy));
        reportTest("callMethodWithoutOutputParameter", callMethodWithoutOutputParameter(proxy));

        // tests with single/multiple parameters
        reportTest("callMethodWithSinglePrimitiveParameters",
                   callMethodWithSinglePrimitiveParameters(proxy));
        reportTest("callMethodWithMultiplePrimitiveParameters",
                   callMethodWithMultiplePrimitiveParameters(proxy));

        // tests with array parameters
        reportTest(
                "callMethodWithSingleArrayParameters", callMethodWithSingleArrayParameters(proxy));
        reportTest("callMethodWithMultipleArrayParameters",
                   callMethodWithMultipleArrayParameters(proxy));

        // tests with enum parameters
        reportTest("callMethodWithSingleEnumParameters", callMethodWithSingleEnumParameters(proxy));
        reportTest("callMethodWithMultipleEnumParameters",
                   callMethodWithMultipleEnumParameters(proxy));

        // tests with struct parameters
        reportTest("callMethodWithSingleStructParameters",
                   callMethodWithSingleStructParameters(proxy));
        reportTest("callMethodWithMultipleStructParameters",
                   callMethodWithMultipleStructParameters(proxy));

        // tests with overloaded methods
        reportTest("callOverloadedMethod_1", callOverloadedMethod_1(proxy));
        reportTest("callOverloadedMethod_2", callOverloadedMethod_2(proxy));
        reportTest("callOverloadedMethod_3", callOverloadedMethod_3(proxy));

        // tests with overloaded method with selectors
        reportTest("callOverloadedMethodWithSelector_1", callOverloadedMethodWithSelector_1(proxy));
        reportTest("callOverloadedMethodWithSelector_2", callOverloadedMethodWithSelector_2(proxy));
        reportTest("callOverloadedMethodWithSelector_3", callOverloadedMethodWithSelector_3(proxy));

        // test with arbitrary string length
        reportTest("callMethodWithStringsAndSpecifiedStringOutLength",
                   callMethodWithStringsAndSpecifiedStringOutLength(proxy));

        // test with exceptions
        reportTest("callMethodWithoutErrorEnum", callMethodWithoutErrorEnum(proxy));
        reportTest("callMethodWithAnonymousErrorEnum", callMethodWithAnonymousErrorEnum(proxy));
        reportTest("callMethodWithExistingErrorEnum", callMethodWithExistingErrorEnum(proxy));
        reportTest("callMethodWithExtendedErrorEnum", callMethodWithExtendedErrorEnum(proxy));

        // asynchronous method calls (selection)
        reportTest("callMethodWithMultipleStructParametersAsync",
                   callMethodWithMultipleStructParametersAsync(proxy));
        reportTest("callMethodWithSingleArrayParametersAsync",
                   callMethodWithSingleArrayParametersAsync(proxy));
        reportTest("callMethodWithSinglePrimitiveParametersAsync",
                   callMethodWithSinglePrimitiveParametersAsync(proxy));
        reportTest("callMethodWithExtendedErrorEnumAsync",
                   callMethodWithExtendedErrorEnumAsync(proxy));

        // tests with synchronous attribute setter and getter
        reportTest("callSetAttributeUInt8", callSetAttributeUInt8(proxy));
        reportTest("callGetAttributeUInt8", callGetAttributeUInt8(proxy));

        reportTest("callSetAttributeDouble", callSetAttributeDouble(proxy));
        reportTest("callGetAttributeDouble", callGetAttributeDouble(proxy));

        // no setter because readonly
        reportTest("callGetAttributeBooleanReadonly", callGetAttributeBooleanReadonly(proxy));

        reportTest("callSetAttributeStringNoSubscriptions",
                   callSetAttributeStringNoSubscriptions(proxy));
        reportTest("callGetAttributeStringNoSubscriptions",
                   callGetAttributeStringNoSubscriptions(proxy));

        reportTest("callGetAttributeInt8readonlyNoSubscriptions",
                   callGetAttributeInt8readonlyNoSubscriptions(proxy));

        reportTest("callSetAttributeArrayOfStringImplicit",
                   callSetAttributeArrayOfStringImplicit(proxy));
        reportTest("callGetAttributeArrayOfStringImplicit",
                   callGetAttributeArrayOfStringImplicit(proxy));

        reportTest("callSetAttributeEnumeration", callSetAttributeEnumeration(proxy));
        reportTest("callGetAttributeEnumeration", callGetAttributeEnumeration(proxy));

        // no setter because readonly
        reportTest("callGetAttributeExtendedEnumerationReadonly",
                   callGetAttributeExtendedEnumerationReadonly(proxy));

        reportTest("callSetAttributeBaseStruct", callSetAttributeBaseStruct(proxy));
        reportTest("callGetAttributeBaseStruct", callGetAttributeBaseStruct(proxy));

        reportTest("callSetAttributeExtendedExtendedBaseStruct",
                   callSetAttributeExtendedExtendedBaseStruct(proxy));
        reportTest("callGetAttributeExtendedExtendedBaseStruct",
                   callGetAttributeExtendedExtendedBaseStruct(proxy));

        // tests with attribute setters / getters that use exceptions
        reportTest("callSetAttributeWithExceptionFromSetter",
                   callSetAttributeWithExceptionFromSetter(proxy));
        reportTest("callGetAttributeWithExceptionFromGetter",
                   callGetAttributeWithExceptionFromGetter(proxy));

        reportTest("callSetAttributeMapStringString", callSetAttributeMapStringString(proxy));
        reportTest("callGetAttributeMapStringString", callGetAttributeMapStringString(proxy));
        reportTest("callmethodWithSingleMapParameters", callMethodWithSingleMapParameters(proxy));

        // TODO: tests with asynchronous getter and setter

        // TODO: subscription to attribute calls
        reportTest("callSubscribeAttributeEnumeration", callSubscribeAttributeEnumeration(proxy));
        reportTest("callSubscribeAttributeWithExceptionFromGetter",
                   callSubscribeAttributeWithExceptionFromGetter(proxy));

        // TODO: subscription to broadcast calls
        reportTest("callSubscribeBroadcastWithSinglePrimitiveParameter",
                   callSubscribeBroadcastWithSinglePrimitiveParameter(proxy));
        reportTest("callSubscribeBroadcastWithMultiplePrimitiveParameters",
                   callSubscribeBroadcastWithMultiplePrimitiveParameters(proxy));
        reportTest("callSubscribeBroadcastWithSingleArrayParameter",
                   callSubscribeBroadcastWithSingleArrayParameter(proxy));
        reportTest("callSubscribeBroadcastWithMultipleArrayParameters",
                   callSubscribeBroadcastWithMultipleArrayParameters(proxy));
        reportTest("callSubscribeBroadcastWithSingleEnumerationParameter",
                   callSubscribeBroadcastWithSingleEnumerationParameter(proxy));
        reportTest("callSubscribeBroadcastWithMultipleEnumerationParameters",
                   callSubscribeBroadcastWithMultipleEnumerationParameters(proxy));
        reportTest("callSubscribeBroadcastWithSingleStructParameter",
                   callSubscribeBroadcastWithSingleStructParameter(proxy));
        reportTest("callSubscribeBroadcastWithMultipleStructParameters",
                   callSubscribeBroadcastWithMultipleStructParameters(proxy));

        reportTest(
                "callSubscribeBroadcastWithFiltering", callSubscribeBroadcastWithFiltering(proxy));

    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        cntFailed++;
        JOYNR_LOG_ERROR(logger, "JoynrRuntimeException");
        exit(1);
    }

    delete proxy;
    delete proxyBuilder;
    delete runtime;

    exitCode = evaluateAndPrintResults();
    exit(exitCode);
}
