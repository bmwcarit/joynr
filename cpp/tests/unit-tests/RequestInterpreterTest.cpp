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

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "joynr/serializer/Serializer.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/Request.h"
#include "joynr/Reply.h"
#include "joynr/vehicle/IGps.h"
#include "joynr/vehicle/GpsRequestInterpreter.h"
#include "joynr/tests/testRequestInterpreter.h"
#include "joynr/IRequestInterpreter.h"
#include "joynr/exceptions/MethodInvocationException.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockCallback.h"
#include "tests/mock/MockTestRequestCaller.h"

using ::testing::A;
using ::testing::_;

MATCHER_P(methodInvocationExceptionWithProviderVersion, expectedProviderVersion, "")
{
    const joynr::exceptions::MethodInvocationException* methodInvocationExceptionPtr;
    return arg.getTypeName() == joynr::exceptions::MethodInvocationException::TYPE_NAME() &&
           (methodInvocationExceptionPtr =
                    dynamic_cast<const joynr::exceptions::MethodInvocationException*>(&arg)) !=
                   nullptr &&
           methodInvocationExceptionPtr->getProviderVersion() == expectedProviderVersion;
}

using namespace joynr;

class RequestInterpreterTest : public ::testing::Test
{
public:
    RequestInterpreterTest() : gpsInterfaceName(vehicle::IGpsBase::INTERFACE_NAME())
    {
    }

protected:
    std::string gpsInterfaceName;
};

// we need to serialize, then deserialize the request to get it in a state which can be passed to a
// request interpreter
template <typename... Ts>
joynr::Request initRequest(std::string methodName,
                           std::vector<std::string> paramDataTypes,
                           Ts... paramValues)
{
    Request outRequest;
    outRequest.setMethodName(methodName);
    outRequest.setParamDatatypes(std::move(paramDataTypes));
    outRequest.setParams(std::move(paramValues)...);

    return outRequest;
}

TEST_F(RequestInterpreterTest, execute_callsMethodOnRequestCallerWithMapParameter)
{
    auto mockCaller = std::make_shared<MockTestRequestCaller>();
    EXPECT_CALL(
            *mockCaller,
            mapParametersMock(
                    A<const types::TestTypes::TStringKeyMap&>(),
                    A<std::function<void(const types::TestTypes::TStringKeyMap&)>>(),
                    A<std::function<void(const std::shared_ptr<exceptions::JoynrException>&)>>()))
            .Times(1);

    tests::testRequestInterpreter interpreter;
    std::string methodName = "mapParameters";
    types::TestTypes::TStringKeyMap inputMap;
    std::vector<std::string> paramDatatypes = {"joynr.types.TestTypes.TStringKeyMap"};

    auto callback = std::make_shared<MockCallback<Reply&&>>();
    auto onSuccess = [inputMap, callback](Reply&& reply) {
        // EXPECT_EQ(inputMap, response.at(0).get<types::TestTypes::TStringKeyMap>());
        callback->onSuccess(std::move(reply));
    };
    auto onError = [](const std::shared_ptr<exceptions::JoynrException>&) {
        ADD_FAILURE() << "unexpected call of onError function";
    };
    // since Google Mock does not support r-value references,
    // the call to onSuccess(Reply&&) is proxied to onSuccess(const Reply&)
    EXPECT_CALL(*callback, onSuccess(A<const Reply&>())).Times(1);

    Request request = initRequest(methodName, paramDatatypes, inputMap);
    interpreter.execute(mockCaller, request, onSuccess, onError);
}

TEST_F(RequestInterpreterTest, execute_callsMethodOnRequestCaller)
{
    auto mockCaller = std::make_shared<MockTestRequestCaller>();
    EXPECT_CALL(*mockCaller,
                getLocationMock(
                        A<std::function<void(const types::Localisation::GpsLocation&)>>(),
                        A<std::function<void(
                                const std::shared_ptr<exceptions::ProviderRuntimeException>&)>>()))
            .Times(1);

    tests::testRequestInterpreter interpreter;
    std::string methodName = "getLocation";
    auto callback = std::make_shared<MockCallback<Reply&&>>();
    auto onSuccess = [callback](Reply&& response) { callback->onSuccess(std::move(response)); };
    auto onError = [](const std::shared_ptr<exceptions::JoynrException>&) {
        ADD_FAILURE() << "unexpected call of onError function";
    };
    // since Google Mock does not support r-value references,
    // the call to onSuccess(Reply&&) is proxied to onSuccess(const Reply&)
    EXPECT_CALL(*callback, onSuccess(A<const Reply&>())).Times(1);

    Request request = initRequest(methodName, {});
    interpreter.execute(mockCaller, request, onSuccess, onError);
}

