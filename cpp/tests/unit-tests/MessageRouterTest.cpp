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
#include "joynr/PrivateCopyAssign.h"
#include "gtest/gtest.h"
#include <QFile>
#include "gmock/gmock.h"
#include "joynr/MessageRouter.h"
#include "tests/utils/MockObjects.h"
#include "joynr/system/ChannelAddress.h"
#include "joynr/MessagingStubFactory.h"


using namespace joynr;

class MessageRouterTest : public ::testing::Test {
public:
    MessageRouterTest() :
        settingsFileName("MessageRouterTest.settings"),
        settings(settingsFileName, QSettings::IniFormat),
        messagingSettings(settings),
        messagingStubFactory(new MockMessagingStubFactory()),
        messageRouter(new MessageRouter(new MessagingStubFactory())),
        joynrMessage(),
        qos()
    {
        // provision global capabilities directory
        QSharedPointer<joynr::system::Address> addressCapabilitiesDirectory(
            new system::ChannelAddress(messagingSettings.getCapabilitiesDirectoryChannelId())
        );
        messageRouter->addProvisionedNextHop(messagingSettings.getCapabilitiesDirectoryParticipantId(), addressCapabilitiesDirectory);
        // provision channel url directory
        QSharedPointer<joynr::system::Address> addressChannelUrlDirectory(
            new system::ChannelAddress(messagingSettings.getChannelUrlDirectoryChannelId())
        );
        messageRouter->addProvisionedNextHop(messagingSettings.getChannelUrlDirectoryParticipantId(), addressChannelUrlDirectory);
    }

    ~MessageRouterTest() {
        QFile::remove(settingsFileName);
    }

    void SetUp(){
        joynrMessage.setType(JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY);
    }
    void TearDown(){
    }
protected:
    QString settingsFileName;
    QSettings settings;
    MessagingSettings messagingSettings;
    MockMessagingStubFactory* messagingStubFactory;
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
