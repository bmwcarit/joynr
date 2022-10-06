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

#include "joynr/InProcessMessagingAddress.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/BrowserAddress.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/system/RoutingTypes/UdsAddress.h"
#include "joynr/system/RoutingTypes/UdsClientAddress.h"
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
using ::testing::HasSubstr;
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
        _runtime = std::make_shared<MockJoynrRuntime>(std::move(settings));
    }

    void TearDown()
    {
        _runtime.reset();
    }

    void removeFromQueue(const std::string& participantId,
                         std::function<void(const bool& resolved)> onSuccess = nullptr,
                         std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                                 onRuntimeError = nullptr,
                         boost::optional<joynr::MessagingQos> qos = boost::none);

protected:
    void testAddNextHopCallsRoutingProxyCorrectly(
            const bool isGloballyVisible,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress);
    void addressIsNotAddedToRoutingTable(
            const std::shared_ptr<const system::RoutingTypes::Address> address);
    void addressIsAddedToRoutingTable(
            const std::shared_ptr<const system::RoutingTypes::Address> address);
    void addNewRoutingEntry(MockRoutingProxy* mockRoutingProxyRef,
                            const std::string providerParticipantId,
                            const std::shared_ptr<const system::RoutingTypes::Address> address);
    void testRoutingEntryUpdate(
            const std::string& participantId,
            std::shared_ptr<const system::RoutingTypes::Address> newAddress,
            std::shared_ptr<const system::RoutingTypes::Address> expectedAddress);
    const bool _isGloballyVisible = false;
    std::shared_ptr<MockJoynrRuntime> _runtime;
    MockRoutingProxy* setParentRouter();
};

TEST_F(LibJoynrMessageRouterTest, routeMessageToWebSocketAddress)
{
    const std::string destinationParticipantId = "TEST_routeMessageToWebSocketAddress";
    auto address = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>();
    const bool isGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;

    this->_messageRouter->addNextHop(
            destinationParticipantId, address, isGloballyVisible, expiryDateMs, isSticky);
    this->_mutableMessage.setRecipient(destinationParticipantId);

    EXPECT_CALL(*(this->_messagingStubFactory), create(Pointee(Eq(*address)))).Times(1);

    this->routeMessageToAddress();
}

TEST_F(LibJoynrMessageRouterTest, routeMessageToUdsAddress)
{
    const std::string destinationParticipantId = "TEST_routeMessageToUdsAddress";
    auto address = std::make_shared<const joynr::system::RoutingTypes::UdsAddress>();
    const bool isGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;

    this->_messageRouter->addNextHop(
            destinationParticipantId, address, isGloballyVisible, expiryDateMs, isSticky);
    this->_mutableMessage.setRecipient(destinationParticipantId);

    EXPECT_CALL(*(this->_messagingStubFactory), create(Pointee(Eq(*address)))).Times(1);

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
    _messageRouter->addProvisionedNextHop(
            providerParticipantId, _localTransport, _isGloballyVisible);
    _messageRouter->addProvisionedNextHop(
            subscriberParticipantId1, inProcessSubscriberAddress, _isGloballyVisible);

    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(_runtime);
    ON_CALL(*mockRoutingProxy, addMulticastReceiverAsyncMock(_, _, _, _, _, _))
            .WillByDefault(DoAll(InvokeArgument<3>(), Return(nullptr)));

    _messageRouter->setParentAddress(std::string("parentParticipantId"), _localTransport);
    _messageRouter->setParentRouter(std::move(mockRoutingProxy));

    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId1,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    EXPECT_CALL(*_messagingStubFactory, create(Pointee(Eq(*_localTransport)))).Times(1);
    EXPECT_CALL(*_messagingStubFactory, create(Pointee(Eq(*inProcessSubscriberAddress)))).Times(1);

    _mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_MULTICAST());
    _mutableMessage.setSender(providerParticipantId);
    _mutableMessage.setRecipient(multicastId);

    std::shared_ptr<joynr::ImmutableMessage> immutableMessage =
            _mutableMessage.getImmutableMessage();
    // The message should be propagated to parentMessageRouter
    immutableMessage->setReceivedFromGlobal(false);

    _messageRouter->route(immutableMessage);
}

