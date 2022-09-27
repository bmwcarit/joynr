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

#include <chrono>
#include <cstdint>
#include <memory>

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "MessageRouterTest.h"

#include "joynr/IPlatformSecurityManager.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/Logger.h"
#include "joynr/MessageQueue.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/MqttMulticastAddressCalculator.h"
#include "joynr/MulticastMessagingSkeletonDirectory.h"
#include "joynr/MutableMessageFactory.h"
#include "joynr/Semaphore.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/Util.h"
#include "joynr/WebSocketMulticastAddressCalculator.h"
#include "joynr/access-control/IAccessController.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/UdsAddress.h"
#include "joynr/system/RoutingTypes/UdsClientAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"

#include "tests/mock/MockAccessController.h"
#include "tests/mock/MockDispatcher.h"
#include "tests/mock/MockInProcessMessagingSkeleton.h"
#include "tests/mock/MockMessageSender.h"
#include "tests/mock/MockMessagingMulticastSubscriber.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Pointee;
using ::testing::Property;
using ::testing::Return;
using ::testing::WhenDynamicCastTo;

using namespace joynr;

class CcMessageRouterTest : public MessageRouterTest<CcMessageRouter>
{
public:
    CcMessageRouterTest() : _DEFAULT_IS_GLOBALLY_VISIBLE(true)
    {
    }

    void checkResolveNextHop(const std::string& participantId, bool expectProviderResolved)
    {
        Semaphore successCallbackCalled;
        _messageRouter->resolveNextHop(
                participantId,
                [&successCallbackCalled, expectProviderResolved](const bool& resolved) {
                    if (resolved == expectProviderResolved) {
                        successCallbackCalled.notify();
                    } else {
                        FAIL() << "resolve delivered unexpected result";
                        successCallbackCalled.notify();
                    }
                },
                [&successCallbackCalled](const joynr::exceptions::ProviderRuntimeException&) {
                    FAIL() << "resolveNextHop did not succeed.";
                    successCallbackCalled.notify();
                });
        EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(3000)));
    }

protected:
    void multicastMsgIsSentToAllMulticastReceivers_webSocketClientAddresses(
            const bool isGloballyVisible);
    void multicastMsgIsSentToAllMulticastReceivers_udsClientAddresses(const bool isGloballyVisible);
    void routeMessageAndCheckQueue(const std::string& type, bool expectedToBeQueued);
    void addNewRoutingEntry(const std::string& testParticipantId,
                            std::shared_ptr<const system::RoutingTypes::Address> address);
    void addressIsNotAddedToRoutingTable(
            const std::shared_ptr<const system::RoutingTypes::Address> address);
    void addressIsAddedToRoutingTable(
            const std::shared_ptr<const system::RoutingTypes::Address> address);
    void testRoutingEntryUpdate(
            const std::string& participantId,
            std::shared_ptr<const system::RoutingTypes::Address> newAddress,
            std::shared_ptr<const system::RoutingTypes::Address> expectedAddress);
    const bool _DEFAULT_IS_GLOBALLY_VISIBLE;

    ADD_LOGGER(CcMessageRouterTest)
};

MATCHER_P(udsClientAddress, expectedId, "")
{
    auto udsClientAddress =
            std::dynamic_pointer_cast<const system::RoutingTypes::UdsClientAddress>(arg);
    if (udsClientAddress) {
        return udsClientAddress->getId() == expectedId;
    }
    return false;
}

MATCHER_P2(addressWithChannelId, addressType, channelId, "")
{
    if (addressType == std::string("mqtt")) {
        auto mqttAddress = std::dynamic_pointer_cast<const system::RoutingTypes::MqttAddress>(arg);
        if (mqttAddress) {
            return mqttAddress->getTopic() == channelId;
        }
        return false;
    }

    return false;
}

TEST_F(CcMessageRouterTest, routeMessageToMqttAddress)
{
    const std::string destinationParticipantId = "TEST_routeMessageToMqttAddress";
    const std::string destinationChannelId = "TEST_routeMessageToMqttAddress_channelId";
    const std::string brokerUri = "brokerUri";
    auto address = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(
            brokerUri, destinationChannelId);
    const bool isGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;

    this->_messageRouter->addNextHop(
            destinationParticipantId, address, isGloballyVisible, expiryDateMs, isSticky);
    this->_mutableMessage.setRecipient(destinationParticipantId);

    EXPECT_CALL(*(this->_messagingStubFactory),
                create(addressWithChannelId("mqtt", destinationChannelId)))
            .Times(1);

    this->routeMessageToAddress();
}

TEST_F(CcMessageRouterTest, routeMessageToUdsClientAddress)
{
    const std::string destinationParticipantId = "TEST_routeMessageToUdsClientAddress";
    const std::string udsClientId = "TEST_routeMessageToUdsClientAddress_clientAddressId";
    auto address =
            std::make_shared<const joynr::system::RoutingTypes::UdsClientAddress>(udsClientId);
    const bool isGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;

    this->_messageRouter->addNextHop(
            destinationParticipantId, address, isGloballyVisible, expiryDateMs, isSticky);
    this->_mutableMessage.setRecipient(destinationParticipantId);

    EXPECT_CALL(*(this->_messagingStubFactory), create(udsClientAddress(udsClientId))).Times(1);

    this->routeMessageToAddress();
}

TEST_F(CcMessageRouterTest, removeMulticastReceiver_failsIfProviderAddressNotAvailable)
{
    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    auto providerAddress =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>();
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    _messageRouter->addNextHop(providerParticipantId,
                               providerAddress,
                               _DEFAULT_IS_GLOBALLY_VISIBLE,
                               expiryDateMs,
                               isSticky);

    auto skeleton = std::make_shared<MockMessagingMulticastSubscriber>();
    _multicastMessagingSkeletonDirectory
            ->registerSkeleton<system::RoutingTypes::WebSocketClientAddress>(skeleton);

    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    _messageRouter->removeNextHop(providerParticipantId);

    Semaphore errorCallbackCalled;
    _messageRouter->removeMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            []() { FAIL() << "onSuccess called"; },
            [&errorCallbackCalled](const joynr::exceptions::ProviderRuntimeException&) {
                errorCallbackCalled.notify();
            });
    EXPECT_TRUE(errorCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

void CcMessageRouterTest::multicastMsgIsSentToAllMulticastReceivers_webSocketClientAddresses(
        const bool isProviderGloballyVisible)
{
    const std::string subscriberParticipantId1("subscriberPartId1");
    const std::string subscriberParticipantId2("subscriberPartId2");
    const std::string subscriberParticipantId3("subscriberPartId3");
    const std::string providerParticipantId("providerParticipantId");
    const std::string multicastNameAndPartitions("multicastName/partition0");
    const std::string multicastId(providerParticipantId + "/" + multicastNameAndPartitions);
    const std::string consumerRuntimeWebSocketClientAddressString(
            "ConsumerRuntimeWebSocketAddress");

    // create a new pointer representing the multicast address
    std::vector<std::shared_ptr<joynr::system::RoutingTypes::MqttAddress>> multicastAddressesVector;
    for (std::uint8_t i = 0; i < _availableGbids.size(); i++) {
        multicastAddressesVector.push_back(
                std::make_shared<joynr::system::RoutingTypes::MqttAddress>(
                        _availableGbids[i], multicastId));
    }

    auto expectedAddress1 =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
                    consumerRuntimeWebSocketClientAddressString);
    auto expectedAddress2 = std::make_shared<const joynr::InProcessMessagingAddress>();
    auto expectedAddress3 =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
                    consumerRuntimeWebSocketClientAddressString);
    auto providerAddress =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
                    "ProviderRuntimeWebSocketAddress");

    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    _messageRouter->addNextHop(subscriberParticipantId1,
                               expectedAddress1,
                               _DEFAULT_IS_GLOBALLY_VISIBLE,
                               expiryDateMs,
                               isSticky);
    _messageRouter->addNextHop(subscriberParticipantId2,
                               expectedAddress2,
                               _DEFAULT_IS_GLOBALLY_VISIBLE,
                               expiryDateMs,
                               isSticky);
    _messageRouter->addNextHop(subscriberParticipantId3,
                               expectedAddress3,
                               _DEFAULT_IS_GLOBALLY_VISIBLE,
                               expiryDateMs,
                               isSticky);
    _messageRouter->addNextHop(providerParticipantId,
                               providerAddress,
                               isProviderGloballyVisible,
                               expiryDateMs,
                               isSticky);

    auto skeleton = std::make_shared<MockMessagingMulticastSubscriber>();
    _multicastMessagingSkeletonDirectory
            ->registerSkeleton<system::RoutingTypes::WebSocketClientAddress>(skeleton);

    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId1,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId2,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId3,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    _mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_MULTICAST());
    _mutableMessage.setSender(providerParticipantId);
    _mutableMessage.setRecipient(multicastId);

    // verify that the publication is sent only once to the websocket address used by
    // both subscriberParticipantId1 and subscriberParticipantId3 identified by
    // consumerRuntimeWebSocketClientAddressString
    auto mockMessagingStub = std::make_shared<MockMessagingStub>();
    EXPECT_CALL(
            *_messagingStubFactory,
            create(Property(
                    &std::shared_ptr<const joynr::system::RoutingTypes::Address>::get,
                    WhenDynamicCastTo<const joynr::system::RoutingTypes::WebSocketClientAddress*>(
                            Pointee(Property(
                                    &joynr::system::RoutingTypes::WebSocketClientAddress::getId,
                                    Eq(consumerRuntimeWebSocketClientAddressString)))))))
            .WillOnce(Return(mockMessagingStub));
    EXPECT_CALL(*_messagingStubFactory, create(Pointee(Eq(*expectedAddress2))))
            .WillOnce(Return(mockMessagingStub));
    size_t count = isProviderGloballyVisible ? 1 : 0;

    for (std::uint8_t i = 0; i < _availableGbids.size(); i++) {
        if (count) {
            EXPECT_CALL(*_messagingStubFactory, create(Pointee(Eq(*multicastAddressesVector[i]))))
                    .Times(count)
                    .WillRepeatedly(Return(mockMessagingStub));
        } else {
            EXPECT_CALL(*_messagingStubFactory, create(Pointee(Eq(*multicastAddressesVector[i]))))
                    .Times(0);
        }
    }

    Semaphore semaphore(0);
    EXPECT_CALL(*mockMessagingStub, transmit(_, _))
            .Times(AtLeast(1))
            .WillOnce(ReleaseSemaphore(&semaphore));

    _messageRouter->route(_mutableMessage.getImmutableMessage());

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(2000)));

    Mock::VerifyAndClearExpectations(_messagingStubFactory.get());

    // cleanup
    _messageRouter->removeNextHop(subscriberParticipantId1);
    _messageRouter->removeNextHop(subscriberParticipantId2);
    _messageRouter->removeNextHop(subscriberParticipantId3);
    _messageRouter->removeNextHop(providerParticipantId);
}

