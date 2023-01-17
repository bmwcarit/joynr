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

class IltConsumerAsyncMethodTest : public IltAbstractConsumerTest<::testing::Test>
{
public:
    IltConsumerAsyncMethodTest() = default;

    template <typename AttributeType>
    void callProxyMethodWithParameterAsyncAndAssertResult(
            std::function<std::shared_ptr<joynr::Future<AttributeType>>(
                    joynr::interlanguagetest::TestInterfaceAsyncProxy*,
                    const AttributeType&,
                    std::function<void(const AttributeType&)>,
                    std::function<void(const joynr::exceptions::JoynrRuntimeException&)>,
                    boost::optional<joynr::MessagingQos>)> testMethod,
            const AttributeType& testValue);

    static volatile bool methodWithMultipleByteBufferParametersAsyncCallbackDone;
    static volatile bool methodWithMultipleByteBufferParametersAsyncCallbackResult;
    static volatile bool methodWithMultipleStructParametersAsyncCallbackDone;
    static volatile bool methodWithMultipleStructParametersAsyncCallbackResult;
    static volatile bool methodWithSingleArrayParametersAsyncCallbackDone;
    static volatile bool methodWithSingleArrayParametersAsyncCallbackResult;
    static volatile bool methodWithSinglePrimitiveParametersAsyncCallbackDone;
    static volatile bool methodWithSinglePrimitiveParametersAsyncCallbackResult;
    static volatile bool methodWithExtendedErrorEnumAsyncCallbackDone;
    static volatile bool methodWithExtendedErrorEnumAsyncCallbackResult;
};

volatile bool IltConsumerAsyncMethodTest::methodWithMultipleByteBufferParametersAsyncCallbackDone =
        false;
volatile bool
        IltConsumerAsyncMethodTest::methodWithMultipleByteBufferParametersAsyncCallbackResult =
                false;
volatile bool IltConsumerAsyncMethodTest::methodWithMultipleStructParametersAsyncCallbackDone =
        false;
volatile bool IltConsumerAsyncMethodTest::methodWithMultipleStructParametersAsyncCallbackResult =
        false;
volatile bool IltConsumerAsyncMethodTest::methodWithSingleArrayParametersAsyncCallbackDone = false;
volatile bool IltConsumerAsyncMethodTest::methodWithSingleArrayParametersAsyncCallbackResult =
        false;
volatile bool IltConsumerAsyncMethodTest::methodWithSinglePrimitiveParametersAsyncCallbackDone =
        false;
volatile bool IltConsumerAsyncMethodTest::methodWithSinglePrimitiveParametersAsyncCallbackResult =
        false;
volatile bool IltConsumerAsyncMethodTest::methodWithExtendedErrorEnumAsyncCallbackDone = false;
volatile bool IltConsumerAsyncMethodTest::methodWithExtendedErrorEnumAsyncCallbackResult = false;

template <typename AttributeType>
void IltConsumerAsyncMethodTest::callProxyMethodWithParameterAsyncAndAssertResult(
        std::function<std::shared_ptr<joynr::Future<AttributeType>>(
                joynr::interlanguagetest::TestInterfaceAsyncProxy*,
                const AttributeType&,
                std::function<void(const AttributeType&)>,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)>,
                boost::optional<joynr::MessagingQos>)> testMethod,
        const AttributeType& testValue)
{
    std::function<void(const AttributeType result)> onSuccess =
            [&testValue](const AttributeType result) {
                // check result
                if (result != testValue) {
                    JOYNR_LOG_ERROR(logger(),
                                    "callProxyMethodWithParameterAsyncAndAssertResult FAILED - "
                                    "invalid result from callback");
                    return;
                }
                JOYNR_LOG_DEBUG(logger(),
                                " - callback - "
                                "got correct value");
            };

    std::function<void(const joynr::exceptions::JoynrException& error)> onError =
            [](const joynr::exceptions::JoynrException& error) {
                JOYNR_LOG_ERROR(logger(),
                                "callProxyMethodWithParameterAsyncAndAssertResult FAILED - "
                                "caught exception");
                JOYNR_LOG_ERROR(logger(), error.getTypeName());
                JOYNR_LOG_ERROR(logger(), error.getMessage());
            };

    std::shared_ptr<joynr::Future<AttributeType>> future =
            testMethod(testInterfaceProxy.get(), testValue, onSuccess, onError, {});

    long timeoutInMilliseconds = 8000;
    JOYNR_ASSERT_NO_THROW(future->wait(timeoutInMilliseconds));
    ASSERT_TRUE(future->isOk());

    AttributeType result;

    // the following call would throw an exception, in case the request status
    // is negative, however we have already returned here
    future->get(result);

    // check results from future
    ASSERT_EQ(result, testValue);
}

