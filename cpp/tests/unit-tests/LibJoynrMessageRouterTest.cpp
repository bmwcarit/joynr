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

#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/InProcessAddress.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/system/RoutingProxy.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"

using ::testing::DoAll;
using ::testing::InvokeArgument;
using ::testing::InSequence;
using ::testing::Pointee;
using ::testing::Return;

using namespace joynr;

class LibJoynrMessageRouterTest : public MessageRouterTest<LibJoynrMessageRouter> {
public:
    LibJoynrMessageRouterTest() = default;
protected:
    void testAddNextHopCallsRoutingProxyCorrectly(const bool isGloballyVisible,
                          std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress);
    const bool isGloballyVisible = false;
};

TEST_F(LibJoynrMessageRouterTest, routeMulticastMessageFromLocalProvider_multicastMsgIsSentToAllMulticastReceivers) {
    const std::string subscriberParticipantId1("subscriberPartId");
    const std::string providerParticipantId("providerParticipantId");
    const std::string multicastNameAndPartitions("multicastName/partition0");
    const std::string multicastId(providerParticipantId + "/" + multicastNameAndPartitions);
    const std::shared_ptr<const joynr::InProcessMessagingAddress> inProcessSubscriberAddress =
            std::make_shared<const joynr::InProcessMessagingAddress>();
    messageRouter->addProvisionedNextHop(providerParticipantId, localTransport, isGloballyVisible);
    messageRouter->addProvisionedNextHop(subscriberParticipantId1, inProcessSubscriberAddress, isGloballyVisible);

    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
    ON_CALL(
        *mockRoutingProxy,
        addMulticastReceiverAsync(_,_,_,_,_)
    ).WillByDefault(
        DoAll(
            InvokeArgument<3>(),
            Return(nullptr)
        )
    );

    messageRouter->setParentAddress(std::string("parentParticipantId"), localTransport);
    messageRouter->setParentRouter(std::move(mockRoutingProxy));

    messageRouter->addMulticastReceiver(
        multicastId,
        subscriberParticipantId1,
        providerParticipantId,
        [](){ },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; }
    );

    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*localTransport)))).Times(1);
    EXPECT_CALL(*messagingStubFactory, create(Pointee(Eq(*inProcessSubscriberAddress)))).Times(1);

    joynrMessage.setType(joynr::JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST);
    joynrMessage.setHeaderFrom(providerParticipantId);
    joynrMessage.setHeaderTo(multicastId);

    // The message should be propagated to parentMessageRouter
    joynrMessage.setReceivedFromGlobal(false);

    messageRouter->route(joynrMessage);
}

TEST_F(LibJoynrMessageRouterTest, addMulticastReceiver_callsParentRouterIfProviderAddressNotAvailable) {
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    EXPECT_CALL(*mockRoutingProxy, resolveNextHopAsync(providerParticipantId,_,_))
            .WillOnce(
                DoAll(
                    InvokeArgument<2>(joynr::exceptions::JoynrRuntimeException("testException")),
                    Return(nullptr)))
            .WillOnce(DoAll(InvokeArgument<1>(false), Return(nullptr)))
            .WillOnce(DoAll(InvokeArgument<1>(true), Return(nullptr)));

    EXPECT_CALL(*mockRoutingProxy, addMulticastReceiverAsync(
                    multicastId,
                    subscriberParticipantId,
                    providerParticipantId,
                    _,
                    _))
            .WillOnce(
                DoAll(
                    InvokeArgument<3>(),
                    Return(nullptr)
                )
            );;

    messageRouter->setParentAddress(std::string("parentParticipantId"), localTransport);
    messageRouter->setParentRouter(std::move(mockRoutingProxy));

    Semaphore errorCallbackCalled;
    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { FAIL() << "onSuccess called"; },
        [&errorCallbackCalled](const joynr::exceptions::ProviderRuntimeException&)
        {
            errorCallbackCalled.notify();
        });
    EXPECT_TRUE(errorCallbackCalled.waitFor(std::chrono::milliseconds(5000)));

    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { FAIL() << "onSuccess called"; },
        [&errorCallbackCalled](const joynr::exceptions::ProviderRuntimeException&)
        {
            errorCallbackCalled.notify();
        });
    EXPECT_TRUE(errorCallbackCalled.waitFor(std::chrono::milliseconds(5000)));

    Semaphore successCallbackCalled;
    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        [&successCallbackCalled]() { successCallbackCalled.notify(); },
        [](const joynr::exceptions::ProviderRuntimeException&) { FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(LibJoynrMessageRouterTest, removeMulticastReceiver_CallsParentRouter) {
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
    auto mockRoutingProxyRef = mockRoutingProxy.get();

    messageRouter->setParentAddress(std::string("parentParticipantId"), localTransport);
    messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    messageRouter->addProvisionedNextHop(providerParticipantId, localTransport, isGloballyVisible);

    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });

    // Call shall be forwarded to the parent proxy
    EXPECT_CALL(*mockRoutingProxyRef,
        removeMulticastReceiverAsync(multicastId, subscriberParticipantId, providerParticipantId, _, _));

    messageRouter->removeMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
}