void CcMessageRouterTest::multicastMsgIsSentToAllMulticastReceivers_udsClientAddresses(
        const bool isProviderGloballyVisible)
{
    const std::string subscriberParticipantId1("subscriberPartId1");
    const std::string subscriberParticipantId2("subscriberPartId2");
    const std::string subscriberParticipantId3("subscriberPartId3");
    const std::string providerParticipantId("providerParticipantId");
    const std::string multicastNameAndPartitions("multicastName/partition0");
    const std::string multicastId(providerParticipantId + "/" + multicastNameAndPartitions);
    const std::string consumerRuntimeUdsClientAddressString("ConsumerRuntimeUdsAddress");

    // create a new pointer representing the multicast address
    std::vector<std::shared_ptr<joynr::system::RoutingTypes::MqttAddress>> multicastAddressesVector;
    for (std::uint8_t i = 0; i < _availableGbids.size(); i++) {
        multicastAddressesVector.push_back(
                std::make_shared<joynr::system::RoutingTypes::MqttAddress>(
                        _availableGbids[i], multicastId));
    }

    auto expectedAddress1 = std::make_shared<const joynr::system::RoutingTypes::UdsClientAddress>(
            consumerRuntimeUdsClientAddressString);
    auto expectedAddress2 = std::make_shared<const joynr::InProcessMessagingAddress>();
    auto expectedAddress3 = std::make_shared<const joynr::system::RoutingTypes::UdsClientAddress>(
            consumerRuntimeUdsClientAddressString);
    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::UdsClientAddress>(
            "ProviderRuntimeUdsAddress");

    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    _messageRouter->addNextHop(subscriberParticipantId1,
                               expectedAddress1,
                               _DEFAULT_IS_GLOBALLY_VISIBLE,
                               expiryDateMs,
                               isSticky);
    _messageRouter->addNextHop(subscriberParticipantId2,
                               expectedAddress2,
                               _DEFAULT_IS_GLOBALLY_VISIBLE,
                               expiryDateMs,
                               isSticky);
    _messageRouter->addNextHop(subscriberParticipantId3,
                               expectedAddress3,
                               _DEFAULT_IS_GLOBALLY_VISIBLE,
                               expiryDateMs,
                               isSticky);
    _messageRouter->addNextHop(providerParticipantId,
                               providerAddress,
                               isProviderGloballyVisible,
                               expiryDateMs,
                               isSticky);

    auto skeleton = std::make_shared<MockMessagingMulticastSubscriber>();
    _multicastMessagingSkeletonDirectory->registerSkeleton<system::RoutingTypes::UdsClientAddress>(
            skeleton);

    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId1,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId2,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId3,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    _mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_MULTICAST());
    _mutableMessage.setSender(providerParticipantId);
    _mutableMessage.setRecipient(multicastId);

    // verify that the publication is sent only once to the uds address used by
    // both subscriberParticipantId1 and subscriberParticipantId3 identified by
    // consumerRuntimeUdsClientAddressString
    auto mockMessagingStub = std::make_shared<MockMessagingStub>();
    EXPECT_CALL(
            *_messagingStubFactory,
            create(Property(
                    &std::shared_ptr<const joynr::system::RoutingTypes::Address>::get,
                    WhenDynamicCastTo<const joynr::system::RoutingTypes::UdsClientAddress*>(
                            Pointee(Property(&joynr::system::RoutingTypes::UdsClientAddress::getId,
                                             Eq(consumerRuntimeUdsClientAddressString)))))))
            .WillOnce(Return(mockMessagingStub));
    EXPECT_CALL(*_messagingStubFactory, create(Pointee(Eq(*expectedAddress2))))
            .WillOnce(Return(mockMessagingStub));
    size_t count = isProviderGloballyVisible ? 1 : 0;

    for (std::uint8_t i = 0; i < _availableGbids.size(); i++) {
        EXPECT_CALL(*_messagingStubFactory, create(Pointee(Eq(*multicastAddressesVector[i]))))
                .Times(count)
                .WillRepeatedly(Return(mockMessagingStub));
    }

    Semaphore semaphore(0);
    EXPECT_CALL(*mockMessagingStub, transmit(_, _))
            .Times(AtLeast(1))
            .WillOnce(ReleaseSemaphore(&semaphore));

    _messageRouter->route(_mutableMessage.getImmutableMessage());

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(2000)));
    Mock::VerifyAndClearExpectations(_messagingStubFactory.get());

    // cleanup
    _messageRouter->removeNextHop(subscriberParticipantId1);
    _messageRouter->removeNextHop(subscriberParticipantId2);
    _messageRouter->removeNextHop(subscriberParticipantId3);
    _messageRouter->removeNextHop(providerParticipantId);
}

TEST_F(CcMessageRouterTest,
       routeMulticastMessageFromWebSocketProvider_withoutAccessController_multicastMsgIsSentToAllMulticastReceivers_webSocketClientAddresses)
{
    bool isGloballyVisible = true;
    multicastMsgIsSentToAllMulticastReceivers_webSocketClientAddresses(isGloballyVisible);
    isGloballyVisible = false;
    multicastMsgIsSentToAllMulticastReceivers_webSocketClientAddresses(isGloballyVisible);
}

TEST_F(CcMessageRouterTest,
       routeMulticastMessageFromUdsProvider_withoutAccessController_multicastMsgIsSentToAllMulticastReceivers_udsClientAddresses)
{
    bool isGloballyVisible = true;
    multicastMsgIsSentToAllMulticastReceivers_udsClientAddresses(isGloballyVisible);
    isGloballyVisible = false;
    multicastMsgIsSentToAllMulticastReceivers_udsClientAddresses(isGloballyVisible);
}

TEST_F(CcMessageRouterTest, removeUnreachableMulticastReceivers)
{
    const std::string subscriberParticipantId1("subscriberPartId1");
    const std::string subscriberParticipantId2("subscriberPartId2");
    const std::string subscriberParticipantId3("subscriberPartId3");
    const std::string providerParticipantId("providerParticipantId");
    const std::string multicastNameAndPartitions("multicastName/partition0");
    const std::string multicastId(providerParticipantId + "/" + multicastNameAndPartitions);
    const std::string consumerRuntimeWebSocketClientAddressString1(
            "ConsumerRuntimeWebSocketAddress1");
    const std::string consumerRuntimeWebSocketClientAddressString2(
            "ConsumerRuntimeWebSocketAddress2");

    // create a new pointer representing the multicast address
    std::shared_ptr<joynr::system::RoutingTypes::MqttAddress> multicastAddress(
            std::make_shared<joynr::system::RoutingTypes::MqttAddress>("testGbid1", multicastId));

    auto expectedAddress1 =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
                    consumerRuntimeWebSocketClientAddressString1);
    auto expectedAddress3 =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
                    consumerRuntimeWebSocketClientAddressString2);
    auto providerAddress =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
                    "ProviderRuntimeWebSocketAddress");

    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    const bool isProviderGloballyVisible = false;

    _messageRouter->addNextHop(subscriberParticipantId1,
                               expectedAddress1,
                               _DEFAULT_IS_GLOBALLY_VISIBLE,
                               expiryDateMs,
                               isSticky);
    _messageRouter->addNextHop(subscriberParticipantId2,
                               expectedAddress1,
                               _DEFAULT_IS_GLOBALLY_VISIBLE,
                               expiryDateMs,
                               isSticky);
    _messageRouter->addNextHop(subscriberParticipantId3,
                               expectedAddress3,
                               _DEFAULT_IS_GLOBALLY_VISIBLE,
                               expiryDateMs,
                               isSticky);
    _messageRouter->addNextHop(providerParticipantId,
                               providerAddress,
                               isProviderGloballyVisible,
                               expiryDateMs,
                               isSticky);

    auto skeleton = std::make_shared<MockMessagingMulticastSubscriber>();
    _multicastMessagingSkeletonDirectory
            ->registerSkeleton<system::RoutingTypes::WebSocketClientAddress>(skeleton);

    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId1,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId2,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId3,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    _mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_MULTICAST());
    _mutableMessage.setSender(providerParticipantId);
    _mutableMessage.setRecipient(multicastId);

    auto mockMessagingStub1 = std::make_shared<MockMessagingStub>();
    auto mockMessagingStub2 = std::make_shared<MockMessagingStub>();

    // since participantIds of both websocket runtimes are subscribed for the broadcast
    // exactly one WebSocketMessagingStub will be created for each runtime. The message would
    // then be distributed again on lower level within that runtime if there are
    // multiple subscribers in the same runtime (e.g. subscriberParticipantId1 and
    // subscriberParticipantId2).

    EXPECT_CALL(
            *_messagingStubFactory,
            create(Property(
                    &std::shared_ptr<const joynr::system::RoutingTypes::Address>::get,
                    WhenDynamicCastTo<const joynr::system::RoutingTypes::WebSocketClientAddress*>(
                            Pointee(Property(
                                    &joynr::system::RoutingTypes::WebSocketClientAddress::getId,
                                    Eq(consumerRuntimeWebSocketClientAddressString1)))))))
            .Times(1)
            .WillRepeatedly(Return(mockMessagingStub1));

    EXPECT_CALL(
            *_messagingStubFactory,
            create(Property(
                    &std::shared_ptr<const joynr::system::RoutingTypes::Address>::get,
                    WhenDynamicCastTo<const joynr::system::RoutingTypes::WebSocketClientAddress*>(
                            Pointee(Property(
                                    &joynr::system::RoutingTypes::WebSocketClientAddress::getId,
                                    Eq(consumerRuntimeWebSocketClientAddressString2)))))))
            .Times(1)
            .WillRepeatedly(Return(mockMessagingStub2));

    Semaphore semaphore1(0);
    Semaphore semaphore2(0);
    EXPECT_CALL(*mockMessagingStub1, transmit(_, _))
            .Times(AtLeast(1))
            .WillOnce(ReleaseSemaphore(&semaphore1));
    EXPECT_CALL(*mockMessagingStub2, transmit(_, _))
            .Times(AtLeast(1))
            .WillOnce(ReleaseSemaphore(&semaphore2));

    _messageRouter->route(_mutableMessage.getImmutableMessage());

    EXPECT_TRUE(semaphore1.waitFor(std::chrono::milliseconds(2000)));
    EXPECT_TRUE(semaphore2.waitFor(std::chrono::milliseconds(2000)));

    Mock::VerifyAndClearExpectations(_messagingStubFactory.get());

    // now the first runtime becomes unreachable, so the creation of a WebSocketMessagingStub fails;
    // the factory returns an empty IMessagingStub in this case

    EXPECT_CALL(
            *_messagingStubFactory,
            create(Property(
                    &std::shared_ptr<const joynr::system::RoutingTypes::Address>::get,
                    WhenDynamicCastTo<const joynr::system::RoutingTypes::WebSocketClientAddress*>(
                            Pointee(Property(
                                    &joynr::system::RoutingTypes::WebSocketClientAddress::getId,
                                    Eq(consumerRuntimeWebSocketClientAddressString1)))))))
            .Times(1)
            .WillRepeatedly(Return(std::shared_ptr<IMessagingStub>()));

    EXPECT_CALL(
            *_messagingStubFactory,
            create(Property(
                    &std::shared_ptr<const joynr::system::RoutingTypes::Address>::get,
                    WhenDynamicCastTo<const joynr::system::RoutingTypes::WebSocketClientAddress*>(
                            Pointee(Property(
                                    &joynr::system::RoutingTypes::WebSocketClientAddress::getId,
                                    Eq(consumerRuntimeWebSocketClientAddressString2)))))))
            .Times(1)
            .WillRepeatedly(Return(mockMessagingStub2));

    // the CcMessageRouter should then automatically remove the participantIds realated to
    // the unreachable runtime from the multicastReceiverDirectory

    Semaphore semaphore3(0);
    EXPECT_CALL(*mockMessagingStub2, transmit(_, _))
            .Times(AtLeast(1))
            .WillOnce(ReleaseSemaphore(&semaphore3));

    _messageRouter->route(_mutableMessage.getImmutableMessage());

    EXPECT_TRUE(semaphore3.waitFor(std::chrono::milliseconds(2000)));

    Mock::VerifyAndClearExpectations(_messagingStubFactory.get());

    // when the next publication is routed, the factory is no longer asked to create a
    // WebSocketMessagingStub for the first runtime, since there are no subscribers
    // left related to this runtime

    EXPECT_CALL(
            *_messagingStubFactory,
            create(Property(
                    &std::shared_ptr<const joynr::system::RoutingTypes::Address>::get,
                    WhenDynamicCastTo<const joynr::system::RoutingTypes::WebSocketClientAddress*>(
                            Pointee(Property(
                                    &joynr::system::RoutingTypes::WebSocketClientAddress::getId,
                                    Eq(consumerRuntimeWebSocketClientAddressString1)))))))
            .Times(0);

    EXPECT_CALL(
            *_messagingStubFactory,
            create(Property(
                    &std::shared_ptr<const joynr::system::RoutingTypes::Address>::get,
                    WhenDynamicCastTo<const joynr::system::RoutingTypes::WebSocketClientAddress*>(
                            Pointee(Property(
                                    &joynr::system::RoutingTypes::WebSocketClientAddress::getId,
                                    Eq(consumerRuntimeWebSocketClientAddressString2)))))))
            .Times(1)
            .WillRepeatedly(Return(mockMessagingStub2));

    Semaphore semaphore4(0);
    EXPECT_CALL(*mockMessagingStub2, transmit(_, _))
            .Times(AtLeast(1))
            .WillOnce(ReleaseSemaphore(&semaphore4));

    _messageRouter->route(_mutableMessage.getImmutableMessage());

    EXPECT_TRUE(semaphore4.waitFor(std::chrono::milliseconds(2000)));
    Mock::VerifyAndClearExpectations(_messagingStubFactory.get());

    // cleanup

    // remove only subscriberParticipantId3 since the other ones have already been
    // removed automatically by the test
    _messageRouter->removeMulticastReceiver(
            multicastId,
            subscriberParticipantId3,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    _messageRouter->removeNextHop(subscriberParticipantId1);
    _messageRouter->removeNextHop(subscriberParticipantId2);
    _messageRouter->removeNextHop(subscriberParticipantId3);
    _messageRouter->removeNextHop(providerParticipantId);
}

