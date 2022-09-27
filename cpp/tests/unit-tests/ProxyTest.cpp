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

#include "AbstractSyncAsyncTest.cpp"

#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/JoynrRuntimeImpl.h"
#include "joynr/MulticastSubscriptionQos.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Settings.h"
#include "joynr/tests/Itest.h"
#include "joynr/tests/ItestConnector.h"
#include "joynr/tests/TestWithoutVersionProxy.h"
#include "joynr/tests/testProxy.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"

#include "tests/mock/MockJoynrRuntime.h"

using ::testing::_;
using ::testing::A;
using ::testing::AllOf;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::NotNull;
using ::testing::Property;
using ::testing::Return;
using ::testing::Unused;

using namespace joynr;

/**
 * These tests test the communication from the GpsProxy through to the JoynrMessageSender.
 */

/**
 * @brief Fixture.
 */
class ProxyTest : public AbstractSyncAsyncTest
{
public:
    ProxyTest() : joynrMessagingConnectorFactory()
    {
    }
    void SetUp() override
    {
        AbstractSyncAsyncTest::SetUp();
        joynrMessagingConnectorFactory = std::make_unique<JoynrMessagingConnectorFactory>(
                mockMessageSender, mockSubscriptionManager);
        auto settings = std::make_unique<Settings>();
        runtime = std::make_shared<MockJoynrRuntime>(std::move(settings));
    }

    void TearDown() override
    {
        AbstractSyncAsyncTest::TearDown();
        joynrMessagingConnectorFactory.reset();
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
                        _,                         // sender participant ID
                        Eq(providerParticipantId), // receiver participant ID
                        _,                         // messaging QoS
                        Property(&Request::getMethodName, Eq(methodName)), // request object to send
                        Property(&std::shared_ptr<IReplyCaller>::get,
                                 NotNull()), // reply caller to notify when reply is received
                        _                    // isLocalFlag
                        ));
    }

    std::shared_ptr<tests::Itest> createItestFixture() override
    {
        std::shared_ptr<tests::testProxy> proxy = createProxy();
        return std::dynamic_pointer_cast<tests::Itest>(proxy);
    }

    std::shared_ptr<tests::ItestSubscription> createItestSubscriptionFixture() override
    {
        std::shared_ptr<tests::testProxy> proxy = createProxy();
        return std::dynamic_pointer_cast<tests::ItestSubscription>(proxy);
    }

protected:
    std::unique_ptr<JoynrMessagingConnectorFactory> joynrMessagingConnectorFactory;
    std::shared_ptr<JoynrRuntimeImpl> runtime;

private:
    std::shared_ptr<tests::testProxy> createProxy()
    {
        std::shared_ptr<tests::testProxy> proxy = std::make_shared<tests::testProxy>(
                runtime, std::move(joynrMessagingConnectorFactory), "myDomain", MessagingQos());
        types::DiscoveryEntryWithMetaInfo discoveryEntry;
        discoveryEntry.setParticipantId(providerParticipantId);
        discoveryEntry.setIsLocal(true);
        proxyParticipantId = proxy->getProxyParticipantId();
        proxy->handleArbitrationFinished(discoveryEntry);

        return proxy;
    }

    DISALLOW_COPY_AND_ASSIGN(ProxyTest);
};

typedef ProxyTest ProxyTestDeathTest;

// need to stub the connector factory for it to always return a Joynr connector
TEST_F(ProxyTest, async_getAttributeNotCached)
{
    testAsync_getAttributeNotCached();
}

TEST_F(ProxyTest, sync_setAttributeNotCached)
{
    testSync_setAttributeNotCached();
}

TEST_F(ProxyTest, sync_getAttributeNotCached)
{
    testSync_getAttributeNotCached();
}

TEST_F(ProxyTest, async_getterCallReturnsProviderRuntimeException)
{
    testAsync_getterCallReturnsProviderRuntimeException();
}

TEST_F(ProxyTest, sync_getterCallReturnsProviderRuntimeException)
{
    testSync_getterCallReturnsProviderRuntimeException();
}

