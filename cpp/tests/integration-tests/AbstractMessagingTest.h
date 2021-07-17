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
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>
#include <vector>

#include "joynr/ClusterControllerSettings.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Dispatcher.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/MutableMessage.h"
#include "joynr/MessageQueue.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/MessageSender.h"
#include "joynr/CcMessageRouter.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/MulticastMessagingSkeletonDirectory.h"
#include "joynr/MqttMulticastAddressCalculator.h"
#include "joynr/Request.h"
#include "joynr/Semaphore.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/IPlatformSecurityManager.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockMessageSender.h"
#include "tests/mock/MockInProcessMessagingSkeleton.h"
#include "tests/mock/MockTransportMessageSender.h"
#include "tests/mock/MockTransportMessageReceiver.h"
#include "tests/mock/MockDispatcher.h"

using namespace ::testing;
using namespace joynr;

class AbstractMessagingTest : public ::testing::Test
{
public:
    std::string _settingsFileName;
    Settings _settings;
    MessagingSettings _messagingSettings;
    ClusterControllerSettings _clusterControllerSettings;
    std::string _senderId;
    std::string _globalClusterControllerAddress;
    std::string _receiverId;
    Request _request;
    std::string _requestId;
    MessagingQos _qos;
    std::shared_ptr<MockDispatcher> _dispatcher;
    std::shared_ptr<MockInProcessMessagingSkeleton> _inProcessMessagingSkeleton;
    Semaphore _semaphore;
    const bool _isLocalMessage;

    MutableMessageFactory _messageFactory;
    std::shared_ptr<MockTransportMessageReceiver> _mockMessageReceiver;
    std::shared_ptr<MockTransportMessageSender> _mockMessageSender;
    std::shared_ptr<MessagingStubFactory> _messagingStubFactory;
    std::shared_ptr<SingleThreadedIOService> _singleThreadedIOService;
    std::shared_ptr<CcMessageRouter> _messageRouter;
    AbstractMessagingTest()
            : _settingsFileName("MessagingTest.settings"),
              _settings(_settingsFileName),
              _messagingSettings(_settings),
              _clusterControllerSettings(_settings),
              _senderId("senderParticipantId"),
              _globalClusterControllerAddress("senderChannelId"),
              _receiverId("receiverParticipantId"),
              _request(),
              _requestId("requestId"),
              _qos(),
              _dispatcher(),
              _inProcessMessagingSkeleton(
                      std::make_shared<MockInProcessMessagingSkeleton>(_dispatcher)),
              _semaphore(0),
              _isLocalMessage(false),
              _messageFactory(),
              _mockMessageReceiver(new MockTransportMessageReceiver()),
              _mockMessageSender(new MockTransportMessageSender()),
              _messagingStubFactory(std::make_shared<MessagingStubFactory>()),
              _singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              _messageRouter(),
              _ownAddress(),
              _knownGbids(std::vector<std::string>{})
    {
        const std::string globalCCAddress("globalAddress");
        const std::string messageNotificationProviderParticipantId(
                "messageNotificationProviderParticipantId");

        _messagingStubFactory->registerStubFactory(
                std::make_unique<InProcessMessagingStubFactory>());
        _messageRouter = std::make_shared<CcMessageRouter>(
                _messagingSettings,
                _clusterControllerSettings,
                _messagingStubFactory,
                std::make_shared<MulticastMessagingSkeletonDirectory>(),
                nullptr,
                _singleThreadedIOService->getIOService(),
                nullptr,
                globalCCAddress,
                messageNotificationProviderParticipantId,
                std::vector<std::shared_ptr<ITransportStatus>>{},
                std::make_unique<MessageQueue<std::string>>(),
                std::make_unique<MessageQueue<std::shared_ptr<ITransportStatus>>>(),
                _ownAddress,
                _knownGbids);
        _messageRouter->init();
        _qos.setTtl(10000);
    }

