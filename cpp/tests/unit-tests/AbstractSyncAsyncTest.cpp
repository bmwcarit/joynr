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
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/exceptions/MethodInvocationException.h"
#include "joynr/IReplyCaller.h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/MessagingQos.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Reply.h"
#include "joynr/ReplyCaller.h"
#include "joynr/Request.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/tests/Itest.h"
#include "joynr/tests/testJoynrMessagingConnector.h"
#include "joynr/types/Localisation/GpsLocation.h"

#include "tests/mock/MockCallback.h"
#include "tests/mock/MockDispatcher.h"
#include "tests/mock/MockMessageSender.h"
#include "tests/mock/MockMessagingStub.h"
#include "tests/mock/MockSubscriptionListener.h"
#include "tests/mock/MockSubscriptionManager.h"

using ::testing::_;
using ::testing::A;
using ::testing::AllOf;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::NotNull;
using ::testing::Pointee;
using ::testing::Property;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::Unused;
using namespace joynr;

/**
 * These tests test the communication from X through to the JoynrMessageSender.
 * Some methods are defined here, that are used in ProxyTest and GpsJoynrMessagingConnectorTest
 */

class CallBackActions
{
public:
    CallBackActions(joynr::types::Localisation::GpsLocation expectedGpsLocation, int expectedInt)
            : expectedGpsLocation(expectedGpsLocation), expectedInt(expectedInt)
    {
    }
    // for test: sync_setAttributeNotCached
    void executeCallBackVoidResult(
            Unused,                                 // sender participant ID
            Unused,                                 // receiver participant ID
            Unused,                                 // messaging QoS
            Unused,                                 // request object to send
            std::shared_ptr<IReplyCaller> callback, // reply caller to notify when reply is received
            bool /*isLocalMessage*/)
    {
        (std::dynamic_pointer_cast<ReplyCaller<void>>(callback))->returnValue();
    }

    // related to test: sync_getAttributeNotCached
    void executeCallBackGpsLocationResult(
            Unused,                                 // sender participant ID
            Unused,                                 // receiver participant ID
            Unused,                                 // messaging QoS
            Unused,                                 // request object to send
            std::shared_ptr<IReplyCaller> callback, // reply caller to notify when reply is received
            bool /*isLocalMessage*/)
    {
        (std::dynamic_pointer_cast<ReplyCaller<types::Localisation::GpsLocation>>(callback))
                ->returnValue(expectedGpsLocation);
    }

    // related to test: sync_OperationWithNoArguments
    void executeCallBackIntResult(
            Unused,                                 // sender participant ID
            Unused,                                 // receiver participant ID
            Unused,                                 // messaging QoS
            Unused,                                 // request object to send
            std::shared_ptr<IReplyCaller> callback, // reply caller to notify when reply is received
            bool /*isLocalMessage*/)
    {

        std::dynamic_pointer_cast<ReplyCaller<int>>(callback)->returnValue(expectedInt);
    }

private:
    joynr::types::Localisation::GpsLocation expectedGpsLocation;
    int expectedInt;
};

/**
 * @brief Fixutre.
 */
class AbstractSyncAsyncTest : public ::testing::Test
{
public:
    AbstractSyncAsyncTest()
            : singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              mockSubscriptionManager(std::make_shared<MockSubscriptionManager>(
                      singleThreadedIOService->getIOService(),
                      nullptr)),
              expectedGpsLocation(1.1,
                                  1.2,
                                  1.3,
                                  types::Localisation::GpsFixEnum::MODE3D,
                                  1.4,
                                  1.5,
                                  1.6,
                                  1.7,
                                  18,
                                  19,
                                  95302963),
              expectedInt(60284917),
              callBackActions(expectedGpsLocation, expectedInt),
              qosSettings(),
              mockDispatcher(),
              mockMessagingStub(),
              callBack(),
              mockMessageSender(),
              proxyParticipantId(),
              providerParticipantId(),
              asyncTestFixture(),
              error(nullptr)
    {
        singleThreadedIOService->start();
    }

