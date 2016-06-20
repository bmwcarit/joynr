/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include "joynr/InterfaceRegistrar.h"
#include "joynr/vehicle/IGps.h"
#include "joynr/vehicle/GpsRequestInterpreter.h"
#include "joynr/tests/testRequestInterpreter.h"
#include "joynr/IRequestInterpreter.h"
#include "tests/utils/MockObjects.h"
#include "utils/MockCallback.h"
#include "joynr/exceptions/MethodInvocationException.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>

using ::testing::A;
using ::testing::_;

MATCHER_P(providerRuntimeException, msg, "") {
    return arg.getTypeName() == joynr::exceptions::ProviderRuntimeException::TYPE_NAME
            && arg.getMessage() == msg;
}

MATCHER_P(methodInvocationException, msg, "") {
    return arg.getTypeName() == joynr::exceptions::MethodInvocationException::TYPE_NAME
            && arg.getMessage() == msg;
}

using namespace joynr;

class RequestInterpreterTest : public ::testing::Test {
public:
    RequestInterpreterTest()
        : gpsInterfaceName(vehicle::IGpsBase::INTERFACE_NAME())
    {

    }

protected:
    std::string gpsInterfaceName;
};

TEST_F(RequestInterpreterTest, execute_callsMethodOnRequestCallerWithMapParameter) {
    auto mockCaller = std::make_shared<MockTestRequestCaller>();
    EXPECT_CALL(
            *mockCaller,
            mapParameters(A<const types::TestTypes::TStringKeyMap&>(),
                          A<std::function<void(const types::TestTypes::TStringKeyMap&)>>(),
                          A<std::function<void(const exceptions::JoynrException&)>>())
    )
            .Times(1);

    tests::testRequestInterpreter interpreter;
    std::string methodName = "mapParameters";
    std::vector<Variant> paramValues;
    types::TestTypes::TStringKeyMap inputMap;
    paramValues.push_back(Variant::make<types::TestTypes::TStringKeyMap>(inputMap));
    std::vector<std::string> paramDatatypes;
    paramDatatypes.push_back("joynr.types.TestTypes.TStringKeyMap");

    auto callback = std::make_shared<MockCallback<std::vector<Variant>>>();
    std::function<void(const std::vector<Variant>& response)> onSuccess = [inputMap, callback] (const std::vector<Variant>& response) {
        EXPECT_EQ(inputMap, response.at(0).get<types::TestTypes::TStringKeyMap>());
        callback->onSuccess(response);
    };
    std::function<void(const exceptions::JoynrException& exception)> onError = [] (const exceptions::JoynrException& exception) {
        ADD_FAILURE()<< "unexpected call of onError function";
    };
    EXPECT_CALL(*callback, onSuccess(A<const std::vector<Variant>&>())).Times(1);

    interpreter.execute(mockCaller, methodName, paramValues, paramDatatypes, onSuccess, onError);
}

TEST_F(RequestInterpreterTest, execute_callsMethodOnRequestCaller) {
    auto mockCaller = std::make_shared<MockTestRequestCaller>();
    EXPECT_CALL(
            *mockCaller,
            getLocation(A<std::function<void(const types::Localisation::GpsLocation&)>>(),
                        A<std::function<void(const exceptions::ProviderRuntimeException&)>>())
    )
            .Times(1);

    tests::testRequestInterpreter interpreter;
    std::string methodName = "getLocation";
    std::vector<Variant> paramValues;
    std::vector<std::string> paramDatatypes;

    auto callback = std::make_shared<MockCallback<std::vector<Variant>>>();
    std::function<void(const std::vector<Variant>& response)> onSuccess = [callback] (const std::vector<Variant>& response) {
        EXPECT_EQ(types::Localisation::GpsLocation(), response.at(0).get<types::Localisation::GpsLocation>());
        callback->onSuccess(response);
    };
    std::function<void(const exceptions::JoynrException& exception)> onError = [] (const exceptions::JoynrException& exception) {
        ADD_FAILURE()<< "unexpected call of onError function";
    };
    EXPECT_CALL(*callback, onSuccess(A<const std::vector<Variant>&>())).Times(1);

    interpreter.execute(mockCaller, methodName, paramValues, paramDatatypes, onSuccess, onError);
}

