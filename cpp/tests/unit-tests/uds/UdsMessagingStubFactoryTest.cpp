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

#include "tests/utils/Gtest.h"

#include "joynr/MessagingStubFactory.h"
#include "joynr/Semaphore.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/UdsAddress.h"
#include "joynr/system/RoutingTypes/UdsClientAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"

#include "libjoynr/uds/UdsMessagingStubFactory.h"

#include "tests/mock/MockIUdsSender.h"

using namespace ::testing;

namespace joynr
{

class UdsMessagingStubFactoryTest : public testing::Test
{
public:
    UdsMessagingStubFactoryTest()
            : _udsServerAddress("some/serverpath"),
              _udsClientAddress("some/clientpath"),
              _mqttAddress("brokerUri", "topic"),
              _webSocketAddress(),
              _webSocketClientAddress("id")
    {
    }

protected:
    ADD_LOGGER(UdsMessagingStubFactoryTest)
    joynr::system::RoutingTypes::UdsAddress _udsServerAddress;
    joynr::system::RoutingTypes::UdsClientAddress _udsClientAddress;
    joynr::system::RoutingTypes::MqttAddress _mqttAddress;
    joynr::system::RoutingTypes::WebSocketAddress _webSocketAddress;
    joynr::system::RoutingTypes::WebSocketClientAddress _webSocketClientAddress;
};

TEST_F(UdsMessagingStubFactoryTest, canCreateUdsAddresses)
{
    UdsMessagingStubFactory udsMessagingStubFactory;

    EXPECT_TRUE(udsMessagingStubFactory.canCreate(_udsClientAddress));
    EXPECT_TRUE(udsMessagingStubFactory.canCreate(_udsServerAddress));
}

TEST_F(UdsMessagingStubFactoryTest, canOnlyCreateUdsAddresses)
{
    UdsMessagingStubFactory udsMessagingStubFactory;

    EXPECT_FALSE(udsMessagingStubFactory.canCreate(_mqttAddress));
    EXPECT_FALSE(udsMessagingStubFactory.canCreate(_webSocketAddress));
    EXPECT_FALSE(udsMessagingStubFactory.canCreate(_webSocketClientAddress));
}

TEST_F(UdsMessagingStubFactoryTest, createReturnsNullForUnknownUdsClientOrServer)
{
    UdsMessagingStubFactory udsMessagingStubFactory;

    EXPECT_TRUE((udsMessagingStubFactory.create(_udsClientAddress)).get() == nullptr);
    EXPECT_TRUE((udsMessagingStubFactory.create(_udsServerAddress)).get() == nullptr);
}

TEST_F(UdsMessagingStubFactoryTest, createReturnsMessagingStubForKnownUdsOrUdsClientAddresses)
{
    UdsMessagingStubFactory udsMessagingStubFactory;
    auto udsClientSender = std::make_shared<MockIUdsSender>();
    auto udsServerSender = std::make_shared<MockIUdsSender>();

    udsMessagingStubFactory.addClient(_udsClientAddress, udsClientSender);
    udsMessagingStubFactory.addServer(_udsServerAddress, udsServerSender);
    EXPECT_TRUE(udsMessagingStubFactory.create(_udsClientAddress).get() != nullptr);
    EXPECT_TRUE(udsMessagingStubFactory.create(_udsServerAddress).get() != nullptr);
    EXPECT_CALL(*udsClientSender, dtorCalled());
    EXPECT_CALL(*udsServerSender, dtorCalled());
}

TEST_F(UdsMessagingStubFactoryTest, addClientOraddServerDoesNotOverwriteExistingEntries)
{
    UdsMessagingStubFactory udsMessagingStubFactory;
    auto udsClientSender1 = std::make_shared<MockIUdsSender>();
    auto udsClientSender2 = std::make_shared<MockIUdsSender>();
    auto udsServerSender1 = std::make_shared<MockIUdsSender>();
    auto udsServerSender2 = std::make_shared<MockIUdsSender>();

    udsMessagingStubFactory.addClient(_udsClientAddress, udsClientSender1);
    udsMessagingStubFactory.addServer(_udsServerAddress, udsServerSender1);

    auto clientStub1 = udsMessagingStubFactory.create(_udsClientAddress);
    auto serverStub1 = udsMessagingStubFactory.create(_udsServerAddress);

    udsMessagingStubFactory.addClient(_udsClientAddress, udsClientSender2);
    udsMessagingStubFactory.addServer(_udsServerAddress, udsServerSender2);

    auto clientStub2 = udsMessagingStubFactory.create(_udsClientAddress);
    auto serverStub2 = udsMessagingStubFactory.create(_udsServerAddress);

    EXPECT_TRUE(clientStub1.get() == clientStub2.get());
    EXPECT_TRUE(serverStub1.get() == serverStub2.get());

    EXPECT_CALL(*udsClientSender1, dtorCalled());
    EXPECT_CALL(*udsServerSender1, dtorCalled());
    EXPECT_CALL(*udsClientSender2, dtorCalled());
    EXPECT_CALL(*udsServerSender2, dtorCalled());
}

TEST_F(UdsMessagingStubFactoryTest, createReturnsNullForNonUdsAddressTypes)
{
    UdsMessagingStubFactory udsMessagingStubFactory;

    EXPECT_TRUE(udsMessagingStubFactory.create(_mqttAddress).get() == nullptr);
    EXPECT_TRUE(udsMessagingStubFactory.create(_webSocketAddress).get() == nullptr);
    EXPECT_TRUE(udsMessagingStubFactory.create(_webSocketClientAddress).get() == nullptr);
}

TEST_F(UdsMessagingStubFactoryTest, closedMessagingStubsAreRemovedFromUdsMessagingStubFactory)
{
    UdsMessagingStubFactory udsMessagingStubFactory;
    auto udsClientSender = std::make_shared<MockIUdsSender>();
    auto udsServerSender = std::make_shared<MockIUdsSender>();
    auto semaphore = std::make_shared<Semaphore>(0);

    auto clientAddressCopy =
            std::make_shared<system::RoutingTypes::UdsClientAddress>(_udsClientAddress);
    auto serverAddressCopy = std::make_shared<system::RoutingTypes::UdsAddress>(_udsServerAddress);

    // test without OnMessagingStubClosedCallback set
    udsMessagingStubFactory.addClient(_udsClientAddress, udsClientSender);
    udsMessagingStubFactory.addServer(_udsServerAddress, udsServerSender);

    EXPECT_TRUE(udsMessagingStubFactory.create(_udsClientAddress).get() != nullptr);
    EXPECT_TRUE(udsMessagingStubFactory.create(_udsServerAddress).get() != nullptr);

    udsMessagingStubFactory.onMessagingStubClosed(*clientAddressCopy);
    udsMessagingStubFactory.onMessagingStubClosed(*serverAddressCopy);

    EXPECT_TRUE((udsMessagingStubFactory.create(_udsClientAddress)).get() == nullptr);
    EXPECT_TRUE((udsMessagingStubFactory.create(_udsServerAddress)).get() == nullptr);

    // test with OnMessagingStubClosedCallback set
    udsMessagingStubFactory.addClient(_udsClientAddress, udsClientSender);
    udsMessagingStubFactory.addServer(_udsServerAddress, udsServerSender);

    EXPECT_TRUE(udsMessagingStubFactory.create(_udsClientAddress).get() != nullptr);
    EXPECT_TRUE(udsMessagingStubFactory.create(_udsServerAddress).get() != nullptr);

    udsMessagingStubFactory.registerOnMessagingStubClosedCallback(
            [semaphore](std::shared_ptr<const joynr::system::RoutingTypes::Address>
                                destinationAddress) {
                std::ignore = destinationAddress;
                semaphore->notify();
            });

    udsMessagingStubFactory.onMessagingStubClosed(*clientAddressCopy);
    EXPECT_TRUE(semaphore->waitFor(std::chrono::seconds(2)));

    udsMessagingStubFactory.onMessagingStubClosed(*serverAddressCopy);
    EXPECT_TRUE(semaphore->waitFor(std::chrono::seconds(2)));

    EXPECT_TRUE((udsMessagingStubFactory.create(_udsClientAddress)).get() == nullptr);
    EXPECT_TRUE((udsMessagingStubFactory.create(_udsServerAddress)).get() == nullptr);

    EXPECT_CALL(*udsClientSender, dtorCalled());
    EXPECT_CALL(*udsServerSender, dtorCalled());
}

TEST_F(UdsMessagingStubFactoryTest, callingOnMessagingStubClosedWithWrongAddressTypeIsIgnored)
{
    UdsMessagingStubFactory udsMessagingStubFactory;

    auto webSocketAddressCopy =
            std::make_shared<system::RoutingTypes::WebSocketAddress>(_webSocketAddress);
    auto webSocketClientAddressCopy =
            std::make_shared<system::RoutingTypes::WebSocketClientAddress>(_webSocketClientAddress);
    auto mqttAddressCopy = std::make_shared<system::RoutingTypes::MqttAddress>(_mqttAddress);

    udsMessagingStubFactory.onMessagingStubClosed(*webSocketAddressCopy);
    udsMessagingStubFactory.onMessagingStubClosed(*webSocketClientAddressCopy);
    udsMessagingStubFactory.onMessagingStubClosed(*mqttAddressCopy);
}

// TODO actually an integration test that should be moved
TEST_F(UdsMessagingStubFactoryTest, closedMessagingStubsAreRemovedFromMessagingStubFactory)
{
    auto udsMessagingStubFactory = std::make_shared<UdsMessagingStubFactory>();
    auto address = std::make_shared<system::RoutingTypes::UdsClientAddress>(_udsClientAddress);
    auto addressCopy = std::make_shared<system::RoutingTypes::UdsClientAddress>(_udsClientAddress);
    auto udsSender = std::make_shared<MockIUdsSender>();

    udsMessagingStubFactory->addClient(
            joynr::system::RoutingTypes::UdsClientAddress(*address), udsSender);

    auto messagingStubFactory = std::make_shared<MessagingStubFactory>();
    udsMessagingStubFactory->registerOnMessagingStubClosedCallback(
            [messagingStubFactory](std::shared_ptr<const joynr::system::RoutingTypes::Address>
                                           destinationAddress) {
                messagingStubFactory->remove(destinationAddress);
            });
    messagingStubFactory->registerStubFactory(udsMessagingStubFactory);

    EXPECT_NE(nullptr, messagingStubFactory->create(address).get());
    EXPECT_TRUE(messagingStubFactory->contains(address));
    EXPECT_TRUE(messagingStubFactory->contains(addressCopy));

    EXPECT_CALL(*udsSender, dtorCalled());
    udsMessagingStubFactory->onMessagingStubClosed(*addressCopy);

    EXPECT_FALSE(messagingStubFactory->contains(address));
    EXPECT_FALSE(messagingStubFactory->contains(addressCopy));
    EXPECT_TRUE(messagingStubFactory->create(address).get() == nullptr);
    messagingStubFactory->shutdown();
}

} // namespace joynr
