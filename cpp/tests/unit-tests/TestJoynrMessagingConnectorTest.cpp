/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "joynr/tests/ItestConnector.h"
#include "joynr/IReplyCaller.h"
#include <string>
#include "utils/MockObjects.h"
#include "joynr/SubscriptionCallback.h"
#include "joynr/StdOnChangeSubscriptionQos.h"

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
    semaphore->release(1);
}

/**
 * @brief Fixutre.
 */
class TestJoynrMessagingConnectorTest : public AbstractSyncAsyncTest {
public:

    TestJoynrMessagingConnectorTest():
        mockSubscriptionManager(new MockSubscriptionManager()),
        gpsLocation(types::GpsLocation(
                        9.0,
                        51.0,
                        508.0,
                        types::GpsFixEnum::MODE2D,
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
            QSharedPointer<IReplyCaller> // reply caller to notify when reply is received
    )>& setExpectationsForSendRequestCall(int expectedTypeId, std::string methodName) {
        return EXPECT_CALL(
                    *mockJoynrMessageSender,
                    sendRequest(
                        Eq(proxyParticipantId), // sender participant ID
                        Eq(providerParticipantId), // receiver participant ID
                        _, // messaging QoS
                        Property(&Request::getMethodName, Eq(QString::fromStdString(methodName))), // request object to send
                        Property(
                            &QSharedPointer<IReplyCaller>::data,
                            AllOf(NotNull(), Property(&IReplyCaller::getTypeId, Eq(expectedTypeId)))
                        ) // reply caller to notify when reply is received
                    )
        );
    }

    MockSubscriptionManager* mockSubscriptionManager;
    joynr::types::GpsLocation gpsLocation;
    float floatValue;
    QSemaphore semaphore;

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

    tests::Itest* createFixture(bool cacheEnabled) {
        return dynamic_cast<tests::Itest*>(createConnector(cacheEnabled));
    }

    void invokeSubscriptionCallback(const QString& subscribeToName,
                                      QSharedPointer<ISubscriptionCallback> callback,
                                      QSharedPointer<SubscriptionQos> qos,
                                      SubscriptionRequest& subscriptionRequest) {
        std::ignore = subscribeToName;
        std::ignore = qos;
        std::ignore = subscriptionRequest;

        QSharedPointer<SubscriptionCallback<joynr::types::GpsLocation, double>> typedCallbackQsp =
                callback.dynamicCast<SubscriptionCallback<joynr::types::GpsLocation, double>>();

        typedCallbackQsp->onSuccess(gpsLocation, floatValue);
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

    std::shared_ptr<MockGpsFloatSubscriptionListener> mockListener(new MockGpsFloatSubscriptionListener());

    EXPECT_CALL(
                        *mockSubscriptionManager,
                        registerSubscription(
                            Eq("locationUpdateWithSpeed"), //broadcastName
                            _,
                            _, // messaging QoS
                            _
                        )).WillRepeatedly(testing::Invoke(this, &TestJoynrMessagingConnectorTest::invokeSubscriptionCallback));
    //   joynr::tests::LocationUpdateWithSpeedSelectiveBroadcastSubscriptionListenerWrapper

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(Eq(joynr::types::GpsLocation::createStd(gpsLocation)), Eq(floatValue)))
            .WillOnce(ReleaseSemaphore(&semaphore));

    joynr::StdOnChangeSubscriptionQos qos;
    connector->subscribeToLocationUpdateWithSpeedBroadcast(mockListener, qos);

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 2000));
}