TEST_F(RequestInterpreterTest, execute_callsMethodOnRequestCallerWithProviderRuntimeException) {
    auto mockCaller = std::make_shared<MockTestRequestCaller>();
    EXPECT_CALL(
            *mockCaller,
            methodWithProviderRuntimeException(A<std::function<void()>>(),
                        A<std::function<void(const exceptions::JoynrException&)>>())
    )
            .Times(1);

    tests::testRequestInterpreter interpreter;
    std::string methodName = "methodWithProviderRuntimeException";
    std::vector<Variant> paramValues;
    std::vector<std::string> paramDatatypes;

    auto callback = std::make_shared<MockCallback<void>>();
    std::function<void(const std::vector<Variant>&& response)> onSuccess = [] (const std::vector<Variant>&& response) {ADD_FAILURE()<< "unexpected call of onSuccess function";};
    std::function<void(const exceptions::JoynrException& exception)> onError = [callback] (const exceptions::JoynrException& exception) {
        callback->onError(exception);
    };
    EXPECT_CALL(*callback, onError(providerRuntimeException(mockCaller->providerRuntimeExceptionTestMsg))).Times(1);

    interpreter.execute(mockCaller, methodName, paramValues, paramDatatypes, onSuccess, onError);
}

TEST_F(RequestInterpreterTest, execute_callsGetterMethodOnRequestCallerWithProviderRuntimeException) {
    auto mockCaller = std::make_shared<MockTestRequestCaller>();
    EXPECT_CALL(
            *mockCaller,
            getAttributeWithProviderRuntimeException(A<std::function<void(const std::int32_t&)>>(),
                        A<std::function<void(const exceptions::ProviderRuntimeException&)>>())
    )
            .Times(1);

    tests::testRequestInterpreter interpreter;
    std::string methodName = "getAttributeWithProviderRuntimeException";
    std::vector<Variant> paramValues;
    std::vector<std::string> paramDatatypes;

    auto callback = std::make_shared<MockCallback<std::int32_t>>();
    std::function<void(const std::vector<Variant>&& response)> onSuccess = [] (const std::vector<Variant>&& response) {ADD_FAILURE()<< "unexpected call of onSuccess function";};
    std::function<void(const exceptions::JoynrException& exception)> onError = [callback] (const exceptions::JoynrException& exception) {
        callback->onError(exception);
    };
    EXPECT_CALL(*callback, onError(providerRuntimeException(mockCaller->providerRuntimeExceptionTestMsg))).Times(1);

    interpreter.execute(mockCaller, methodName, paramValues, paramDatatypes, onSuccess, onError);
}

TEST_F(RequestInterpreterTest, execute_callsMethodWithInvalidArguments) {
    auto mockCaller = std::make_shared<MockTestRequestCaller>();

    tests::testRequestInterpreter interpreter;
    std::string methodName = "sumInts";
    std::vector<Variant> paramValues;
    std::vector<std::string> paramDatatypes;
    paramValues.push_back(Variant::make<std::string>("invalidParamCannotBeConvertedToInteger[]"));
    paramDatatypes.push_back("Integer[]");

    auto callback = std::make_shared<MockCallback<std::int32_t>>();
    std::function<void(const std::vector<Variant>&& response)> onSuccess = [] (const std::vector<Variant>&& response) {ADD_FAILURE()<< "unexpected call of onSuccess function";};
    std::function<void(const exceptions::JoynrException& exception)> onError = [callback] (const exceptions::JoynrException& exception) {
        callback->onError(exception);
    };
    EXPECT_CALL(*callback, onError(methodInvocationException("Illegal argument for method sumInts: ints (Integer[])"))).Times(1);

    interpreter.execute(mockCaller, methodName, paramValues, paramDatatypes, onSuccess, onError);
}

TEST_F(RequestInterpreterTest, execute_callsSetterMethodWithInvalidArguments) {
    auto mockCaller = std::make_shared<MockTestRequestCaller>();

    tests::testRequestInterpreter interpreter;
    std::string methodName = "setTestAttribute";
    std::vector<Variant> paramValues;
    std::vector<std::string> paramDatatypes;
    paramValues.push_back(Variant::make<std::string>("invalidParamCannotBeConvertedTostd::Int32_t"));
    paramDatatypes.push_back("Doesn'tMatter");

    auto callback = std::make_shared<MockCallback<std::int32_t>>();
    std::function<void(const std::vector<Variant>&& response)> onSuccess = [] (const std::vector<Variant>&& response) {ADD_FAILURE()<< "unexpected call of onSuccess function";};
    std::function<void(const exceptions::JoynrException& exception)> onError = [callback] (const exceptions::JoynrException& exception) {
        callback->onError(exception);
    };
    EXPECT_CALL(*callback, onError(methodInvocationException("Illegal argument for attribute setter setTestAttribute (Integer)"))).Times(1);

    interpreter.execute(mockCaller, methodName, paramValues, paramDatatypes, onSuccess, onError);
}

