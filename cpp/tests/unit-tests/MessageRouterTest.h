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

#include <cstdint>
#include <chrono>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "tests/JoynrTest.h"

#include "joynr/CcMessageRouter.h"
#include "joynr/ClusterControllerSettings.h"
#include "joynr/LibJoynrMessageRouter.h"

#include "joynr/Semaphore.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/MessageQueue.h"
#include "joynr/MulticastMessagingSkeletonDirectory.h"
#include "joynr/MqttMulticastAddressCalculator.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/WebSocketMulticastAddressCalculator.h"
#include "joynr/Message.h"
#include "joynr/MutableMessage.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/Settings.h"

#include "tests/mock/MockMessagingStub.h"
#include "tests/mock/MockMessagingStubFactory.h"

using namespace joynr;

template <typename T>
class MessageRouterTest : public ::testing::Test
{
public:
    MessageRouterTest()
            : singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              settings(),
              messagingSettings(settings),
              clusterControllerSettings(settings),
              messageQueue(nullptr),
              messagingStubFactory(nullptr),
              messageRouter(),
              mutableMessage(),
              multicastMessagingSkeletonDirectory(
                      std::make_shared<MulticastMessagingSkeletonDirectory>()),
              brokerURL("mqtt://globalTransport.example.com"),
              mqttTopic(""),
              localTransport(std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>(
                      joynr::system::RoutingTypes::WebSocketProtocol::Enum::WS,
                      "host",
                      4242,
                      "path")),
              webSocketClientAddress(
                      std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
                              "testWebSocketClientAddress")),
              globalTransport(
                      std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(brokerURL,
                                                                                       mqttTopic)),
              enablePersistency(true),
              sendMsgRetryInterval(1000),
              ownAddress(std::make_shared<const system::RoutingTypes::Address>())
    {
        singleThreadedIOService->start();

        messagingStubFactory = std::make_shared<MockMessagingStubFactory>();
        messageRouter = createMessageRouter();

        const TimePoint now = TimePoint::now();
        mutableMessage.setExpiryDate(now + std::chrono::milliseconds(500));
        mutableMessage.setType(Message::VALUE_MESSAGE_TYPE_ONE_WAY());
    }

    ~MessageRouterTest() override
    {
        messageRouter->shutdown();
        singleThreadedIOService->stop();
        std::remove(settingsFileName.c_str());
    }

