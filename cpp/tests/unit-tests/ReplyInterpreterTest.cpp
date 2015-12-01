/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
//#needed:?
#include "joynr/JoynrMessageSender.h"
#include "joynr/MessagingQos.h"
#include "joynr/JoynrMessage.h"
#include "joynr/JoynrMessageFactory.h"
#include "joynr/Dispatcher.h"
#include "joynr/vehicle/IGps.h"
#include "joynr/Request.h"
#include "tests/utils/MockObjects.h"
#include "joynr/JsonSerializer.h"

using ::testing::A;
using ::testing::_;

using namespace joynr;

class ReplyInterpreterTest : public ::testing::Test {
public:
    ReplyInterpreterTest()
    {
    }
protected:
};




TEST_F(ReplyInterpreterTest, execute_calls_caller) {
    // Register metatypes
    //qRegisterMetaType<Reply>();
    //qRegisterMetaType<types::Localisation::GpsLocation>();
    MetaTypeRegistrar& registrar = MetaTypeRegistrar::instance();
    registrar.registerReplyMetaType<types::Localisation::GpsLocation>();

    // Create a mock callback
    std::shared_ptr<MockCallback<joynr::types::Localisation::GpsLocation>> callback(new MockCallback<joynr::types::Localisation::GpsLocation>());
    int myAltitude = 13;
    EXPECT_CALL(*callback, onSuccess(Property(&types::Localisation::GpsLocation::getAltitude, myAltitude)))
                .Times(1);

    // Create a reply caller
    std::shared_ptr<IReplyCaller> icaller(new ReplyCaller<types::Localisation::GpsLocation>(
            [callback](const RequestStatus& status, const types::Localisation::GpsLocation& location) {
                callback->onSuccess(location);
            },
            [](const RequestStatus& status, std::shared_ptr<exceptions::JoynrException> error){
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

