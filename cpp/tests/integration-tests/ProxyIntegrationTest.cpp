/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/SubscriptionStop.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/Request.h"
#include "joynr/vehicle/GpsProxy.h"
#include "joynr/InProcessConnectorFactory.h"
#include "joynr/ConnectorFactory.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"

#include "tests/utils/MockObjects.h"
#include "joynr/PrivateCopyAssign.h"

using ::testing::A;
using ::testing::_;
using ::testing::Return;
using ::testing::Eq;
using ::testing::AllOf;
using ::testing::Property;
using ::testing::Invoke;
using ::testing::Unused;

using namespace ::testing;

using namespace joynr;



class ProxyIntegrationTest : public ::testing::Test {
public:

    ProxyIntegrationTest() :
        mockInProcessConnectorFactory(new MockInProcessConnectorFactory()),
        mockClientCache(new MockClientCache()),
        mockJoynrMessageSender(std::make_shared<MockJoynrMessageSender>()),
        domain("cppProxyIntegrationTestDomain"),
        messagingQos(),
        endPointAddress(new system::RoutingTypes::ChannelAddress("http://endpoint:8080/bounceproxy", "endPointAddress"))
    {
    }

    ~ProxyIntegrationTest(){
        delete mockInProcessConnectorFactory;
        delete mockClientCache;
    }

protected:

    MockInProcessConnectorFactory* mockInProcessConnectorFactory;
    MockClientCache* mockClientCache;
    std::shared_ptr<MockJoynrMessageSender> mockJoynrMessageSender;
    std::string domain;
    MessagingQos messagingQos;
    std::shared_ptr<joynr::system::RoutingTypes::ChannelAddress> endPointAddress;


private:
    DISALLOW_COPY_AND_ASSIGN(ProxyIntegrationTest);
};

typedef ProxyIntegrationTest GpsProxyDeathTest;


TEST_F(ProxyIntegrationTest, proxyInitialisation)
{
    JoynrMessagingConnectorFactory* joynrMessagingConnectorFactory = new JoynrMessagingConnectorFactory(mockJoynrMessageSender, nullptr);
    ConnectorFactory* connectorFactory = new ConnectorFactory(mockInProcessConnectorFactory, joynrMessagingConnectorFactory);
    EXPECT_CALL(*mockInProcessConnectorFactory, canBeCreated(_)).WillRepeatedly(Return(false));
    vehicle::GpsProxy* proxy =  new vehicle::GpsProxy(endPointAddress, connectorFactory, mockClientCache, domain, messagingQos, false);
    ASSERT_TRUE(proxy != nullptr);
}