    virtual ~AbstractSyncAsyncTest()
    {
        singleThreadedIOService->stop();
    }

    void SetUp()
    {
        qosSettings = MessagingQos(456000);
        proxyParticipantId = "participantId";
        providerParticipantId = "providerParticipantId";
        mockMessageSender = std::make_shared<MockMessageSender>();
        // asyncGpsFixture must be created after derived objects have run Setup()
    }

    // sets the expectations on the call expected on the MessageSender from the connector
    virtual testing::internal::TypedExpectation<void(const std::string&,
                                                     const std::string&,
                                                     const MessagingQos&,
                                                     const Request&,
                                                     std::shared_ptr<IReplyCaller>,
                                                     bool isLocalMessage)>&
    setExpectationsForSendRequestCall(std::string methodName) = 0;

    // sets the exception which shall be returned by the ReplyCaller
    virtual testing::internal::TypedExpectation<void(const std::string&,
                                                     const std::string&,
                                                     const MessagingQos&,
                                                     const Request&,
                                                     std::shared_ptr<IReplyCaller>,
                                                     bool isLocalMessage)>&
    setExpectedExceptionForSendRequestCall(const exceptions::JoynrException& error)
    {
        this->error.reset(error.clone());
        return EXPECT_CALL(*mockMessageSender,
                           sendRequest(_,                         // sender participant ID
                                       Eq(providerParticipantId), // receiver participant ID
                                       _,                         // messaging QoS
                                       _,                         // request object to send
                                       Pointee(_), // reply caller to notify when reply is received
                                                   // A<IReplyCaller>()
                                       _)          // isLocal flag
                           )
                .Times(1)
                .WillRepeatedly(Invoke(this, &AbstractSyncAsyncTest::returnError));
    }

    void returnError(const std::string& /*senderParticipantId*/,
                     const std::string& /*receiverParticipantId*/,
                     const MessagingQos& /*qos*/,
                     const Request& /*request*/,
                     std::shared_ptr<IReplyCaller> callback,
                     bool /*isLocalMessage*/)
    {
        callback->returnError(error);
    }

    template <typename T>
    static void checkApplicationException(const exceptions::ApplicationException& expected,
                                          const exceptions::ApplicationException& actual,
                                          T expectedEnum)
    {
        EXPECT_EQ(expected, actual);
        EXPECT_EQ(expectedEnum, actual.getError<T>());
    }

    virtual std::shared_ptr<tests::Itest> createItestFixture() = 0;
    virtual std::shared_ptr<tests::ItestSubscription> createItestSubscriptionFixture() = 0;

    void testAsync_getAttributeNotCached()
    {
        asyncTestFixture = createItestFixture();

        MockCallbackWithJoynrException<joynr::types::Localisation::GpsLocation>* callback =
                new MockCallbackWithJoynrException<joynr::types::Localisation::GpsLocation>();

        setExpectationsForSendRequestCall("getLocation");
        asyncTestFixture->getLocationAsync(
                [callback](const joynr::types::Localisation::GpsLocation& location) {
                    callback->onSuccess(location);
                });

        delete callback;
    }

    void testSync_setAttributeNotCached()
    {
        auto testFixture = createItestFixture();

        EXPECT_CALL(
                *mockMessageSender,
                sendRequest(_, // Eq(proxyParticipantId), // sender participant ID
                            Eq(providerParticipantId), // receiver participant ID
                            _,                         // messaging QoS
                            AllOf(Property(&Request::getMethodName, Eq("setLocation")),
                                  Property(&Request::getParamDatatypes,
                                           (Property(&std::vector<std::string>::size,
                                                     Eq(1))))), // request object to send
                            Property(&std::shared_ptr<IReplyCaller>::get,
                                     NotNull()), // reply caller to notify when reply is received
                            _                    // isLocal flag
                            ))
                .WillOnce(Invoke(&callBackActions, &CallBackActions::executeCallBackVoidResult));

        testFixture->setLocation(expectedGpsLocation);
    }

