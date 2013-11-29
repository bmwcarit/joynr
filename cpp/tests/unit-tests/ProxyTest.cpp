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
#include "joynr/PrivateCopyAssign.h"
#include "AbstractSyncAsyncTest.cpp"
#include "vector"
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
 * These tests test the communication from the GpsProxy through to the JoynrMessageSender.
 */

/**
 * @brief Fixutre.
 */
class ProxyTest : public AbstractSyncAsyncTest {
public:

    ProxyTest() :
        mockConnectorFactory(),
        mockInProcessConnectorFactory(),
        mockCapabilities()
    {}
    void SetUp() {
        AbstractSyncAsyncTest::SetUp();
        mockInProcessConnectorFactory = new MockInProcessConnectorFactory();
        JoynrMessagingConnectorFactory* joynrMessagingConnectorFactory = new JoynrMessagingConnectorFactory(mockJoynrMessageSender, (SubscriptionManager*) NULL);
        mockConnectorFactory = new ConnectorFactory(mockInProcessConnectorFactory, joynrMessagingConnectorFactory);
        mockCapabilities = new MockCapabilitiesStub();
    }

    void TearDown(){
        AbstractSyncAsyncTest::TearDown();
        delete mockConnectorFactory;
        delete mockCapabilities;
    }

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
                        _, // sender participant ID
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

    tests::ITest* createFixture(bool cacheEnabled) {
        EXPECT_CALL(*mockInProcessConnectorFactory, canBeCreated(_)).WillRepeatedly(Return(false));
        tests::TestProxy* proxy = new tests::TestProxy(
                    mockCapabilities,
                    endPointAddress,
                    mockConnectorFactory,
                    &mockClientCache,
                    QString("myDomain"),
                    ProxyQos(),
                    MessagingQos(),
                    cacheEnabled
                    );
        proxy->handleArbitrationFinished(providerParticipantId, endPointAddress);
        return dynamic_cast<tests::ITest*>(proxy);
    }

protected:
    ConnectorFactory* mockConnectorFactory;
    MockInProcessConnectorFactory* mockInProcessConnectorFactory;
    MockCapabilitiesStub* mockCapabilities;
private:
    DISALLOW_COPY_AND_ASSIGN(ProxyTest);
};

typedef ProxyTest ProxyTestDeathTest;

// need to stub the connector factory for it to always return a Joynr connector
TEST_F(ProxyTest, async_getAttributeNotCached) {
    testAsync_getAttributeNotCached();
}

TEST_F(ProxyTest, sync_setAttributeNotCached) {
    testSync_setAttributeNotCached();
}

TEST_F(ProxyTest, sync_getAttributeNotCached) {
    testSync_getAttributeNotCached();
}

TEST_F(ProxyTest, async_getAttributeCached) {
    testAsync_getAttributeCached();
}

TEST_F(ProxyTest, sync_getAttributeCached) {
    testSync_getAttributeCached();
}

TEST_F(ProxyTest, async_OperationWithNoArguments) {
    testAsync_OperationWithNoArguments();
}

TEST_F(ProxyTest, sync_OperationWithNoArguments) {
    testSync_OperationWithNoArguments();
}

TEST_F(ProxyTest, subscribeToAttribute) {
    testSubscribeToAttribute();
}