void invokeConsumerPermissionCallbackWithPermissionYes(
        std::shared_ptr<ImmutableMessage> /*message*/,
        std::shared_ptr<IAccessController::IHasConsumerPermissionCallback> callback,
        bool isLocalRecipient)
{
    std::ignore = isLocalRecipient;
    callback->hasConsumerPermission(IAccessController::Enum::YES);
}

void invokeConsumerPermissionCallbackWithPermissionRetry(
        std::shared_ptr<ImmutableMessage> /*message*/,
        std::shared_ptr<IAccessController::IHasConsumerPermissionCallback> callback,
        bool isLocalRecipient)
{
    std::ignore = isLocalRecipient;
    callback->hasConsumerPermission(IAccessController::Enum::RETRY);
}

TEST_F(CcMessageRouterTest,
       routeMulticastMessageFromWebSocketProvider_withAccessController_multicastMsgIsSentToAllMulticastReceivers_webSocketClientAddresses)
{
    auto mockAccessController = std::make_shared<MockAccessController>();
    ON_CALL(*mockAccessController, hasConsumerPermission(_, _, _))
            .WillByDefault(Invoke(invokeConsumerPermissionCallbackWithPermissionYes));
    EXPECT_CALL(*mockAccessController, hasConsumerPermission(_, _, _)).Times(AtLeast(1));
    _messageRouter->setAccessController(util::as_weak_ptr(mockAccessController));

    bool isGloballyVisible = true;
    multicastMsgIsSentToAllMulticastReceivers_webSocketClientAddresses(isGloballyVisible);
    isGloballyVisible = false;
    multicastMsgIsSentToAllMulticastReceivers_webSocketClientAddresses(isGloballyVisible);
}

TEST_F(CcMessageRouterTest,
       routeMulticastMessageFromUdsProvider_withAccessController_multicastMsgIsSentToAllMulticastReceivers_udsClientAddresses)
{
    auto mockAccessController = std::make_shared<MockAccessController>();
    ON_CALL(*mockAccessController, hasConsumerPermission(_, _, _))
            .WillByDefault(Invoke(invokeConsumerPermissionCallbackWithPermissionYes));
    EXPECT_CALL(*mockAccessController, hasConsumerPermission(_, _, _)).Times(AtLeast(1));
    _messageRouter->setAccessController(util::as_weak_ptr(mockAccessController));

    bool isGloballyVisible = true;
    multicastMsgIsSentToAllMulticastReceivers_udsClientAddresses(isGloballyVisible);
    isGloballyVisible = false;
    multicastMsgIsSentToAllMulticastReceivers_udsClientAddresses(isGloballyVisible);
}