TEST_F(LibJoynrMessageRouterTest, removeMulticastReceiverOfInProcessProvider_callsParentRouter) {
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
    auto mockRoutingProxyRef = mockRoutingProxy.get();

    messageRouter->setParentAddress(std::string("parentParticipantId"), localTransport);
    messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");

    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>();
    auto providerAddress = std::make_shared<const joynr::InProcessMessagingAddress>(skeleton);
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress, isGloballyVisible);

    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });

    EXPECT_CALL(*mockRoutingProxyRef,
        removeMulticastReceiverAsync(multicastId, subscriberParticipantId, providerParticipantId, _, _))
            .Times(1)
            .WillOnce(
                DoAll(
                    InvokeArgument<3>(),
                    Return(nullptr)
                )
            );

    Semaphore successCallbackCalled;
    messageRouter->removeMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        [&successCallbackCalled]() { successCallbackCalled.notify(); },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(LibJoynrMessageRouterTest, addMulticastReceiver_callsParentRouter) {
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
    auto mockRoutingProxyRef = mockRoutingProxy.get();

    messageRouter->setParentAddress(std::string("parentParticipantId"), localTransport);
    messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");
    const std::string providerParticipantId("providerParticipantId");
    messageRouter->addProvisionedNextHop(providerParticipantId, localTransport, isGloballyVisible);

    // Call shall be forwarded to the parent proxy
    EXPECT_CALL(*mockRoutingProxyRef,
        addMulticastReceiverAsync(multicastId, subscriberParticipantId, providerParticipantId, _, _));

    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        []() { },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
}

TEST_F(LibJoynrMessageRouterTest, addMulticastReceiverForWebSocketProvider_callsParentRouter) {
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
    auto mockRoutingProxyRef = mockRoutingProxy.get();

    messageRouter->setParentAddress(std::string("parentParticipantId"), localTransport);
    messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");

    const std::string providerParticipantId("providerParticipantId");
    auto providerAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>();
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress, isGloballyVisible);

    EXPECT_CALL(*mockRoutingProxyRef,
        addMulticastReceiverAsync(multicastId, subscriberParticipantId, providerParticipantId, _, _))
            .Times(1)
            .WillOnce(
                DoAll(
                    InvokeArgument<3>(),
                    Return(nullptr)
                )
            );

    Semaphore successCallbackCalled;
    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        [&successCallbackCalled]() { successCallbackCalled.notify(); },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(LibJoynrMessageRouterTest, addMulticastReceiverForInProcessProvider_callsParentRouter) {
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
    auto mockRoutingProxyRef = mockRoutingProxy.get();

    messageRouter->setParentAddress(std::string("parentParticipantId"), localTransport);
    messageRouter->setParentRouter(std::move(mockRoutingProxy));

    const std::string multicastId("multicastId");
    const std::string subscriberParticipantId("subscriberParticipantId");

    const std::string providerParticipantId("providerParticipantId");
    auto skeleton = std::make_shared<MockInProcessMessagingSkeleton>();
    auto providerAddress = std::make_shared<const joynr::InProcessMessagingAddress>(skeleton);
    messageRouter->addProvisionedNextHop(providerParticipantId, providerAddress, isGloballyVisible);

    EXPECT_CALL(*mockRoutingProxyRef,
        addMulticastReceiverAsync(multicastId, subscriberParticipantId, providerParticipantId, _, _))
            .Times(1)
            .WillOnce(
                DoAll(
                    InvokeArgument<3>(),
                    Return(nullptr)
                )
            );

    Semaphore successCallbackCalled;
    messageRouter->addMulticastReceiver(multicastId,
        subscriberParticipantId,
        providerParticipantId,
        [&successCallbackCalled]() { successCallbackCalled.notify(); },
        [](const joynr::exceptions::ProviderRuntimeException&){ FAIL() << "onError called"; });
    EXPECT_TRUE(successCallbackCalled.waitFor(std::chrono::milliseconds(5000)));
}

void LibJoynrMessageRouterTest::testAddNextHopCallsRoutingProxyCorrectly(const bool isGloballyVisible,
                      std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress)
{
    const std::string providerParticipantId("providerParticipantId");
    auto mockRoutingProxy = std::make_unique<MockRoutingProxy>();
    const std::string proxyParticipantId = mockRoutingProxy->getProxyParticipantId();

    {
        InSequence inSequence;
        EXPECT_CALL(*mockRoutingProxy,
            addNextHopAsync(Eq(proxyParticipantId),_,_,_,_)
        );
        // call under test
        EXPECT_CALL(*mockRoutingProxy,
            addNextHopAsync(Eq(providerParticipantId),Eq(*webSocketClientAddress),Eq(isGloballyVisible),_,_)
        );
    }

    messageRouter->setParentAddress(std::string("parentParticipantId"), localTransport);

    messageRouter->setParentRouter(std::move(mockRoutingProxy));

    messageRouter->addNextHop(providerParticipantId, providerAddress, isGloballyVisible);
}

TEST_F(LibJoynrMessageRouterTest, addNextHop_callsAddNextHopInRoutingProxy) {
    bool isGloballyVisible;

    // InProcessAddress
    auto mockRequestCaller = std::make_shared<MockTestRequestCaller>();
    const auto providerAddress1 = std::make_shared<const joynr::InProcessAddress>(mockRequestCaller);
    isGloballyVisible = false;
    testAddNextHopCallsRoutingProxyCorrectly(isGloballyVisible, providerAddress1);
    isGloballyVisible = true;
    testAddNextHopCallsRoutingProxyCorrectly(isGloballyVisible, providerAddress1);

    // InprocessMessagingAddress
    auto mockSkeleton = std::make_shared<MockInProcessMessagingSkeleton>();
    const auto providerAddress2 = std::make_shared<const joynr::InProcessMessagingAddress>(mockSkeleton);
    isGloballyVisible = false;
    testAddNextHopCallsRoutingProxyCorrectly(isGloballyVisible, providerAddress2);
    isGloballyVisible = true;
    testAddNextHopCallsRoutingProxyCorrectly(isGloballyVisible, providerAddress2);
}
