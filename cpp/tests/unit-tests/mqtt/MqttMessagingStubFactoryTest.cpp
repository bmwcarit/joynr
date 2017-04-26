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
#include <gtest/gtest.h>

#include "joynr/IMessaging.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/CommonApiDbusAddress.h"
#include "joynr/system/RoutingTypes/BrowserAddress.h"

#include "libjoynrclustercontroller/messaging/joynr-messaging/MqttMessagingStubFactory.h"
#include "libjoynrclustercontroller/messaging/joynr-messaging/MqttMessagingStub.h"

#include "utils/MockObjects.h"

using namespace ::testing;

namespace joynr {

class MqttMessagingStubFactoryTest : public testing::Test {
public:
    MqttMessagingStubFactoryTest() :
        mqttAddress("brokerUri", "clientId"),
        webSocketServerAddress(joynr::system::RoutingTypes::WebSocketProtocol::WS, "localhost", 42, "path"),
        webSocketClientAddress("clientId"),
        channelAddress("endPointUrl", "channelId"),
        commonApiDbusAddress("domain", "serviceName", "participantId"),
        browserAddress("windowId")
    {
    }

protected:
    ADD_LOGGER(MqttMessagingStubFactoryTest);
    joynr::system::RoutingTypes::MqttAddress mqttAddress;
    joynr::system::RoutingTypes::WebSocketAddress webSocketServerAddress;
    joynr::system::RoutingTypes::WebSocketClientAddress webSocketClientAddress;
    joynr::system::RoutingTypes::ChannelAddress channelAddress;
    joynr::system::RoutingTypes::CommonApiDbusAddress commonApiDbusAddress;
    joynr::system::RoutingTypes::BrowserAddress browserAddress;
};

INIT_LOGGER(MqttMessagingStubFactoryTest);

TEST_F(MqttMessagingStubFactoryTest, canCreateMqttAddressses) {
    auto mockMessageSender = std::make_shared<MockTransportMessageSender>();
    MqttMessagingStubFactory factory(mockMessageSender);

    EXPECT_TRUE(factory.canCreate(mqttAddress));
}

TEST_F(MqttMessagingStubFactoryTest, canOnlyCreateMqttAddressses) {
    auto mockMessageSender = std::make_shared<MockTransportMessageSender>();
    MqttMessagingStubFactory factory(mockMessageSender);

    EXPECT_FALSE(factory.canCreate(channelAddress));
    EXPECT_FALSE(factory.canCreate(commonApiDbusAddress));
    EXPECT_FALSE(factory.canCreate(browserAddress));
    EXPECT_FALSE(factory.canCreate(webSocketClientAddress));
    EXPECT_FALSE(factory.canCreate(webSocketServerAddress));
}

TEST_F(MqttMessagingStubFactoryTest, createReturnsMessagingStub) {
    auto mockMessageSender = std::make_shared<MockTransportMessageSender>();
    MqttMessagingStubFactory factory(mockMessageSender);

    EXPECT_TRUE(factory.create(mqttAddress).get() != nullptr);
}

} // namespace joynr