TEST_F(CcMessageRouterTest, removeMulticastReceiver_NonChildRouter_succeedsIfSkeletonNotAvailable)
{
    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>();
    _messageRouter->addProvisionedNextHop(
            providerParticipantId, providerAddress, _DEFAULT_IS_GLOBALLY_VISIBLE);

    auto skeleton = std::make_shared<MockMessagingMulticastSubscriber>();
    _multicastMessagingSkeletonDirectory->registerSkeleton<system::RoutingTypes::MqttAddress>(
            skeleton);

    EXPECT_CALL(*skeleton, registerMulticastSubscription(multicastId)).Times(1);

    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    _multicastMessagingSkeletonDirectory->unregisterSkeletons<system::RoutingTypes::MqttAddress>();

    Semaphore successCallbackCalled;
    _messageRouter->removeMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            [&successCallbackCalled]() { successCallbackCalled.notify(); },
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(CcMessageRouterTest,
       removeMulticastReceiverOfStandaloneProvider_NonChildRouter_callsSkeleton)
{
    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>();

    _messageRouter->addProvisionedNextHop(
            providerParticipantId, providerAddress, _DEFAULT_IS_GLOBALLY_VISIBLE);

    auto skeleton = std::make_shared<MockMessagingMulticastSubscriber>();

    _multicastMessagingSkeletonDirectory->registerSkeleton<system::RoutingTypes::MqttAddress>(
            skeleton);

    EXPECT_CALL(*skeleton, registerMulticastSubscription(multicastId)).Times(1);

    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    // The message router shall unregister the subscription at the multicast skeleton
    EXPECT_CALL(*skeleton, unregisterMulticastSubscription(multicastId)).Times(1);

    Semaphore successCallbackCalled;
    _messageRouter->removeMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            [&successCallbackCalled]() { successCallbackCalled.notify(); },
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(CcMessageRouterTest, removeMulticastReceiverOfWebSocketProvider_NonChildRouter_succeeds)
{
    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    auto providerAddress =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>();
    _messageRouter->addProvisionedNextHop(
            providerParticipantId, providerAddress, _DEFAULT_IS_GLOBALLY_VISIBLE);

    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    Semaphore successCallbackCalled;
    _messageRouter->removeMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            [&successCallbackCalled]() { successCallbackCalled.notify(); },
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(CcMessageRouterTest, removeMulticastReceiverOfInProcessProvider_NonChildRouter_succeeds)
{
    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    auto dispatcher = std::make_shared<MockDispatcher>();
    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>(dispatcher);

    auto providerAddress = std::make_shared<const joynr::InProcessMessagingAddress>(skeleton);
    _messageRouter->addProvisionedNextHop(
            providerParticipantId, providerAddress, _DEFAULT_IS_GLOBALLY_VISIBLE);

    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    Semaphore successCallbackCalled;
    _messageRouter->removeMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            [&successCallbackCalled]() { successCallbackCalled.notify(); },
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(CcMessageRouterTest, addMulticastReceiverForMqttProvider_NonChildRouter_callsSkeleton)
{
    const std::string subscriberParticipantId("subscriberPartId1");
    const std::string providerParticipantId("providerParticipantId");
    const std::string multicastId("participantId1/methodName/partition0");

    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>();
    _messageRouter->addProvisionedNextHop(
            providerParticipantId, providerAddress, _DEFAULT_IS_GLOBALLY_VISIBLE);

    auto mockMqttMessagingMulticastSubscriber =
            std::make_shared<MockMessagingMulticastSubscriber>();
    std::shared_ptr<IMessagingMulticastSubscriber> mqttMessagingMulticastSubscriber =
            mockMqttMessagingMulticastSubscriber;

    _multicastMessagingSkeletonDirectory->registerSkeleton<system::RoutingTypes::MqttAddress>(
            mqttMessagingMulticastSubscriber);

    EXPECT_CALL(*mockMqttMessagingMulticastSubscriber, registerMulticastSubscription(multicastId));

    Semaphore successCallbackCalled;
    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            [&successCallbackCalled]() { successCallbackCalled.notify(); },
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(CcMessageRouterTest, addMulticastReceiverForWebSocketProvider_NonChildRouter_succeeds)
{
    const std::string subscriberParticipantId("subscriberPartId1");
    const std::string providerParticipantId("providerParticipantId");
    const std::string multicastId("participantId1/methodName/partition0");

    auto providerAddress =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>();
    _messageRouter->addProvisionedNextHop(
            providerParticipantId, providerAddress, _DEFAULT_IS_GLOBALLY_VISIBLE);

    Semaphore successCallbackCalled;
    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            [&successCallbackCalled]() { successCallbackCalled.notify(); },
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(CcMessageRouterTest, addMulticastReceiverForInProcessProvider_NonChildRouter_succeeds)
{
    const std::string subscriberParticipantId("subscriberPartId1");
    const std::string providerParticipantId("providerParticipantId");
    const std::string multicastId("participantId1/methodName/partition0");

    auto dispatcher = std::make_shared<MockDispatcher>();
    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>(dispatcher);

    auto providerAddress = std::make_shared<const joynr::InProcessMessagingAddress>(skeleton);
    _messageRouter->addProvisionedNextHop(
            providerParticipantId, providerAddress, _DEFAULT_IS_GLOBALLY_VISIBLE);

    Semaphore successCallbackCalled;
    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            [&successCallbackCalled]() { successCallbackCalled.notify(); },
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(CcMessageRouterTest, addMulticastReceiver_NonChildRouter_failsIfProviderAddressNotAvailable)
{
    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    Semaphore errorCallbackCalled;
    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            []() { FAIL() << "onSuccess called"; },
            [&errorCallbackCalled](const joynr::exceptions::ProviderRuntimeException&) {
                errorCallbackCalled.notify();
            });
    EXPECT_TRUE(errorCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(CcMessageRouterTest, addMulticastReceiver_NonChildRouter_succeedsIfSkeletonNotAvailable)
{
    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>();
    _messageRouter->addProvisionedNextHop(
            providerParticipantId, providerAddress, _DEFAULT_IS_GLOBALLY_VISIBLE);

    Semaphore successCallbackCalled;
    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            [&successCallbackCalled]() { successCallbackCalled.notify(); },
            [](const joynr::exceptions::ProviderRuntimeException& exception) {
                FAIL() << "onError called: " << exception.what();
            });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(CcMessageRouterTest, routingTableGetsCleaned)
{
    const std::string providerParticipantId("providerParticipantId");

    auto dispatcher = std::make_shared<MockDispatcher>();
    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>(dispatcher);
    auto providerAddress = std::make_shared<const joynr::InProcessMessagingAddress>(skeleton);

    const bool isGloballyVisible = true;
    const bool isSticky = false;
    std::int64_t expiryDateMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::system_clock::now().time_since_epoch())
                                        .count() +
                                4000;
    EXPECT_CALL(*_messagingStubFactory, shutdown()).Times(1);
    _messageRouter->shutdown();
    _messageRouter = createMessageRouter();
    _messageRouter->addNextHop(
            providerParticipantId, providerAddress, isGloballyVisible, expiryDateMs, isSticky);

    Semaphore successCallbackCalled;
    _messageRouter->resolveNextHop(
            providerParticipantId,
            [&successCallbackCalled](const bool& resolved) {
                if (resolved) {
                    successCallbackCalled.notify();
                } else {
                    FAIL() << "resolve should not succeed.";
                    successCallbackCalled.notify();
                }
            },
            [&successCallbackCalled](const joynr::exceptions::ProviderRuntimeException&) {
                FAIL() << "resolveNextHop did not succeed.";
                successCallbackCalled.notify();
            });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(3000)));

    std::this_thread::sleep_for(std::chrono::milliseconds(6000));
    // in the meantime the garbage collector should have removed the entry

    _messageRouter->resolveNextHop(
            providerParticipantId,
            [&successCallbackCalled](const bool& resolved) {
                if (resolved) {
                    FAIL() << "resolve should not succeed.";
                    successCallbackCalled.notify();
                } else {
                    successCallbackCalled.notify();
                }
            },
            [&successCallbackCalled](const joynr::exceptions::ProviderRuntimeException&) {
                FAIL() << "resolveNextHop did not succeed.";
                successCallbackCalled.notify();
            });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(3000)));
}

void CcMessageRouterTest::routeMessageAndCheckQueue(const std::string& msgType,
                                                    bool msgShouldBeQueued)
{
    // setup the message
    auto localMutableMessage = std::make_shared<MutableMessage>();
    localMutableMessage->setType(msgType);
    localMutableMessage->setSender("sender");
    std::string recipient = "unknownRecipient";
    localMutableMessage->setRecipient(recipient);
    const TimePoint now = TimePoint::now();
    localMutableMessage->setExpiryDate(now + std::chrono::milliseconds(60000));
    std::shared_ptr<ImmutableMessage> immutableMessage = localMutableMessage->getImmutableMessage();

    // verify that the recipient is unknown
    Semaphore successCallbackCalled;
    this->_messageRouter->resolveNextHop(
            recipient,
            [&successCallbackCalled](const bool& resolved) {
                if (resolved) {
                    FAIL() << "resolve should not succeed.";
                    successCallbackCalled.notify();
                } else {
                    successCallbackCalled.notify();
                }
            },
            [&successCallbackCalled](const joynr::exceptions::ProviderRuntimeException&) {
                FAIL() << "resolveNextHop did not succeed.";
                successCallbackCalled.notify();
            });

    this->_messageRouter->route(immutableMessage);
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(2000)));
    EXPECT_EQ(this->_messageQueue->getQueueLength(), msgShouldBeQueued ? 1 : 0);
    EXPECT_EQ(this->_messageRouter->getNumberOfRoutedMessages(), 1);
}

TEST_F(CcMessageRouterTest, accessControllerIsCalledForQueuedMsgs)
{
    const std::string providerParticipantId("providerParticipantId");
    auto providerAddress =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>();

    // setup the message
    auto localMutableMessage = std::make_shared<MutableMessage>();
    localMutableMessage->setType(joynr::Message::VALUE_MESSAGE_TYPE_REQUEST());
    localMutableMessage->setSender("sender");
    localMutableMessage->setRecipient(providerParticipantId);
    const TimePoint nowTime = TimePoint::now();
    localMutableMessage->setExpiryDate(nowTime + std::chrono::milliseconds(16000));

    auto mockAccessController = std::make_shared<MockAccessController>();
    _messageRouter->setAccessController(util::as_weak_ptr(mockAccessController));

    std::shared_ptr<ImmutableMessage> immutableMessage = localMutableMessage->getImmutableMessage();
    this->_messageRouter->route(immutableMessage);

    EXPECT_EQ(1, _messageQueue->getQueueLength());
    EXPECT_CALL(*mockAccessController, hasConsumerPermission(_, _, _)).Times(0);

    Mock::VerifyAndClearExpectations(mockAccessController.get());

    EXPECT_CALL(*mockAccessController, hasConsumerPermission(Eq(immutableMessage), _, _));

    const bool isSticky = false;
    const std::int64_t expiryDate = std::numeric_limits<std::int64_t>::max();
    _messageRouter->addNextHop(providerParticipantId, providerAddress, _DEFAULT_IS_GLOBALLY_VISIBLE,
                               expiryDate, isSticky);
}

TEST_F(CcMessageRouterTest, testAccessControlRetryWithDelay)
{
    Semaphore semaphore(0);
    auto mockMessagingStub = std::make_shared<MockMessagingStub>();
    ON_CALL(*_messagingStubFactory, create(_)).WillByDefault(Return(mockMessagingStub));
    EXPECT_CALL(*_messagingStubFactory, create(_)).Times(AtLeast(1));

    const std::string providerParticipantId("providerParticipantId");
    auto providerAddress =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>();

    // setup the message
    auto localMutableMessage = std::make_shared<MutableMessage>();
    localMutableMessage->setType(joynr::Message::VALUE_MESSAGE_TYPE_REQUEST());
    localMutableMessage->setSender("sender");
    localMutableMessage->setRecipient(providerParticipantId);
    const TimePoint nowTime = TimePoint::now();
    localMutableMessage->setExpiryDate(nowTime + std::chrono::milliseconds(16000));
    _messageRouter->addProvisionedNextHop(
            providerParticipantId, providerAddress, _DEFAULT_IS_GLOBALLY_VISIBLE);

    auto mockAccessController = std::make_shared<MockAccessController>();
    _messageRouter->setAccessController(util::as_weak_ptr(mockAccessController));

    std::shared_ptr<ImmutableMessage> immutableMessage = localMutableMessage->getImmutableMessage();

    EXPECT_CALL(*mockAccessController, hasConsumerPermission(immutableMessage, _, _))
            .WillOnce(Invoke(invokeConsumerPermissionCallbackWithPermissionRetry));

    this->_messageRouter->route(immutableMessage);
    Mock::VerifyAndClearExpectations(mockAccessController.get());

    // 1st retry
    EXPECT_CALL(*mockAccessController, hasConsumerPermission(immutableMessage, _, _)).Times(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1 * _sendMsgRetryInterval - 200));
    Mock::VerifyAndClearExpectations(mockAccessController.get());

    EXPECT_CALL(*mockAccessController, hasConsumerPermission(immutableMessage, _, _))
            .WillOnce(DoAll(ReleaseSemaphore(&semaphore),
                            Invoke(invokeConsumerPermissionCallbackWithPermissionRetry)));
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(_sendMsgRetryInterval)));
    Mock::VerifyAndClearExpectations(mockAccessController.get());

    // 2nd retry
    EXPECT_CALL(*mockAccessController, hasConsumerPermission(immutableMessage, _, _)).Times(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(2 * _sendMsgRetryInterval - 200));
    Mock::VerifyAndClearExpectations(mockAccessController.get());
    EXPECT_CALL(*mockAccessController, hasConsumerPermission(immutableMessage, _, _))
            .WillOnce(DoAll(ReleaseSemaphore(&semaphore),
                            Invoke(invokeConsumerPermissionCallbackWithPermissionRetry)));
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(_sendMsgRetryInterval)));
    Mock::VerifyAndClearExpectations(mockAccessController.get());

    // 3rd retry (stub.transmit has not been called so far)
    EXPECT_CALL(*mockMessagingStub, transmit(immutableMessage, _)).Times(0);
    EXPECT_CALL(*mockAccessController, hasConsumerPermission(immutableMessage, _, _)).Times(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(4 * _sendMsgRetryInterval - 200));
    Mock::VerifyAndClearExpectations(mockAccessController.get());
    Mock::VerifyAndClearExpectations(mockMessagingStub.get());
    EXPECT_CALL(*mockAccessController, hasConsumerPermission(immutableMessage, _, _))
            .WillOnce(DoAll(ReleaseSemaphore(&semaphore),
                            Invoke(invokeConsumerPermissionCallbackWithPermissionYes)));
    EXPECT_CALL(*mockMessagingStub, transmit(immutableMessage, _))
            .WillOnce(ReleaseSemaphore(&semaphore));

    // wait for 2 invocations: accessController.hasConsumerPermission and stub.transmit
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(_sendMsgRetryInterval)));
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(std::chrono::seconds(1))));
}

