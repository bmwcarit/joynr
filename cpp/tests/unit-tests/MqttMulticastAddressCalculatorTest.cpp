/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "joynr/ImmutableMessage.h"
#include "joynr/MqttMulticastAddressCalculator.h"
#include "joynr/MutableMessage.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"

using namespace joynr;
using namespace testing;

class MqttMulticastAddressCalculatorTest : public Test
{
public:
    virtual ~MqttMulticastAddressCalculatorTest() = default;

    MqttMulticastAddressCalculatorTest()
            : recipient("multicastId"),
              mutableMessage(),
              mqttMulticastTopicPrefix("prefix"),
              availableGbids{"testGbid1", "testGbid2", "testGbid3"},
              mqttMulticastAddressCalculator(
                      std::make_shared<MqttMulticastAddressCalculator>(mqttMulticastTopicPrefix,
                                                                       availableGbids))
    {
        mutableMessage.setRecipient(recipient);
        mutableMessage.setExpiryDate(TimePoint(std::chrono::system_clock::time_point::max()));
    }

private:
    const std::string recipient;

protected:
    MutableMessage mutableMessage;
    const std::string mqttMulticastTopicPrefix;
    const std::vector<std::string> availableGbids;
    std::shared_ptr<MqttMulticastAddressCalculator> mqttMulticastAddressCalculator;
};

TEST_F(MqttMulticastAddressCalculatorTest, compute)
{
    auto message = mutableMessage.getImmutableMessage();
    const ImmutableMessage& immutableMessage(*message);

    std::vector<std::shared_ptr<const system::RoutingTypes::MqttAddress>>
            expectedMqttAddressesVector;
    for (std::uint8_t i = 0; i < availableGbids.size(); i++) {
        expectedMqttAddressesVector.push_back(
                std::make_shared<const system::RoutingTypes::MqttAddress>(
                        availableGbids[i],
                        mqttMulticastTopicPrefix + immutableMessage.getRecipient()));
    }

    std::vector<std::shared_ptr<const system::RoutingTypes::Address>> actualAddressesVector =
            mqttMulticastAddressCalculator->compute(immutableMessage);

    EXPECT_EQ(expectedMqttAddressesVector.size(), actualAddressesVector.size());

    for (std::uint8_t i = 0; i < availableGbids.size(); i++) {
        std::shared_ptr<const system::RoutingTypes::Address> address = actualAddressesVector[i];
        auto actualMqttAddress =
                dynamic_cast<const system::RoutingTypes::MqttAddress*>(address.get());
        EXPECT_EQ(*expectedMqttAddressesVector[i], *actualMqttAddress);
        EXPECT_EQ(
                expectedMqttAddressesVector[i]->getBrokerUri(), actualMqttAddress->getBrokerUri());
        EXPECT_EQ(expectedMqttAddressesVector[i]->getTopic(), actualMqttAddress->getTopic());
    }
}
