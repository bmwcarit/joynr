/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#include "tests/unit-tests/AbstractSyncAsyncTest.cpp"

#include <string>

#include <gmock/gmock.h>

#include "joynr/IReplyCaller.h"
#include "joynr/ISubscriptionCallback.h"
#include "joynr/MulticastSubscriptionQos.h"
#include "joynr/tests/Itest.h"
#include "joynr/tests/ItestConnector.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockSubscriptionManager.h"
#include "tests/mock/MockGpsFloatSubscriptionListener.h"

using ::testing::A;
using ::testing::_;
using ::testing::Return;
using ::testing::Eq;
using ::testing::NotNull;
using ::testing::AllOf;
using ::testing::Property;
using ::testing::Invoke;
using ::testing::Unused;

using namespace joynr;

/**
 * @brief Fixutre.
 */
class TestJoynrMessagingConnectorTest : public AbstractSyncAsyncTest
{
public:
    TestJoynrMessagingConnectorTest()
            : gpsLocation(types::Localisation::GpsLocation(9.0,
                                                           51.0,
                                                           508.0,
                                                           types::Localisation::GpsFixEnum::MODE2D,
                                                           0.0,
                                                           0.0,
                                                           0.0,
                                                           0.0,
                                                           444,
                                                           444,
                                                           2)),
              floatValue(123.45),
              semaphore(0)
    {
    }

    ~TestJoynrMessagingConnectorTest()
    {
    }

    // sets the expectations on the call expected on the MessageSender from the connector
    testing::internal::TypedExpectation<
            void(const std::string&,            // sender participant ID
                 const std::string&,            // receiver participant ID
                 const MessagingQos&,           // messaging QoS
                 const Request&,                // request object to send
                 std::shared_ptr<IReplyCaller>, // reply caller to notify when reply is received
                 bool isLocalMessage)>&
    setExpectationsForSendRequestCall(std::string methodName) override
    {
        return EXPECT_CALL(
                *mockMessageSender,
                sendRequest(
                        Eq(proxyParticipantId),    // sender participant ID
                        Eq(providerParticipantId), // receiver participant ID
                        _,                         // messaging QoS
                        Property(&Request::getMethodName, Eq(methodName)), // request object to send
                        Property(&std::shared_ptr<IReplyCaller>::get,
                                 NotNull()), // reply caller to notify when reply is received
                        _                    // isLocalMessage flag
                        ));
    }

    joynr::types::Localisation::GpsLocation gpsLocation;
    float floatValue;
    Semaphore semaphore;

    std::shared_ptr<tests::testJoynrMessagingConnector> createConnector()
    {
        types::DiscoveryEntryWithMetaInfo discoveryEntry;

        discoveryEntry.setParticipantId(providerParticipantId);
        discoveryEntry.setIsLocal(false);

        return std::make_shared<tests::testJoynrMessagingConnector>(mockMessageSender,
                                                                    mockSubscriptionManager,
                                                                    "myDomain",
                                                                    proxyParticipantId,
                                                                    MessagingQos(),
                                                                    discoveryEntry);
    }

    std::shared_ptr<tests::Itest> createItestFixture() override
    {
        return std::dynamic_pointer_cast<tests::Itest>(createConnector());
    }

    std::shared_ptr<tests::ItestSubscription> createItestSubscriptionFixture() override
    {
        return std::dynamic_pointer_cast<tests::ItestSubscription>(createConnector());
    }