    void testSync_getAttributeNotCached()
    {
        auto testFixture = createItestFixture();
        setExpectationsForSendRequestCall("getLocation").WillOnce(
                Invoke(&callBackActions, &CallBackActions::executeCallBackGpsLocationResult));

        types::Localisation::GpsLocation gpsLocation;
        try {
            testFixture->getLocation(gpsLocation);
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE() << "getLocation was not successful";
        }
        EXPECT_EQ(expectedGpsLocation, gpsLocation);
    }

    void testAsync_getterCallReturnsProviderRuntimeException()
    {
        asyncTestFixture = createItestFixture();

        MockCallback<std::int32_t>* callback = new MockCallback<std::int32_t>();

        exceptions::ProviderRuntimeException expected(
                "getterCallReturnsProviderRuntimeExceptionAsyncError");
        setExpectedExceptionForSendRequestCall(expected);

        EXPECT_CALL(*callback, onSuccess(_)).Times(0);
        EXPECT_CALL(*callback, onError(_)).Times(1);

        asyncTestFixture->getAttributeWithProviderRuntimeExceptionAsync(
                [callback](const std::int32_t& value) { callback->onSuccess(value); },
                [callback, expected](const exceptions::JoynrRuntimeException& error) {
                    EXPECT_EQ(expected.getTypeName(), error.getTypeName());
                    EXPECT_EQ(expected.getMessage(), error.getMessage());
                    callback->onError(error);
                });

        delete callback;
    }

    void testSync_getterCallReturnsProviderRuntimeException()
    {
        auto testFixture = createItestFixture();

        exceptions::ProviderRuntimeException expected(
                "getterCallReturnsProviderRuntimeExceptionError");
        setExpectedExceptionForSendRequestCall(expected);

        std::int32_t result;
        try {
            testFixture->getAttributeWithProviderRuntimeException(result);
            ADD_FAILURE() << "getterCallReturnsProviderRuntimeException was not successful "
                             "(expected ProviderRuntimeException)";
        } catch (const exceptions::ProviderRuntimeException& e) {
            EXPECT_EQ(expected.getTypeName(), e.getTypeName());
            EXPECT_EQ(expected.getMessage(), e.getMessage());
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE() << "getterCallReturnsProviderRuntimeException was not successful "
                             "(unexpected exception)";
        }
    }

    void testAsync_getterCallReturnsMethodInvocationException()
    {
        asyncTestFixture = createItestFixture();

        MockCallback<std::int32_t>* callback = new MockCallback<std::int32_t>();

        exceptions::MethodInvocationException expected(
                "getterCallReturnsMethodInvocationExceptionAsyncError");
        setExpectedExceptionForSendRequestCall(expected);

        EXPECT_CALL(*callback, onSuccess(_)).Times(0);
        EXPECT_CALL(*callback, onError(_)).Times(1);

        asyncTestFixture->getAttributeWithProviderRuntimeExceptionAsync(
                [callback](const std::int32_t& value) { callback->onSuccess(value); },
                [callback, expected](const exceptions::JoynrRuntimeException& error) {
                    EXPECT_EQ(expected.getTypeName(), error.getTypeName());
                    EXPECT_EQ(expected.getMessage(), error.getMessage());
                    callback->onError(error);
                });

        delete callback;
    }

    void testSync_getterCallReturnsMethodInvocationException()
    {
        auto testFixture = createItestFixture();

        exceptions::MethodInvocationException expected(
                "getterCallReturnsMethodInvocationExceptionError");
        setExpectedExceptionForSendRequestCall(expected);

        std::int32_t result;
        try {
            testFixture->getAttributeWithProviderRuntimeException(result);
            ADD_FAILURE() << "getterCallReturnsMethodInvocationException was not successful "
                             "(expected MethodInvocationException)";
        } catch (const exceptions::MethodInvocationException& e) {
            EXPECT_EQ(expected.getTypeName(), e.getTypeName());
            EXPECT_EQ(expected.getMessage(), e.getMessage());
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE() << "getterCallReturnsMethodInvocationException was not successful "
                             "(unexpected exception)";
        }
    }

