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

#include "joynr/types/Localisation_QtGpsLocation.h"
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
    qRegisterMetaType<Reply>();
    qRegisterMetaType<types::Localisation::QtGpsLocation>();
    MetaTypeRegistrar& registrar = MetaTypeRegistrar::instance();
    registrar.registerReplyMetaType<types::Localisation::QtGpsLocation>();

    // Create a mock callback
    std::shared_ptr<MockCallback<joynr::types::Localisation::QtGpsLocation>> callback(new MockCallback<joynr::types::Localisation::QtGpsLocation>());
    int myAltitude = 13;
    EXPECT_CALL(*callback, onSuccess(Property(&types::Localisation::QtGpsLocation::getAltitude, myAltitude)))
                .Times(1);

    // Create a reply caller
    std::shared_ptr<IReplyCaller> icaller(new ReplyCaller<types::Localisation::QtGpsLocation>(
            [callback](const RequestStatus& status, const types::Localisation::QtGpsLocation& location) {
                callback->onSuccess(location);
            },
            [](const RequestStatus& status, std::shared_ptr<exceptions::JoynrException> error){
            }));

    // Create a reply
    types::Localisation::QtGpsLocation location;
    location.setAltitude(myAltitude);
    QList<QVariant> response;
    response.append(QVariant::fromValue(location));
    Reply reply;
    reply.setResponse(response);

    // Interpret the reply
    IReplyInterpreter& interpreter = registrar.getReplyInterpreter(Util::getTypeId<types::Localisation::QtGpsLocation>());
    interpreter.execute(icaller, reply);
}

TEST_F(ReplyInterpreterTest, create_createsGpsInterpreterOnlyOnce) {

    // Register metatypes
    qRegisterMetaType<types::Localisation::QtGpsLocation>();
    qRegisterMetaType<types::Localisation::QtTrip>();
    MetaTypeRegistrar& registrar = MetaTypeRegistrar::instance();

    registrar.registerReplyMetaType<types::Localisation::QtGpsLocation>();
    registrar.registerReplyMetaType<types::Localisation::QtTrip>();

    IReplyInterpreter& interpreter1 = registrar.getReplyInterpreter(Util::getTypeId<types::Localisation::QtGpsLocation>());
    IReplyInterpreter& interpreter2 = registrar.getReplyInterpreter(Util::getTypeId<types::Localisation::QtGpsLocation>());
    IReplyInterpreter& interpreter3 = registrar.getReplyInterpreter(Util::getTypeId<types::Localisation::QtTrip>());

    EXPECT_TRUE(&interpreter1 == &interpreter2);
    EXPECT_TRUE(&interpreter2 != &interpreter3);

}