TEST_F(CcMessageRouterTest, checkReplyToNonExistingProxyIsNotDiscardedWhenDisabled)
{
    bool msgShouldBeQueued = true;
    routeMessageAndCheckQueue(Message::VALUE_MESSAGE_TYPE_REPLY(), msgShouldBeQueued);
}

TEST_F(CcMessageRouterTest, checkSubscriptionReplyToNonExistingRecipientIsNotDiscardedWhenDisabled)
{
    bool msgShouldBeQueued = true;
    routeMessageAndCheckQueue(Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY(), msgShouldBeQueued);
}

TEST_F(CcMessageRouterTest, checkPublicationToNonExistingRecipientIsNotDiscardedWhenDisabled)
{
    bool msgShouldBeQueued = true;
    routeMessageAndCheckQueue(Message::VALUE_MESSAGE_TYPE_PUBLICATION(), msgShouldBeQueued);
}

TEST_F(CcMessageRouterTest, checkMulticastToNonExistingRecipientIsDiscarded)
{
    bool msgShouldBeQueued = false;
    routeMessageAndCheckQueue(Message::VALUE_MESSAGE_TYPE_MULTICAST(), msgShouldBeQueued);
}

TEST_F(CcMessageRouterTest, checkRequestToNonExistingRecipientIsQueued)
{
    bool msgShouldBeQueued = true;
    routeMessageAndCheckQueue(Message::VALUE_MESSAGE_TYPE_REQUEST(), msgShouldBeQueued);
}

TEST_F(CcMessageRouterTest, checkOneWayToNonExistingRecipientIsQueued)
{
    bool msgShouldBeQueued = true;
    routeMessageAndCheckQueue(Message::VALUE_MESSAGE_TYPE_ONE_WAY(), msgShouldBeQueued);
}

TEST_F(CcMessageRouterTest, checkSubscriptionRequestToNonExistingRecipientIsQueued)
{
    bool msgShouldBeQueued = true;
    routeMessageAndCheckQueue(
            Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST(), msgShouldBeQueued);
}

TEST_F(CcMessageRouterTest, checkMulticastSubscriptionRequestToNonExistingRecipientIsQueued)
{
    bool msgShouldBeQueued = true;
    routeMessageAndCheckQueue(
            Message::VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST(), msgShouldBeQueued);
}

TEST_F(CcMessageRouterTest, checkBroadcastSubscriptionRequestToNonExistingRecipientIsQueued)
{
    bool msgShouldBeQueued = true;
    routeMessageAndCheckQueue(
            Message::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST(), msgShouldBeQueued);
}

TEST_F(CcMessageRouterTest, checkSubscriptionStopToNonExistingRecipientIsQueued)
{
    bool msgShouldBeQueued = true;
    routeMessageAndCheckQueue(Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP(), msgShouldBeQueued);
}

// special check if discarding is enabled

TEST_F(CcMessageRouterTest, checkReplyToNonExistingProxyIsDiscardedWhenEnabled)
{
    EXPECT_CALL(*_messagingStubFactory, shutdown()).Times(1);
    _messageRouter->shutdown();
    _messagingSettings.setDiscardUnroutableRepliesAndPublications(true);
    _messageRouter = createMessageRouter();
    bool msgShouldBeQueued = false;
    routeMessageAndCheckQueue(Message::VALUE_MESSAGE_TYPE_REPLY(), msgShouldBeQueued);
}

TEST_F(CcMessageRouterTest, checkSubscriptionReplyToNonExistingRecipientIsDiscardedWhenEnabled)
{
    EXPECT_CALL(*_messagingStubFactory, shutdown()).Times(1);
    _messageRouter->shutdown();
    _messagingSettings.setDiscardUnroutableRepliesAndPublications(true);
    _messageRouter = createMessageRouter();
    bool msgShouldBeQueued = false;
    routeMessageAndCheckQueue(Message::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY(), msgShouldBeQueued);
}

TEST_F(CcMessageRouterTest, checkPublicationToNonExistingRecipientIsDiscardedWhenEnabled)
{
    EXPECT_CALL(*_messagingStubFactory, shutdown()).Times(1);
    _messageRouter->shutdown();
    _messagingSettings.setDiscardUnroutableRepliesAndPublications(true);
    _messageRouter = createMessageRouter();
    bool msgShouldBeQueued = false;
    routeMessageAndCheckQueue(Message::VALUE_MESSAGE_TYPE_PUBLICATION(), msgShouldBeQueued);
}

TEST_F(CcMessageRouterTest, setToKnownDoesNotChangeRoutingTable)
{
    Semaphore resolveNextHopDone(0);
    const std::string testParticipantId("testParticipantId");

    auto expectNotResolved = [&resolveNextHopDone](const bool& resolved) {
        ASSERT_FALSE(resolved);
        resolveNextHopDone.notify();
    };

    _messageRouter->resolveNextHop(testParticipantId, expectNotResolved, nullptr);
    ASSERT_TRUE(resolveNextHopDone.waitFor(std::chrono::milliseconds(1000)));

    _messageRouter->setToKnown(testParticipantId);

    _messageRouter->resolveNextHop(testParticipantId, expectNotResolved, nullptr);
    ASSERT_TRUE(resolveNextHopDone.waitFor(std::chrono::milliseconds(1000)));
}

void CcMessageRouterTest::addNewRoutingEntry(
        const std::string& testParticipantId,
        std::shared_ptr<const system::RoutingTypes::Address> address)
{
    Semaphore resolveNextHopDone(0);

    auto expectResolved = [&resolveNextHopDone, &address](const bool& resolved) {
        ASSERT_TRUE(resolved) << "address: " + address->toString();
        resolveNextHopDone.notify();
    };

    auto expectNotResolved = [&resolveNextHopDone, &address](const bool& resolved) {
        ASSERT_FALSE(resolved) << "address: " + address->toString();
        resolveNextHopDone.notify();
    };

    _messageRouter->resolveNextHop(testParticipantId, expectNotResolved, nullptr);
    ASSERT_TRUE(resolveNextHopDone.waitFor(std::chrono::milliseconds(1000)));

    const bool isParticipantGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    _messageRouter->addNextHop(
            testParticipantId, address, isParticipantGloballyVisible, expiryDateMs, isSticky);

    _messageRouter->resolveNextHop(testParticipantId, expectResolved, nullptr);
    ASSERT_TRUE(resolveNextHopDone.waitFor(std::chrono::milliseconds(1000)));
}

void CcMessageRouterTest::addressIsNotAddedToRoutingTable(
        const std::shared_ptr<const system::RoutingTypes::Address> address)
{
    Semaphore resolveNextHopDone(0);
    const std::string testParticipantId("testParticipantId");
    const bool isParticipantGloballyVisible = true;

    auto expectNotResolved = [&resolveNextHopDone, &address](const bool& resolved) {
        ASSERT_FALSE(resolved) << "address: " + address->toString();
        resolveNextHopDone.notify();
    };

    _messageRouter->resolveNextHop(testParticipantId, expectNotResolved, nullptr);
    ASSERT_TRUE(resolveNextHopDone.waitFor(std::chrono::milliseconds(1000)));

    _messageRouter->addProvisionedNextHop(testParticipantId, address, isParticipantGloballyVisible);

    _messageRouter->resolveNextHop(testParticipantId, expectNotResolved, nullptr);
    ASSERT_TRUE(resolveNextHopDone.waitFor(std::chrono::milliseconds(1000)));
}