    void testAsync_setterCallReturnsProviderRuntimeException()
    {
        asyncTestFixture = createItestFixture();

        MockCallback<void>* callback = new MockCallback<void>();

        exceptions::ProviderRuntimeException expected(
                "setterCallReturnsProviderRuntimeExceptionAsyncError");
        setExpectedExceptionForSendRequestCall(expected);

        EXPECT_CALL(*callback, onSuccess()).Times(0);
        EXPECT_CALL(*callback, onError(_)).Times(1);

        std::int32_t value = 0;
        asyncTestFixture->setAttributeWithProviderRuntimeExceptionAsync(
                value,
                [callback]() { callback->onSuccess(); },
                [callback, expected](const exceptions::JoynrRuntimeException& error) {
                    EXPECT_EQ(expected.getTypeName(), error.getTypeName());
                    EXPECT_EQ(expected.getMessage(), error.getMessage());
                    callback->onError(error);
                });

        delete callback;
    }

    void testSync_setterCallReturnsProviderRuntimeException()
    {
        auto testFixture = createItestFixture();

        exceptions::ProviderRuntimeException expected(
                "setterCallReturnsProviderRuntimeExceptionError");
        setExpectedExceptionForSendRequestCall(expected);

        std::int32_t value = 0;
        try {
            testFixture->setAttributeWithProviderRuntimeException(value);
            ADD_FAILURE() << "setterCallReturnsProviderRuntimeException was not successful "
                             "(expected ProviderRuntimeException)";
        } catch (const exceptions::ProviderRuntimeException& e) {
            EXPECT_EQ(expected.getTypeName(), e.getTypeName());
            EXPECT_EQ(expected.getMessage(), e.getMessage());
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE() << "setterCallReturnsProviderRuntimeException was not successful "
                             "(unexpected exception)";
        }
    }

    void testAsync_setterCallReturnsMethodInvocationException()
    {
        asyncTestFixture = createItestFixture();

        MockCallback<void>* callback = new MockCallback<void>();

        exceptions::MethodInvocationException expected(
                "setterCallReturnsMethodInvocationExceptionAsyncError");
        setExpectedExceptionForSendRequestCall(expected);

        EXPECT_CALL(*callback, onSuccess()).Times(0);
        EXPECT_CALL(*callback, onError(_)).Times(1);

        std::int32_t value = 0;
        asyncTestFixture->setAttributeWithProviderRuntimeExceptionAsync(
                value,
                [callback]() { callback->onSuccess(); },
                [callback, expected](const exceptions::JoynrRuntimeException& error) {
                    EXPECT_EQ(expected.getTypeName(), error.getTypeName());
                    EXPECT_EQ(expected.getMessage(), error.getMessage());
                    callback->onError(error);
                });

        delete callback;
    }

    void testSync_setterCallReturnsMethodInvocationException()
    {
        auto testFixture = createItestFixture();

        exceptions::MethodInvocationException expected(
                "setterCallReturnsMethodInvocationExceptionError");
        setExpectedExceptionForSendRequestCall(expected);

        std::int32_t value = 0;
        try {
            testFixture->setAttributeWithProviderRuntimeException(value);
            ADD_FAILURE() << "setterCallReturnsMethodInvocationException was not successful "
                             "(expected MethodInvocationException)";
        } catch (const exceptions::MethodInvocationException& e) {
            EXPECT_EQ(expected.getTypeName(), e.getTypeName());
            EXPECT_EQ(expected.getMessage(), e.getMessage());
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE() << "setterCallReturnsMethodInvocationException was not successful "
                             "(unexpected exception)";
        }
    }