    void WaitXTimes(std::uint64_t x)
    {
        for (std::uint64_t i = 0; i < x; ++i) {
            ASSERT_TRUE(_semaphore.waitFor(std::chrono::seconds(1)));
        }
    }

    ~AbstractMessagingTest()
    {
        _messageRouter->shutdown();
        std::remove(_settingsFileName.c_str());
    }

    void sendMsgFromMessageSenderViaInProcessMessagingAndMessageRouterToCommunicationManager(
            std::shared_ptr<system::RoutingTypes::Address> joynrMessagingEndpointAddr)
    {
        // Test Outline: send message from JoynrMessageSender to ICommunicationManager
        // - JoynrMessageSender.sendRequest (IJoynrMessageSender)
        //   -> adds reply caller to dispatcher (IDispatcher.addReplyCaller)
        // - InProcessMessagingStub.transmit (IMessaging)
        // - InProcessClusterControllerMessagingSkeleton.transmit (IMessaging)
        // - MessageRouter.route
        // - MessageRunnable.run
        // - *MessagingStub.transmit (IMessaging)
        // - MessageSender.send

        auto mockDispatcher = std::make_shared<MockDispatcher>();
        // InProcessMessagingSkeleton should receive the message
        EXPECT_CALL(*_inProcessMessagingSkeleton, transmit(_, _)).Times(0);

        // MessageSender should receive the message
        EXPECT_CALL(*_mockMessageSender, sendMessage(_, _, _)).Times(1).WillRepeatedly(
                ReleaseSemaphore(&_semaphore));

        EXPECT_CALL(*mockDispatcher, addReplyCaller(_, _, _)).Times(1).WillRepeatedly(
                ReleaseSemaphore(&_semaphore));

        MessageSender messageSender(_messageRouter, nullptr);
        std::shared_ptr<IReplyCaller> replyCaller;
        messageSender.registerDispatcher(mockDispatcher);

        // local messages
        const bool isGloballyVisible = false;
        constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
        const bool isSticky = false;
        _messageRouter->addNextHop(
                _receiverId, joynrMessagingEndpointAddr, isGloballyVisible, expiryDateMs, isSticky);

        messageSender.sendRequest(_senderId, _receiverId, _qos, _request, replyCaller, _isLocalMessage);

        WaitXTimes(2);
    }

    void routeMsgWithInvalidParticipantId()
    {
        std::string invalidReceiverId("invalidReceiverId");
        MutableMessage mutableMessage = _messageFactory.createRequest(
                _senderId, invalidReceiverId, _qos, _request, _isLocalMessage);

        _messageRouter->route(mutableMessage.getImmutableMessage());
        SUCCEED();
    }

    void routeMsgToInProcessMessagingSkeleton()
    {
        MutableMessage mutableMessage =
                _messageFactory.createRequest(_senderId, _receiverId, _qos, _request, _isLocalMessage);

        // We must set the reply address here. Otherwise the message router will
        // set it and the message which was created will differ from the message
        // which is passed to the messaging-skeleton.
        mutableMessage.setReplyTo(_globalClusterControllerAddress);

        std::shared_ptr<ImmutableMessage> immutableMessage = mutableMessage.getImmutableMessage();
        // InProcessMessagingSkeleton should receive the message
        EXPECT_CALL(*_inProcessMessagingSkeleton, transmit(Eq(immutableMessage), _))
                .Times(1)
                .WillRepeatedly(ReleaseSemaphore(&_semaphore));

        // MessageSender should not receive the message
        EXPECT_CALL(*_mockMessageSender, sendMessage(_, _, _)).Times(0);

        EXPECT_CALL(*_mockMessageReceiver, getSerializedGlobalClusterControllerAddress()).Times(0);

        auto messagingSkeletonEndpointAddr =
                std::make_shared<InProcessMessagingAddress>(_inProcessMessagingSkeleton);
        const bool isGloballyVisible = false;
        constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
        const bool isSticky = false;

        _messageRouter->addNextHop(_receiverId,
                                  messagingSkeletonEndpointAddr,
                                  isGloballyVisible,
                                  expiryDateMs,
                                  isSticky);

        _messageRouter->route(immutableMessage);

        WaitXTimes(1);
    }