void CcMessageRouterTest::addressIsAddedToRoutingTable(
        const std::shared_ptr<const system::RoutingTypes::Address> address)
{
    Semaphore resolveNextHopDone(0);
    const std::string testParticipantId("testParticipantId");

    auto expectNotResolved = [&resolveNextHopDone, &address](const bool& resolved) {
        ASSERT_FALSE(resolved) << "address: " + address->toString();
        resolveNextHopDone.notify();
    };

    addNewRoutingEntry(testParticipantId, address);

    // cleanup
    _messageRouter->removeNextHop(testParticipantId);
    _messageRouter->resolveNextHop(testParticipantId, expectNotResolved, nullptr);
    ASSERT_TRUE(resolveNextHopDone.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(CcMessageRouterTest, addressValidation_globalAddressMustNotReferToOurClusterController)
{
    // see also addressValidation_otherAddressesOfOwnAddressTypeAreAddedToRoutingTable
    auto ownAddress =
            std::make_shared<const system::RoutingTypes::MqttAddress>("brokerUri", "ownTopic");
    setOwnAddress(ownAddress);
    EXPECT_CALL(*_messagingStubFactory, shutdown()).Times(1);
    _messageRouter->shutdown();
    _messageRouter = createMessageRouter();

    addressIsNotAddedToRoutingTable(ownAddress);

    auto newAddress = std::make_shared<system::RoutingTypes::MqttAddress>();
    newAddress->setTopic(ownAddress->getTopic());

    addressIsNotAddedToRoutingTable(newAddress);

    newAddress->setBrokerUri("otherBroker");
    addressIsNotAddedToRoutingTable(newAddress);

    newAddress->setBrokerUri(ownAddress->getBrokerUri());
    addressIsNotAddedToRoutingTable(newAddress);
}

TEST_F(CcMessageRouterTest, addressValidation_otherAddressesOfOwnAddressTypeAreAddedToRoutingTable)
{
    // see also addressValidation_globalAddressMustNotReferToOurClusterController
    auto ownAddress =
            std::make_shared<const system::RoutingTypes::MqttAddress>("brokerUri", "ownTopic");
    setOwnAddress(ownAddress);
    EXPECT_CALL(*_messagingStubFactory, shutdown()).Times(1);
    _messageRouter->shutdown();
    _messageRouter = createMessageRouter();

    addressIsNotAddedToRoutingTable(ownAddress);

    auto newAddress =
            std::make_shared<system::RoutingTypes::MqttAddress>("otherBroker", "otherTopic");
    addressIsAddedToRoutingTable(newAddress);

    newAddress->setBrokerUri(ownAddress->getBrokerUri());
    addressIsAddedToRoutingTable(newAddress);
}

TEST_F(CcMessageRouterTest, addressValidation_webSocketAddressIsNotAddedToRoutingTable)
{
    // see also otherAddressesTypesAreAddedToRoutingTable
    auto webSocketAddress = std::make_shared<const system::RoutingTypes::WebSocketAddress>();
    addressIsNotAddedToRoutingTable(webSocketAddress);
}

TEST_F(CcMessageRouterTest, addressValidation_udsAddressIsNotAddedToRoutingTable)
{
    // see also otherAddressesTypesAreAddedToRoutingTable
    auto udsAddress = std::make_shared<const system::RoutingTypes::UdsAddress>();
    addressIsNotAddedToRoutingTable(udsAddress);
}

TEST_F(CcMessageRouterTest, addressValidation_otherAddressesTypesAreAddedToRoutingTable)
{
    // see also webSocketAddressIsNotAddedToRoutingTable
    auto ownAddress =
            std::make_shared<const system::RoutingTypes::MqttAddress>("brokerUri", "ownTopic");
    setOwnAddress(ownAddress);
    EXPECT_CALL(*_messagingStubFactory, shutdown()).Times(1);
    _messageRouter->shutdown();
    _messageRouter = createMessageRouter();
    addressIsNotAddedToRoutingTable(ownAddress);

    auto websocketClientAddress =
            std::make_shared<const system::RoutingTypes::WebSocketClientAddress>();
    addressIsAddedToRoutingTable(websocketClientAddress);

    auto udsClientAddress = std::make_shared<const system::RoutingTypes::UdsClientAddress>();
    addressIsAddedToRoutingTable(udsClientAddress);

    auto inprocessAddress = std::make_shared<const InProcessMessagingAddress>();
    addressIsAddedToRoutingTable(inprocessAddress);
}

void CcMessageRouterTest::testRoutingEntryUpdate(
        const std::string& participantId,
        std::shared_ptr<const system::RoutingTypes::Address> newAddress,
        std::shared_ptr<const system::RoutingTypes::Address> expectedAddress)
{
    Semaphore semaphore(0);
    const TimePoint now = TimePoint::now();
    _mutableMessage.setExpiryDate(now + std::chrono::milliseconds(1024));
    _mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_REQUEST());
    _mutableMessage.setRecipient(participantId);
    std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();

    const bool isParticipantGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    _messageRouter->addNextHop(
            participantId, newAddress, isParticipantGloballyVisible, expiryDateMs, isSticky);

    auto mockMessagingStub = std::make_shared<MockMessagingStub>();
    EXPECT_CALL(*_messagingStubFactory, create(expectedAddress))
            .WillOnce(Return(mockMessagingStub));
    EXPECT_CALL(*mockMessagingStub, transmit(Eq(immutableMessage), _))
            .WillOnce(ReleaseSemaphore(&semaphore));

    _messageRouter->route(immutableMessage);

    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
    Mock::VerifyAndClearExpectations(_messagingStubFactory.get());
    Mock::VerifyAndClearExpectations(mockMessagingStub.get());
}

TEST_F(CcMessageRouterTest, addressValidation_allowUpdateOfInProcessAddress)
{
    // inProcessAddress can only be replaced with InProcessAddress
    // precedence: InProcessAddress > WebSocketClientAddress/UdsClientAddress >
    // MqttAddress > WebSocketAddress/UdsAddress
    const std::string testParticipantId = "allowInProcessUpdateParticipantId";
    auto dispatcher = std::make_shared<MockDispatcher>();
    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>(dispatcher);
    auto oldAddress = std::make_shared<const InProcessMessagingAddress>(skeleton);

    addNewRoutingEntry(testParticipantId, oldAddress);

    auto webSocketAddress = std::make_shared<const system::RoutingTypes::WebSocketAddress>();
    testRoutingEntryUpdate(testParticipantId, webSocketAddress, oldAddress);

    auto udsAddress = std::make_shared<const system::RoutingTypes::UdsAddress>();
    testRoutingEntryUpdate(testParticipantId, udsAddress, oldAddress);

    auto mqttAddress = std::make_shared<const system::RoutingTypes::MqttAddress>();
    testRoutingEntryUpdate(testParticipantId, mqttAddress, oldAddress);

    auto webSocketClientAddress =
            std::make_shared<const system::RoutingTypes::WebSocketClientAddress>();
    testRoutingEntryUpdate(testParticipantId, webSocketClientAddress, oldAddress);

    auto udsClientAddress = std::make_shared<const system::RoutingTypes::UdsClientAddress>();
    testRoutingEntryUpdate(testParticipantId, udsClientAddress, oldAddress);

    auto inProcessAddress = std::make_shared<const InProcessMessagingAddress>();
    testRoutingEntryUpdate(testParticipantId, inProcessAddress, inProcessAddress);

    // cleanup
    _messageRouter->removeNextHop(testParticipantId);
}

TEST_F(CcMessageRouterTest, addressValidation_allowUpdateOfWebSocketClientAddress)
{
    // precedence: InProcessAddress > WebSocketClientAddress/UdsClientAddress >
    // MqttAddress > WebSocketAddress/UdsAddress
    const std::string testParticipantId = "allowWebSocketClientUpdateParticipantId";
    auto oldAddress =
            std::make_shared<const system::RoutingTypes::WebSocketClientAddress>("testWebSocketId");

    addNewRoutingEntry(testParticipantId, oldAddress);

    auto webSocketAddress = std::make_shared<const system::RoutingTypes::WebSocketAddress>(
            system::RoutingTypes::WebSocketProtocol::WS, "host", 4242, "path");
    testRoutingEntryUpdate(testParticipantId, webSocketAddress, oldAddress);

    auto udsAddress = std::make_shared<const system::RoutingTypes::UdsAddress>("path");
    testRoutingEntryUpdate(testParticipantId, udsAddress, oldAddress);

    auto mqttAddress =
            std::make_shared<const system::RoutingTypes::MqttAddress>("brokerUri", "topic");
    testRoutingEntryUpdate(testParticipantId, mqttAddress, oldAddress);

    auto webSocketClientAddress =
            std::make_shared<const system::RoutingTypes::WebSocketClientAddress>("webSocketId");
    testRoutingEntryUpdate(testParticipantId, webSocketClientAddress, webSocketClientAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId);
    addNewRoutingEntry(testParticipantId, oldAddress);

    auto udsClientAddress = std::make_shared<const system::RoutingTypes::UdsClientAddress>("udsId");
    testRoutingEntryUpdate(testParticipantId, udsClientAddress, udsClientAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId);
    addNewRoutingEntry(testParticipantId, oldAddress);

    auto inProcessAddress = std::make_shared<const InProcessMessagingAddress>();
    testRoutingEntryUpdate(testParticipantId, inProcessAddress, inProcessAddress);

    // cleanup
    _messageRouter->removeNextHop(testParticipantId);
}

TEST_F(CcMessageRouterTest, addressValidation_allowUpdateOfUdsClientAddress)
{
    // precedence: InProcessAddress > WebSocketClientAddress/UdsClientAddress >
    // MqttAddress > WebSocketAddress/UdsAddress
    const std::string testParticipantId = "allowUdsClientUpdateParticipantId";
    auto oldAddress = std::make_shared<const system::RoutingTypes::UdsClientAddress>("testUdsId");

    addNewRoutingEntry(testParticipantId, oldAddress);

    auto webSocketAddress = std::make_shared<const system::RoutingTypes::WebSocketAddress>(
            system::RoutingTypes::WebSocketProtocol::WS, "host", 4242, "path");
    testRoutingEntryUpdate(testParticipantId, webSocketAddress, oldAddress);

    auto udsAddress = std::make_shared<const system::RoutingTypes::UdsAddress>("path");
    testRoutingEntryUpdate(testParticipantId, udsAddress, oldAddress);

    auto mqttAddress =
            std::make_shared<const system::RoutingTypes::MqttAddress>("brokerUri", "topic");
    testRoutingEntryUpdate(testParticipantId, mqttAddress, oldAddress);

    auto webSocketClientAddress =
            std::make_shared<const system::RoutingTypes::WebSocketClientAddress>("webSocketId");
    testRoutingEntryUpdate(testParticipantId, webSocketClientAddress, webSocketClientAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId);
    addNewRoutingEntry(testParticipantId, oldAddress);

    auto udsClientAddress = std::make_shared<const system::RoutingTypes::UdsClientAddress>("udsId");
    testRoutingEntryUpdate(testParticipantId, udsClientAddress, udsClientAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId);
    addNewRoutingEntry(testParticipantId, oldAddress);

    auto inProcessAddress = std::make_shared<const InProcessMessagingAddress>();
    testRoutingEntryUpdate(testParticipantId, inProcessAddress, inProcessAddress);

    // cleanup
    _messageRouter->removeNextHop(testParticipantId);
}

TEST_F(CcMessageRouterTest, addressValidation_allowUpdateOfMqttAddress)
{
    // precedence: InProcessAddress > WebSocketClientAddress/UdsClientAddress >
    // MqttAddress > WebSocketAddress/UdsAddress
    const std::string testParticipantId = "allowMqttUpdateParticipantId";
    auto oldAddress =
            std::make_shared<const system::RoutingTypes::MqttAddress>("testbrokerUri", "testTopic");

    addNewRoutingEntry(testParticipantId, oldAddress);

    auto webSocketAddress = std::make_shared<const system::RoutingTypes::WebSocketAddress>(
            system::RoutingTypes::WebSocketProtocol::WS, "host", 4242, "path");
    testRoutingEntryUpdate(testParticipantId, webSocketAddress, oldAddress);

    auto udsAddress = std::make_shared<const system::RoutingTypes::UdsAddress>("path");
    testRoutingEntryUpdate(testParticipantId, udsAddress, oldAddress);

    auto mqttAddress =
            std::make_shared<const system::RoutingTypes::MqttAddress>("brokerUri", "topic");
    testRoutingEntryUpdate(testParticipantId, mqttAddress, mqttAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId);
    addNewRoutingEntry(testParticipantId, oldAddress);

    auto webSocketClientAddress =
            std::make_shared<const system::RoutingTypes::WebSocketClientAddress>("webSocketId");
    testRoutingEntryUpdate(testParticipantId, webSocketClientAddress, webSocketClientAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId);
    addNewRoutingEntry(testParticipantId, oldAddress);

    auto udsClientAddress = std::make_shared<const system::RoutingTypes::UdsClientAddress>("udsId");
    testRoutingEntryUpdate(testParticipantId, udsClientAddress, udsClientAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId);
    addNewRoutingEntry(testParticipantId, oldAddress);

    auto inProcessAddress = std::make_shared<const InProcessMessagingAddress>();
    testRoutingEntryUpdate(testParticipantId, inProcessAddress, inProcessAddress);

    // cleanup
    _messageRouter->removeNextHop(testParticipantId);
}

TEST_F(CcMessageRouterTest, DISABLED_addressValidation_allowUpdateOfWebSocketAddress)
{
    // Disabled: WebSocketAddress is not allowed in CcMessageRouter
    // Precedence cannot be tested without refactoring MessageRouter and RoutingTable, e.g. like in
    // Java

    // precedence: InProcessAddress > WebSocketClientAddress > MqttAddress > WebSocketAddress
    const std::string testParticipantId = "allowWebSocketUpdateParticipantId";
    auto oldAddress = std::make_shared<const system::RoutingTypes::WebSocketAddress>(
            system::RoutingTypes::WebSocketProtocol::WSS, "testHost", 23, "testPath");

    addNewRoutingEntry(testParticipantId, oldAddress);

    auto webSocketAddress = std::make_shared<const system::RoutingTypes::WebSocketAddress>(
            system::RoutingTypes::WebSocketProtocol::WS, "host", 4242, "path");
    testRoutingEntryUpdate(testParticipantId, webSocketAddress, webSocketAddress);

    auto udsAddress = std::make_shared<const system::RoutingTypes::UdsAddress>("path");
    testRoutingEntryUpdate(testParticipantId, udsAddress, udsAddress);

    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId);
    addNewRoutingEntry(testParticipantId, oldAddress);

    auto mqttAddress =
            std::make_shared<const system::RoutingTypes::MqttAddress>("brokerUri", "topic");
    testRoutingEntryUpdate(testParticipantId, mqttAddress, mqttAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId);
    addNewRoutingEntry(testParticipantId, oldAddress);

    auto webSocketClientAddress =
            std::make_shared<const system::RoutingTypes::WebSocketClientAddress>("webSocketId");
    testRoutingEntryUpdate(testParticipantId, webSocketClientAddress, webSocketClientAddress);

    auto udsClientAddress = std::make_shared<const system::RoutingTypes::UdsClientAddress>("udsId");
    testRoutingEntryUpdate(testParticipantId, udsClientAddress, udsClientAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId);
    addNewRoutingEntry(testParticipantId, oldAddress);

    auto inProcessAddress = std::make_shared<const InProcessMessagingAddress>();
    testRoutingEntryUpdate(testParticipantId, inProcessAddress, inProcessAddress);

    // cleanup
    _messageRouter->removeNextHop(testParticipantId);
}