TEST_F(RequestInterpreterTest, execute_callsMethodOnRequestCallerWithProviderRuntimeException)
{
    auto mockCaller = std::make_shared<MockTestRequestCaller>();
    EXPECT_CALL(
            *mockCaller,
            methodWithProviderRuntimeExceptionMock(
                    A<std::function<void()>>(),
                    A<std::function<void(const std::shared_ptr<exceptions::JoynrException>&)>>()))
            .Times(1);

    tests::testRequestInterpreter interpreter;
    std::string methodName = "methodWithProviderRuntimeException";

    auto callback = std::make_shared<MockCallback<void>>();
    auto onSuccess =
            [](joynr::Reply&&) { ADD_FAILURE() << "unexpected call of onSuccess function"; };
    auto onError = [callback](const std::shared_ptr<exceptions::JoynrException>& exception) {
        callback->onError(*exception);
    };
    EXPECT_CALL(*callback,
                onError(joynrException(joynr::exceptions::ProviderRuntimeException::TYPE_NAME(),
                                       mockCaller->_providerRuntimeExceptionTestMsg))).Times(1);

    Request request = initRequest(methodName, {});
    interpreter.execute(mockCaller, request, onSuccess, onError);
}

TEST_F(RequestInterpreterTest, execute_callsGetterMethodOnRequestCallerWithProviderRuntimeException)
{
    auto mockCaller = std::make_shared<MockTestRequestCaller>();
    EXPECT_CALL(*mockCaller,
                getAttributeWithProviderRuntimeExceptionMock(
                        A<std::function<void(const std::int32_t&)>>(),
                        A<std::function<void(
                                const std::shared_ptr<exceptions::ProviderRuntimeException>&)>>()))
            .Times(1);

    tests::testRequestInterpreter interpreter;
    std::string methodName = "getAttributeWithProviderRuntimeException";

    auto callback = std::make_shared<MockCallback<std::int32_t>>();
    auto onSuccess =
            [](joynr::Reply&&) { ADD_FAILURE() << "unexpected call of onSuccess function"; };
    auto onError = [callback](const std::shared_ptr<exceptions::JoynrException>& exception) {
        callback->onError(*exception);
    };
    EXPECT_CALL(*callback,
                onError(joynrException(joynr::exceptions::ProviderRuntimeException::TYPE_NAME(),
                                       mockCaller->_providerRuntimeExceptionTestMsg))).Times(1);

    Request request = initRequest(methodName, {});
    interpreter.execute(mockCaller, request, onSuccess, onError);
}

TEST_F(RequestInterpreterTest, execute_callsMethodWithInvalidArguments)
{
    auto mockCaller = std::make_shared<MockTestRequestCaller>();
    auto expectedProviderVersion = mockCaller->getProviderVersion();

    tests::testRequestInterpreter interpreter;
    std::string methodName = "sumInts";
    std::vector<std::string> paramDatatypes;
    paramDatatypes.push_back("Integer[]");

    auto callback = std::make_shared<MockCallback<std::int32_t>>();
    auto onSuccess =
            [](joynr::Reply&&) { ADD_FAILURE() << "unexpected call of onSuccess function"; };
    auto onError = [callback](const std::shared_ptr<exceptions::JoynrException>& exception) {
        callback->onError(*exception);
    };
    EXPECT_CALL(*callback,
                onError(methodInvocationExceptionWithProviderVersion(expectedProviderVersion)))
            .Times(1);

    Request request = initRequest(
            methodName, paramDatatypes, std::string("invalidParamCannotBeConvertedToInteger[]"));
    interpreter.execute(mockCaller, request, onSuccess, onError);
}

TEST_F(RequestInterpreterTest, execute_callsSetterMethodWithInvalidArguments)
{
    auto mockCaller = std::make_shared<MockTestRequestCaller>();
    auto expectedProviderVersion = mockCaller->getProviderVersion();

    tests::testRequestInterpreter interpreter;
    std::string methodName = "setTestAttribute";
    std::vector<std::string> paramDatatypes = {"Doesn'tMatter"};

    auto callback = std::make_shared<MockCallback<std::int32_t>>();
    auto onSuccess =
            [](joynr::Reply&&) { ADD_FAILURE() << "unexpected call of onSuccess function"; };
    auto onError = [callback](const std::shared_ptr<exceptions::JoynrException>& exception) {
        callback->onError(*exception);
    };
    EXPECT_CALL(*callback,
                onError(methodInvocationExceptionWithProviderVersion(expectedProviderVersion)))
            .Times(1);

    Request request = initRequest(
            methodName, paramDatatypes, std::string("invalidParamCannotBeConvertedTostd::Int32_t"));
    interpreter.execute(mockCaller, request, onSuccess, onError);
}