TEST_F(IltConsumerAsyncMethodTest, callMethodWithMultipleStructParametersAsync)
{
    typedef std::shared_ptr<joynr::Future<
            joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements,
            joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct>>
            futureType;
    JOYNR_ASSERT_NO_THROW({
        // setup input parameters
        joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives
                extendedStructOfPrimitivesArg = IltUtil::createExtendedStructOfPrimitives();
        joynr::interlanguagetest::namedTypeCollection2::BaseStruct baseStructArg =
                IltUtil::createBaseStruct();

        std::function<void(
                const joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements&,
                const joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct&)>
                onSuccess = [](const joynr::interlanguagetest::namedTypeCollection2::
                                       BaseStructWithoutElements& baseStructWithoutElementsOut,
                               const joynr::interlanguagetest::namedTypeCollection2::
                                       ExtendedExtendedBaseStruct& extendedExtendedBaseStructOut) {
                    // check results
                    if (!IltUtil::checkBaseStructWithoutElements(baseStructWithoutElementsOut)) {
                        methodWithMultipleStructParametersAsyncCallbackResult = false;
                        methodWithMultipleStructParametersAsyncCallbackDone = true;
                        JOYNR_LOG_INFO(
                                logger(),
                                "callMethodWithMultipleStructParametersAsync - callback - invalid "
                                "baseStructWithoutElementsOut");
                        JOYNR_LOG_INFO(
                                logger(), "callMethodWithMultipleStructParametersAsync - FAILED");
                        return;
                    }

                    if (!IltUtil::checkExtendedExtendedBaseStruct(extendedExtendedBaseStructOut)) {
                        methodWithMultipleStructParametersAsyncCallbackResult = false;
                        methodWithMultipleStructParametersAsyncCallbackDone = true;
                        JOYNR_LOG_INFO(
                                logger(),
                                "callMethodWithMultipleStructParametersAsync - callback - invalid "
                                "extendedExtendedBaseStructOut");
                        JOYNR_LOG_INFO(
                                logger(), "callMethodWithMultipleStructParametersAsync - FAILED");
                        return;
                    }
                    methodWithMultipleStructParametersAsyncCallbackResult = true;
                    methodWithMultipleStructParametersAsyncCallbackDone = true;
                    JOYNR_LOG_INFO(logger(),
                                   "callMethodWithMultipleStructParametersAsync - callback - "
                                   "got correct values");
                };

        std::function<void(const joynr::exceptions::JoynrException&)> onError =
                [](const joynr::exceptions::JoynrException& error) {
                    methodWithMultipleStructParametersAsyncCallbackResult = false;
                    methodWithMultipleStructParametersAsyncCallbackDone = true;
                    JOYNR_LOG_INFO(logger(),
                                   "callMethodWithMultipleStructParametersAsync - callback - "
                                   "caught exception");
                    JOYNR_LOG_INFO(logger(), error.getTypeName());
                    JOYNR_LOG_INFO(logger(), error.getMessage());
                };

        futureType future = testInterfaceProxy->methodWithMultipleStructParametersAsync(
                extendedStructOfPrimitivesArg, baseStructArg, onSuccess, onError);

        long timeoutInMilliseconds = 8000;
        future->wait(timeoutInMilliseconds);
        ASSERT_TRUE(future->isOk());

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
        ASSERT_TRUE(IltUtil::checkBaseStructWithoutElements(baseStructWithoutElementsOut));
        ASSERT_TRUE(IltUtil::checkExtendedExtendedBaseStruct(extendedExtendedBaseStructOut));

        // check results from callback; expect to be finished within 1 second
        // should have been called ahead anyway
        waitForChange(methodWithMultipleStructParametersAsyncCallbackDone, 1000);
        ASSERT_TRUE(methodWithMultipleStructParametersAsyncCallbackDone);
        ASSERT_TRUE(methodWithMultipleStructParametersAsyncCallbackResult);
    });
}

