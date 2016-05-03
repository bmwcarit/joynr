/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include "AbstractSyncAsyncTest.cpp"
#include "joynr/tests/testJoynrMessagingConnector.h"
#include "joynr/IReplyCaller.h"
#include <string>
#include "utils/MockObjects.h"
#include "joynr/SubscriptionCallback.h"
#include "joynr/OnChangeSubscriptionQos.h"

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

ACTION_P(ReleaseSemaphore,semaphore)
{
    semaphore->notify();
}

/**
 * @brief Fixutre.
 */
class TestJoynrMessagingConnectorTest : public AbstractSyncAsyncTest {
public:

    TestJoynrMessagingConnectorTest():
        mockSubscriptionManager(new MockSubscriptionManager()),
        gpsLocation(types::Localisation::GpsLocation(
                        9.0,
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
        delete mockSubscriptionManager;
    }

    // sets the expectations on the call expected on the MessageSender from the connector
    testing::internal::TypedExpectation<void(
            const std::string&, // sender participant ID
            const std::string&, // receiver participant ID
            const MessagingQos&, // messaging QoS
            const Request&, // request object to send
            std::shared_ptr<IReplyCaller> // reply caller to notify when reply is received
    )>& setExpectationsForSendRequestCall(std::string methodName) override {
        return EXPECT_CALL(
                    *mockJoynrMessageSender,
                    sendRequest(
                        Eq(proxyParticipantId), // sender participant ID
                        Eq(providerParticipantId), // receiver participant ID
                        _, // messaging QoS
                        Property(&Request::getMethodName, Eq(methodName)), // request object to send
                        Property(&std::shared_ptr<IReplyCaller>::get,NotNull()) // reply caller to notify when reply is received
                    )
        );
    }

    MockSubscriptionManager* mockSubscriptionManager;
    joynr::types::Localisation::GpsLocation gpsLocation;
    float floatValue;
    Semaphore semaphore;

    tests::testJoynrMessagingConnector* createConnector(bool cacheEnabled) {
        return new tests::testJoynrMessagingConnector(
                    mockJoynrMessageSender,
                    mockSubscriptionManager,
                    "myDomain",
                    proxyParticipantId,
                    providerParticipantId,
                    MessagingQos(),
                    &mockClientCache,
                    cacheEnabled);
    }

    tests::Itest* createFixture(bool cacheEnabled) override {
        return dynamic_cast<tests::Itest*>(createConnector(cacheEnabled));
    }

    void invokeSubscriptionCallback(const std::string& subscribeToName,
                                      std::shared_ptr<ISubscriptionCallback> callback,
                                      const Variant& qosVariant,
                                      SubscriptionRequest& subscriptionRequest) {
        std::ignore = subscribeToName;
        std::ignore = qosVariant;
        std::ignore = subscriptionRequest;

        std::shared_ptr<SubscriptionCallback<joynr::types::Localisation::GpsLocation, float>> typedCallback =
                std::dynamic_pointer_cast<SubscriptionCallback<joynr::types::Localisation::GpsLocation, float>>(callback);

        typedCallback->onSuccess(gpsLocation, floatValue);
    }
};

typedef TestJoynrMessagingConnectorTest TestJoynrMessagingConnectorTestDeathTest;

/*
 * Tests
 */

TEST_F(TestJoynrMessagingConnectorTest, async_getAttributeNotCached) {
    testAsync_getAttributeNotCached();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_setAttributeNotCached) {
    testSync_setAttributeNotCached();
}


TEST_F(TestJoynrMessagingConnectorTest, sync_getAttributeNotCached) {
    testSync_getAttributeNotCached();
}

TEST_F(TestJoynrMessagingConnectorTest, async_getAttributeCached) {
    testAsync_getAttributeCached();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_getAttributeCached) {
    testSync_getAttributeCached();
}

TEST_F(TestJoynrMessagingConnectorTest, async_getterCallReturnsProviderRuntimeException) {
    testAsync_getterCallReturnsProviderRuntimeException();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_getterCallReturnsProviderRuntimeException) {
    testSync_getterCallReturnsProviderRuntimeException();
}

TEST_F(TestJoynrMessagingConnectorTest, async_getterCallReturnsMethodInvocationException) {
    testAsync_getterCallReturnsMethodInvocationException();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_getterCallReturnsMethodInvocationException) {
    testSync_getterCallReturnsMethodInvocationException();
}

TEST_F(TestJoynrMessagingConnectorTest, async_setterCallReturnsProviderRuntimeException) {
    testAsync_setterCallReturnsProviderRuntimeException();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_setterCallReturnsProviderRuntimeException) {
    testSync_setterCallReturnsProviderRuntimeException();
}

TEST_F(TestJoynrMessagingConnectorTest, async_setterCallReturnsMethodInvocationException) {
    testAsync_setterCallReturnsMethodInvocationException();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_setterCallReturnsMethodInvocationException) {
    testSync_setterCallReturnsMethodInvocationException();
}

TEST_F(TestJoynrMessagingConnectorTest, async_methodCallReturnsProviderRuntimeException) {
    testAsync_methodCallReturnsProviderRuntimeException();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_methodCallReturnsProviderRuntimeException) {
    testSync_methodCallReturnsProviderRuntimeException();
}

TEST_F(TestJoynrMessagingConnectorTest, async_methodCallReturnsMethodInvocationException) {
    testAsync_methodCallReturnsMethodInvocationException();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_methodCallReturnsMethodInvocationException) {
    testSync_methodCallReturnsMethodInvocationException();
}

TEST_F(TestJoynrMessagingConnectorTest, async_methodCallReturnsErrorEnum) {
    testAsync_methodCallReturnsErrorEnum();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_methodCallReturnsErrorEnum) {
    testSync_methodCallReturnsErrorEnum();
}

TEST_F(TestJoynrMessagingConnectorTest, async_methodCallReturnsExtendedErrorEnum) {
    testAsync_methodCallReturnsExtendedErrorEnum();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_methodCallReturnsExtendedErrorEnum) {
    testSync_methodCallReturnsExtendedErrorEnum();
}

TEST_F(TestJoynrMessagingConnectorTest, async_methodCallReturnsInlineErrorEnum) {
    testAsync_methodCallReturnsInlineErrorEnum();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_methodCallReturnsInlineErrorEnum) {
    testSync_methodCallReturnsInlineErrorEnum();
}

TEST_F(TestJoynrMessagingConnectorTest, async_OperationWithNoArguments) {
    testAsync_OperationWithNoArguments();
}

TEST_F(TestJoynrMessagingConnectorTest, sync_OperationWithNoArguments) {
    testSync_OperationWithNoArguments();
}

TEST_F(TestJoynrMessagingConnectorTest, subscribeToAttribute) {
    testSubscribeToAttribute();
}

TEST_F(TestJoynrMessagingConnectorTest, testBroadcastListenerWrapper) {
    tests::testJoynrMessagingConnector* connector = createConnector(false);

    auto mockListener = std::make_shared<MockGpsFloatSubscriptionListener>();

    EXPECT_CALL(
                        *mockSubscriptionManager,
                        registerSubscription(
                            Eq("locationUpdateWithSpeed"), //broadcastName
                            _,
                            _, // messaging QoS
                            _
                        )).WillOnce(testing::Invoke(this, &TestJoynrMessagingConnectorTest::invokeSubscriptionCallback));
    //   joynr::tests::LocationUpdateWithSpeedSelectiveBroadcastSubscriptionListenerWrapper

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(Eq(gpsLocation), Eq(floatValue)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    OnChangeSubscriptionQos qos;
    connector->subscribeToLocationUpdateWithSpeedBroadcast(mockListener, qos);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(2)));
}
