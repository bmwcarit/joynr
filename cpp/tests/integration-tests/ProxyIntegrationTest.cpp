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

#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/ConnectorFactory.h"
#include "joynr/InProcessConnectorFactory.h"
#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/vehicle/GpsProxy.h"

#include "tests/utils/MockObjects.h"

using ::testing::Return;

using namespace ::testing;
using namespace joynr;

class ProxyIntegrationTest : public ::testing::Test {
public:
    ProxyIntegrationTest() :
        mockInProcessConnectorFactory(nullptr),
        connectorFactory(nullptr)
    {
        auto mockInProcessConnectorFactoryPtr = std::make_unique<MockInProcessConnectorFactory>();
        mockInProcessConnectorFactory = mockInProcessConnectorFactoryPtr.get();
        auto mockMessageSender = std::make_shared<MockMessageSender>();
        auto joynrMessagingConnectorFactory = std::make_unique<JoynrMessagingConnectorFactory>(std::move(mockMessageSender), nullptr);
        connectorFactory = new ConnectorFactory(std::move(mockInProcessConnectorFactoryPtr), std::move(joynrMessagingConnectorFactory));
    }

    ~ProxyIntegrationTest() override{
        // manually deleting the connectorFactory (this will also trigger deletion of mockInProcessConnectorFactory)
        // normally performed in the ProxyFactory
        delete connectorFactory;
    }

protected:
    MockInProcessConnectorFactory* mockInProcessConnectorFactory;
    ConnectorFactory* connectorFactory;

private:
    DISALLOW_COPY_AND_ASSIGN(ProxyIntegrationTest);
};

TEST_F(ProxyIntegrationTest, proxyInitialisation)
{
    const std::string domain = "cppProxyIntegrationTestDomain";
    MessagingQos messagingQos;
    auto endPointAddress = std::make_shared<system::RoutingTypes::ChannelAddress>("http://endpoint:8080/bounceproxy", "endPointAddress");

    EXPECT_CALL(*mockInProcessConnectorFactory, canBeCreated(_)).WillRepeatedly(Return(false));
    auto proxy =  std::make_unique<vehicle::GpsProxy>(endPointAddress, connectorFactory, domain, messagingQos);
    ASSERT_TRUE(proxy != nullptr);
}