TEST_F(IltConsumerAsyncMethodTest, callMethodWithSingleArrayParametersAsync)
{
    JOYNR_ASSERT_NO_THROW({
        // setup input parameters
        std::vector<double> arg = IltUtil::createDoubleArray();

        std::function<void(const std::vector<std::string>& result)> onSuccess =
                [](const std::vector<std::string>& result) {
                    // check results
                    if (!IltUtil::checkStringArray(result)) {
                        methodWithSingleArrayParametersAsyncCallbackResult = false;
                        methodWithSingleArrayParametersAsyncCallbackDone = true;
                        JOYNR_LOG_INFO(
                                logger(),
                                "callMethodWithSingleArrayParametersAsync - callback - invalid "
                                "baseStructWithoutElementsOut");
                        JOYNR_LOG_INFO(
                                logger(), "callMethodWithSingleArrayParametersAsync - FAILED");
                        return;
                    }

                    methodWithSingleArrayParametersAsyncCallbackResult = true;
                    methodWithSingleArrayParametersAsyncCallbackDone = true;
                    JOYNR_LOG_INFO(logger(),
                                   "callMethodWithSingleArrayParametersAsync - callback - got "
                                   "correct values");
                };

        std::function<void(const joynr::exceptions::JoynrException&)> onError =
                [](const joynr::exceptions::JoynrException& error) {
                    methodWithSingleArrayParametersAsyncCallbackResult = false;
                    methodWithSingleArrayParametersAsyncCallbackDone = true;
                    JOYNR_LOG_INFO(logger(),
                                   "callMethodWithSingleArrayParametersAsync - callback - caught "
                                   "exception");
                    JOYNR_LOG_INFO(logger(), error.getTypeName());
                    JOYNR_LOG_INFO(logger(), error.getMessage());
                };

        std::shared_ptr<joynr::Future<std::vector<std::string>>> future =
                testInterfaceProxy->methodWithSingleArrayParametersAsync(arg, onSuccess, onError);

        long timeoutInMilliseconds = 8000;
        future->wait(timeoutInMilliseconds);
        ASSERT_TRUE(future->isOk());

        std::vector<std::string> result;

        // the following call would throw an exception, in case the Requeststatus
        // is negative, however we have already returned here
        // future->getValues(result);
        future->get(result);

        // check results from future
        ASSERT_TRUE(IltUtil::checkStringArray(result));

        // check results from callback; expect to be finished within 1 second
        // should have been called ahead anyway
        waitForChange(methodWithSingleArrayParametersAsyncCallbackDone, 1000);
        ASSERT_TRUE(methodWithSingleArrayParametersAsyncCallbackDone);
        ASSERT_TRUE(methodWithSingleArrayParametersAsyncCallbackResult);
    });
}

TEST_F(IltConsumerAsyncMethodTest, callMethodWithSinglePrimitiveParametersAsync)
{
    JOYNR_ASSERT_NO_THROW({
        // setup input parameters
        uint16_t arg = 32767;

        std::function<void(const std::string&)> onSuccess = [arg](const std::string& result) {
            // check results
            if (result != std::to_string(arg)) {
                methodWithSinglePrimitiveParametersAsyncCallbackResult = false;
                methodWithSinglePrimitiveParametersAsyncCallbackDone = true;
                JOYNR_LOG_INFO(logger(),
                               "callMethodWithSinglePrimitiveParametersAsync - callback - invalid "
                               "baseStructWithoutElementsOut");
                JOYNR_LOG_INFO(logger(), "callMethodWithSinglePrimitiveParametersAsync - FAILED");
                return;
            }

            methodWithSinglePrimitiveParametersAsyncCallbackResult = true;
            methodWithSinglePrimitiveParametersAsyncCallbackDone = true;
            JOYNR_LOG_INFO(logger(),
                           "callMethodWithSinglePrimitiveParametersAsync - callback - "
                           "got correct values");
        };

        std::function<void(const joynr::exceptions::JoynrException&)> onError =
                [](const joynr::exceptions::JoynrException& error) {
                    methodWithSinglePrimitiveParametersAsyncCallbackResult = false;
                    methodWithSinglePrimitiveParametersAsyncCallbackDone = true;
                    JOYNR_LOG_INFO(logger(),
                                   "callMethodWithSinglePrimitiveParametersAsync - callback - "
                                   "caught exception");
                    JOYNR_LOG_INFO(logger(), error.getTypeName());
                    JOYNR_LOG_INFO(logger(), error.getMessage());
                };

        std::shared_ptr<joynr::Future<std::string>> future =
                testInterfaceProxy->methodWithSinglePrimitiveParametersAsync(
                        arg, onSuccess, onError);

        long timeoutInMilliseconds = 8000;
        future->wait(timeoutInMilliseconds);
        ASSERT_TRUE(future->isOk());

        std::string result;

        // the following call would throw an exception, in case the Requeststatus
        // is negative, however we have already returned here
        future->get(result);

        // check results from future
        ASSERT_EQ(result, std::to_string(arg));

        // check results from callback; expect to be finished within 1 second
        // should have been called ahead anyway
        waitForChange(methodWithSinglePrimitiveParametersAsyncCallbackDone, 1000);
        ASSERT_TRUE(methodWithSinglePrimitiveParametersAsyncCallbackDone);
        ASSERT_TRUE(methodWithSinglePrimitiveParametersAsyncCallbackResult);
    });
}