    void testAsync_methodCallReturnsProviderRuntimeException()
    {
        asyncTestFixture = createItestFixture();

        MockCallback<void>* callback = new MockCallback<void>();

        exceptions::ProviderRuntimeException expected(
                "testAsync_methodCallReturnsProviderRuntimeException-ERROR");
        setExpectedExceptionForSendRequestCall(expected);

        EXPECT_CALL(*callback, onSuccess()).Times(0);
        EXPECT_CALL(*callback, onError(_)).Times(1);

        asyncTestFixture->methodWithProviderRuntimeExceptionAsync(
                [callback]() { callback->onSuccess(); },
                [callback, expected](const exceptions::JoynrRuntimeException& error) {
                    EXPECT_EQ(expected.getTypeName(), error.getTypeName());
                    EXPECT_EQ(expected.getMessage(), error.getMessage());
                    callback->onError(error);
                });

        delete callback;
    }

    void testSync_methodCallReturnsProviderRuntimeException()
    {
        auto testFixture = createItestFixture();

        exceptions::ProviderRuntimeException expected(
                "testSync_methodCallReturnsProviderRuntimeException-ERROR");
        setExpectedExceptionForSendRequestCall(expected);

        try {
            testFixture->methodWithProviderRuntimeException();
            ADD_FAILURE() << "testSync_methodCallReturnsProviderRuntimeException was not "
                             "successful (expected ProviderRuntimeException)";
        } catch (const exceptions::ProviderRuntimeException& e) {
            EXPECT_EQ(expected.getTypeName(), e.getTypeName());
            EXPECT_EQ(expected.getMessage(), e.getMessage());
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE() << "testSync_methodCallReturnsProviderRuntimeException was not "
                             "successful (unexpected exception)";
        }
    }

    void testAsync_methodCallReturnsMethodInvocationException()
    {
        asyncTestFixture = createItestFixture();

        MockCallback<void>* callback = new MockCallback<void>();

        exceptions::MethodInvocationException expected(
                "testAsync_methodCallReturnsMethodInvocationException-ERROR");
        setExpectedExceptionForSendRequestCall(expected);

        EXPECT_CALL(*callback, onSuccess()).Times(0);
        EXPECT_CALL(*callback, onError(_)).Times(1);

        asyncTestFixture->methodWithProviderRuntimeExceptionAsync(
                [callback]() { callback->onSuccess(); },
                [callback, expected](const exceptions::JoynrRuntimeException& error) {
                    EXPECT_EQ(expected.getTypeName(), error.getTypeName());
                    EXPECT_EQ(expected.getMessage(), error.getMessage());
                    callback->onError(error);
                });

        delete callback;
    }

    void testSync_methodCallReturnsMethodInvocationException()
    {
        auto testFixture = createItestFixture();

        exceptions::MethodInvocationException expected(
                "testSync_methodCallReturnsMethodInvocationException-ERROR");
        setExpectedExceptionForSendRequestCall(expected);

        try {
            testFixture->methodWithProviderRuntimeException();
            ADD_FAILURE() << "testSync_methodCallReturnsMethodInvocationException was not "
                             "successful (expected MethodInvocationException)";
        } catch (const exceptions::MethodInvocationException& e) {
            EXPECT_EQ(expected.getTypeName(), e.getTypeName());
            EXPECT_EQ(expected.getMessage(), e.getMessage());
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE() << "testSync_methodCallReturnsMethodInvocationException was not "
                             "successful (unexpected exception)";
        }
    }

    void testAsync_methodCallReturnsErrorEnum()
    {
        asyncTestFixture = createItestFixture();

        using tests::testTypes::ErrorEnumBase;
        auto callback =
                std::make_shared<MockCallbackWithApplicationError<void, ErrorEnumBase::Enum>>();

        ErrorEnumBase::Enum expectedErrorEnum = ErrorEnumBase::BASE_ERROR_TYPECOLLECTION;
        std::string literal = ErrorEnumBase::getLiteral(expectedErrorEnum);
        std::string typeName = ErrorEnumBase::getTypeName();
        // TODO remove workaround after the new serializer has been introduced: until then, the
        // correct error enumeration has to be reconstructed by the connector
        exceptions::ApplicationException expected(
                typeName + "::" + literal, std::make_shared<ErrorEnumBase>(literal));
        setExpectedExceptionForSendRequestCall(expected);

        EXPECT_CALL(*callback, onSuccess()).Times(0);
        EXPECT_CALL(*callback, onRuntimeError(_)).Times(0);
        EXPECT_CALL(*callback, onApplicationError(_)).Times(1);

        asyncTestFixture->methodWithErrorEnumAsync(
                [callback]() { callback->onSuccess(); },
                [callback, expectedErrorEnum](const ErrorEnumBase::Enum& errorEnum) {
                    EXPECT_EQ(expectedErrorEnum, errorEnum);
                    callback->onApplicationError(errorEnum);
                });
    }