protected:
    template <typename U = T,
              typename = std::enable_if_t<std::is_same<U, LibJoynrMessageRouter>::value>>
    std::shared_ptr<LibJoynrMessageRouter> createMessageRouter(
            std::vector<std::shared_ptr<ITransportStatus>> transportStatuses = {})
    {
        auto messageQueueForMessageRouter = std::make_unique<MessageQueue<std::string>>();
        messageQueue = messageQueueForMessageRouter.get();

        auto transportNotAvailableQueue =
                std::make_unique<MessageQueue<std::shared_ptr<ITransportStatus>>>();
        transportNotAvailableQueueRef = transportNotAvailableQueue.get();

        auto libJoynrMessageRouter = std::make_shared<LibJoynrMessageRouter>(
                messagingSettings,
                webSocketClientAddress,
                messagingStubFactory,
                singleThreadedIOService->getIOService(),
                std::make_unique<WebSocketMulticastAddressCalculator>(localTransport),
                enablePersistency,
                std::move(transportStatuses),
                std::move(messageQueueForMessageRouter),
                std::move(transportNotAvailableQueue));
        libJoynrMessageRouter->init();

        return libJoynrMessageRouter;
    }

    template <typename U = T, typename = std::enable_if_t<std::is_same<U, CcMessageRouter>::value>>
    std::shared_ptr<CcMessageRouter> createMessageRouter(
            std::vector<std::shared_ptr<ITransportStatus>> transportStatuses = {})
    {
        const std::string globalCcAddress("globalAddress");
        const std::string messageNotificationProviderParticipantId(
                "messageNotificationProviderParticipantId");
        ClusterControllerSettings ccSettings(settings);
        ccSettings.setMulticastReceiverDirectoryPersistencyEnabled(true);

        messagingSettings.setRoutingTableCleanupIntervalMs(5000);
        messagingSettings.setSendMsgRetryInterval(sendMsgRetryInterval);
        auto messageQueueForMessageRouter = std::make_unique<MessageQueue<std::string>>();
        messageQueue = messageQueueForMessageRouter.get();

        auto transportNotAvailableQueue =
                std::make_unique<MessageQueue<std::shared_ptr<ITransportStatus>>>();
        transportNotAvailableQueueRef = transportNotAvailableQueue.get();

        auto ccMessageRouter = std::make_shared<CcMessageRouter>(
                messagingSettings,
                clusterControllerSettings,
                messagingStubFactory,
                multicastMessagingSkeletonDirectory,
                std::unique_ptr<IPlatformSecurityManager>(),
                singleThreadedIOService->getIOService(),
                std::make_unique<MqttMulticastAddressCalculator>(
                        globalTransport, ccSettings.getMqttMulticastTopicPrefix()),
                globalCcAddress,
                messageNotificationProviderParticipantId,
                enablePersistency,
                std::move(transportStatuses),
                std::move(messageQueueForMessageRouter),
                std::move(transportNotAvailableQueue),
                *ownAddress);
        ccMessageRouter->init();
        return ccMessageRouter;
    }

    void setOwnAddress(std::shared_ptr<const system::RoutingTypes::Address> ownAddress)
    {
        this->ownAddress = ownAddress;
    }

    std::shared_ptr<SingleThreadedIOService> singleThreadedIOService;
    std::string settingsFileName;
    Settings settings;
    MessagingSettings messagingSettings;
    ClusterControllerSettings clusterControllerSettings;
    MessageQueue<std::string>* messageQueue;
    MessageQueue<std::shared_ptr<ITransportStatus>>* transportNotAvailableQueueRef;
    std::shared_ptr<MockMessagingStubFactory> messagingStubFactory;

    std::shared_ptr<T> messageRouter;

    MutableMessage mutableMessage;
    std::shared_ptr<MulticastMessagingSkeletonDirectory> multicastMessagingSkeletonDirectory;
    std::string brokerURL;
    std::string mqttTopic;

    std::shared_ptr<const joynr::system::RoutingTypes::WebSocketAddress> localTransport;
    const std::shared_ptr<const joynr::system::RoutingTypes::WebSocketClientAddress>
            webSocketClientAddress;
    std::shared_ptr<const joynr::system::RoutingTypes::MqttAddress> globalTransport;

    const bool enablePersistency;
    const std::uint32_t sendMsgRetryInterval;

    void routeMessageToAddress()
    {
        joynr::Semaphore semaphore(0);
        std::shared_ptr<ImmutableMessage> immutableMessage = mutableMessage.getImmutableMessage();
        auto mockMessagingStub = std::make_shared<MockMessagingStub>();
        using ::testing::Return;
        using ::testing::_;
        using ::testing::A;
        ON_CALL(*messagingStubFactory, create(_)).WillByDefault(Return(mockMessagingStub));
        ON_CALL(*mockMessagingStub,
                transmit(immutableMessage,
                         A<const std::function<
                                 void(const joynr::exceptions::JoynrRuntimeException&)>&>()))
                .WillByDefault(ReleaseSemaphore(&semaphore));
        EXPECT_CALL(*mockMessagingStub,
                    transmit(immutableMessage,
                             A<const std::function<
                                     void(const joynr::exceptions::JoynrRuntimeException&)>&>()));
        messageRouter->route(immutableMessage);
        EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(2)));
    }

private:
    std::shared_ptr<const system::RoutingTypes::Address> ownAddress;
};

typedef ::testing::Types<LibJoynrMessageRouter, CcMessageRouter> MessageRouterTypes;

TYPED_TEST_CASE(MessageRouterTest, MessageRouterTypes);