TEST_F(ProxyTest, async_getterCallReturnsMethodInvocationException)
{
    testAsync_getterCallReturnsMethodInvocationException();
}

TEST_F(ProxyTest, sync_getterCallReturnsMethodInvocationException)
{
    testSync_getterCallReturnsMethodInvocationException();
}

TEST_F(ProxyTest, async_setterCallReturnsProviderRuntimeException)
{
    testAsync_setterCallReturnsProviderRuntimeException();
}

TEST_F(ProxyTest, sync_setterCallReturnsProviderRuntimeException)
{
    testSync_setterCallReturnsProviderRuntimeException();
}

TEST_F(ProxyTest, async_setterCallReturnsMethodInvocationException)
{
    testAsync_setterCallReturnsMethodInvocationException();
}

TEST_F(ProxyTest, sync_setterCallReturnsMethodInvocationException)
{
    testSync_setterCallReturnsMethodInvocationException();
}

TEST_F(ProxyTest, async_methodCallReturnsProviderRuntimeException)
{
    testAsync_methodCallReturnsProviderRuntimeException();
}

TEST_F(ProxyTest, sync_methodCallReturnsProviderRuntimeException)
{
    testSync_methodCallReturnsProviderRuntimeException();
}

TEST_F(ProxyTest, async_methodCallReturnsMethodInvocationException)
{
    testAsync_methodCallReturnsMethodInvocationException();
}

TEST_F(ProxyTest, sync_methodCallReturnsMethodInvocationException)
{
    testSync_methodCallReturnsMethodInvocationException();
}

TEST_F(ProxyTest, async_methodCallReturnsErrorEnum)
{
    testAsync_methodCallReturnsErrorEnum();
}

TEST_F(ProxyTest, sync_methodCallReturnsErrorEnum)
{
    testSync_methodCallReturnsErrorEnum();
}

TEST_F(ProxyTest, async_methodCallReturnsExtendedErrorEnum)
{
    testAsync_methodCallReturnsExtendedErrorEnum();
}

TEST_F(ProxyTest, sync_methodCallReturnsExtendedErrorEnum)
{
    testSync_methodCallReturnsExtendedErrorEnum();
}

TEST_F(ProxyTest, async_methodCallReturnsInlineErrorEnum)
{
    testAsync_methodCallReturnsInlineErrorEnum();
}

TEST_F(ProxyTest, sync_methodCallReturnsInlineErrorEnum)
{
    testSync_methodCallReturnsInlineErrorEnum();
}

TEST_F(ProxyTest, async_OperationWithNoArguments)
{
    testAsync_OperationWithNoArguments();
}

TEST_F(ProxyTest, sync_OperationWithNoArguments)
{
    testSync_OperationWithNoArguments();
}

TEST_F(ProxyTest, subscribeToAttribute)
{
    testSubscribeToAttribute();
}

TEST_F(ProxyTest, versionIsSetCorrectly)
{
    std::uint32_t expectedMajorVersion = 47;
    std::uint32_t expectedMinorVersion = 11;
    EXPECT_EQ(expectedMajorVersion, tests::testProxy::MAJOR_VERSION);
    EXPECT_EQ(expectedMinorVersion, tests::testProxy::MINOR_VERSION);
}

TEST_F(ProxyTest, defaultVersionIsSetCorrectly)
{
    std::uint32_t expectedDefaultMajorVersion = 0;
    std::uint32_t expectedDefaultMinorVersion = 0;
    EXPECT_EQ(expectedDefaultMajorVersion, tests::TestWithoutVersionProxy::MAJOR_VERSION);
    EXPECT_EQ(expectedDefaultMinorVersion, tests::TestWithoutVersionProxy::MINOR_VERSION);
}

TEST_F(ProxyTest, doNotSendSubscriptionStopForMulticastSubscription)
{
    doNotSendSubscriptionStopForMulticastSubscription();
}

TEST_F(ProxyTest, sendSubscriptionStopForSelectiveSubscription)
{
    sendSubscriptionStopForSelectiveSubscription();
}
