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
#include "joynr/IReplyCaller.h"
#include <string>

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
class TestJoynrMessagingConnectorTest : public AbstractSyncAsyncTest {
public:

    TestJoynrMessagingConnectorTest() {}
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

    tests::Itest* createFixture(bool cacheEnabled) {

        tests::testJoynrMessagingConnector* connector = new tests::testJoynrMessagingConnector(
                    mockJoynrMessageSender,
                    (SubscriptionManager*) NULL,
                    "myDomain",
                    proxyParticipantId,
                    providerParticipantId,
                    MessagingQos(),
                    &mockClientCache,
                    cacheEnabled);

        return dynamic_cast<tests::Itest*>(connector);
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
