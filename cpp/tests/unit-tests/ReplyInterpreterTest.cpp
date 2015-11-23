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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "joynr/ReplyInterpreter.h"
#include "joynr/MetaTypeRegistrar.h"

#include "joynr/types/Localisation/GpsLocation.h"
#include "joynr/types/Localisation/Trip.h"
#include "joynr/IReplyCaller.h"
#include "joynr/RequestStatus.h"
#include "tests/utils/MockObjects.h"

using ::testing::A;
using ::testing::_;

MATCHER_P(joynrException, other, "") {
    return arg.getTypeName() == other.getTypeName() && arg.getMessage() == other.getMessage();
}

using namespace joynr;

class ReplyInterpreterTest : public ::testing::Test {
public:
    ReplyInterpreterTest()
    {
    }
protected:
};




TEST_F(ReplyInterpreterTest, execute_calls_caller) {
    MetaTypeRegistrar& registrar = MetaTypeRegistrar::instance();
    registrar.registerReplyMetaType<types::Localisation::GpsLocation>();

    // Create a mock callback
    std::shared_ptr<MockCallbackWithOnErrorHavingRequestStatus<joynr::types::Localisation::GpsLocation>> callback(new MockCallbackWithOnErrorHavingRequestStatus<joynr::types::Localisation::GpsLocation>());
    int myAltitude = 13;
    EXPECT_CALL(*callback, onSuccess(Property(&types::Localisation::GpsLocation::getAltitude, myAltitude)))
                .Times(1);
    EXPECT_CALL(*callback, onError(_,_)).Times(0);

    // Create a reply caller
    std::shared_ptr<IReplyCaller> icaller(new ReplyCaller<types::Localisation::GpsLocation>(
            [callback](const RequestStatus& status, const types::Localisation::GpsLocation& location) {
                callback->onSuccess(location);
            },
            [callback](const RequestStatus& status, const exceptions::JoynrException& error){
                callback->onError(status, error);
            }));

    // Create a reply
    types::Localisation::GpsLocation location;
    location.setAltitude(myAltitude);
    std::vector<Variant> response;
    response.push_back(Variant::make<types::Localisation::GpsLocation>(location));
    Reply reply;
    reply.setResponse(std::move(response));

    // Interpret the reply
    IReplyInterpreter& interpreter = registrar.getReplyInterpreter(Util::getTypeId<types::Localisation::GpsLocation>());
    interpreter.execute(icaller, reply);
}

TEST_F(ReplyInterpreterTest, execute_calls_caller_void) {
    // Register metatypes
    MetaTypeRegistrar& registrar = MetaTypeRegistrar::instance();

    // Create a mock callback
    std::shared_ptr<MockCallbackWithOnErrorHavingRequestStatus<void>> callback(new MockCallbackWithOnErrorHavingRequestStatus<void>());
    EXPECT_CALL(*callback, onSuccess())
                .Times(1);
    EXPECT_CALL(*callback, onError(_,_)).Times(0);

    // Create a reply caller
    std::shared_ptr<IReplyCaller> icaller(new ReplyCaller<void>(
            [callback](const RequestStatus& status) {
                callback->onSuccess();
            },
            [callback](const RequestStatus& status, const exceptions::JoynrException& error){
                callback->onError(status, error);
            }));

    // Create a reply
    Reply reply;

    // Interpret the reply
    IReplyInterpreter& interpreter = registrar.getReplyInterpreter(Util::getTypeId<void>());
    interpreter.execute(icaller, reply);
}

TEST_F(ReplyInterpreterTest, execute_calls_caller_with_error) {
    // Register metatypes
    MetaTypeRegistrar& registrar = MetaTypeRegistrar::instance();
    registrar.registerReplyMetaType<types::Localisation::GpsLocation>();

    // Create a reply
    exceptions::ProviderRuntimeException error("ReplyInterpreterTestProviderRuntimeExeption");
    Reply reply;
    reply.setError(Variant::make<exceptions::ProviderRuntimeException>(error));

    // Create a mock callback
    std::shared_ptr<MockCallbackWithOnErrorHavingRequestStatus<joynr::types::Localisation::GpsLocation>> callback(new MockCallbackWithOnErrorHavingRequestStatus<joynr::types::Localisation::GpsLocation>());
    EXPECT_CALL(*callback, onSuccess(_))
                .Times(0);
    EXPECT_CALL(*callback, onError(Property(&RequestStatus::getCode, RequestStatusCode::ERROR),joynrException(error)))
                .Times(1);

    // Create a reply caller
    std::shared_ptr<IReplyCaller> icaller(new ReplyCaller<types::Localisation::GpsLocation>(
            [callback](const RequestStatus&, const types::Localisation::GpsLocation& location) {
                callback->onSuccess(location);
            },
            [callback](const RequestStatus& status, const exceptions::JoynrException& error){
                callback->onError(status, error);
            }));

    // Interpret the reply
    IReplyInterpreter& interpreter = registrar.getReplyInterpreter(Util::getTypeId<types::Localisation::GpsLocation>());
    interpreter.execute(icaller, reply);
}

