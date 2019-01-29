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

#include "MessageRouterTest.h"

#include <chrono>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/InProcessMessagingAddress.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/BrowserAddress.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"

#include "tests/mock/MockDispatcher.h"
#include "tests/mock/MockInProcessMessagingSkeleton.h"
#include "tests/mock/MockJoynrRuntime.h"
#include "tests/mock/MockMessagingStub.h"
#include "tests/mock/MockRoutingProxy.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Eq;
using ::testing::InSequence;
using ::testing::InvokeArgument;
using ::testing::Mock;
using ::testing::Pointee;
using ::testing::Return;

using namespace joynr;

class LibJoynrMessageRouterTest : public MessageRouterTest<LibJoynrMessageRouter>
{
public:
    LibJoynrMessageRouterTest() = default;

    void SetUp()
    {
        auto settings = std::make_unique<Settings>();
        runtime = std::make_shared<MockJoynrRuntime>(std::move(settings));
    }

    void TearDown()
    {
        runtime.reset();
    }

    void removeFromQueue(const std::string& participantId,
                         std::function<void(const bool& resolved)> onSuccess = nullptr,
                         std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError = nullptr,
                         boost::optional<joynr::MessagingQos> qos = boost::none);

protected:
    void testAddNextHopCallsRoutingProxyCorrectly(
            const bool isGloballyVisible,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress);
    void addressIsNotAddedToRoutingTable(const std::shared_ptr<const system::RoutingTypes::Address> address);
    void addressIsAddedToRoutingTable(const std::shared_ptr<const system::RoutingTypes::Address> address);
    const bool isGloballyVisible = false;
    std::shared_ptr<MockJoynrRuntime> runtime;
};

TEST_F(LibJoynrMessageRouterTest, routeMessageToWebSocketAddress)
{
    const std::string destinationParticipantId = "TEST_routeMessageToWebSocketAddress";
    auto address = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>();
    const bool isGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    const bool allowUpdate = false;

    this->messageRouter->addNextHop(destinationParticipantId,
                                    address,
                                    isGloballyVisible,
                                    expiryDateMs,
                                    isSticky,
                                    allowUpdate);
    this->mutableMessage.setRecipient(destinationParticipantId);

    EXPECT_CALL(*(this->messagingStubFactory), create(Pointee(Eq(*address)))).Times(1);

    this->routeMessageToAddress();
}

TEST_F(LibJoynrMessageRouterTest,
       routeMulticastMessageFromLocalProvider_multicastMsgIsSentToAllMulticastReceivers)
{
    const std::string subscriberParticipantId1("subscriberPartId");
    const std::string providerParticipantId("providerParticipantId");
    const std::string multicastNameAndPartitions("multicastName/partition0");
    const std::string multicastId(providerParticipantId + "/" + multicastNameAndPartitions);
    const std::shared_ptr<const joynr::InProcessMessagingAddress> inProcessSubscriberAddress =
            std::make_shared<const joynr::InProcessMessagingAddress>();
    messageRouter->addProvisionedNextHop(providerParticipantId, localTransport, isGloballyVisible);
    messageRouter->addProvisionedNextHop(
            subscriberParticipantId1, inProcessSubscriberAddress, isGloballyVisible);

    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(runtime);
    ON_CALL(*mockRoutingProxy, addMulticastReceiverAsyncMock(_, _, _, _, _, _))
            .WillByDefault(DoAll(InvokeArgument<3>(), Return(nullptr)));

    messageRouter->setParentAddress(std::string("parentParticipantId"), localTransport);
    messageRouter->setParentRouter(std::move(mockRoutingProxy));

    messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId1,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*localTransport)))).Times(1);
    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*inProcessSubscriberAddress)))).Times(1);

    mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_MULTICAST());
    mutableMessage.setSender(providerParticipantId);
    mutableMessage.setRecipient(multicastId);

    std::shared_ptr<joynr::ImmutableMessage> immutableMessage =
            mutableMessage.getImmutableMessage();
    // The message should be propagated to parentMessageRouter
    immutableMessage->setReceivedFromGlobal(false);

    messageRouter->route(immutableMessage);
}