    void invokeMulticastSubscriptionCallback(
            const std::string& subscribeToName,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            const std::vector<std::string>& partitions,
            std::shared_ptr<joynr::ISubscriptionCallback> subscriptionCaller,
            std::shared_ptr<ISubscriptionListenerBase> subscriptionListener,
            std::shared_ptr<joynr::SubscriptionQos> qos,
            joynr::MulticastSubscriptionRequest& subscriptionRequest,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
    {
        std::ignore = subscribeToName;
        std::ignore = subscriberParticipantId;
        std::ignore = providerParticipantId;
        std::ignore = partitions;
        std::ignore = subscriptionCaller;
        std::ignore = onSuccess;
        std::ignore = onError;
        subscriptionRequest.setQos(qos);
        std::shared_ptr<ISubscriptionListener<joynr::types::Localisation::GpsLocation, float>>
                typedListener = std::dynamic_pointer_cast<
                        ISubscriptionListener<joynr::types::Localisation::GpsLocation, float>>(
                        subscriptionListener);

        typedListener->onReceive(gpsLocation, floatValue);
    }
};

typedef TestJoynrMessagingConnectorTest TestJoynrMessagingConnectorTestDeathTest;

/*
 * Tests
 */

TEST_F(TestJoynrMessagingConnectorTest, async_getAttributeNotCached)
{
    testAsync_getAttributeNotCached();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_setAttributeNotCached)
{
    testSync_setAttributeNotCached();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_getAttributeNotCached)
{
    testSync_getAttributeNotCached();
}

TEST_F(TestJoynrMessagingConnectorTest, async_getterCallReturnsProviderRuntimeException)
{
    testAsync_getterCallReturnsProviderRuntimeException();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_getterCallReturnsProviderRuntimeException)
{
    testSync_getterCallReturnsProviderRuntimeException();
}

TEST_F(TestJoynrMessagingConnectorTest, async_getterCallReturnsMethodInvocationException)
{
    testAsync_getterCallReturnsMethodInvocationException();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_getterCallReturnsMethodInvocationException)
{
    testSync_getterCallReturnsMethodInvocationException();
}

TEST_F(TestJoynrMessagingConnectorTest, async_setterCallReturnsProviderRuntimeException)
{
    testAsync_setterCallReturnsProviderRuntimeException();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_setterCallReturnsProviderRuntimeException)
{
    testSync_setterCallReturnsProviderRuntimeException();
}

TEST_F(TestJoynrMessagingConnectorTest, async_setterCallReturnsMethodInvocationException)
{
    testAsync_setterCallReturnsMethodInvocationException();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_setterCallReturnsMethodInvocationException)
{
    testSync_setterCallReturnsMethodInvocationException();
}

TEST_F(TestJoynrMessagingConnectorTest, async_methodCallReturnsProviderRuntimeException)
{
    testAsync_methodCallReturnsProviderRuntimeException();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_methodCallReturnsProviderRuntimeException)
{
    testSync_methodCallReturnsProviderRuntimeException();
}

TEST_F(TestJoynrMessagingConnectorTest, async_methodCallReturnsMethodInvocationException)
{
    testAsync_methodCallReturnsMethodInvocationException();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_methodCallReturnsMethodInvocationException)
{
    testSync_methodCallReturnsMethodInvocationException();
}

TEST_F(TestJoynrMessagingConnectorTest, async_methodCallReturnsErrorEnum)
{
    testAsync_methodCallReturnsErrorEnum();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_methodCallReturnsErrorEnum)
{
    testSync_methodCallReturnsErrorEnum();
}

TEST_F(TestJoynrMessagingConnectorTest, async_methodCallReturnsExtendedErrorEnum)
{
    testAsync_methodCallReturnsExtendedErrorEnum();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_methodCallReturnsExtendedErrorEnum)
{
    testSync_methodCallReturnsExtendedErrorEnum();
}

TEST_F(TestJoynrMessagingConnectorTest, async_methodCallReturnsInlineErrorEnum)
{
    testAsync_methodCallReturnsInlineErrorEnum();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_methodCallReturnsInlineErrorEnum)
{
    testSync_methodCallReturnsInlineErrorEnum();
}

TEST_F(TestJoynrMessagingConnectorTest, async_OperationWithNoArguments)
{
    testAsync_OperationWithNoArguments();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_OperationWithNoArguments)
{
    testSync_OperationWithNoArguments();
}

TEST_F(TestJoynrMessagingConnectorTest, subscribeToAttribute)
{
    testSubscribeToAttribute();
}

TEST_F(TestJoynrMessagingConnectorTest,
       doNotSendSubscriptionStopForMulticastSubscription)
{
    doNotSendSubscriptionStopForMulticastSubscription();
}

TEST_F(TestJoynrMessagingConnectorTest, sendSubscriptionStopForSelectiveSubscription)
{
    sendSubscriptionStopForSelectiveSubscription();
}

TEST_F(TestJoynrMessagingConnectorTest, testBroadcastListenerWrapper)
{
    std::shared_ptr<tests::testJoynrMessagingConnector> connector(createConnector());

    auto mockListener = std::make_shared<MockGpsFloatSubscriptionListener>();

    EXPECT_CALL(*mockSubscriptionManager,
                registerSubscription(Eq("locationUpdateWithSpeed"), // subscribeToName
                                     _, // subscriberParticipantId
                                     _, // providerParticipantId
                                     _, // partitions
                                     _, // subscriptionCaller
                                     _, // subscriptionListener
                                     _, // messaging QoS
                                     _, // subscriptionRequest
                                     _, // onSuccess
                                     _  // onError
                                     ))
            .WillOnce(testing::Invoke(
                    this, &TestJoynrMessagingConnectorTest::invokeMulticastSubscriptionCallback));

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation), Eq(floatValue)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    auto qos = std::make_shared<MulticastSubscriptionQos>();
    connector->subscribeToLocationUpdateWithSpeedBroadcast(mockListener, qos);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(2)));
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(mockSubscriptionManager.get()));
}