TEST_F(CcMessageRouterTest, DISABLED_addressValidation_allowUpdateOfUdsAddress)
{
    // Disabled: WebSocketAddress/UdsAddress is not allowed in CcMessageRouter
    // Precedence cannot be tested without refactoring MessageRouter and RoutingTable, e.g. like in
    // Java

    // precedence: InProcessAddress > WebSocketClientAddress/UdsClientAddress >
    // MqttAddress > WebSocketAddress/UdsAddress
    const std::string testParticipantId = "allowUdsUpdateParticipantId";
    auto oldAddress = std::make_shared<const system::RoutingTypes::UdsAddress>("testPath");

    addNewRoutingEntry(testParticipantId, oldAddress);

    auto udsAddress = std::make_shared<const system::RoutingTypes::UdsAddress>("path");
    testRoutingEntryUpdate(testParticipantId, udsAddress, udsAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId);
    addNewRoutingEntry(testParticipantId, oldAddress);

    auto mqttAddress =
            std::make_shared<const system::RoutingTypes::MqttAddress>("brokerUri", "topic");
    testRoutingEntryUpdate(testParticipantId, mqttAddress, mqttAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId);
    addNewRoutingEntry(testParticipantId, oldAddress);

    auto udsClientAddress = std::make_shared<const system::RoutingTypes::UdsClientAddress>("udsId");
    testRoutingEntryUpdate(testParticipantId, udsClientAddress, udsClientAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId);
    addNewRoutingEntry(testParticipantId, oldAddress);

    auto inProcessAddress = std::make_shared<const InProcessMessagingAddress>();
    testRoutingEntryUpdate(testParticipantId, inProcessAddress, inProcessAddress);

    // cleanup
    _messageRouter->removeNextHop(testParticipantId);
}

TEST_F(CcMessageRouterTest,
       checkIfMessageIsNotSentIfAddressIsNotAllowedInRoutingTable_webSocketAddress)
{
    const std::string destinationParticipantId = "TEST_routeMessageToMqttAddress";

    auto webSocketAddress = std::make_shared<const system::RoutingTypes::WebSocketAddress>(
            system::RoutingTypes::WebSocketProtocol::WS, "host", 4242, "path");
    const bool isGloballyVisible = true;
    const bool isSticky = false;
    const TimePoint now = TimePoint::now();
    this->_mutableMessage.setExpiryDate(now + std::chrono::seconds(500));
    this->_mutableMessage.setRecipient(destinationParticipantId);
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();

    std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();
    this->_messageRouter->route(immutableMessage);

    EXPECT_EQ(this->_messageQueue->getQueueLength(), 1);
    EXPECT_EQ(this->_messageRouter->getNumberOfRoutedMessages(), 1);

    this->_messageRouter->addNextHop(
            destinationParticipantId, webSocketAddress, isGloballyVisible, expiryDateMs, isSticky);

    EXPECT_EQ(this->_messageQueue->getQueueLength(), 1);
    EXPECT_EQ(this->_messageRouter->getNumberOfRoutedMessages(), 1);
}

TEST_F(CcMessageRouterTest, checkIfMessageIsNotSendIfAddressIsNotAllowedInRoutingTable_udsAddress)
{
    const std::string destinationParticipantId = "TEST_routeMessageToMqttAddress";

    auto udsAddress = std::make_shared<const system::RoutingTypes::UdsAddress>("path");
    const bool isGloballyVisible = true;
    const bool isSticky = false;
    const TimePoint now = TimePoint::now();
    this->_mutableMessage.setExpiryDate(now + std::chrono::seconds(500));
    this->_mutableMessage.setRecipient(destinationParticipantId);
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();

    std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();
    this->_messageRouter->route(immutableMessage);

    EXPECT_EQ(this->_messageQueue->getQueueLength(), 1);
    EXPECT_EQ(this->_messageRouter->getNumberOfRoutedMessages(), 1);

    this->_messageRouter->addNextHop(
            destinationParticipantId, udsAddress, isGloballyVisible, expiryDateMs, isSticky);

    EXPECT_EQ(this->_messageQueue->getQueueLength(), 1);
    EXPECT_EQ(this->_messageRouter->getNumberOfRoutedMessages(), 1);
}

TEST_F(CcMessageRouterTest, subscriptionStopIsSentWhenProxyIsUnreachable_webSocketClientAddress)
{
    const std::string subscriberParticipantId("subscriberPartId");
    const std::string providerParticipantId("providerParticipantId");
    const std::string consumerRuntimeWebSocketClientAddressString(
            "ConsumerRuntimeWebSocketAddress1");
    const std::string providerRuntimeWebSocketClientAddressString(
            "ProviderRuntimeWebSocketAddress1");

    auto subscriberAddress =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
                    consumerRuntimeWebSocketClientAddressString);
    auto providerAddress =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
                    providerRuntimeWebSocketClientAddressString);

    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    const bool isProviderGloballyVisible = false;

    auto mockMessageSender = std::make_shared<MockMessageSender>();

    _messageRouter->setMessageSender(mockMessageSender);

    _messageRouter->addNextHop(subscriberParticipantId,
                               subscriberAddress,
                               _DEFAULT_IS_GLOBALLY_VISIBLE,
                               expiryDateMs,
                               isSticky);
    _messageRouter->addNextHop(providerParticipantId,
                               providerAddress,
                               isProviderGloballyVisible,
                               expiryDateMs,
                               isSticky);

    // Prepare a SubcriptionPublication message from provider to subscriber

    std::string subscriptionId = "subscriptionId";
    MutableMessageFactory mutableMessageFactory;
    MessagingQos qos = MessagingQos(456000);
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(subscriptionId);
    MutableMessage subscriptionPublicationMutableMessage =
            mutableMessageFactory.createSubscriptionPublication(
                    providerParticipantId, subscriberParticipantId, qos, subscriptionPublication);

    // consumer runtime is unreachable, so the creation of a WebSocketMessagingStub fails;
    // the factory returns an empty IMessagingStub in this case

    EXPECT_CALL(
            *_messagingStubFactory,
            create(Property(
                    &std::shared_ptr<const joynr::system::RoutingTypes::Address>::get,
                    WhenDynamicCastTo<const joynr::system::RoutingTypes::WebSocketClientAddress*>(
                            Pointee(Property(
                                    &joynr::system::RoutingTypes::WebSocketClientAddress::getId,
                                    Eq(consumerRuntimeWebSocketClientAddressString)))))))
            .Times(1)
            .WillRepeatedly(Return(std::shared_ptr<IMessagingStub>()));

    // messageSender should be invoked to send faked SubscriptionStop message for same
    // subscriptionId from subscriber to the provider to end the ghost subscription

    EXPECT_CALL(*mockMessageSender,
                sendSubscriptionStop(Eq(subscriberParticipantId), // sender participantId
                                     Eq(providerParticipantId),   // recipient participantId
                                     _,                           // MessagingQos
                                     Property(&SubscriptionStop::getSubscriptionId,
                                              Eq(subscriptionId)) // SubscriptionStop object to send
                                     ))
            .Times(1);

    _messageRouter->route(subscriptionPublicationMutableMessage.getImmutableMessage());
    Mock::VerifyAndClearExpectations(_messagingStubFactory.get());

    // cleanup
    _messageRouter->removeNextHop(subscriberParticipantId);
    _messageRouter->removeNextHop(providerParticipantId);
}

