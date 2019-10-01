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
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/ReplyInterpreter.h"
#include "joynr/ReplyCaller.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/types/Localisation/GpsLocation.h"
#include "joynr/types/Localisation/Trip.h"
#include "joynr/types/TestTypes/TEverythingMap.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockCallback.h"

using ::testing::A;
using ::testing::_;
using ::testing::Property;
using ::testing::Eq;
using ::testing::Pointee;

using namespace joynr;

class ReplyInterpreterTest : public ::testing::Test
{
};

TEST_F(ReplyInterpreterTest, execute_calls_caller_with_maps)
{
    // Create a mock callback
    auto callback = std::make_shared<
            MockCallbackWithJoynrException<joynr::types::TestTypes::TEverythingMap>>();
    types::TestTypes::TEverythingMap responseValue;
    EXPECT_CALL(*callback, onSuccess(Eq(responseValue))).Times(1);
    EXPECT_CALL(*callback, onError(_)).Times(0);

    // Create a reply caller
    auto icaller = std::make_shared<ReplyCaller<types::TestTypes::TEverythingMap>>(
            [callback](const types::TestTypes::TEverythingMap& map) { callback->onSuccess(map); },
            [callback](const std::shared_ptr<exceptions::JoynrException>& error) {
                callback->onError(error);
            });

    // Create a reply
    Reply reply;
    reply.setResponse(responseValue);

    // Interpret the reply
    icaller->execute(std::move(reply));
}

TEST_F(ReplyInterpreterTest, execute_calls_caller)
{
    // Create a mock callback
    auto callback = std::make_shared<
            MockCallbackWithJoynrException<joynr::types::Localisation::GpsLocation>>();
    int myAltitude = 13;
    EXPECT_CALL(*callback,
                onSuccess(Property(&types::Localisation::GpsLocation::getAltitude, myAltitude)))
            .Times(1);
    EXPECT_CALL(*callback, onError(_)).Times(0);

    // Create a reply caller
    auto icaller = std::make_shared<ReplyCaller<types::Localisation::GpsLocation>>(
            [callback](const types::Localisation::GpsLocation& location) {
                callback->onSuccess(location);
            },
            [callback](const std::shared_ptr<exceptions::JoynrException>& error) {
                callback->onError(error);
            });

    // Create a reply
    types::Localisation::GpsLocation location;
    location.setAltitude(myAltitude);
    Reply reply;
    reply.setResponse(location);

    // Interpret the reply
    icaller->execute(std::move(reply));
}

TEST_F(ReplyInterpreterTest, execute_calls_caller_void)
{
    // Create a mock callback
    auto callback = std::make_shared<MockCallbackWithJoynrException<void>>();
    EXPECT_CALL(*callback, onSuccess()).Times(1);
    EXPECT_CALL(*callback, onError(_)).Times(0);

    // Create a reply caller
    auto icaller = std::make_shared<ReplyCaller<void>>(
            [callback]() { callback->onSuccess(); },
            [callback](const std::shared_ptr<exceptions::JoynrException>& error) {
                callback->onError(error);
            });

    // Create a reply
    Reply reply;

    // Interpret the reply
    icaller->execute(std::move(reply));
}

TEST_F(ReplyInterpreterTest, execute_calls_caller_with_error)
{
    // Create a reply
    exceptions::ProviderRuntimeException error("ReplyInterpreterTestProviderRuntimeException");
    Reply reply;
    reply.setError(std::make_shared<exceptions::ProviderRuntimeException>(error));

    // Create a mock callback
    auto callback = std::make_shared<
            MockCallbackWithJoynrException<joynr::types::Localisation::GpsLocation>>();
    EXPECT_CALL(*callback, onSuccess(_)).Times(0);
    EXPECT_CALL(*callback,
                onError(Pointee(joynrException(error.getTypeName(), error.getMessage())))).Times(1);

    // Create a reply caller
    auto icaller = std::make_shared<ReplyCaller<types::Localisation::GpsLocation>>(
            [callback](const types::Localisation::GpsLocation& location) {
                callback->onSuccess(location);
            },
            [callback](const std::shared_ptr<exceptions::JoynrException>& exceptionError) {
                callback->onError(exceptionError);
            });

    icaller->execute(std::move(reply));
}

TEST_F(ReplyInterpreterTest, execute_calls_caller_void_with_error)
{
    // Create a reply
    exceptions::ProviderRuntimeException error("ReplyInterpreterTestProviderRuntimeException");
    Reply reply;
    reply.setError(std::make_shared<exceptions::ProviderRuntimeException>(error));

    // Create a mock callback
    auto callback = std::make_shared<MockCallbackWithJoynrException<void>>();
    EXPECT_CALL(*callback, onSuccess()).Times(0);
    EXPECT_CALL(*callback,
                onError(Pointee(joynrException(error.getTypeName(), error.getMessage())))).Times(1);

    // Create a reply caller
    auto icaller = std::make_shared<ReplyCaller<void>>(
            [callback]() { callback->onSuccess(); },
            [callback](const std::shared_ptr<exceptions::JoynrException>& exceptionError) {
                callback->onError(exceptionError);
            });

    // Interpret the reply
    icaller->execute(std::move(reply));
}

TEST_F(ReplyInterpreterTest, execute_empty_reply)
{
    exceptions::JoynrRuntimeException error("Reply object had no response.");

    // Create a mock callback
    auto callback = std::make_shared<
            MockCallbackWithJoynrException<joynr::types::Localisation::GpsLocation>>();
    EXPECT_CALL(*callback, onSuccess(_)).Times(0);
    EXPECT_CALL(*callback,
                onError(Pointee(joynrException(error.getTypeName(), error.getMessage())))).Times(1);

    // Create a reply caller
    auto icaller = std::make_shared<ReplyCaller<types::Localisation::GpsLocation>>(
            [callback](const types::Localisation::GpsLocation& location) {
                callback->onSuccess(location);
            },
            [callback](const std::shared_ptr<exceptions::JoynrException>& exceptionError) {
                callback->onError(exceptionError);
            });

    // Create a reply
    Reply reply;

    // Interpret the reply
    icaller->execute(std::move(reply));
}