TEST_F(IltConsumerAsyncMethodTest, callMethodWithSingleByteBufferParameter)
{
    callProxyMethodWithParameterAsyncAndAssertResult<joynr::ByteBuffer>(
            &joynr::interlanguagetest::TestInterfaceProxy::methodWithSingleByteBufferParameterAsync,
            joynr::ByteBuffer{0x00, 0x64, 0xFF});
}

TEST_F(IltConsumerAsyncMethodTest, callMethodWithMultipleByteBufferParameters)
{
    // setup input parameters
    joynr::ByteBuffer arg1 = {5, 125};
    joynr::ByteBuffer arg2 = {78, 0};

    std::function<void(const joynr::ByteBuffer& result)> onSuccess =
            [&arg1, &arg2](const joynr::ByteBuffer& result) {
                // check result
                if (result != IltUtil::concatByteBuffers(arg1, arg2)) {
                    methodWithMultipleByteBufferParametersAsyncCallbackResult = false;
                    methodWithMultipleByteBufferParametersAsyncCallbackDone = true;
                    JOYNR_LOG_DEBUG(logger(),
                                    "callMethodWithMultipleByteBufferParametersAsync - callback -"
                                    "invalid result");
                    JOYNR_LOG_DEBUG(
                            logger(), "callMethodWithMultipleByteBufferParametersAsync - FAILED");
                    return;
                }

                methodWithMultipleByteBufferParametersAsyncCallbackResult = true;
                methodWithMultipleByteBufferParametersAsyncCallbackDone = true;
                JOYNR_LOG_DEBUG(logger(),
                                "callMethodWithMultipleByteBufferParametersAsync - callback - "
                                "got correct value");
            };

    std::function<void(const joynr::exceptions::JoynrException& error)> onError =
            [](const joynr::exceptions::JoynrException& error) {
                methodWithMultipleByteBufferParametersAsyncCallbackResult = false;
                methodWithMultipleByteBufferParametersAsyncCallbackDone = true;
                JOYNR_LOG_DEBUG(logger(),
                                "callMethodWithMultipleByteBufferParametersAsync - callback - "
                                "caught exception");
                JOYNR_LOG_DEBUG(logger(), error.getTypeName());
                JOYNR_LOG_DEBUG(logger(), error.getMessage());
            };

    std::shared_ptr<joynr::Future<joynr::ByteBuffer>> future =
            testInterfaceProxy->methodWithMultipleByteBufferParametersAsync(
                    arg1, arg2, onSuccess, onError);

    long timeoutInMilliseconds = 8000;
    JOYNR_ASSERT_NO_THROW(future->wait(timeoutInMilliseconds));
    ASSERT_TRUE(future->isOk());

    joynr::ByteBuffer result;

    // the following call would throw an exception, in case the request status
    // is negative, however we have already returned here
    future->get(result);

    // check results from future
    ASSERT_EQ(result, IltUtil::concatByteBuffers(arg1, arg2));

    // check results from callback; expect to be finished within 1 second
    // should have been called ahead anyway
    waitForChange(methodWithMultipleByteBufferParametersAsyncCallbackDone, 1000);
    ASSERT_TRUE(methodWithMultipleByteBufferParametersAsyncCallbackDone);
    ASSERT_TRUE(methodWithMultipleByteBufferParametersAsyncCallbackResult);
}

