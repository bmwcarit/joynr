/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
#include "joynr/MutableMessage.h"
#include "joynr/UdsMulticastAddressCalculator.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/UdsAddress.h"

using namespace joynr;
using namespace testing;

class UdsMulticastAddressCalculatorTest : public Test
{
public:
    virtual ~UdsMulticastAddressCalculatorTest() = default;

    UdsMulticastAddressCalculatorTest()
            : recipient("multicastId"),
              mutableMessage(),
              udsMulticastAddressCalculator(std::make_shared<UdsMulticastAddressCalculator>(
                      std::make_shared<system::RoutingTypes::UdsAddress>("path")))
    {
        mutableMessage.setRecipient(recipient);
        mutableMessage.setExpiryDate(TimePoint(std::chrono::system_clock::time_point::max()));
    }

private:
    const std::string recipient;

protected:
    MutableMessage mutableMessage;
    std::shared_ptr<UdsMulticastAddressCalculator> udsMulticastAddressCalculator;
};

TEST_F(UdsMulticastAddressCalculatorTest, compute)
{
    auto message = mutableMessage.getImmutableMessage();
    const ImmutableMessage& immutableMessage(*message);

    std::vector<std::shared_ptr<const system::RoutingTypes::UdsAddress>> expectedUdsAddressesVector;
    expectedUdsAddressesVector.push_back(
            std::make_shared<const system::RoutingTypes::UdsAddress>("path"));

    std::vector<std::shared_ptr<const system::RoutingTypes::Address>> actualAddressesVector =
            udsMulticastAddressCalculator->compute(immutableMessage);

    EXPECT_EQ(expectedUdsAddressesVector.size(), actualAddressesVector.size());
    std::shared_ptr<const system::RoutingTypes::Address> address = actualAddressesVector[0];
    auto actualUdsAddress = dynamic_cast<const system::RoutingTypes::UdsAddress*>(address.get());
    EXPECT_EQ(*expectedUdsAddressesVector[0], *actualUdsAddress);
    EXPECT_EQ(expectedUdsAddressesVector[0]->getPath(), actualUdsAddress->getPath());
}
