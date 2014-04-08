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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "tests/utils/MockObjects.h"
#include "joynr/ProxyBuilder.h"
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "joynr/HttpCommunicationManager.h"
#include "joynr/vehicle/GpsProxy.h"
#include "joynr/vehicle/GpsProxy.h"
#include "joynr/InProcessConnectorFactory.h"
#include "joynr/ConnectorFactory.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/system/ChannelAddress.h"
#include "joynr/Request.h"
#include "joynr/Reply.h"

using ::testing::A;
using ::testing::_;
using ::testing::Return;
using ::testing::Eq;
using ::testing::NotNull;
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
        mockJoynrMessageSender(new MockJoynrMessageSender()),
        domain("cppProxyIntegrationTestDomain"),
        proxyQos(),
        messagingQos(),
        endPointAddress(new system::ChannelAddress("endPointAddress"))
    {
        //moved to initializationlist
        //endPointAddress = QSharedPointer<system::ChannelAddress>(new system::ChannelAddress("endPointAddress"));
    }

    // Sets up the test fixture.
    void SetUp(){
    }

    // Tears down the test fixture.
    void TearDown(){

    }

    ~ProxyIntegrationTest(){
        delete mockInProcessConnectorFactory;
        delete mockClientCache;
        delete mockJoynrMessageSender;
    }

protected:

    MockInProcessConnectorFactory* mockInProcessConnectorFactory;
    MockClientCache* mockClientCache;
    MockJoynrMessageSender* mockJoynrMessageSender;
    QString domain;
    ProxyQos proxyQos;
    MessagingQos messagingQos;
    QSharedPointer<system::ChannelAddress> endPointAddress;


private:
    DISALLOW_COPY_AND_ASSIGN(ProxyIntegrationTest);
};

typedef ProxyIntegrationTest GpsProxyDeathTest;


TEST_F(ProxyIntegrationTest, proxyInitialisation)
{
    JoynrMessagingConnectorFactory* joynrMessagingConnectorFactory = new JoynrMessagingConnectorFactory(mockJoynrMessageSender, NULL);
    ConnectorFactory* connectorFactory = new ConnectorFactory(mockInProcessConnectorFactory, joynrMessagingConnectorFactory);
    EXPECT_CALL(*mockInProcessConnectorFactory, canBeCreated(_)).WillRepeatedly(Return(false));
    vehicle::GpsProxy* proxy =  new vehicle::GpsProxy((ICapabilities*)NULL, endPointAddress, connectorFactory, mockClientCache, domain, proxyQos, messagingQos, false);
    ASSERT_TRUE(proxy != NULL);
}