TEST_F(LibJoynrMessageRouterTest,
       addMulticastReceiver_callsParentRouterIfProviderAddressNotAvailable)
{
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(runtime);

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    EXPECT_CALL(*mockRoutingProxy, resolveNextHopAsyncMock(providerParticipantId, _, _, _))
            .WillOnce(DoAll(
                    InvokeArgument<2>(joynr::exceptions::JoynrRuntimeException("testException")),
                    Return(nullptr)))
            .WillOnce(DoAll(InvokeArgument<1>(false), Return(nullptr)))
            .WillOnce(DoAll(InvokeArgument<1>(true), Return(nullptr)));

    EXPECT_CALL(*mockRoutingProxy,
                addMulticastReceiverAsyncMock(
                        multicastId, subscriberParticipantId, providerParticipantId, _, _, _))
            .WillOnce(DoAll(InvokeArgument<3>(), Return(nullptr)));
    ;

    messageRouter->setParentAddress(std::string("parentParticipantId"), localTransport);
    messageRouter->setParentRouter(std::move(mockRoutingProxy));

    Semaphore errorCallbackCalled;
    messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            []() { FAIL() << "onSuccess called"; },
            [&errorCallbackCalled](const joynr::exceptions::ProviderRuntimeException&) {
                errorCallbackCalled.notify();
            });
    EXPECT_TRUE(errorCallbackCalled.waitFor(std::chrono::milliseconds(5000)));

    messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            []() { FAIL() << "onSuccess called"; },
            [&errorCallbackCalled](const joynr::exceptions::ProviderRuntimeException&) {
                errorCallbackCalled.notify();
            });
    EXPECT_TRUE(errorCallbackCalled.waitFor(std::chrono::milliseconds(5000)));

    Semaphore successCallbackCalled;
    messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            [&successCallbackCalled]() { successCallbackCalled.notify(); },
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(LibJoynrMessageRouterTest, removeMulticastReceiver_CallsParentRouter)
{
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(runtime);
    auto mockRoutingProxyRef = mockRoutingProxy.get();

    messageRouter->setParentAddress(std::string("parentParticipantId"), localTransport);
    messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    messageRouter->addProvisionedNextHop(providerParticipantId, localTransport, isGloballyVisible);

    messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    // Call shall be forwarded to the parent proxy
    EXPECT_CALL(*mockRoutingProxyRef,
                removeMulticastReceiverAsyncMock(
                        multicastId, subscriberParticipantId, providerParticipantId, _, _, _));

    messageRouter->removeMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
}

TEST_F(LibJoynrMessageRouterTest, removeMulticastReceiverOfInProcessProvider_callsParentRouter)
{
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(runtime);
    auto mockRoutingProxyRef = mockRoutingProxy.get();

    messageRouter->setParentAddress(std::string("parentParticipantId"), localTransport);
    messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    auto dispatcher = std::make_shared<MockDispatcher>();
    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>(dispatcher);
    auto providerAddress = std::make_shared<const joynr::InProcessMessagingAddress>(skeleton);
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress, isGloballyVisible);

    messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    EXPECT_CALL(*mockRoutingProxyRef,
                removeMulticastReceiverAsyncMock(
                        multicastId, subscriberParticipantId, providerParticipantId, _, _, _))
            .Times(1)
            .WillOnce(DoAll(InvokeArgument<3>(), Return(nullptr)));

    Semaphore successCallbackCalled;
    messageRouter->removeMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            [&successCallbackCalled]() { successCallbackCalled.notify(); },
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(LibJoynrMessageRouterTest, addMulticastReceiver_callsParentRouter)
{
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(runtime);
    auto mockRoutingProxyRef = mockRoutingProxy.get();

    messageRouter->setParentAddress(std::string("parentParticipantId"), localTransport);
    messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");
    messageRouter->addProvisionedNextHop(providerParticipantId, localTransport, isGloballyVisible);

    // Call shall be forwarded to the parent proxy
    EXPECT_CALL(*mockRoutingProxyRef,
                addMulticastReceiverAsyncMock(
                        multicastId, subscriberParticipantId, providerParticipantId, _, _, _));

    messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
}