    void testSync_methodCallReturnsErrorEnum()
    {
        auto testFixture = createItestFixture();

        using tests::testTypes::ErrorEnumBase;

        ErrorEnumBase::Enum expectedErrorEnum = ErrorEnumBase::BASE_ERROR_TYPECOLLECTION;
        std::string literal = ErrorEnumBase::getLiteral(expectedErrorEnum);
        std::string typeName = ErrorEnumBase::getTypeName();
        // TODO remove workaround after the new serializer has been introduced: until then, the
        // correct error enumeration has to be reconstructed by the connector
        exceptions::ApplicationException expected(
                typeName + "::" + literal, std::make_shared<ErrorEnumBase>(literal));
        setExpectedExceptionForSendRequestCall(expected);

        try {
            testFixture->methodWithErrorEnum();
            ADD_FAILURE() << "testSync_methodCallReturnsErrorEnum was not successful (expected "
                             "MethodInvocationException)";
        } catch (const exceptions::ApplicationException& e) {
            checkApplicationException(expected, e, expectedErrorEnum);
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE() << "testSync_methodCallReturnsErrorEnum was not successful (unexpected "
                             "exception)";
        }
    }

    void testAsync_methodCallReturnsExtendedErrorEnum()
    {
        asyncTestFixture = createItestFixture();
        using tests::test::MethodWithErrorEnumExtendedErrorEnum;

        auto callback = std::make_shared<
                MockCallbackWithApplicationError<void,
                                                 MethodWithErrorEnumExtendedErrorEnum::Enum>>();

        MethodWithErrorEnumExtendedErrorEnum::Enum expectedErrorEnum =
                MethodWithErrorEnumExtendedErrorEnum::IMPLICIT_ERROR_TYPECOLLECTION;
        std::string literal = MethodWithErrorEnumExtendedErrorEnum::getLiteral(expectedErrorEnum);
        std::string typeName = MethodWithErrorEnumExtendedErrorEnum::getTypeName();
        // TODO remove workaround after the new serializer has been introduced: until then, the
        // correct error enumeration has to be reconstructed by the connector
        exceptions::ApplicationException expected(
                typeName + "::" + literal,
                std::make_shared<MethodWithErrorEnumExtendedErrorEnum>(literal));
        setExpectedExceptionForSendRequestCall(expected);

        EXPECT_CALL(*callback, onSuccess()).Times(0);
        EXPECT_CALL(*callback, onRuntimeError(_)).Times(0);
        EXPECT_CALL(*callback, onApplicationError(_)).Times(1);

        asyncTestFixture->methodWithErrorEnumExtendedAsync(
                [callback]() { callback->onSuccess(); },
                [callback, expectedErrorEnum](
                        const MethodWithErrorEnumExtendedErrorEnum::Enum& errorEnum) {
                    EXPECT_EQ(expectedErrorEnum, errorEnum);
                    callback->onApplicationError(errorEnum);
                });
    }

