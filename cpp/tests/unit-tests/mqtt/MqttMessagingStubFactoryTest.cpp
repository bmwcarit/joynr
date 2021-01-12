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

#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/system/RoutingTypes/BrowserAddress.h"

#include "libjoynrclustercontroller/messaging/joynr-messaging/MqttMessagingStubFactory.h"
#include "libjoynrclustercontroller/messaging/joynr-messaging/MqttMessagingStub.h"

#include "tests/mock/MockTransportMessageSender.h"

using namespace ::testing;

namespace joynr
{

class MqttMessagingStubFactoryTest : public testing::Test
{
public:
    MqttMessagingStubFactoryTest()
            : testGbid("testGbid"),
              mqttAddress(testGbid, "clientId"),
              webSocketServerAddress(joynr::system::RoutingTypes::WebSocketProtocol::WS,
                                     "localhost",
                                     42,
                                     "path"),
              webSocketClientAddress("clientId")
    {
    }

protected:
    ADD_LOGGER(MqttMessagingStubFactoryTest)
    std::string testGbid;
    joynr::system::RoutingTypes::MqttAddress mqttAddress;
    joynr::system::RoutingTypes::WebSocketAddress webSocketServerAddress;
    joynr::system::RoutingTypes::WebSocketClientAddress webSocketClientAddress;
};

TEST_F(MqttMessagingStubFactoryTest, canCreateMqttAddressses)
{
    auto mockMessageSender = std::make_shared<MockTransportMessageSender>();
    MqttMessagingStubFactory factory(mockMessageSender, testGbid);

    EXPECT_TRUE(factory.canCreate(mqttAddress));
}

TEST_F(MqttMessagingStubFactoryTest, canCreateMqttAddresssesWrongGbid)
{
    auto mockMessageSender = std::make_shared<MockTransportMessageSender>();
    MqttMessagingStubFactory factory(mockMessageSender, testGbid);

    joynr::system::RoutingTypes::MqttAddress mqttAddressWithWrongGbid("wrongGbid", "clientId");
    EXPECT_FALSE(factory.canCreate(mqttAddressWithWrongGbid));
}

TEST_F(MqttMessagingStubFactoryTest, canOnlyCreateMqttAddressses)
{
    auto mockMessageSender = std::make_shared<MockTransportMessageSender>();
    MqttMessagingStubFactory factory(mockMessageSender, testGbid);

    EXPECT_FALSE(factory.canCreate(webSocketClientAddress));
    EXPECT_FALSE(factory.canCreate(webSocketServerAddress));
}

TEST_F(MqttMessagingStubFactoryTest, createReturnsMessagingStub)
{
    auto mockMessageSender = std::make_shared<MockTransportMessageSender>();
    MqttMessagingStubFactory factory(mockMessageSender, testGbid);

    EXPECT_TRUE(factory.create(mqttAddress).get() != nullptr);
}

TEST_F(MqttMessagingStubFactoryTest, createReturnsNullStubForWrongGbid)
{
    auto mockMessageSender = std::make_shared<MockTransportMessageSender>();
    MqttMessagingStubFactory factory(mockMessageSender, testGbid);

    joynr::system::RoutingTypes::MqttAddress mqttAddressWithWrongGbid("wrongGbid", "clientId");

    EXPECT_TRUE(factory.create(mqttAddressWithWrongGbid).get() == nullptr);
}

TEST_F(MqttMessagingStubFactoryTest, createReturnsNullStubForWrongAddressType)
{
    auto mockMessageSender = std::make_shared<MockTransportMessageSender>();
    MqttMessagingStubFactory factory(mockMessageSender, testGbid);

    EXPECT_TRUE(factory.create(webSocketClientAddress).get() == nullptr);
    EXPECT_TRUE(factory.create(webSocketServerAddress).get() == nullptr);
}

} // namespace joynr
