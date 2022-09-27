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

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include <memory>

#include "joynr/MulticastMessagingSkeletonDirectory.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "tests/mock/MockMessagingMulticastSubscriber.h"

class MulticastMessagingSkeletonDirectoryTest : public ::testing::Test
{
public:
    MulticastMessagingSkeletonDirectoryTest()
            : mockMessagingMulticastSubscriber(std::make_shared<MockMessagingMulticastSubscriber>())
    {
    }

protected:
    std::shared_ptr<MockMessagingMulticastSubscriber> mockMessagingMulticastSubscriber;
    joynr::MulticastMessagingSkeletonDirectory multicastMessagingSkeletonDirectory;
};

TEST_F(MulticastMessagingSkeletonDirectoryTest, getSkeletonReturnsNullptrForNonExistingSkeleton)
{
    auto mqttAddress = std::make_shared<joynr::system::RoutingTypes::MqttAddress>();
    EXPECT_EQ(multicastMessagingSkeletonDirectory.getSkeleton(mqttAddress), nullptr);
}

TEST_F(MulticastMessagingSkeletonDirectoryTest, unregisterNonExistingEntryDoesNotThrow)
{
    EXPECT_NO_THROW(multicastMessagingSkeletonDirectory
                            .unregisterSkeletons<joynr::system::RoutingTypes::MqttAddress>());
}

TEST_F(MulticastMessagingSkeletonDirectoryTest, getsSkeletonAfterRegister)
{
    auto mqttAddress = std::make_shared<joynr::system::RoutingTypes::MqttAddress>();
    multicastMessagingSkeletonDirectory.registerSkeleton<joynr::system::RoutingTypes::MqttAddress>(
            mockMessagingMulticastSubscriber);

    EXPECT_EQ(mockMessagingMulticastSubscriber,
              multicastMessagingSkeletonDirectory.getSkeleton(mqttAddress));
}

TEST_F(MulticastMessagingSkeletonDirectoryTest, unregisterRemovesSkeleton)
{
    auto mqttAddress = std::make_shared<joynr::system::RoutingTypes::MqttAddress>();
    multicastMessagingSkeletonDirectory.registerSkeleton<joynr::system::RoutingTypes::MqttAddress>(
            mockMessagingMulticastSubscriber);

    EXPECT_EQ(mockMessagingMulticastSubscriber,
              multicastMessagingSkeletonDirectory.getSkeleton(mqttAddress));

    multicastMessagingSkeletonDirectory
            .unregisterSkeletons<joynr::system::RoutingTypes::MqttAddress>();
    EXPECT_EQ(nullptr, multicastMessagingSkeletonDirectory.getSkeleton(mqttAddress));
}
