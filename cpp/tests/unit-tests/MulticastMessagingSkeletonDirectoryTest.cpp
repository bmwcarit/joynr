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
#include <gmock/gmock.h>

#include <memory>

#include "joynr/MulticastMessagingSkeletonDirectory.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "tests/mock/MockObjects.h"
#include "tests/mock/MockMessagingMulticastSubscriber.h"

class MulticastMessagingSkeletonDirectoryTest : public ::testing::Test {
public:
    MulticastMessagingSkeletonDirectoryTest() :
        mockMessagingMulticastSubscriber(std::make_shared<MockMessagingMulticastSubscriber>())
    {
    }
protected:
    std::shared_ptr<MockMessagingMulticastSubscriber> mockMessagingMulticastSubscriber;
    joynr::MulticastMessagingSkeletonDirectory multicastMessagingSkeletonDirectory;
};

TEST_F(MulticastMessagingSkeletonDirectoryTest, emptyDirectoryDoesNotContainEntry)
{
    auto mqttAddress = std::make_shared<joynr::system::RoutingTypes::MqttAddress>();
    EXPECT_FALSE(multicastMessagingSkeletonDirectory.contains(mqttAddress));

    auto httpAddress = std::make_shared<joynr::system::RoutingTypes::ChannelAddress>();
    EXPECT_FALSE(multicastMessagingSkeletonDirectory.contains(httpAddress));

    auto webSocketClientAddress =
            std::make_shared<joynr::system::RoutingTypes::WebSocketClientAddress>();
    EXPECT_FALSE(multicastMessagingSkeletonDirectory.contains(webSocketClientAddress));
}

TEST_F(MulticastMessagingSkeletonDirectoryTest, getSkeletonReturnsNullptrForNonExistingSkeleton)
{
    auto mqttAddress = std::make_shared<joynr::system::RoutingTypes::MqttAddress>();
    EXPECT_EQ(multicastMessagingSkeletonDirectory.getSkeleton(mqttAddress), nullptr);
}

TEST_F(MulticastMessagingSkeletonDirectoryTest, unregisterNonExistingEntryDoesNotThrow)
{
    EXPECT_NO_THROW(multicastMessagingSkeletonDirectory
                    .unregisterSkeleton<joynr::system::RoutingTypes::MqttAddress>());
}

TEST_F(MulticastMessagingSkeletonDirectoryTest, containsEntryAfterRegister)
{
    auto mqttAddress = std::make_shared<joynr::system::RoutingTypes::MqttAddress>();
    multicastMessagingSkeletonDirectory
            .registerSkeleton<joynr::system::RoutingTypes::MqttAddress>(
                mockMessagingMulticastSubscriber);

    EXPECT_TRUE(multicastMessagingSkeletonDirectory.contains(mqttAddress));

    auto httpAddress = std::make_shared<joynr::system::RoutingTypes::ChannelAddress>();
    multicastMessagingSkeletonDirectory
            .registerSkeleton<joynr::system::RoutingTypes::ChannelAddress>(
                mockMessagingMulticastSubscriber);

    EXPECT_TRUE(multicastMessagingSkeletonDirectory.contains(httpAddress));
}

TEST_F(MulticastMessagingSkeletonDirectoryTest, getsSkeletonAfterRegister)
{
    auto mqttAddress = std::make_shared<joynr::system::RoutingTypes::MqttAddress>();
    multicastMessagingSkeletonDirectory
            .registerSkeleton<joynr::system::RoutingTypes::MqttAddress>(
                mockMessagingMulticastSubscriber);

    EXPECT_EQ(mockMessagingMulticastSubscriber,
              multicastMessagingSkeletonDirectory.getSkeleton(mqttAddress));

    auto httpAddress = std::make_shared<joynr::system::RoutingTypes::ChannelAddress>();
    multicastMessagingSkeletonDirectory
            .registerSkeleton<joynr::system::RoutingTypes::ChannelAddress>(
                mockMessagingMulticastSubscriber);

    EXPECT_EQ(mockMessagingMulticastSubscriber,
              multicastMessagingSkeletonDirectory.getSkeleton(httpAddress));
}

TEST_F(MulticastMessagingSkeletonDirectoryTest, unregisterRemovesSkeleton)
{
    auto mqttAddress = std::make_shared<joynr::system::RoutingTypes::MqttAddress>();
    multicastMessagingSkeletonDirectory
            .registerSkeleton<joynr::system::RoutingTypes::MqttAddress>(
                mockMessagingMulticastSubscriber);

    multicastMessagingSkeletonDirectory
            .unregisterSkeleton<joynr::system::RoutingTypes::MqttAddress>();
    EXPECT_FALSE(multicastMessagingSkeletonDirectory.contains(mqttAddress));

    auto httpAddress = std::make_shared<joynr::system::RoutingTypes::ChannelAddress>();
    multicastMessagingSkeletonDirectory
            .registerSkeleton<joynr::system::RoutingTypes::ChannelAddress>(
                mockMessagingMulticastSubscriber);

    multicastMessagingSkeletonDirectory
            .unregisterSkeleton<joynr::system::RoutingTypes::ChannelAddress>();
    EXPECT_FALSE(multicastMessagingSkeletonDirectory.contains(httpAddress));
}