TEST_F(RequestInterpreterTest, execute_callsSetterMethodWithInvalidArguments2)
{
    auto mockCaller = std::make_shared<MockTestRequestCaller>();
    auto expectedProviderVersion = mockCaller->getProviderVersion();

    tests::testRequestInterpreter interpreter;
    std::string methodName = "setTestAttribute";
    std::vector<std::string> paramDatatypes = {"Doesn'tMatter"};

    auto callback = std::make_shared<MockCallback<std::int32_t>>();
    auto onSuccess =
            [](joynr::Reply&&) { ADD_FAILURE() << "unexpected call of onSuccess function"; };
    auto onError = [callback](const std::shared_ptr<exceptions::JoynrException>& exception) {
        callback->onError(*exception);
    };
    EXPECT_CALL(*callback,
                onError(methodInvocationExceptionWithProviderVersion(expectedProviderVersion)))
            .Times(1);

    Request request = initRequest(methodName, paramDatatypes, types::Localisation::GpsLocation());
    interpreter.execute(mockCaller, request, onSuccess, onError);
}

TEST_F(RequestInterpreterTest, execute_callsNonExistingMethod)
{
    auto mockCaller = std::make_shared<MockTestRequestCaller>();
    auto expectedProviderVersion = mockCaller->getProviderVersion();

    tests::testRequestInterpreter interpreter;
    std::string methodName = "execute_callsNonExistingMethod";

    auto callback = std::make_shared<MockCallback<std::int32_t>>();
    auto onSuccess =
            [](joynr::Reply&&) { ADD_FAILURE() << "unexpected call of onSuccess function"; };
    auto onError = [callback](const std::shared_ptr<exceptions::JoynrException>& exception) {
        callback->onError(*exception);
    };
    EXPECT_CALL(*callback,
                onError(methodInvocationExceptionWithProviderVersion(expectedProviderVersion)))
            .Times(1);

    Request request = initRequest(methodName, {});
    interpreter.execute(mockCaller, request, onSuccess, onError);
}

TEST_F(RequestInterpreterTest, getUnknownInterfaceReturnsNullptr)
{
    InterfaceRegistrar& registrar = InterfaceRegistrar::instance();

    ASSERT_EQ(nullptr, registrar.getRequestInterpreter("unknown interface"));
}

TEST_F(RequestInterpreterTest, create_createsGpsInterpreter)
{
    InterfaceRegistrar& registrar = InterfaceRegistrar::instance();
    registrar.reset();
    registrar.registerRequestInterpreter<vehicle::GpsRequestInterpreter>(gpsInterfaceName);

    std::shared_ptr<IRequestInterpreter> gpsInterpreter =
            registrar.getRequestInterpreter(gpsInterfaceName);

    EXPECT_FALSE(gpsInterpreter.get() == 0);
}

TEST_F(RequestInterpreterTest, create_multipleCallsReturnSameInterpreter)
{
    InterfaceRegistrar& registrar = InterfaceRegistrar::instance();
    registrar.reset();
    registrar.registerRequestInterpreter<vehicle::GpsRequestInterpreter>(gpsInterfaceName);

    std::shared_ptr<IRequestInterpreter> gpsInterpreter1 =
            registrar.getRequestInterpreter(gpsInterfaceName);
    std::shared_ptr<IRequestInterpreter> gpsInterpreter2 =
            registrar.getRequestInterpreter(gpsInterfaceName);

    EXPECT_EQ(gpsInterpreter1, gpsInterpreter2);
}

TEST_F(RequestInterpreterTest, registerUnregister)
{
    InterfaceRegistrar& registrar = InterfaceRegistrar::instance();
    registrar.reset();

    // Register the interface twice and check that the interpreter does not change
    registrar.registerRequestInterpreter<vehicle::GpsRequestInterpreter>(gpsInterfaceName);
    std::shared_ptr<IRequestInterpreter> gpsInterpreter1 =
            registrar.getRequestInterpreter(gpsInterfaceName);
    registrar.registerRequestInterpreter<vehicle::GpsRequestInterpreter>(gpsInterfaceName);
    std::shared_ptr<IRequestInterpreter> gpsInterpreter2 =
            registrar.getRequestInterpreter(gpsInterfaceName);
    EXPECT_EQ(gpsInterpreter1, gpsInterpreter2);

    // Unregister once
    registrar.unregisterRequestInterpreter(gpsInterfaceName);
    std::shared_ptr<IRequestInterpreter> gpsInterpreter3 =
            registrar.getRequestInterpreter(gpsInterfaceName);
    EXPECT_EQ(gpsInterpreter1, gpsInterpreter3);

    // Unregister again
    registrar.unregisterRequestInterpreter(gpsInterfaceName);

    // Register the interface - this should create a new request interpreter
    registrar.registerRequestInterpreter<vehicle::GpsRequestInterpreter>(gpsInterfaceName);
    std::shared_ptr<IRequestInterpreter> gpsInterpreter4 =
            registrar.getRequestInterpreter(gpsInterfaceName);
    EXPECT_NE(gpsInterpreter1, gpsInterpreter4);
}