TEST_F(LibJoynrMessageRouterTest, addMulticastReceiverForInProcessProvider_DoesNotCallParentRouter)
{
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(runtime);
    auto mockRoutingProxyRef = mockRoutingProxy.get();

    messageRouter->setParentAddress(std::string("parentParticipantId"), localTransport);
    messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");

    const std::string providerParticipantId("providerParticipantId");
    auto dispatcher = std::make_shared<MockDispatcher>();
    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>(dispatcher);
    auto providerAddress = std::make_shared<const joynr::InProcessMessagingAddress>(skeleton);
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress, isGloballyVisible);

    EXPECT_CALL(*mockRoutingProxyRef,
                addMulticastReceiverAsyncMock(
                        multicastId, subscriberParticipantId, providerParticipantId, _, _, _))
            .Times(0);

    Semaphore successCallbackCalled;
    messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            [&successCallbackCalled]() { successCallbackCalled.notify(); },
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

void LibJoynrMessageRouterTest::testAddNextHopCallsRoutingProxyCorrectly(
        const bool isGloballyVisible,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress)
{
    const std::string providerParticipantId("providerParticipantId");
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(runtime);
    const std::string proxyParticipantId = mockRoutingProxy->getProxyParticipantId();

    {
        InSequence inSequence;
        EXPECT_CALL(*mockRoutingProxy, addNextHopAsyncMock(Eq(proxyParticipantId), _, _, _, _, _));
        // call under test
        EXPECT_CALL(*mockRoutingProxy,
                    addNextHopAsyncMock(Eq(providerParticipantId),
                                        Eq(*webSocketClientAddress),
                                        Eq(isGloballyVisible),
                                        _,
                                        _,
                                        _));
    }

    messageRouter->setParentAddress(std::string("parentParticipantId"), localTransport);

    messageRouter->setParentRouter(std::move(mockRoutingProxy));

    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    const bool allowUpdate = false;

    messageRouter->addNextHop(providerParticipantId,
                              providerAddress,
                              isGloballyVisible,
                              expiryDateMs,
                              isSticky,
                              allowUpdate);
}

TEST_F(LibJoynrMessageRouterTest, addNextHop_callsAddNextHopInRoutingProxy)
{
    bool isGloballyVisible;

    // InprocessMessagingAddress
    auto dispatcher = std::make_shared<MockDispatcher>();
    auto mockSkeleton = std::make_shared<MockInProcessMessagingSkeleton>(dispatcher);
    const auto providerAddress2 =
            std::make_shared<const joynr::InProcessMessagingAddress>(mockSkeleton);
    isGloballyVisible = false;
    testAddNextHopCallsRoutingProxyCorrectly(isGloballyVisible, providerAddress2);
    isGloballyVisible = true;
    testAddNextHopCallsRoutingProxyCorrectly(isGloballyVisible, providerAddress2);
}