    void routeMsgToCommunicationManager(
            std::shared_ptr<system::RoutingTypes::Address> joynrMessagingEndpointAddr)
    {
        MutableMessage mutableMessage =
                _messageFactory.createRequest(_senderId, _receiverId, _qos, _request, _isLocalMessage);
        mutableMessage.setReplyTo(_globalClusterControllerAddress);
        std::shared_ptr<ImmutableMessage> immutableMessage = mutableMessage.getImmutableMessage();
        // InProcessMessagingSkeleton should not receive the message
        EXPECT_CALL(*_inProcessMessagingSkeleton, transmit(Eq(immutableMessage), _)).Times(0);

        // *CommunicationManager should receive the message
        EXPECT_CALL(*_mockMessageSender, sendMessage(_, Eq(immutableMessage), _))
                .Times(1)
                .WillRepeatedly(ReleaseSemaphore(&_semaphore));

        const bool isGloballyVisible = false;
        constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
        const bool isSticky = false;
        _messageRouter->addNextHop(
                _receiverId, joynrMessagingEndpointAddr, isGloballyVisible, expiryDateMs, isSticky);

        _messageRouter->route(immutableMessage);

        WaitXTimes(1);
    }

    void routeMultipleMessages(
            std::shared_ptr<system::RoutingTypes::Address> joynrMessagingEndpointAddr)
    {
        MutableMessage mutableMessage1 =
                _messageFactory.createRequest(_senderId, _receiverId, _qos, _request, _isLocalMessage);
        mutableMessage1.setReplyTo(_globalClusterControllerAddress);

        std::string receiverId2("receiverId2");
        MutableMessage mutableMessage2 =
                _messageFactory.createRequest(_senderId, receiverId2, _qos, _request, _isLocalMessage);
        mutableMessage2.setReplyTo(_globalClusterControllerAddress);

        std::shared_ptr<ImmutableMessage> immutableMessage1 = mutableMessage1.getImmutableMessage();
        std::shared_ptr<ImmutableMessage> immutableMessage2 = mutableMessage2.getImmutableMessage();

        // MessageSender should receive message
        EXPECT_CALL(*_mockMessageSender, sendMessage(_, Eq(immutableMessage1), _))
                .Times(1)
                .WillRepeatedly(ReleaseSemaphore(&_semaphore));

        // InProcessMessagingSkeleton should receive twice message2
        EXPECT_CALL(*_inProcessMessagingSkeleton, transmit(Eq(immutableMessage2), _))
                .Times(2)
                .WillRepeatedly(ReleaseSemaphore(&_semaphore));

        EXPECT_CALL(*_mockMessageReceiver, getSerializedGlobalClusterControllerAddress())
                .WillRepeatedly(Return(_globalClusterControllerAddress));

        auto messagingSkeletonEndpointAddr =
                std::make_shared<InProcessMessagingAddress>(_inProcessMessagingSkeleton);
        const bool isGloballyVisible = false;
        constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
        const bool isSticky = false;

        _messageRouter->addNextHop(receiverId2,
                                  messagingSkeletonEndpointAddr,
                                  isGloballyVisible,
                                  expiryDateMs,
                                  isSticky);
        _messageRouter->addNextHop(
                _receiverId, joynrMessagingEndpointAddr, isGloballyVisible, expiryDateMs, isSticky);

        _messageRouter->route(immutableMessage1);
        _messageRouter->route(immutableMessage2);
        _messageRouter->route(immutableMessage2);

        WaitXTimes(3);
    }

private:
    DISALLOW_COPY_AND_ASSIGN(AbstractMessagingTest);
    const system::RoutingTypes::Address _ownAddress;
    const std::vector<std::string> _knownGbids;
};