    void testSync_methodCallReturnsExtendedErrorEnum()
    {
        auto testFixture = createItestFixture();

        using tests::test::MethodWithErrorEnumExtendedErrorEnum;
        MethodWithErrorEnumExtendedErrorEnum::Enum error =
                MethodWithErrorEnumExtendedErrorEnum::IMPLICIT_ERROR_TYPECOLLECTION;
        std::string literal = MethodWithErrorEnumExtendedErrorEnum::getLiteral(error);
        std::string typeName = MethodWithErrorEnumExtendedErrorEnum::getTypeName();
        // TODO remove workaround after the new serializer has been introduced: until then, the
        // correct error enumeration has to be reconstructed by the connector
        exceptions::ApplicationException expected(
                typeName + "::" + literal,
                std::make_shared<MethodWithErrorEnumExtendedErrorEnum>(literal));
        setExpectedExceptionForSendRequestCall(expected);

        try {
            testFixture->methodWithErrorEnumExtended();
            ADD_FAILURE() << "testSync_methodCallReturnsExtendedErrorEnum was not successful "
                             "(expected MethodInvocationException)";
        } catch (const exceptions::ApplicationException& e) {
            checkApplicationException(expected, e, error);
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE() << "testSync_methodCallReturnsExtendedErrorEnum was not successful "
                             "(unexpected exception)";
        }
    }

    void testAsync_methodCallReturnsInlineErrorEnum()
    {
        asyncTestFixture = createItestFixture();

        using tests::test::MethodWithImplicitErrorEnumErrorEnum;

        auto callback = std::make_shared<
                MockCallbackWithApplicationError<void,
                                                 MethodWithImplicitErrorEnumErrorEnum::Enum>>();

        MethodWithImplicitErrorEnumErrorEnum::Enum expectedErrorEnum =
                MethodWithImplicitErrorEnumErrorEnum::IMPLICIT_ERROR;
        std::string literal = MethodWithImplicitErrorEnumErrorEnum::getLiteral(expectedErrorEnum);
        std::string typeName = MethodWithImplicitErrorEnumErrorEnum::getTypeName();
        // TODO remove workaround after the new serializer has been introduced: until then, the
        // correct error enumeration has to be reconstructed by the connector
        exceptions::ApplicationException expected(
                typeName + "::" + literal,
                std::make_shared<MethodWithImplicitErrorEnumErrorEnum>(literal));
        setExpectedExceptionForSendRequestCall(expected);

        EXPECT_CALL(*callback, onSuccess()).Times(0);
        EXPECT_CALL(*callback, onRuntimeError(_)).Times(0);
        EXPECT_CALL(*callback, onApplicationError(_)).Times(1);

        asyncTestFixture->methodWithImplicitErrorEnumAsync(
                [callback]() { callback->onSuccess(); },
                [callback, expectedErrorEnum](
                        const MethodWithImplicitErrorEnumErrorEnum::Enum& errorEnum) {
                    EXPECT_EQ(expectedErrorEnum, errorEnum);
                    callback->onApplicationError(errorEnum);
                });
    }

    void testSync_methodCallReturnsInlineErrorEnum()
    {
        auto testFixture = createItestFixture();

        using tests::test::MethodWithImplicitErrorEnumErrorEnum;

        MethodWithImplicitErrorEnumErrorEnum::Enum error =
                MethodWithImplicitErrorEnumErrorEnum::IMPLICIT_ERROR;
        std::string literal = MethodWithImplicitErrorEnumErrorEnum::getLiteral(error);
        std::string typeName = MethodWithImplicitErrorEnumErrorEnum::getTypeName();
        // TODO remove workaround after the new serializer has been introduced: until then, the
        // correct error enumeration has to be reconstructed by the connector
        exceptions::ApplicationException expected(
                typeName + "::" + literal,
                std::make_shared<MethodWithImplicitErrorEnumErrorEnum>(literal));
        setExpectedExceptionForSendRequestCall(expected);

        try {
            testFixture->methodWithImplicitErrorEnum();
            ADD_FAILURE() << "testSync_methodCallReturnsInlineErrorEnum was not successful "
                             "(expected MethodInvocationException)";
        } catch (const exceptions::ApplicationException& e) {
            checkApplicationException(expected, e, error);
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE() << "testSync_methodCallReturnsInlineErrorEnum was not successful "
                             "(unexpected exception)";
        }
    }

