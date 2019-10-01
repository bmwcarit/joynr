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
            : _singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              _settings(),
              _messagingSettings(_settings),
              _clusterControllerSettings(_settings),
              _messageQueue(nullptr),
              _messagingStubFactory(nullptr),
              _messageRouter(),
              _mutableMessage(),
              _multicastMessagingSkeletonDirectory(
                      std::make_shared<MulticastMessagingSkeletonDirectory>()),
              _localTransport(std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>(
                      joynr::system::RoutingTypes::WebSocketProtocol::Enum::WS,
                      "host",
                      4242,
                      "path")),
              _webSocketClientAddress(
                      std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
                              "testWebSocketClientAddress")),
              _enablePersistency(true),
              _sendMsgRetryInterval(1000),
              _availableGbids{"testGbid1", "testGbid2", "testGbid3"},
              _ownAddress(std::make_shared<const system::RoutingTypes::Address>())
    {
        _singleThreadedIOService->start();

        _messagingStubFactory = std::make_shared<MockMessagingStubFactory>();
        _messageRouter = createMessageRouter();

        const TimePoint now = TimePoint::now();
        _mutableMessage.setExpiryDate(now + std::chrono::milliseconds(500));
        _mutableMessage.setType(Message::VALUE_MESSAGE_TYPE_ONE_WAY());
    }

    ~MessageRouterTest() override
    {
        _messageRouter->shutdown();
        _singleThreadedIOService->stop();
        std::remove(_settingsFileName.c_str());
    }

protected:
    template <typename U = T,
              typename = std::enable_if_t<std::is_same<U, LibJoynrMessageRouter>::value>>
    std::shared_ptr<LibJoynrMessageRouter> createMessageRouter(
            std::vector<std::shared_ptr<ITransportStatus>> transportStatuses = {})
    {
        auto messageQueueForMessageRouter = std::make_unique<MessageQueue<std::string>>();
        _messageQueue = messageQueueForMessageRouter.get();

        auto transportNotAvailableQueue =
                std::make_unique<MessageQueue<std::shared_ptr<ITransportStatus>>>();
        _transportNotAvailableQueueRef = transportNotAvailableQueue.get();

        auto libJoynrMessageRouter = std::make_shared<LibJoynrMessageRouter>(
                _messagingSettings,
                _webSocketClientAddress,
                _messagingStubFactory,
                _singleThreadedIOService->getIOService(),
                std::make_unique<WebSocketMulticastAddressCalculator>(_localTransport),
                _enablePersistency,
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
        ClusterControllerSettings ccSettings(_settings);
        ccSettings.setMulticastReceiverDirectoryPersistencyEnabled(true);

        _messagingSettings.setRoutingTableCleanupIntervalMs(5000);
        _messagingSettings.setSendMsgRetryInterval(_sendMsgRetryInterval);
        auto messageQueueForMessageRouter = std::make_unique<MessageQueue<std::string>>();
        _messageQueue = messageQueueForMessageRouter.get();

        auto transportNotAvailableQueue =
                std::make_unique<MessageQueue<std::shared_ptr<ITransportStatus>>>();
        _transportNotAvailableQueueRef = transportNotAvailableQueue.get();

        auto ccMessageRouter = std::make_shared<CcMessageRouter>(
                _messagingSettings,
                _clusterControllerSettings,
                _messagingStubFactory,
                _multicastMessagingSkeletonDirectory,
                std::unique_ptr<IPlatformSecurityManager>(),
                _singleThreadedIOService->getIOService(),
                std::make_unique<MqttMulticastAddressCalculator>(
                        ccSettings.getMqttMulticastTopicPrefix(), _availableGbids),
                globalCcAddress,
                messageNotificationProviderParticipantId,
                _enablePersistency,
                std::move(transportStatuses),
                std::move(messageQueueForMessageRouter),
                std::move(transportNotAvailableQueue),
                *_ownAddress);
        ccMessageRouter->init();
        return ccMessageRouter;
    }

    void setOwnAddress(std::shared_ptr<const system::RoutingTypes::Address> ownAddress)
    {
        this->_ownAddress = ownAddress;
    }

    std::shared_ptr<SingleThreadedIOService> _singleThreadedIOService;
    std::string _settingsFileName;
    Settings _settings;
    MessagingSettings _messagingSettings;
    ClusterControllerSettings _clusterControllerSettings;
    MessageQueue<std::string>* _messageQueue;
    MessageQueue<std::shared_ptr<ITransportStatus>>* _transportNotAvailableQueueRef;
    std::shared_ptr<MockMessagingStubFactory> _messagingStubFactory;

    std::shared_ptr<T> _messageRouter;

    MutableMessage _mutableMessage;
    std::shared_ptr<MulticastMessagingSkeletonDirectory> _multicastMessagingSkeletonDirectory;

    std::shared_ptr<const joynr::system::RoutingTypes::WebSocketAddress> _localTransport;
    const std::shared_ptr<const joynr::system::RoutingTypes::WebSocketClientAddress>
            _webSocketClientAddress;

    const bool _enablePersistency;
    const std::uint32_t _sendMsgRetryInterval;
    std::vector<std::string> _availableGbids;

    void routeMessageToAddress()
    {
        joynr::Semaphore semaphore(0);
        std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();
        auto mockMessagingStub = std::make_shared<MockMessagingStub>();
        using ::testing::Return;
        using ::testing::_;
        using ::testing::A;
        ON_CALL(*_messagingStubFactory, create(_)).WillByDefault(Return(mockMessagingStub));
        ON_CALL(*mockMessagingStub,
                transmit(immutableMessage,
                         A<const std::function<
                                 void(const joynr::exceptions::JoynrRuntimeException&)>&>()))
                .WillByDefault(ReleaseSemaphore(&semaphore));
        EXPECT_CALL(*mockMessagingStub,
                    transmit(immutableMessage,
                             A<const std::function<
                                     void(const joynr::exceptions::JoynrRuntimeException&)>&>()));
        _messageRouter->route(immutableMessage);
        EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(2)));
    }

private:
    std::shared_ptr<const system::RoutingTypes::Address> _ownAddress;
};

typedef ::testing::Types<LibJoynrMessageRouter, CcMessageRouter> MessageRouterTypes;

TYPED_TEST_CASE(MessageRouterTest, MessageRouterTypes);
