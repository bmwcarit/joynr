/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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
#include "joynr/vehicle/GpsJoynrMessagingConnector.h"
#include "joynr/IReplyCaller.h"

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
class GpsJoynrMessagingConnectorTest : public AbstractSyncAsyncTest {
public:

    GpsJoynrMessagingConnectorTest() {}
    // sets the expectations on the call expected on the MessageSender from the connector
    testing::internal::TypedExpectation<void(
            const QString&, // sender participant ID
            const QString&, // receiver participant ID
            const MessagingQos&, // messaging QoS
            const Request&, // request object to send
            QSharedPointer<IReplyCaller> // reply caller to notify when reply is received
    )>& setExpectationsForSendRequestCall(QString expectedType, QString methodName) {
        return EXPECT_CALL(
                    *mockJoynrMessageSender,
                    sendRequest(
                        Eq(proxyParticipantId), // sender participant ID
                        Eq(providerParticipantId), // receiver participant ID
                        _, // messaging QoS
                        Property(&Request::getMethodName, Eq(methodName)), // request object to send
                        Property(
                            &QSharedPointer<IReplyCaller>::data,
                            AllOf(NotNull(), Property(&IReplyCaller::getTypeName, Eq(expectedType)))
                        ) // reply caller to notify when reply is received
                    )
        );
    }

    vehicle::IGps* createFixture(bool cacheEnabled) {

        vehicle::GpsJoynrMessagingConnector* connector = new vehicle::GpsJoynrMessagingConnector(
                    mockJoynrMessageSender,
                    (SubscriptionManager*) NULL,
                    "myDomain",
                    proxyParticipantId,
                    providerParticipantId,
                    MessagingQos(),
                    &mockClientCache,
                    cacheEnabled,
                    0);

        return dynamic_cast<vehicle::IGps*>(connector);
    }

};

typedef GpsJoynrMessagingConnectorTest GpsJoynrMessagingConnectorTestDeathTest;


/*
 * Tests
 */

TEST_F(GpsJoynrMessagingConnectorTest, async_getAttributeNotCached) {
    testAsync_getAttributeNotCached();
}

TEST_F(GpsJoynrMessagingConnectorTest, sync_setAttributeNotCached) {
    testSync_setAttributeNotCached();
}


TEST_F(GpsJoynrMessagingConnectorTest, sync_getAttributeNotCached) {
    testSync_getAttributeNotCached();
}

TEST_F(GpsJoynrMessagingConnectorTest, async_getAttributeCached) {
    testAsync_getAttributeCached();
}

TEST_F(GpsJoynrMessagingConnectorTest, sync_getAttributeCached) {
    testSync_getAttributeCached();
}

TEST_F(GpsJoynrMessagingConnectorTest, async_OperationWithNoArguments) {
    testAsync_OperationWithNoArguments();
}

TEST_F(GpsJoynrMessagingConnectorTest, sync_OperationWithNoArguments) {
    testSync_OperationWithNoArguments();
}

TEST_F(GpsJoynrMessagingConnectorTest, subscribeToAttribute) {
    testSubscribeToAttribute();
}
