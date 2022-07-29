/*
 * #%L
 * %%
 * Copyright (C) 2022 BMW Car IT GmbH
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

#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/tests/ItestConnector.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"

#include "tests/mock/MockMessageSender.h"
#include "tests/mock/MockSubscriptionManager.h"

#include "joynr/tests/testJoynrMessagingConnector.h"

using namespace joynr;

/**
 * These tests test the Messaging Connector Factory
 */

/**
 * @brief Fixture.
 */
class MessagingConnectorFactoryTest : public ::testing::Test
{
public:
    MessagingConnectorFactoryTest()
            : joynrMessagingConnectorFactory(),
              mockMessageSender(),
              singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              mockSubscriptionManager(std::make_shared<MockSubscriptionManager>(
                      singleThreadedIOService->getIOService(),
                      nullptr))
    {
    }

    void SetUp() override
    {
        joynrMessagingConnectorFactory = std::make_unique<JoynrMessagingConnectorFactory>(
                mockMessageSender, mockSubscriptionManager);
    }

    void TearDown() override
    {
        joynrMessagingConnectorFactory.reset();
    }

protected:
    std::unique_ptr<JoynrMessagingConnectorFactory> joynrMessagingConnectorFactory;
    std::shared_ptr<MockMessageSender> mockMessageSender;
    std::shared_ptr<SingleThreadedIOService> singleThreadedIOService;
    std::shared_ptr<MockSubscriptionManager> mockSubscriptionManager;
    const std::string _domain{"myDomain"};
    const std::string _proxyParticipantId{"1234"};
};

TEST_F(MessagingConnectorFactoryTest, getSharedPtr)
{
    types::DiscoveryEntryWithMetaInfo discoveryEntry;
    std::shared_ptr<joynr::tests::ItestConnector> connector =
            joynrMessagingConnectorFactory->create<joynr::tests::ItestConnector>(
                    _domain, _proxyParticipantId, MessagingQos(), discoveryEntry);
    EXPECT_NE(connector.get(), nullptr);
}