TEST_F(IltConsumerAsyncMethodTest, callMethodWithInt64TypeDefParameter)
{
    callProxyMethodWithParameterAsyncAndAssertResult<std::int64_t>(
            &joynr::interlanguagetest::TestInterfaceProxy::methodWithInt64TypeDefParameterAsync,
            1L);
}

TEST_F(IltConsumerAsyncMethodTest, callMethodWithStringTypeDefParameter)
{
    callProxyMethodWithParameterAsyncAndAssertResult<std::string>(
            &joynr::interlanguagetest::TestInterfaceProxy::methodWithStringTypeDefParameterAsync,
            "TypeDefTestString");
}

TEST_F(IltConsumerAsyncMethodTest, callMethodWithStructTypeDefParameter)
{
    callProxyMethodWithParameterAsyncAndAssertResult<
            joynr::interlanguagetest::namedTypeCollection2::BaseStruct>(
            &joynr::interlanguagetest::TestInterfaceProxy::methodWithStructTypeDefParameterAsync,
            IltUtil::createBaseStruct());
}

TEST_F(IltConsumerAsyncMethodTest, callMethodWithMapTypeDefParameter)
{
    joynr::interlanguagetest::namedTypeCollection2::MapStringString mapTypeDefArg;
    mapTypeDefArg.insert(std::pair<std::string, std::string>("keyString1", "valueString1"));
    mapTypeDefArg.insert(std::pair<std::string, std::string>("keyString2", "valueString2"));
    mapTypeDefArg.insert(std::pair<std::string, std::string>("keyString3", "valueString3"));

    callProxyMethodWithParameterAsyncAndAssertResult<
            joynr::interlanguagetest::namedTypeCollection2::MapStringString>(
            &joynr::interlanguagetest::TestInterfaceProxy::methodWithMapTypeDefParameterAsync,
            mapTypeDefArg);
}

TEST_F(IltConsumerAsyncMethodTest, callMethodWithEnumTypeDefParameter)
{
    callProxyMethodWithParameterAsyncAndAssertResult<joynr::interlanguagetest::Enumeration::Enum>(
            &joynr::interlanguagetest::TestInterfaceProxy::methodWithEnumTypeDefParameterAsync,
            joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_1);
}

TEST_F(IltConsumerAsyncMethodTest, callMethodWithByteBufferTypeDefParameter)
{
    callProxyMethodWithParameterAsyncAndAssertResult<joynr::ByteBuffer>(
            &joynr::interlanguagetest::TestInterfaceProxy::
                    methodWithByteBufferTypeDefParameterAsync,
            joynr::ByteBuffer{0x00, 0x64, 0xFF});
}

TEST_F(IltConsumerAsyncMethodTest, callMethodWithArrayTypeDefParameter)
{
    std::vector<std::string> stringArray = IltUtil::createStringArray();
    joynr::interlanguagetest::typeDefCollection::ArrayTypeDefStruct arrayTypeDefArg;
    arrayTypeDefArg.setTypeDefStringArray(stringArray);

    callProxyMethodWithParameterAsyncAndAssertResult<
            joynr::interlanguagetest::typeDefCollection::ArrayTypeDefStruct>(
            &joynr::interlanguagetest::TestInterfaceProxy::methodWithArrayTypeDefParameterAsync,
            arrayTypeDefArg);
}