TEST_F(LibJoynrMessageRouterTest, setToKnown_addsParentAddress)
{
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(runtime);
    MockRoutingProxy* mockRoutingProxyRef = mockRoutingProxy.get();
    ON_CALL(*mockRoutingProxy, resolveNextHopAsyncMock(_,_,_,_))
            .WillByDefault(
                DoAll(
                    Invoke(this, &LibJoynrMessageRouterTest::removeFromQueue),
                    InvokeArgument<1>(false),
                    Return(nullptr)));
    messageRouter->setParentAddress(std::string("parentParticipantId"), localTransport);
    messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string providerParticipantId = "providerParticipantId";
    mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_REQUEST());
    mutableMessage.setSender("sender");
    mutableMessage.setRecipient(providerParticipantId);

    EXPECT_CALL(*mockRoutingProxyRef, resolveNextHopAsyncMock(providerParticipantId,_,_,_));
    EXPECT_CALL(*messagingStubFactory, create(_)).Times(0);

    messageRouter->route(mutableMessage.getImmutableMessage());

    ASSERT_EQ(0, messageQueue->getQueueLength());

    Mock::VerifyAndClearExpectations(messagingStubFactory.get());
    Mock::VerifyAndClearExpectations(mockRoutingProxyRef);

    messageRouter->setToKnown(providerParticipantId);

    EXPECT_CALL(*mockRoutingProxyRef, resolveNextHopAsyncMock(Eq(providerParticipantId),_,_,_))
            .Times(0);
    auto mockMessagingStub = std::make_shared<MockMessagingStub>();
    EXPECT_CALL(*messagingStubFactory, create(Eq(localTransport))).WillOnce(Return(mockMessagingStub));

    messageRouter->route(mutableMessage.getImmutableMessage());

    ASSERT_EQ(0, messageQueue->getQueueLength());
}

TEST_F(LibJoynrMessageRouterTest, checkAllowUpdateTrue)
{
    const bool allowUpdate = true;
    const bool updateExpected = false;
    this->checkAllowUpdate(allowUpdate, updateExpected);
}
TEST_F(LibJoynrMessageRouterTest, checkAllowUpdateFalse)
{
    const bool allowUpdate = false;
    const bool updateExpected = false;
    this->checkAllowUpdate(allowUpdate, updateExpected);
}

void LibJoynrMessageRouterTest::removeFromQueue(const std::string& participantId,
                     std::function<void(const bool& resolved)> onSuccess,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
                     boost::optional<joynr::MessagingQos> qos)
{
    std::ignore = onSuccess;
    std::ignore = onRuntimeError;
    std::ignore = qos;

    ASSERT_EQ(1, messageQueue->getQueueLength());
    messageQueue->getNextMessageFor(participantId);
    ASSERT_EQ(0, messageQueue->getQueueLength());
}

void LibJoynrMessageRouterTest::addressIsNotAddedToRoutingTable(
        const std::shared_ptr<const system::RoutingTypes::Address> address)
{
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(runtime);
    MockRoutingProxy* mockRoutingProxyRef = mockRoutingProxy.get();
    ON_CALL(*mockRoutingProxy, resolveNextHopAsyncMock(_,_,_,_))
            .WillByDefault(DoAll(Invoke(this, &LibJoynrMessageRouterTest::removeFromQueue), InvokeArgument<1>(false), Return(nullptr)));

    messageRouter->setParentAddress(std::string("parentParticipantId"), localTransport);
    messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string consumerParticipantId("consumerParticipantId");
    const std::string providerParticipantId("providerParticipantId");
    const bool isRecipientGloballyVisible = true;

    const TimePoint now = TimePoint::now();
    mutableMessage.setExpiryDate(now + std::chrono::milliseconds(1000));
    mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_REQUEST());
    mutableMessage.setSender(consumerParticipantId);
    mutableMessage.setRecipient(providerParticipantId);

    EXPECT_CALL(*mockRoutingProxyRef, resolveNextHopAsyncMock(Eq(providerParticipantId),_,_,_));
    EXPECT_CALL(*messagingStubFactory, create(_)).Times(0);

    messageRouter->route(mutableMessage.getImmutableMessage());

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    ASSERT_EQ(0, messageQueue->getQueueLength());
    Mock::VerifyAndClearExpectations(messagingStubFactory.get());
    Mock::VerifyAndClearExpectations(mockRoutingProxyRef);

    messageRouter->addProvisionedNextHop(
            providerParticipantId, address, isRecipientGloballyVisible);

    EXPECT_CALL(*mockRoutingProxyRef, resolveNextHopAsyncMock(Eq(providerParticipantId),_,_,_));
    EXPECT_CALL(*messagingStubFactory, create(_)).Times(0);

    messageRouter->route(mutableMessage.getImmutableMessage());

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    ASSERT_EQ(0, messageQueue->getQueueLength());
    Mock::VerifyAndClearExpectations(messagingStubFactory.get());
    Mock::VerifyAndClearExpectations(mockRoutingProxyRef);
}