TEST_F(ReplyInterpreterTest, execute_calls_caller_void_with_error) {
    // Register metatypes
    MetaTypeRegistrar& registrar = MetaTypeRegistrar::instance();

    // Create a reply
    exceptions::ProviderRuntimeException error("ReplyInterpreterTestProviderRuntimeExeption");
    Reply reply;
    reply.setError(Variant::make<exceptions::ProviderRuntimeException>(error));

    // Create a mock callback
    std::shared_ptr<MockCallbackWithOnErrorHavingRequestStatus<void>> callback(new MockCallbackWithOnErrorHavingRequestStatus<void>());
    EXPECT_CALL(*callback, onSuccess())
                .Times(0);
    EXPECT_CALL(*callback, onError(Property(&RequestStatus::getCode, RequestStatusCode::ERROR),joynrException(error)))
                .Times(1);

    // Create a reply caller
    std::shared_ptr<IReplyCaller> icaller(new ReplyCaller<void>(
            [callback](const RequestStatus& status) {
                callback->onSuccess();
            },
            [callback](const RequestStatus& status, const exceptions::JoynrException& error){
                callback->onError(status, error);
            }));

    // Interpret the reply
    IReplyInterpreter& interpreter = registrar.getReplyInterpreter(Util::getTypeId<void>());
    interpreter.execute(icaller, reply);
}

TEST_F(ReplyInterpreterTest, execute_empty_reply) {
    // Register metatypes
    MetaTypeRegistrar& registrar = MetaTypeRegistrar::instance();
    registrar.registerReplyMetaType<types::Localisation::GpsLocation>();

    exceptions::JoynrRuntimeException error("Reply object had no response.");

    // Create a mock callback
    std::shared_ptr<MockCallbackWithOnErrorHavingRequestStatus<joynr::types::Localisation::GpsLocation>> callback(new MockCallbackWithOnErrorHavingRequestStatus<joynr::types::Localisation::GpsLocation>());
    EXPECT_CALL(*callback, onSuccess(_))
                .Times(0);
    EXPECT_CALL(*callback, onError(Property(&RequestStatus::getCode, RequestStatusCode::ERROR),joynrException(error)))
                .Times(1);

    // Create a reply caller
    std::shared_ptr<IReplyCaller> icaller(new ReplyCaller<types::Localisation::GpsLocation>(
            [callback](const RequestStatus& status, const types::Localisation::GpsLocation& location) {
                callback->onSuccess(location);
            },
            [callback](const RequestStatus& status, const exceptions::JoynrException& error){
                callback->onError(status, error);
            }));

    // Create a reply
    Reply reply;

    // Interpret the reply
    IReplyInterpreter& interpreter = registrar.getReplyInterpreter(Util::getTypeId<types::Localisation::GpsLocation>());
    interpreter.execute(icaller, reply);
}


TEST_F(ReplyInterpreterTest, create_createsGpsInterpreterOnlyOnce) {

    MetaTypeRegistrar& registrar = MetaTypeRegistrar::instance();

    registrar.registerReplyMetaType<types::Localisation::GpsLocation>();
    registrar.registerReplyMetaType<types::Localisation::Trip>();

    IReplyInterpreter& interpreter1 = registrar.getReplyInterpreter(Util::getTypeId<types::Localisation::GpsLocation>());
    IReplyInterpreter& interpreter2 = registrar.getReplyInterpreter(Util::getTypeId<types::Localisation::GpsLocation>());
    IReplyInterpreter& interpreter3 = registrar.getReplyInterpreter(Util::getTypeId<types::Localisation::Trip>());

    EXPECT_TRUE(&interpreter1 == &interpreter2);
    EXPECT_TRUE(&interpreter2 != &interpreter3);

}