TEST_F(CcMessageRouterTest, subscriptionStopIsSentWhenProxyIsUnreachable_udsClientAddress)
{
    const std::string subscriberParticipantId("subscriberPartId");
    const std::string providerParticipantId("providerParticipantId");
    const std::string consumerRuntimeUdsClientAddressString("ConsumerRuntimeUdsAddress1");
    const std::string providerRuntimeUdsClientAddressString("ProviderRuntimeUdsAddress1");

    auto subscriberAddress = std::make_shared<const joynr::system::RoutingTypes::UdsClientAddress>(
            consumerRuntimeUdsClientAddressString);
    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::UdsClientAddress>(
            providerRuntimeUdsClientAddressString);

    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    const bool isProviderGloballyVisible = false;

    auto mockMessageSender = std::make_shared<MockMessageSender>();

    _messageRouter->setMessageSender(mockMessageSender);

    _messageRouter->addNextHop(subscriberParticipantId,
                               subscriberAddress,
                               _DEFAULT_IS_GLOBALLY_VISIBLE,
                               expiryDateMs,
                               isSticky);
    _messageRouter->addNextHop(providerParticipantId,
                               providerAddress,
                               isProviderGloballyVisible,
                               expiryDateMs,
                               isSticky);

    // Prepare a SubcriptionPublication message from provider to subscriber

    std::string subscriptionId = "subscriptionId";
    MutableMessageFactory mutableMessageFactory;
    MessagingQos qos = MessagingQos(456000);
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(subscriptionId);
    MutableMessage subscriptionPublicationMutableMessage =
            mutableMessageFactory.createSubscriptionPublication(
                    providerParticipantId, subscriberParticipantId, qos, subscriptionPublication);

    // consumer runtime is unreachable, so the creation of a UdsMessagingStub fails;
    // the factory returns an empty IMessagingStub in this case

    EXPECT_CALL(
            *_messagingStubFactory,
            create(Property(
                    &std::shared_ptr<const joynr::system::RoutingTypes::Address>::get,
                    WhenDynamicCastTo<const joynr::system::RoutingTypes::UdsClientAddress*>(
                            Pointee(Property(&joynr::system::RoutingTypes::UdsClientAddress::getId,
                                             Eq(consumerRuntimeUdsClientAddressString)))))))
            .Times(1)
            .WillRepeatedly(Return(std::shared_ptr<IMessagingStub>()));

    // messageSender should be invoked to send faked SubscriptionStop message for same
    // subscriptionId from subscriber to the provider to end the ghost subscription

    EXPECT_CALL(*mockMessageSender,
                sendSubscriptionStop(Eq(subscriberParticipantId), // sender participantId
                                     Eq(providerParticipantId),   // recipient participantId
                                     _,                           // MessagingQos
                                     Property(&SubscriptionStop::getSubscriptionId,
                                              Eq(subscriptionId)) // SubscriptionStop object to send
                                     ))
            .Times(1);

    _messageRouter->route(subscriptionPublicationMutableMessage.getImmutableMessage());
    Mock::VerifyAndClearExpectations(_messagingStubFactory.get());

    // cleanup
    _messageRouter->removeNextHop(subscriberParticipantId);
    _messageRouter->removeNextHop(providerParticipantId);
}

TEST_F(CcMessageRouterTest, routingTableRemoveEntriesWorksForWebsocket)
{
    const std::string providerParticipantId1("providerParticipantId1");
    const std::string providerParticipantId2("providerParticipantId2");
    const std::string providerParticipantId3("providerParticipantId3");

    EXPECT_CALL(*_messagingStubFactory, shutdown()).Times(1);
    _messageRouter->shutdown();
    _messageRouter = createMessageRouter();

    auto wsClientAddress1 =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
                    "ws-client-id-1");
    auto wsClientAddress2 =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
                    "ws-client-id-2");

    const bool isGloballyVisible = true;
    const bool isSticky = false;
    std::int64_t expiryDateMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::system_clock::now().time_since_epoch())
                                        .count() +
                                4000;
    _messageRouter->addNextHop(
            providerParticipantId1, wsClientAddress1, isGloballyVisible, expiryDateMs, isSticky);
    _messageRouter->addNextHop(
            providerParticipantId2, wsClientAddress1, isGloballyVisible, expiryDateMs, isSticky);
    _messageRouter->addNextHop(
            providerParticipantId3, wsClientAddress2, isGloballyVisible, expiryDateMs, isSticky);

    // all providers should be around
    checkResolveNextHop(providerParticipantId1, true);
    checkResolveNextHop(providerParticipantId2, true);
    checkResolveNextHop(providerParticipantId3, true);

    _messageRouter->removeRoutingEntries(wsClientAddress1);

    // providerParticipantId1 and 2 should have been removed
    checkResolveNextHop(providerParticipantId1, false);
    checkResolveNextHop(providerParticipantId2, false);

    // providerParticipantId3 should still be around
    checkResolveNextHop(providerParticipantId3, true);
}

TEST_F(CcMessageRouterTest, routingTableRemoveEntriesWorksForUds)
{
    const std::string providerParticipantId1("providerParticipantId1");
    const std::string providerParticipantId2("providerParticipantId2");
    const std::string providerParticipantId3("providerParticipantId3");

    EXPECT_CALL(*_messagingStubFactory, shutdown()).Times(1);
    _messageRouter->shutdown();
    _messageRouter = createMessageRouter();

    auto udsClientAddress1 = std::make_shared<const joynr::system::RoutingTypes::UdsClientAddress>(
            "uds-client-id-1");
    auto udsClientAddress2 = std::make_shared<const joynr::system::RoutingTypes::UdsClientAddress>(
            "uds-client-id-2");

    const bool isGloballyVisible = true;
    const bool isSticky = false;
    std::int64_t expiryDateMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::system_clock::now().time_since_epoch())
                                        .count() +
                                4000;
    _messageRouter->addNextHop(
            providerParticipantId1, udsClientAddress1, isGloballyVisible, expiryDateMs, isSticky);
    _messageRouter->addNextHop(
            providerParticipantId2, udsClientAddress1, isGloballyVisible, expiryDateMs, isSticky);
    _messageRouter->addNextHop(
            providerParticipantId3, udsClientAddress2, isGloballyVisible, expiryDateMs, isSticky);

    // all providers should be around
    checkResolveNextHop(providerParticipantId1, true);
    checkResolveNextHop(providerParticipantId2, true);
    checkResolveNextHop(providerParticipantId3, true);

    _messageRouter->removeRoutingEntries(udsClientAddress1);

    // providerParticipantId1 and 2 should have been removed
    checkResolveNextHop(providerParticipantId1, false);
    checkResolveNextHop(providerParticipantId2, false);

    // providerParticipantId3 should still be around
    checkResolveNextHop(providerParticipantId3, true);
}

TEST_F(CcMessageRouterTest, accessControllerIsCalledWithCorrectIsLocalRecipient)
{
    const std::string providerParticipantId1("providerParticipantId1");
    const std::string providerParticipantId2("providerParticipantId2");
    const std::string providerParticipantId3("providerParticipantId3");
    const std::string providerParticipantId4("providerParticipantId4");

    auto providerAddress1 =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>();
    auto providerAddress2 = std::make_shared<const joynr::system::RoutingTypes::UdsClientAddress>();
    auto providerAddress3 = std::make_shared<const joynr::InProcessMessagingAddress>();
    auto providerAddress4 = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(
            "testGbid", "testTopic");

    const bool isSticky = false;
    const std::int64_t expiryDate = std::numeric_limits<std::int64_t>::max();
    _messageRouter->addNextHop(providerParticipantId1, providerAddress1,
                               _DEFAULT_IS_GLOBALLY_VISIBLE, expiryDate, isSticky);
    _messageRouter->addNextHop(providerParticipantId2, providerAddress2,
                               _DEFAULT_IS_GLOBALLY_VISIBLE, expiryDate, isSticky);
    _messageRouter->addNextHop(providerParticipantId3, providerAddress3,
                               _DEFAULT_IS_GLOBALLY_VISIBLE, expiryDate, isSticky);
    _messageRouter->addNextHop(providerParticipantId4, providerAddress4,
                               _DEFAULT_IS_GLOBALLY_VISIBLE, expiryDate, isSticky);

    // setup the messages
    auto localMutableMessage = std::make_shared<MutableMessage>();
    localMutableMessage->setType(joynr::Message::VALUE_MESSAGE_TYPE_REQUEST());
    localMutableMessage->setSender("sender");
    const TimePoint nowTime = TimePoint::now();
    localMutableMessage->setExpiryDate(nowTime + std::chrono::milliseconds(16000));

    localMutableMessage->setRecipient(providerParticipantId1);
    std::shared_ptr<ImmutableMessage> immutableMessage1 =
            localMutableMessage->getImmutableMessage();
    localMutableMessage->setRecipient(providerParticipantId2);
    std::shared_ptr<ImmutableMessage> immutableMessage2 =
            localMutableMessage->getImmutableMessage();
    localMutableMessage->setRecipient(providerParticipantId3);
    std::shared_ptr<ImmutableMessage> immutableMessage3 =
            localMutableMessage->getImmutableMessage();
    localMutableMessage->setRecipient(providerParticipantId4);
    std::shared_ptr<ImmutableMessage> immutableMessage4 =
            localMutableMessage->getImmutableMessage();

    auto mockAccessController = std::make_shared<MockAccessController>();

    const bool isLocalRecipient = true;
    // handle the rest
    EXPECT_CALL(*mockAccessController, hasConsumerPermission(_, _, _)).Times(0);
    // unless handled by specifc expectations
    EXPECT_CALL(*mockAccessController,
                hasConsumerPermission(Eq(immutableMessage1), _, isLocalRecipient))
            .Times(1);
    EXPECT_CALL(*mockAccessController,
                hasConsumerPermission(Eq(immutableMessage2), _, isLocalRecipient))
            .Times(1);
    EXPECT_CALL(*mockAccessController,
                hasConsumerPermission(Eq(immutableMessage3), _, isLocalRecipient))
            .Times(1);
    EXPECT_CALL(*mockAccessController,
                hasConsumerPermission(Eq(immutableMessage4), _, !isLocalRecipient))
            .Times(1);

    _messageRouter->setAccessController(util::as_weak_ptr(mockAccessController));

    this->_messageRouter->route(immutableMessage1);
    this->_messageRouter->route(immutableMessage2);
    this->_messageRouter->route(immutableMessage3);
    this->_messageRouter->route(immutableMessage4);
}
