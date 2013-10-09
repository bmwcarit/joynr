/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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
#include "joynr/PrivateCopyAssign.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "joynr/MessageRouter.h"
#include "tests/utils/MockObjects.h"


using namespace joynr;

class MessageRouterTest : public ::testing::Test {
public:
    MessageRouterTest() :
        messagingStubFactory(new MockMessagingStubFactory()),
        messagingEndpointDirectory(new MessagingEndpointDirectory(QString("MessagingEndpointDirectory"))),
        messageRouter(new MessageRouter(messagingEndpointDirectory)),
        joynrMessage(),
        qos()
    {}
    void SetUp(){
        joynrMessage.setType(JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY);
    }
    void TearDown(){
    }
protected:
    MockMessagingStubFactory* messagingStubFactory;
    MessagingEndpointDirectory* messagingEndpointDirectory;
    MessageRouter* messageRouter;
    JoynrMessage joynrMessage;
    MessagingQos qos;
private:
    DISALLOW_COPY_AND_ASSIGN(MessageRouterTest);
};

TEST_F(MessageRouterTest, DISABLED_routeDelegatesToStubFactory){
    // cH: this thest doesn't make sense anymore, since the MessageRouter
    // will create the MessagingStubFactory internally and therefor couldn't
    // be mocked. However, this test was already disabled.
    EXPECT_CALL(*messagingStubFactory, create(_,_)).Times(1);

    messageRouter->route(joynrMessage, qos);

}