TEST_F(LibJoynrMessageRouterTest,
       addMulticastReceiver_callsParentRouterIfProviderAddressNotAvailable)
{
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(_runtime);

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

    _messageRouter->setParentAddress(std::string("parentParticipantId"), _localTransport);
    _messageRouter->setParentRouter(std::move(mockRoutingProxy));

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

    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            []() { FAIL() << "onSuccess called"; },
            [&errorCallbackCalled](const joynr::exceptions::ProviderRuntimeException&) {
                errorCallbackCalled.notify();
            });
    EXPECT_TRUE(errorCallbackCalled.waitFor(std::chrono::milliseconds(5000)));

    Semaphore successCallbackCalled;
    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            [&successCallbackCalled]() { successCallbackCalled.notify(); },
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(LibJoynrMessageRouterTest, removeMulticastReceiver_CallsParentRouter)
{
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(_runtime);
    auto mockRoutingProxyRef = mockRoutingProxy.get();

    _messageRouter->setParentAddress(std::string("parentParticipantId"), _localTransport);
    _messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    _messageRouter->addProvisionedNextHop(
            providerParticipantId, _localTransport, _isGloballyVisible);

    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });

    // Call shall be forwarded to the parent proxy
    EXPECT_CALL(*mockRoutingProxyRef,
                removeMulticastReceiverAsyncMock(
                        multicastId, subscriberParticipantId, providerParticipantId, _, _, _));

    _messageRouter->removeMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
}