TEST_F(IltConsumerAsyncMethodTest, callMethodWithExtendedErrorEnumAsync)
{
#ifdef USE_PROVIDER_RUNTIME_EXCEPTION
    JOYNR_ASSERT_NO_THROW({
        // setup input parameters
        std::string arg = "ProviderRuntimeException";

        std::function<void()> onSuccess = []() {
            // check results
            JOYNR_LOG_INFO(logger(),
                           "callMethodWithExtendedErrorEnumAsync - 1st - callback - unexpected "
                           "call to "
                           "onSuccess");
            methodWithExtendedErrorEnumAsyncCallbackResult = false;
            methodWithExtendedErrorEnumAsyncCallbackDone = true;
        };

        auto onRuntimeError = [](const joynr::exceptions::JoynrRuntimeException& error) {
            if (error.getTypeName() == "joynr.exceptions.ProviderRuntimeException") {
                if (error.getMessage() == "Exception from methodWithExtendedErrorEnum") {
                    JOYNR_LOG_INFO(logger(),
                                   "callMethodWithExtendedErrorEnumAsync - 1st - callback - "
                                   "got expected "
                                   "exception");
                    methodWithExtendedErrorEnumAsyncCallbackResult = true;
                } else {
                    JOYNR_LOG_INFO(logger(),
                                   "callMethodWithExtendedErrorEnumAsync - 1st - callback - got "
                                   "ProviderRuntimeException with wrong message");
                    JOYNR_LOG_INFO(logger(), error.getMessage());
                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                }
            } else {
                JOYNR_LOG_INFO(
                        logger(),
                        "callMethodWithExtendedErrorEnumAsync - 1st - callback - got invalid "
                        "exception "
                        "type");
                JOYNR_LOG_INFO(logger(), error.getTypeName());
                JOYNR_LOG_INFO(logger(), error.getMessage());
                methodWithExtendedErrorEnumAsyncCallbackResult = false;
            }
            methodWithExtendedErrorEnumAsyncCallbackDone = true;
        };

        std::shared_ptr<joynr::Future<void>> future =
                testInterfaceProxy->methodWithExtendedErrorEnumAsync(
                        arg, onSuccess, nullptr, onRuntimeError);

        try {
            long timeoutInMilliseconds = 8000;
            future->get();
            JOYNR_LOG_INFO(logger(),
                           "callMethodWithExtendedErrorEnumAsync - 1st - returned from "
                           "future->get()");
            ASSERT_FALSE(future->isOk());
        } catch (joynr::exceptions::ProviderRuntimeException& error) {
            // expected case
            JOYNR_LOG_INFO(logger(),
                           "callMethodWithExtendedErrorEnumAsync - 1st - caught "
                           "ProviderRuntimeException");
            ASSERT_TRUE(error.getTypeName(), "joynr.exceptions.ProviderRuntimeException");
            ASSERT_TRUE(error.getMessage(), "Exception from methodWithExtendedErrorEnum");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            waitForChange(methodWithExtendedErrorEnumAsyncCallbackDone, 1000);
            ASSERT_TRUE(methodWithExtendedErrorEnumAsyncCallbackDone);
            ASSERT_TRUE(methodWithExtendedErrorEnumAsyncCallbackResult);
            // fallthrough
        } catch (joynr::exceptions::ApplicationException& e) {
            FAIL() << "callMethodWithExtendedErrorEnumAsync - 1st - caught "
                      "ApplicationException";
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            FAIL() << "callMethodWithExtendedErrorEnumAsync - 1st - caught "
                      "JoynrRuntimeException";
        } catch (...) {
            FAIL() << "callMethodWithExtendedErrorEnumAsync - 1st - caught unknown exception";
        }
    });
#endif

    // Checks with ApplicationException

    methodWithExtendedErrorEnumAsyncCallbackResult = false;
    methodWithExtendedErrorEnumAsyncCallbackDone = false;
    JOYNR_ASSERT_NO_THROW({
        // setup input parameters
        std::string arg = "ApplicationException_1";

        std::function<void()> onSuccess = []() {
            // check results
            methodWithExtendedErrorEnumAsyncCallbackResult = false;
            methodWithExtendedErrorEnumAsyncCallbackDone = true;
            JOYNR_LOG_INFO(logger(),
                           "callMethodWithExtendedErrorEnumAsync - 2nd - callback - unexpected "
                           "call to "
                           "onSuccess");
        };

        using joynr::interlanguagetest::TestInterface::MethodWithExtendedErrorEnumErrorEnum;
        auto onApplicationError = [](const MethodWithExtendedErrorEnumErrorEnum::Enum& errorEnum) {
            if (errorEnum != MethodWithExtendedErrorEnumErrorEnum::ERROR_3_3_NTC) {
                JOYNR_LOG_INFO(logger(),
                               "callMethodWithExtendedErrorEnumAsync - 2nd - callback - got "
                               "ApplicationException with wrong enumeration");
                methodWithExtendedErrorEnumAsyncCallbackResult = false;
            } else {
                JOYNR_LOG_INFO(logger(),
                               "callMethodWithExtendedErrorEnumAsync - 2nd - callback - got "
                               "expected ApplicationException");
                methodWithExtendedErrorEnumAsyncCallbackResult = true;
            }
            methodWithExtendedErrorEnumAsyncCallbackDone = true;
        };
        auto onRuntimeError = [](const exceptions::JoynrRuntimeException& error) {
            JOYNR_LOG_INFO(logger(),
                           "callMethodWithExtendedErrorEnumAsync - 2nd - callback - got invalid "
                           "exception "
                           "type");
            JOYNR_LOG_INFO(logger(), error.getTypeName());
            JOYNR_LOG_INFO(logger(), error.getMessage());
            methodWithExtendedErrorEnumAsyncCallbackResult = false;
            methodWithExtendedErrorEnumAsyncCallbackDone = true;
        };

        std::shared_ptr<joynr::Future<void>> future =
                testInterfaceProxy->methodWithExtendedErrorEnumAsync(
                        arg, onSuccess, onApplicationError, onRuntimeError);

        try {
            long timeoutInMilliseconds = 8000;
            JOYNR_LOG_INFO(logger(),
                           "callMethodWithExtendedErrorEnumAsync - 2nd - about to call "
                           "future.waitForFinished()");
            // future->wait(timeoutInMilliseconds);
            future->get();
            JOYNR_LOG_INFO(logger(),
                           "callMethodWithExtendedErrorEnumAsync - 2nd - returned from "
                           "future.get()");
            ASSERT_FALSE(future->isOk());
        } catch (joynr::exceptions::ProviderRuntimeException& error) {
            FAIL() << "callMethodWithExtendedErrorEnumAsync - 2nd - caught "
                      "ProviderRuntimeException";
        } catch (joynr::exceptions::ApplicationException& error) {
            // expected case
            ASSERT_EQ(error.getTypeName(), "joynr.exceptions.ApplicationException");
            try {
                const joynr::exceptions::ApplicationException& appError =
                        dynamic_cast<const joynr::exceptions::ApplicationException&>(error);
                // check enum value
                ASSERT_EQ(appError.getError<joynr::interlanguagetest::TestInterface::
                                                    MethodWithExtendedErrorEnumErrorEnum::Enum>(),
                          joynr::interlanguagetest::TestInterface::
                                  MethodWithExtendedErrorEnumErrorEnum::ERROR_3_3_NTC);
            } catch (std::bad_cast& bc) {
                FAIL() << "callMethodWithExtendedErrorEnumAsync - 2nd - cast to "
                          "ApplicationException failed";
            }

            JOYNR_LOG_INFO(
                    logger(), "callMethodWithExtendedErrorEnumAsync - 2nd - catch error checks OK");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            waitForChange(methodWithExtendedErrorEnumAsyncCallbackDone, 1000);
            ASSERT_TRUE(methodWithExtendedErrorEnumAsyncCallbackDone);
            ASSERT_TRUE(methodWithExtendedErrorEnumAsyncCallbackResult);
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            FAIL() << "callMethodWithExtendedErrorEnumAsync - 2nd - caught "
                      "JoynrRuntimeException";
        } catch (...) {
            FAIL() << "callMethodWithExtendedErrorEnumAsync - 2nd - caught unknown exception";
        }
    });

    // further checks

    methodWithExtendedErrorEnumAsyncCallbackResult = false;
    methodWithExtendedErrorEnumAsyncCallbackDone = false;
    JOYNR_ASSERT_NO_THROW({
        // setup input parameters
        std::string arg = "ApplicationException_2";

        std::function<void()> onSuccess = []() {
            // check results
            methodWithExtendedErrorEnumAsyncCallbackResult = false;
            methodWithExtendedErrorEnumAsyncCallbackDone = true;
            JOYNR_LOG_INFO(logger(),
                           "callMethodWithExtendedErrorEnumAsync - 3rd - callback - unexpected "
                           "call to "
                           "onSuccess");
        };

        using joynr::interlanguagetest::TestInterface::MethodWithExtendedErrorEnumErrorEnum;
        auto onApplicationError = [](const MethodWithExtendedErrorEnumErrorEnum::Enum& errorEnum) {
            if (errorEnum != MethodWithExtendedErrorEnumErrorEnum::ERROR_2_1_TC2) {
                JOYNR_LOG_INFO(logger(),
                               "callMethodWithExtendedErrorEnumAsync - 3rd - callback - got "
                               "ApplicationException with wrong enumeration");
                methodWithExtendedErrorEnumAsyncCallbackResult = false;
            } else {
                JOYNR_LOG_INFO(logger(),
                               "callMethodWithExtendedErrorEnumAsync - 3rd - callback - got "
                               "expected ApplicationException");
                methodWithExtendedErrorEnumAsyncCallbackResult = true;
            }
            methodWithExtendedErrorEnumAsyncCallbackDone = true;
        };

        auto onRuntimeError = [](const exceptions::JoynrRuntimeException& error) {
            JOYNR_LOG_INFO(logger(),
                           "callMethodWithExtendedErrorEnumAsync - 3rd - callback - got invalid "
                           "exception "
                           "type");
            JOYNR_LOG_INFO(logger(), error.getTypeName());
            JOYNR_LOG_INFO(logger(), error.getMessage());
            methodWithExtendedErrorEnumAsyncCallbackResult = false;
            methodWithExtendedErrorEnumAsyncCallbackDone = true;
        };

        // the method call is not expected to throw
        std::shared_ptr<joynr::Future<void>> future =
                testInterfaceProxy->methodWithExtendedErrorEnumAsync(
                        arg, onSuccess, onApplicationError, onRuntimeError);

        try {
            long timeoutInMilliseconds = 8000;
            JOYNR_LOG_INFO(logger(),
                           "callMethodWithExtendedErrorEnumAsync - 3rd - about to call "
                           "future->waitForFinished()");
            // future->wait(timeoutInMilliseconds);
            future->get();
            JOYNR_LOG_INFO(logger(),
                           "callMethodWithExtendedErrorEnumAsync - 3rd - returned from "
                           "future->get()");
            ASSERT_FALSE(future->isOk());
        } catch (joynr::exceptions::ProviderRuntimeException& error) {
            FAIL() << "callMethodWithExtendedErrorEnumAsync - 3rd - caught "
                      "ProviderRuntimeException";
        } catch (joynr::exceptions::ApplicationException& error) {
            ASSERT_EQ(error.getTypeName(), "joynr.exceptions.ApplicationException");
            try {
                const joynr::exceptions::ApplicationException& appError =
                        dynamic_cast<const joynr::exceptions::ApplicationException&>(error);
                // check enum value
                ASSERT_EQ(appError.getError<joynr::interlanguagetest::TestInterface::
                                                    MethodWithExtendedErrorEnumErrorEnum::Enum>(),
                          joynr::interlanguagetest::TestInterface::
                                  MethodWithExtendedErrorEnumErrorEnum::ERROR_2_1_TC2);
                JOYNR_LOG_INFO(logger(),
                               "callMethodWithExtendedErrorEnumAsync - 3rd - got "
                               "expected ApplicationException");
                // fallthrough
            } catch (std::bad_cast& bc) {
                FAIL() << "callMethodWithExtendedErrorEnumAsync - 3rd - cast to "
                          "ApplicationException failed";
            }

            JOYNR_LOG_INFO(
                    logger(), "callMethodWithExtendedErrorEnumAsync - 3rd - catch error checks OK");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            waitForChange(methodWithExtendedErrorEnumAsyncCallbackDone, 1000);
            ASSERT_TRUE(methodWithExtendedErrorEnumAsyncCallbackDone);
            ASSERT_TRUE(methodWithExtendedErrorEnumAsyncCallbackResult);
            JOYNR_LOG_INFO(
                    logger(),
                    "callMethodWithExtendedErrorEnumAsync - 3rd - callback has set OK flags");
        } catch (joynr::exceptions::JoynrRuntimeException& e) {
            FAIL() << "callMethodWithExtendedErrorEnumAsync - 3rd - caught "
                      "JoynrRuntimeException";
        } catch (...) {
            FAIL() << "callMethodWithExtendedErrorEnumAsync - 3rd - caught unknown exception";
        }
    });
}