void LibJoynrMessageRouterTest::addressIsAddedToRoutingTable(const std::shared_ptr<const system::RoutingTypes::Address> address)
{
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(runtime);
    MockRoutingProxy* mockRoutingProxyRef = mockRoutingProxy.get();
    ON_CALL(*mockRoutingProxy, resolveNextHopAsyncMock(_,_,_,_))
            .WillByDefault(DoAll(Invoke(this, &LibJoynrMessageRouterTest::removeFromQueue), InvokeArgument<1>(false), Return(nullptr)));

    const std::string parentParticipantId = "parentParticipantId";
    messageRouter->setParentAddress(parentParticipantId, localTransport);
    messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string consumerParticipantId("consumerParticipantId");
    const std::string providerParticipantId("providerParticipantId");
    const bool isRecipientGloballyVisible = true;

    const TimePoint now = TimePoint::now();
    mutableMessage.setExpiryDate(now + std::chrono::milliseconds(100042));
    mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_REQUEST());
    mutableMessage.setSender(consumerParticipantId);
    mutableMessage.setRecipient(providerParticipantId);

    EXPECT_CALL(*mockRoutingProxyRef, resolveNextHopAsyncMock(Eq(providerParticipantId),_,_,_));
    EXPECT_CALL(*messagingStubFactory, create(_)).Times(0);

    messageRouter->route(mutableMessage.getImmutableMessage());

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    ASSERT_EQ(0, messageQueue->getQueueLength());
    Mock::VerifyAndClearExpectations(messagingStubFactory.get());
    Mock::VerifyAndClearExpectations(mockRoutingProxyRef);

    messageRouter->addProvisionedNextHop(
            providerParticipantId, address, isRecipientGloballyVisible);

    EXPECT_CALL(*mockRoutingProxyRef, resolveNextHopAsyncMock(Eq(providerParticipantId),_,_,_))
            .Times(0);
    auto mockMessagingStub = std::make_shared<MockMessagingStub>();
    EXPECT_CALL(*messagingStubFactory, create(Eq(address))).WillOnce(Return(mockMessagingStub));

    messageRouter->route(mutableMessage.getImmutableMessage());

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    ASSERT_EQ(0, messageQueue->getQueueLength());
    Mock::VerifyAndClearExpectations(messagingStubFactory.get());
    Mock::VerifyAndClearExpectations(mockRoutingProxyRef);

    // cleanup
    messageRouter->removeNextHop(providerParticipantId, nullptr, nullptr);
}

TEST_F(LibJoynrMessageRouterTest, inProcessAndWebSocketAddressTypesAreAddedToRoutingTable)
{
    auto webSocketAddress = std::make_shared<const system::RoutingTypes::WebSocketAddress>();
    addressIsAddedToRoutingTable(webSocketAddress);

    auto inProcessAddress = std::make_shared<const InProcessMessagingAddress>();
    addressIsAddedToRoutingTable(inProcessAddress);
}

TEST_F(LibJoynrMessageRouterTest, otherAddressTypesAreNotAddedToRoutingTable)
{
    auto mqttAddress = std::make_shared<const system::RoutingTypes::MqttAddress>();
    addressIsNotAddedToRoutingTable(mqttAddress);

    auto channelAddress = std::make_shared<const system::RoutingTypes::ChannelAddress>();
    addressIsNotAddedToRoutingTable(channelAddress);

    auto browserAddress = std::make_shared<const system::RoutingTypes::BrowserAddress>();
    addressIsNotAddedToRoutingTable(browserAddress);

    auto webSocketClientAddress = std::make_shared<const system::RoutingTypes::WebSocketClientAddress>();
    addressIsNotAddedToRoutingTable(webSocketClientAddress);
}
