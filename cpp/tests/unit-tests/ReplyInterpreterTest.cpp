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

#include <QSharedPointer>
#include "joynr/types/QtGpsLocation.h"
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
    qRegisterMetaType<types::QtGpsLocation>();
    MetaTypeRegistrar& registrar = MetaTypeRegistrar::instance();
    registrar.registerReplyMetaType<types::QtGpsLocation>();

    // Create a mock callback
    QSharedPointer<MockCallback<joynr::types::QtGpsLocation>> callback(new MockCallback<joynr::types::QtGpsLocation>());
    int myAltitude = 13;
    EXPECT_CALL(*callback, onSuccess(Property(&types::QtGpsLocation::getAltitude, myAltitude)))
                .Times(1);

    // Create a reply caller
    QSharedPointer<IReplyCaller> icaller(new ReplyCaller<types::QtGpsLocation>(
            [callback](const RequestStatus& status, const types::QtGpsLocation& location) {
                callback->onSuccess(location);
            },
            [](const RequestStatus& status){
            }));

    // Create a reply
    types::QtGpsLocation location;
    location.setAltitude(myAltitude);
    QList<QVariant> response;
    response.append(QVariant::fromValue(location));
    Reply reply;
    reply.setResponse(response);

    // Interpret the reply
    IReplyInterpreter& interpreter = registrar.getReplyInterpreter(Util::getTypeId<types::QtGpsLocation>());
    interpreter.execute(icaller, reply);
}

TEST_F(ReplyInterpreterTest, create_createsGpsInterpreterOnlyOnce) {

    // Register metatypes
    qRegisterMetaType<types::QtGpsLocation>();
    qRegisterMetaType<types::QtTrip>();
    MetaTypeRegistrar& registrar = MetaTypeRegistrar::instance();

    registrar.registerReplyMetaType<types::QtGpsLocation>();
    registrar.registerReplyMetaType<types::QtTrip>();

    IReplyInterpreter& interpreter1 = registrar.getReplyInterpreter(Util::getTypeId<types::QtGpsLocation>());
    IReplyInterpreter& interpreter2 = registrar.getReplyInterpreter(Util::getTypeId<types::QtGpsLocation>());
    IReplyInterpreter& interpreter3 = registrar.getReplyInterpreter(Util::getTypeId<types::QtTrip>());

    EXPECT_TRUE(&interpreter1 == &interpreter2);
    EXPECT_TRUE(&interpreter2 != &interpreter3);

}