TEST_F(RequestInterpreterTest, execute_callsSetterMethodWithInvalidArguments2) {
    auto mockCaller = std::make_shared<MockTestRequestCaller>();

    tests::testRequestInterpreter interpreter;
    std::string methodName = "setTestAttribute";
    std::vector<Variant> paramValues;
    std::vector<std::string> paramDatatypes;
    paramValues.push_back(Variant::make<joynr::types::Localisation::GpsLocation>(types::Localisation::GpsLocation()));
    paramDatatypes.push_back("Doesn'tMatter");

    auto callback = std::make_shared<MockCallback<std::int32_t>>();
    std::function<void(const std::vector<Variant>&& response)> onSuccess = [] (const std::vector<Variant>&& response) {ADD_FAILURE()<< "unexpected call of onSuccess function";};
    std::function<void(const exceptions::JoynrException& exception)> onError = [callback] (const exceptions::JoynrException& exception) {
        callback->onError(exception);
    };
    EXPECT_CALL(*callback, onError(methodInvocationException("Illegal argument for attribute setter setTestAttribute (Integer)"))).Times(1);

    interpreter.execute(mockCaller, methodName, paramValues, paramDatatypes, onSuccess, onError);
}

TEST_F(RequestInterpreterTest, execute_callsNonExistingMethod) {
    auto mockCaller = std::make_shared<MockTestRequestCaller>();

    tests::testRequestInterpreter interpreter;
    std::string methodName = "execute_callsNonExistingMethod";
    std::vector<Variant> paramValues;
    std::vector<std::string> paramDatatypes;

    auto callback = std::make_shared<MockCallback<std::int32_t>>();
    std::function<void(const std::vector<Variant>&& response)> onSuccess = [] (const std::vector<Variant>&& response) {ADD_FAILURE()<< "unexpected call of onSuccess function";};
    std::function<void(const exceptions::JoynrException& exception)> onError = [callback] (const exceptions::JoynrException& exception) {
        callback->onError(exception);
    };
    EXPECT_CALL(*callback, onError(methodInvocationException("unknown method name for interface test: execute_callsNonExistingMethod"))).Times(1);

    interpreter.execute(mockCaller, methodName, paramValues, paramDatatypes, onSuccess, onError);
}


TEST(RequestInterpreterDeathTest, get_assertsUnknownInterface) {
    InterfaceRegistrar& registrar = InterfaceRegistrar::instance();

    ASSERT_DEATH(registrar.getRequestInterpreter("unknown interface"), "Assertion.*");
}


TEST_F(RequestInterpreterTest, create_createsGpsInterpreter) {
    InterfaceRegistrar& registrar = InterfaceRegistrar::instance();
    registrar.reset();
    registrar.registerRequestInterpreter<vehicle::GpsRequestInterpreter>(gpsInterfaceName);

    std::shared_ptr<IRequestInterpreter> gpsInterpreter = registrar.getRequestInterpreter(gpsInterfaceName);

    EXPECT_FALSE(gpsInterpreter.get() == 0);
}

TEST_F(RequestInterpreterTest, create_multipleCallsReturnSameInterpreter) {
    InterfaceRegistrar& registrar = InterfaceRegistrar::instance();
    registrar.reset();
    registrar.registerRequestInterpreter<vehicle::GpsRequestInterpreter>(gpsInterfaceName);

    std::shared_ptr<IRequestInterpreter> gpsInterpreter1 = registrar.getRequestInterpreter(gpsInterfaceName);
    std::shared_ptr<IRequestInterpreter> gpsInterpreter2 = registrar.getRequestInterpreter(gpsInterfaceName);

    EXPECT_EQ(gpsInterpreter1, gpsInterpreter2);
}

TEST_F(RequestInterpreterTest, registerUnregister) {
    InterfaceRegistrar& registrar = InterfaceRegistrar::instance();
    registrar.reset();

    // Register the interface twice and check that the interpreter does not change
    registrar.registerRequestInterpreter<vehicle::GpsRequestInterpreter>(gpsInterfaceName);
    std::shared_ptr<IRequestInterpreter> gpsInterpreter1 = registrar.getRequestInterpreter(gpsInterfaceName);
    registrar.registerRequestInterpreter<vehicle::GpsRequestInterpreter>(gpsInterfaceName);
    std::shared_ptr<IRequestInterpreter> gpsInterpreter2 = registrar.getRequestInterpreter(gpsInterfaceName);
    EXPECT_EQ(gpsInterpreter1, gpsInterpreter2);

    // Unregister once
    registrar.unregisterRequestInterpreter(gpsInterfaceName);
    std::shared_ptr<IRequestInterpreter> gpsInterpreter3 = registrar.getRequestInterpreter(gpsInterfaceName);
    EXPECT_EQ(gpsInterpreter1, gpsInterpreter3);

    // Unregister again
    registrar.unregisterRequestInterpreter(gpsInterfaceName);

    // Register the interface - this should create a new request interpreter
    registrar.registerRequestInterpreter<vehicle::GpsRequestInterpreter>(gpsInterfaceName);
    std::shared_ptr<IRequestInterpreter> gpsInterpreter4 = registrar.getRequestInterpreter(gpsInterfaceName);
    EXPECT_NE(gpsInterpreter1, gpsInterpreter4);
}