    void testAsync_OperationWithNoArguments()
    {
        asyncTestFixture = createItestFixture();

        MockCallbackWithJoynrException<int>* callback = new MockCallbackWithJoynrException<int>();

        setExpectationsForSendRequestCall("methodWithNoInputParameters");

        asyncTestFixture->methodWithNoInputParametersAsync(
                [callback](const int& value) { callback->onSuccess(value); });
        delete callback;
    }

    void testSync_OperationWithNoArguments()
    {
        auto testFixture = createItestFixture();
        setExpectationsForSendRequestCall("methodWithNoInputParameters")
                .WillOnce(Invoke(&callBackActions, &CallBackActions::executeCallBackIntResult));

        int result;
        try {
            testFixture->methodWithNoInputParameters(result);
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE() << "methodWithNoInputParameters was not successful";
        }
        EXPECT_EQ(expectedInt, result);
    }

    void testSubscribeToAttribute()
    {
        // EXPECT_CALL(*mockMessageSender,
        //            sendSubscriptionRequest(_,_,_,_)).Times(1);

        std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation>>
                subscriptionListener(new MockGpsSubscriptionListener());
        // TODO uncomment once the connector has the correct signature!
        // std::shared_ptr<vehicle::IGps> gpsFixture = createFixture(false);
        // SubscriptionQos subscriptionQos(100, 200, true, 80, 80);
        // gpsFixture->subscribeToLocation(subscriptionListener, subscriptionQos);
    }

    void doNotSendSubscriptionStopForMulticastSubscription()
    {
        const std::string subscriptionId = "multicast";
        const std::string expectedSubscriptionId = subscriptionId;

        auto testFixture = createItestSubscriptionFixture();

        EXPECT_CALL(*mockSubscriptionManager, unregisterSubscription(Eq(expectedSubscriptionId)));

        // sendSubscriptionStop should NOT be called on MessageSender
        EXPECT_CALL(*mockMessageSender, sendSubscriptionStop(_,_,_,_)).Times(0);

        testFixture->unsubscribeFromEmptyBroadcastBroadcast(subscriptionId);
    }

    void sendSubscriptionStopForSelectiveSubscription()
    {
        const std::string subscriptionId = "selectiveBroadcast";
        const std::string expectedSubscriptionId = subscriptionId;

        auto testFixture = createItestSubscriptionFixture();

        EXPECT_CALL(*mockSubscriptionManager, unregisterSubscription(Eq(expectedSubscriptionId)));

        // sendSubscriptionStop SHOULD be called on MessageSender
        joynr::SubscriptionStop capturedSubscriptionStop;
        EXPECT_CALL(*mockMessageSender,
                    sendSubscriptionStop(Eq(proxyParticipantId),
                                         Eq(providerParticipantId),
                                         _,
                                         _)).Times(1)
                .WillOnce(SaveArg<3>(&capturedSubscriptionStop));

        testFixture->unsubscribeFromBooleanBroadcastBroadcast(subscriptionId);

        EXPECT_EQ(subscriptionId, capturedSubscriptionStop.getSubscriptionId());
    }

private:
    std::shared_ptr<SingleThreadedIOService> singleThreadedIOService;
    DISALLOW_COPY_AND_ASSIGN(AbstractSyncAsyncTest);

protected:
    std::shared_ptr<MockSubscriptionManager> mockSubscriptionManager;
    joynr::types::Localisation::GpsLocation expectedGpsLocation;
    int expectedInt;
    CallBackActions callBackActions;
    MessagingQos qosSettings;
    MockDispatcher mockDispatcher;
    MockMessagingStub mockMessagingStub;
    std::shared_ptr<IReplyCaller> callBack;
    std::shared_ptr<MockMessageSender> mockMessageSender;
    std::string proxyParticipantId;
    std::string providerParticipantId;
    std::shared_ptr<tests::Itest> asyncTestFixture;
    std::shared_ptr<exceptions::JoynrException> error;

};