TEST_F(LibJoynrMessageRouterTest, removeMulticastReceiverOfInProcessProvider_callsParentRouter)
{
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(_runtime);
    auto mockRoutingProxyRef = mockRoutingProxy.get();

    _messageRouter->setParentAddress(std::string("parentParticipantId"), _localTransport);
    _messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    auto dispatcher = std::make_shared<MockDispatcher>();
    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>(dispatcher);
    auto providerAddress = std::make_shared<const joynr::InProcessMessagingAddress>(skeleton);
    _messageRouter->addProvisionedNextHop(
            providerParticipantId, providerAddress, _isGloballyVisible);

    _messageRouter->addMulticastReceiver(
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
    _messageRouter->removeMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            [&successCallbackCalled]() { successCallbackCalled.notify(); },
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(LibJoynrMessageRouterTest, removeMulticastReceiver_callsOnErrorWhenNoRoutingEntryFound)
{
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(_runtime);
    auto mockRoutingProxyRef = mockRoutingProxy.get();

    _messageRouter->setParentAddress(std::string("parentParticipantId"), _localTransport);
    _messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("testProviderParticipantId");
    EXPECT_CALL(*mockRoutingProxyRef, removeMulticastReceiverAsyncMock(_, _, _, _, _, _)).Times(0);
    Semaphore errorCallbackCalled;
    joynr::exceptions::ProviderRuntimeException callException;
    std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError =
            [&errorCallbackCalled,
             &callException](const joynr::exceptions::ProviderRuntimeException& e) {
                callException = e;
                errorCallbackCalled.notify();
            };
    auto onSuccess = []() { FAIL() << "onSuccess called"; };
    _messageRouter->removeMulticastReceiver(
            multicastId, subscriberParticipantId, providerParticipantId, onSuccess, onError);
    EXPECT_TRUE(errorCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
    EXPECT_THAT(callException.what(), HasSubstr("unable to removeMulticastReceiver. "));
    EXPECT_THAT(callException.what(),
                HasSubstr("No routing entry for multicast provider "
                          "(providerParticipantId=testProviderParticipantId) found."));
}

TEST_F(LibJoynrMessageRouterTest, addMulticastReceiver_callsParentRouter)
{
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(_runtime);
    auto mockRoutingProxyRef = mockRoutingProxy.get();

    _messageRouter->setParentAddress(std::string("parentParticipantId"), _localTransport);
    _messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");
    _messageRouter->addProvisionedNextHop(
            providerParticipantId, _localTransport, _isGloballyVisible);

    // Call shall be forwarded to the parent proxy
    EXPECT_CALL(*mockRoutingProxyRef,
                addMulticastReceiverAsyncMock(
                        multicastId, subscriberParticipantId, providerParticipantId, _, _, _));

    _messageRouter->addMulticastReceiver(
            multicastId,
            subscriberParticipantId,
            providerParticipantId,
            []() {},
            [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
}

TEST_F(LibJoynrMessageRouterTest, addMulticastReceiverForInProcessProvider_DoesNotCallParentRouter)
{
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(_runtime);
    auto mockRoutingProxyRef = mockRoutingProxy.get();

    _messageRouter->setParentAddress(std::string("parentParticipantId"), _localTransport);
    _messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");

    const std::string providerParticipantId("providerParticipantId");
    auto dispatcher = std::make_shared<MockDispatcher>();
    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>(dispatcher);
    auto providerAddress = std::make_shared<const joynr::InProcessMessagingAddress>(skeleton);
    _messageRouter->addProvisionedNextHop(
            providerParticipantId, providerAddress, _isGloballyVisible);

    EXPECT_CALL(*mockRoutingProxyRef,
                addMulticastReceiverAsyncMock(
                        multicastId, subscriberParticipantId, providerParticipantId, _, _, _))
            .Times(0);

    Semaphore successCallbackCalled;
    _messageRouter->addMulticastReceiver(
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
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(_runtime);
    const std::string proxyParticipantId = mockRoutingProxy->getProxyParticipantId();

    {
        InSequence inSequence;
        EXPECT_CALL(
                *mockRoutingProxy, addNextHopAsyncMockWs(Eq(proxyParticipantId), _, _, _, _, _));
        // call under test
        EXPECT_CALL(*mockRoutingProxy,
                    addNextHopAsyncMockWs(Eq(providerParticipantId),
                                          Eq(*_webSocketClientAddress),
                                          Eq(isGloballyVisible),
                                          _,
                                          _,
                                          _));
    }

    _messageRouter->setParentAddress(std::string("parentParticipantId"), _localTransport);

    _messageRouter->setParentRouter(std::move(mockRoutingProxy));

    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;

    _messageRouter->addNextHop(
            providerParticipantId, providerAddress, isGloballyVisible, expiryDateMs, isSticky);
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
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(_runtime);
    MockRoutingProxy* mockRoutingProxyRef = mockRoutingProxy.get();
    ON_CALL(*mockRoutingProxy, resolveNextHopAsyncMock(_, _, _, _))
            .WillByDefault(DoAll(Invoke(this, &LibJoynrMessageRouterTest::removeFromQueue),
                                 InvokeArgument<1>(false),
                                 Return(nullptr)));
    _messageRouter->setParentAddress(std::string("parentParticipantId"), _localTransport);
    _messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string providerParticipantId = "providerParticipantId";
    _mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_REQUEST());
    _mutableMessage.setSender("sender");
    _mutableMessage.setRecipient(providerParticipantId);

    EXPECT_CALL(*mockRoutingProxyRef, resolveNextHopAsyncMock(providerParticipantId, _, _, _));
    EXPECT_CALL(*_messagingStubFactory, create(_)).Times(0);

    _messageRouter->route(_mutableMessage.getImmutableMessage());

    ASSERT_EQ(0, _messageQueue->getQueueLength());

    Mock::VerifyAndClearExpectations(_messagingStubFactory.get());
    Mock::VerifyAndClearExpectations(mockRoutingProxyRef);

    _messageRouter->setToKnown(providerParticipantId);

    EXPECT_CALL(*mockRoutingProxyRef, resolveNextHopAsyncMock(Eq(providerParticipantId), _, _, _))
            .Times(0);
    auto mockMessagingStub = std::make_shared<MockMessagingStub>();
    EXPECT_CALL(*_messagingStubFactory, create(Eq(_localTransport)))
            .WillOnce(Return(mockMessagingStub));

    _messageRouter->route(_mutableMessage.getImmutableMessage());

    ASSERT_EQ(0, _messageQueue->getQueueLength());
}

void LibJoynrMessageRouterTest::removeFromQueue(
        const std::string& participantId,
        std::function<void(const bool& resolved)> onSuccess,
        std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
        boost::optional<joynr::MessagingQos> qos)
{
    std::ignore = onSuccess;
    std::ignore = onRuntimeError;
    std::ignore = qos;

    ASSERT_EQ(1, _messageQueue->getQueueLength());
    _messageQueue->getNextMessageFor(participantId);
    ASSERT_EQ(0, _messageQueue->getQueueLength());
}

void LibJoynrMessageRouterTest::addressIsNotAddedToRoutingTable(
        const std::shared_ptr<const system::RoutingTypes::Address> address)
{
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(_runtime);
    MockRoutingProxy* mockRoutingProxyRef = mockRoutingProxy.get();
    ON_CALL(*mockRoutingProxy, resolveNextHopAsyncMock(_, _, _, _))
            .WillByDefault(DoAll(Invoke(this, &LibJoynrMessageRouterTest::removeFromQueue),
                                 InvokeArgument<1>(false),
                                 Return(nullptr)));

    _messageRouter->setParentAddress(std::string("parentParticipantId"), _localTransport);
    _messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string consumerParticipantId("consumerParticipantId");
    const std::string providerParticipantId("providerParticipantId");
    const bool isRecipientGloballyVisible = true;

    const TimePoint now = TimePoint::now();
    _mutableMessage.setExpiryDate(now + std::chrono::milliseconds(1000));
    _mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_REQUEST());
    _mutableMessage.setSender(consumerParticipantId);
    _mutableMessage.setRecipient(providerParticipantId);

    EXPECT_CALL(*mockRoutingProxyRef, resolveNextHopAsyncMock(Eq(providerParticipantId), _, _, _));
    EXPECT_CALL(*_messagingStubFactory, create(_)).Times(0);

    _messageRouter->route(_mutableMessage.getImmutableMessage());

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    ASSERT_EQ(0, _messageQueue->getQueueLength());
    Mock::VerifyAndClearExpectations(_messagingStubFactory.get());
    Mock::VerifyAndClearExpectations(mockRoutingProxyRef);

    _messageRouter->addProvisionedNextHop(
            providerParticipantId, address, isRecipientGloballyVisible);

    EXPECT_CALL(*mockRoutingProxyRef, resolveNextHopAsyncMock(Eq(providerParticipantId), _, _, _));
    EXPECT_CALL(*_messagingStubFactory, create(_)).Times(0);

    _messageRouter->route(_mutableMessage.getImmutableMessage());

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    ASSERT_EQ(0, _messageQueue->getQueueLength());
    Mock::VerifyAndClearExpectations(_messagingStubFactory.get());
    Mock::VerifyAndClearExpectations(mockRoutingProxyRef);
}

MockRoutingProxy* LibJoynrMessageRouterTest::setParentRouter()
{
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(_runtime);
    MockRoutingProxy* mockRoutingProxyRef = mockRoutingProxy.get();

    const std::string parentParticipantId = "parentParticipantId";
    _messageRouter->setParentAddress(parentParticipantId, _localTransport);
    _messageRouter->setParentRouter(std::move(mockRoutingProxy));

    return mockRoutingProxyRef;
}

void LibJoynrMessageRouterTest::addNewRoutingEntry(
        MockRoutingProxy* mockRoutingProxyRef,
        const std::string providerParticipantId,
        const std::shared_ptr<const system::RoutingTypes::Address> address)
{
    Semaphore semaphore(0);
    ON_CALL(*mockRoutingProxyRef, resolveNextHopAsyncMock(_, _, _, _))
            .WillByDefault(DoAll(Invoke(this, &LibJoynrMessageRouterTest::removeFromQueue),
                                 InvokeArgument<1>(false),
                                 Return(nullptr)));

    const TimePoint now = TimePoint::now();
    _mutableMessage.setExpiryDate(now + std::chrono::milliseconds(1024));
    _mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_REQUEST());
    _mutableMessage.setRecipient(providerParticipantId);
    std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();

    EXPECT_CALL(*mockRoutingProxyRef, resolveNextHopAsyncMock(Eq(providerParticipantId), _, _, _));
    EXPECT_CALL(*_messagingStubFactory, create(_)).Times(0);

    _messageRouter->route(immutableMessage);

    ASSERT_EQ(0, _messageQueue->getQueueLength());
    Mock::VerifyAndClearExpectations(_messagingStubFactory.get());
    Mock::VerifyAndClearExpectations(mockRoutingProxyRef);

    const bool isParticipantGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    _messageRouter->addNextHop(
            providerParticipantId, address, isParticipantGloballyVisible, expiryDateMs, isSticky);

    EXPECT_CALL(*mockRoutingProxyRef, resolveNextHopAsyncMock(Eq(providerParticipantId), _, _, _))
            .Times(0);
    auto mockMessagingStub = std::make_shared<MockMessagingStub>();
    EXPECT_CALL(*_messagingStubFactory, create(Eq(address))).WillOnce(Return(mockMessagingStub));
    EXPECT_CALL(*mockMessagingStub, transmit(Eq(immutableMessage), _))
            .WillOnce(ReleaseSemaphore(&semaphore));

    _messageRouter->route(immutableMessage);

    ASSERT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
    ASSERT_EQ(0, _messageQueue->getQueueLength());
    Mock::VerifyAndClearExpectations(_messagingStubFactory.get());
    Mock::VerifyAndClearExpectations(mockRoutingProxyRef);
}

void LibJoynrMessageRouterTest::addressIsAddedToRoutingTable(
        const std::shared_ptr<const system::RoutingTypes::Address> address)
{
    MockRoutingProxy* mockRoutingProxyRef = setParentRouter();
    const std::string providerParticipantId("providerParticipantId");

    addNewRoutingEntry(mockRoutingProxyRef, providerParticipantId, address);

    // cleanup
    _messageRouter->removeNextHop(providerParticipantId, nullptr, nullptr);
}

TEST_F(LibJoynrMessageRouterTest, addressValidation_inProcessAddressTypesAreAddedToRoutingTable)
{
    // see also addressValidation_otherAddressTypesAreNotAddedToRoutingTable
    auto inProcessAddress = std::make_shared<const InProcessMessagingAddress>();
    addressIsAddedToRoutingTable(inProcessAddress);
}

TEST_F(LibJoynrMessageRouterTest, addressValidation_webSocketAddressTypesAreAddedToRoutingTable)
{
    // see also addressValidation_otherAddressTypesAreNotAddedToRoutingTable
    auto webSocketAddress = std::make_shared<const system::RoutingTypes::WebSocketAddress>();
    addressIsAddedToRoutingTable(webSocketAddress);
}

TEST_F(LibJoynrMessageRouterTest, addressValidation_udsAddressTypesAreAddedToRoutingTable)
{
    // see also addressValidation_otherAddressTypesAreNotAddedToRoutingTable
    auto udsAddress = std::make_shared<const system::RoutingTypes::UdsAddress>();
    addressIsAddedToRoutingTable(udsAddress);
}

TEST_F(LibJoynrMessageRouterTest, addressValidation_otherAddressTypesAreNotAddedToRoutingTable)
{
    // see also addressValidation_inProcessAndWebSocketAddressTypesAreAddedToRoutingTable
    auto mqttAddress = std::make_shared<const system::RoutingTypes::MqttAddress>();
    addressIsNotAddedToRoutingTable(mqttAddress);

    auto webSocketClientAddress =
            std::make_shared<const system::RoutingTypes::WebSocketClientAddress>();
    addressIsNotAddedToRoutingTable(webSocketClientAddress);
}

void LibJoynrMessageRouterTest::testRoutingEntryUpdate(
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
}

TEST_F(LibJoynrMessageRouterTest, addressValidation_allowUpdateOfInProcessAddress)
{
    // inProcessAddress can only be replaced with InProcessAddress
    // precedence: InProcessAddress > WebSocketAddress/UdsAddress >
    // WebSocketClientAddress/UdsClientAddress > MqttAddress
    const std::string testParticipantId = "allowInProcessUpdateParticipantId";
    auto dispatcher = std::make_shared<MockDispatcher>();
    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>(dispatcher);
    auto oldAddress = std::make_shared<const InProcessMessagingAddress>(skeleton);

    MockRoutingProxy* mockRoutingProxyRef = setParentRouter();
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

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
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
}

TEST_F(LibJoynrMessageRouterTest, DISABLED_addressValidation_allowUpdateOfWebSocketClientAddress)
{
    // Disabled: WebSocketClientAddress is not allowed in LibJoynrMessageRouter
    // Precedence cannot be tested without refactoring MessageRouter and RoutingTable, e.g. like in
    // Java

    // precedence: InProcessAddress > WebSocketAddress/UdsAddress >
    // WebSocketClientAddress/UdsClientAddress > MqttAddress
    const std::string testParticipantId = "allowWebSocketClientUpdateParticipantId";
    auto oldAddress =
            std::make_shared<const system::RoutingTypes::WebSocketClientAddress>("testWebSocketId");

    MockRoutingProxy* mockRoutingProxyRef = setParentRouter();
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto mqttAddress =
            std::make_shared<const system::RoutingTypes::MqttAddress>("brokerUri", "topic");
    testRoutingEntryUpdate(testParticipantId, mqttAddress, oldAddress);

    auto webSocketClientAddress =
            std::make_shared<const system::RoutingTypes::WebSocketClientAddress>("webSocketId");
    testRoutingEntryUpdate(testParticipantId, webSocketClientAddress, webSocketClientAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto webSocketAddress = std::make_shared<const system::RoutingTypes::WebSocketAddress>(
            system::RoutingTypes::WebSocketProtocol::WS, "host", 4242, "path");
    testRoutingEntryUpdate(testParticipantId, webSocketAddress, webSocketAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto inProcessAddress = std::make_shared<const InProcessMessagingAddress>();
    testRoutingEntryUpdate(testParticipantId, inProcessAddress, inProcessAddress);

    // cleanup
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
}

TEST_F(LibJoynrMessageRouterTest, DISABLED_addressValidation_allowUpdateOfUdsClientAddress)
{
    // Disabled: UdsClientAddress is not allowed in LibJoynrMessageRouter
    // Precedence cannot be tested without refactoring MessageRouter and RoutingTable,
    // e.g. like in Java

    // precedence: InProcessAddress > WebSocketAddress/UdsAddress >
    // WebSoccketClientAddress/UdsClientAddress > MqttAddress
    const std::string testParticipantId = "allowUdsClientUpdateParticipantId";
    auto oldAddress = std::make_shared<const system::RoutingTypes::UdsClientAddress>("testUdsId");

    MockRoutingProxy* mockRoutingProxyRef = setParentRouter();
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto mqttAddress =
            std::make_shared<const system::RoutingTypes::MqttAddress>("brokerUri", "topic");
    testRoutingEntryUpdate(testParticipantId, mqttAddress, oldAddress);

    auto udsClientAddress = std::make_shared<const system::RoutingTypes::UdsClientAddress>("udsId");
    testRoutingEntryUpdate(testParticipantId, udsClientAddress, udsClientAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto webSocketClientAddress =
            std::make_shared<const system::RoutingTypes::WebSocketClientAddress>("webSocketId");
    testRoutingEntryUpdate(testParticipantId, webSocketClientAddress, webSocketClientAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto udsAddress = std::make_shared<const system::RoutingTypes::UdsAddress>("path");
    testRoutingEntryUpdate(testParticipantId, udsAddress, udsAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto webSocketAddress = std::make_shared<const system::RoutingTypes::WebSocketAddress>(
            system::RoutingTypes::WebSocketProtocol::WS, "host", 4242, "path");
    testRoutingEntryUpdate(testParticipantId, webSocketAddress, webSocketAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto inProcessAddress = std::make_shared<const InProcessMessagingAddress>();
    testRoutingEntryUpdate(testParticipantId, inProcessAddress, inProcessAddress);

    // cleanup
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
}

TEST_F(LibJoynrMessageRouterTest, addressValidation_allowUpdateOfWebSocketAddress)
{
    // precedence: InProcessAddress > WebSocketAddress/UdsAddress >
    // WebSocketClientAddress/UdsClientAddress > MqttAddress
    const std::string testParticipantId = "allowWebSocketUpdateParticipantId";
    auto oldAddress = std::make_shared<const system::RoutingTypes::WebSocketAddress>(
            system::RoutingTypes::WebSocketProtocol::WSS, "testHost", 23, "testPath");

    MockRoutingProxy* mockRoutingProxyRef = setParentRouter();
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto mqttAddress =
            std::make_shared<const system::RoutingTypes::MqttAddress>("brokerUri", "topic");
    testRoutingEntryUpdate(testParticipantId, mqttAddress, oldAddress);

    auto webSocketClientAddress =
            std::make_shared<const system::RoutingTypes::WebSocketClientAddress>("webSocketId");
    testRoutingEntryUpdate(testParticipantId, webSocketClientAddress, oldAddress);

    auto webSocketAddress = std::make_shared<const system::RoutingTypes::WebSocketAddress>(
            system::RoutingTypes::WebSocketProtocol::WS, "host", 4242, "path");
    testRoutingEntryUpdate(testParticipantId, webSocketAddress, webSocketAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto udsClientAddress = std::make_shared<const system::RoutingTypes::UdsClientAddress>("udsId");
    testRoutingEntryUpdate(testParticipantId, udsClientAddress, oldAddress);

    auto udsAddress = std::make_shared<const system::RoutingTypes::UdsAddress>("path");
    testRoutingEntryUpdate(testParticipantId, udsAddress, udsAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto inProcessAddress = std::make_shared<const InProcessMessagingAddress>();
    testRoutingEntryUpdate(testParticipantId, inProcessAddress, inProcessAddress);

    // cleanup
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
}

TEST_F(LibJoynrMessageRouterTest, addressValidation_allowUpdateOfUdsAddress)
{
    // precedence: InProcessAddress > WebSocketAddress/UdsAddress >
    // WebSocketClientAddress/UdsClientAddress > MqttAddress
    const std::string testParticipantId = "allowUdsUpdateParticipantId";
    auto oldAddress = std::make_shared<const system::RoutingTypes::UdsAddress>("testPath");

    MockRoutingProxy* mockRoutingProxyRef = setParentRouter();
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto mqttAddress =
            std::make_shared<const system::RoutingTypes::MqttAddress>("brokerUri", "topic");
    testRoutingEntryUpdate(testParticipantId, mqttAddress, oldAddress);

    auto udsClientAddress = std::make_shared<const system::RoutingTypes::UdsClientAddress>("udsId");
    testRoutingEntryUpdate(testParticipantId, udsClientAddress, oldAddress);

    auto webSocketClientAddress =
            std::make_shared<const system::RoutingTypes::WebSocketClientAddress>("webSocketId");
    testRoutingEntryUpdate(testParticipantId, webSocketClientAddress, oldAddress);

    auto udsAddress = std::make_shared<const system::RoutingTypes::UdsAddress>("path");
    testRoutingEntryUpdate(testParticipantId, udsAddress, udsAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto webSocketAddress = std::make_shared<const system::RoutingTypes::WebSocketAddress>(
            system::RoutingTypes::WebSocketProtocol::WS, "host", 4242, "path");
    testRoutingEntryUpdate(testParticipantId, webSocketAddress, webSocketAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto inProcessAddress = std::make_shared<const InProcessMessagingAddress>();
    testRoutingEntryUpdate(testParticipantId, inProcessAddress, inProcessAddress);

    // cleanup
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
}

TEST_F(LibJoynrMessageRouterTest, DISABLED_addressValidation_allowUpdateOfMqttAddress)
{
    // Disabled: MqttAddress is not allowed in LibJoynrMessageRouter
    // Precedence cannot be tested without refactoring MessageRouter and RoutingTable, e.g. like in
    // Java

    // precedence: InProcessAddress > WebSocketAddress/UdsAddress >
    // WebSocketClientAddress/UdsClientAddress > MqttAddress
    const std::string testParticipantId = "allowMqttUpdateParticipantId";
    auto oldAddress =
            std::make_shared<const system::RoutingTypes::MqttAddress>("testbrokerUri", "testTopic");

    MockRoutingProxy* mockRoutingProxyRef = setParentRouter();
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto mqttAddress =
            std::make_shared<const system::RoutingTypes::MqttAddress>("brokerUri", "topic");
    testRoutingEntryUpdate(testParticipantId, mqttAddress, mqttAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto webSocketClientAddress =
            std::make_shared<const system::RoutingTypes::WebSocketClientAddress>("webSocketId");
    testRoutingEntryUpdate(testParticipantId, webSocketClientAddress, webSocketClientAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto webSocketAddress = std::make_shared<const system::RoutingTypes::WebSocketAddress>(
            system::RoutingTypes::WebSocketProtocol::WS, "host", 4242, "path");
    testRoutingEntryUpdate(testParticipantId, webSocketAddress, webSocketAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto udsClientAddress = std::make_shared<const system::RoutingTypes::UdsClientAddress>("udsId");
    testRoutingEntryUpdate(testParticipantId, udsClientAddress, udsClientAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto udsAddress = std::make_shared<const system::RoutingTypes::UdsAddress>("path");
    testRoutingEntryUpdate(testParticipantId, udsAddress, udsAddress);
    // restore oldAddress
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
    addNewRoutingEntry(mockRoutingProxyRef, testParticipantId, oldAddress);

    auto inProcessAddress = std::make_shared<const InProcessMessagingAddress>();
    testRoutingEntryUpdate(testParticipantId, inProcessAddress, inProcessAddress);

    // cleanup
    _messageRouter->removeNextHop(testParticipantId, nullptr, nullptr);
}

TEST_F(LibJoynrMessageRouterTest, checkIfMessageIsNotSendIfAddressIsNotAllowedInRoutingTable)
{
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>(_runtime);
    MockRoutingProxy* mockRoutingProxyRef = mockRoutingProxy.get();

    _messageRouter->setParentAddress(std::string("parentParticipantId"), _localTransport);
    _messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string providerParticipantId = "TestProviderParticipantId";
    const std::string consumerParticipantId("TestConsumerParticipantId");
    EXPECT_CALL(*mockRoutingProxyRef, resolveNextHopAsyncMock(Eq(providerParticipantId), _, _, _));

    const TimePoint now = TimePoint::now();
    _mutableMessage.setExpiryDate(now + std::chrono::seconds(1000));
    _mutableMessage.setType(joynr::Message::VALUE_MESSAGE_TYPE_REQUEST());
    _mutableMessage.setSender(consumerParticipantId);
    _mutableMessage.setRecipient(providerParticipantId);
    std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();

    _messageRouter->route(immutableMessage);

    EXPECT_EQ(this->_messageQueue->getQueueLength(), 1);
    EXPECT_EQ(this->_messageRouter->getNumberOfRoutedMessages(), 1);

    auto webSocketClientAddress =
            std::make_shared<const system::RoutingTypes::WebSocketClientAddress>("webSocketId");

    const bool isParticipantGloballyVisible = true;
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky = false;
    _messageRouter->addNextHop(providerParticipantId,
                               webSocketClientAddress,
                               isParticipantGloballyVisible,
                               expiryDateMs,
                               isSticky);

    EXPECT_EQ(this->_messageQueue->getQueueLength(), 1);
    EXPECT_EQ(this->_messageRouter->getNumberOfRoutedMessages(), 1);
}

TEST_F(LibJoynrMessageRouterTest, invalidIncomingAddress)
{
    Settings settings;
    MessagingSettings messagingSettings(settings);
    std::shared_ptr<const joynr::system::RoutingTypes::Address> nullptrIncomingAddress;
    auto stubFactory = std::make_shared<MockMessagingStubFactory>();
    EXPECT_CALL(*stubFactory, create(_)).Times(0);
    EXPECT_CALL(*stubFactory, remove(_)).Times(0);
    EXPECT_CALL(*stubFactory, contains(_)).Times(0);
    EXPECT_CALL(*stubFactory, shutdown()).Times(1);
    boost::asio::io_service ioService;
    std::unique_ptr<IMulticastAddressCalculator> noMultiCast(nullptr);
    std::vector<std::shared_ptr<ITransportStatus>> transportStatuses;

    LibJoynrMessageRouter messageRouter(
            messagingSettings,
            nullptrIncomingAddress,
            stubFactory,
            ioService,
            std::move(noMultiCast),
            transportStatuses,
            std::make_unique<MessageQueue<std::string>>(),
            std::make_unique<MessageQueue<std::shared_ptr<ITransportStatus>>>());

    messageRouter.setParentAddress(std::string("parentParticipantId"), _localTransport);
    auto parentProxyMock = std::make_shared<MockRoutingProxy>(_runtime);
    std::function<void()> onSuccess = []() { FAIL() << "Call should not be successful"; };
    joynr::exceptions::ProviderRuntimeException callException;
    std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError =
            [&callException](const joynr::exceptions::ProviderRuntimeException& e) {
                callException = e;
            };
    messageRouter.setParentRouter(parentProxyMock, onSuccess, onError);

    EXPECT_THAT(callException.what(), HasSubstr("incoming address"));
    EXPECT_THAT(callException.what(), HasSubstr("NULL pointer"));
    messageRouter.shutdown();
}

TEST_F(LibJoynrMessageRouterTest, udsIncomingAddress)
{
    Settings settings;
    MessagingSettings messagingSettings(settings);
    auto udsIncomingAddress =
            std::make_shared<joynr::system::RoutingTypes::UdsClientAddress>("Some ID");
    auto stubFactory = std::make_shared<MockMessagingStubFactory>();
    EXPECT_CALL(*stubFactory, create(_)).Times(0);
    EXPECT_CALL(*stubFactory, remove(_)).Times(0);
    EXPECT_CALL(*stubFactory, contains(_)).Times(0);
    EXPECT_CALL(*stubFactory, shutdown()).Times(1);
    boost::asio::io_service ioService;
    std::unique_ptr<IMulticastAddressCalculator> noMultiCast(nullptr);
    std::vector<std::shared_ptr<ITransportStatus>> transportStatuses;

    LibJoynrMessageRouter messageRouter(
            messagingSettings,
            udsIncomingAddress,
            stubFactory,
            ioService,
            std::move(noMultiCast),
            transportStatuses,
            std::make_unique<MessageQueue<std::string>>(),
            std::make_unique<MessageQueue<std::shared_ptr<ITransportStatus>>>());
    messageRouter.setParentAddress("routing UUID", _localTransport);

    auto parentProxyMock = std::make_shared<MockRoutingProxy>(_runtime);
    const auto proxyUuid = parentProxyMock->getProxyParticipantId();
    const bool parentRoutersAreNotGloballyVisible = false;
    EXPECT_CALL(*parentProxyMock,
                addNextHopAsyncMockUds(Eq(proxyUuid),
                                       Eq(*udsIncomingAddress),
                                       Eq(parentRoutersAreNotGloballyVisible),
                                       _,
                                       _,
                                       _));
    messageRouter.setParentRouter(parentProxyMock);

    const std::string serverConnectionId{"Provider UUID"};
    constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
    const bool isSticky{false};
    const bool isGloballyVisible{true};
    std::shared_ptr<const joynr::system::RoutingTypes::Address> serverConnectionAddress =
            std::make_shared<joynr::system::RoutingTypes::UdsAddress>("server.sock.path");

    EXPECT_CALL(*parentProxyMock,
                addNextHopAsyncMockUds(Eq(serverConnectionId),
                                       Eq(*udsIncomingAddress),
                                       Eq(isGloballyVisible),
                                       _,
                                       _,
                                       _));
    messageRouter.addNextHop(
            serverConnectionId, serverConnectionAddress, isGloballyVisible, expiryDateMs, isSticky);

    messageRouter.shutdown();
}
