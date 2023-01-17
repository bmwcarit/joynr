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
#include "IltAbstractConsumerTest.h"

#include "joynr/ISubscriptionListener.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/Semaphore.h"
#include "joynr/SubscriptionListener.h"
#include "joynr/exceptions/JoynrException.h"

using namespace ::testing;

class MockInt32SubscriptionListener : public joynr::ISubscriptionListener<std::int32_t>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD1(onReceive, void(const std::int32_t& value));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

/**
 * Class to test fireAndForget methods.
 *
 * FireAndForget methods do not have a return value and the calling proxy does not receive an answer
 *to a fireAndForget method call.
 * The attribute attributeFireAndForget is used in fireAndForget method calls to check if the method
 *is called at the provider.
 * The provider will change the attribute to a (fireAndForget) method specific value which will be
 *checked in the subscription listener.
 */
class IltConsumerFireAndForgetMethodTest : public IltAbstractConsumerTest<::testing::Test>
{
public:
    IltConsumerFireAndForgetMethodTest()
            : publicationSemaphore(0),
              onSubscribedSemaphore(0),
              mockInt32SubscriptionListener(new MockInt32SubscriptionListener()),
              attributeFireAndForgetSubscriptionId()
    {
    }

    void SetUp()
    {
        ON_CALL(*mockInt32SubscriptionListener, onSubscribed(_))
                .WillByDefault(ReleaseSemaphore(&onSubscribedSemaphore));
        ON_CALL(*mockInt32SubscriptionListener, onReceive(_))
                .WillByDefault(ReleaseSemaphore(&publicationSemaphore));
        ON_CALL(*mockInt32SubscriptionListener, onError(_))
                .WillByDefault(ReleaseSemaphore(&publicationSemaphore));
    }

    void TearDown()
    {
    }

    void waitForAttributeFireAndForgetPublication();

    void subscribeToAttributeFireAndForget(
            interlanguagetest::TestInterfaceProxy& testInterfaceProxy);

    void unsubscribeAttributeFireAndForget();

    joynr::Semaphore publicationSemaphore;
    joynr::Semaphore onSubscribedSemaphore;
    std::shared_ptr<MockInt32SubscriptionListener> mockInt32SubscriptionListener;
    std::string attributeFireAndForgetSubscriptionId;

protected:
    ADD_LOGGER(IltConsumerFireAndForgetMethodTest)
};

/**
 * Wait for a subsriptionPublication for the attributeFireAndForget.
 */
void IltConsumerFireAndForgetMethodTest::waitForAttributeFireAndForgetPublication()
{
    JOYNR_LOG_INFO(logger(), "waitForAttributeFireAndForgetPublication");
    EXPECT_TRUE(publicationSemaphore.waitFor(publicationTimeoutMs));
    Mock::VerifyAndClearExpectations(mockInt32SubscriptionListener.get());
    JOYNR_LOG_INFO(logger(), "waitForAttributeFireAndForgetPublication - DONE");
}

/**
 * Subscribe to attributeFireAndForget to get notified when the attribute attributeFireAndForget is
 * changed.
 * AttributeFireAndForget is set to 0 first since it might have been set to the expected value by
 * another test.
 */
void IltConsumerFireAndForgetMethodTest::subscribeToAttributeFireAndForget(
        interlanguagetest::TestInterfaceProxy& testInterfaceProxy)
{
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    int64_t publicationTtl = UnicastSubscriptionQos::DEFAULT_PUBLICATION_TTL_MS();
    auto subscriptionQos = std::make_shared<joynr::OnChangeSubscriptionQos>(
            validity, publicationTtl, minInterval_ms);
    EXPECT_CALL(*mockInt32SubscriptionListener, onSubscribed(_)).Times(1);
    EXPECT_CALL(*mockInt32SubscriptionListener, onReceive(0)).Times(1);
    EXPECT_CALL(*mockInt32SubscriptionListener, onError(_)).Times(0);
    JOYNR_ASSERT_NO_THROW({
        testInterfaceProxy.setAttributeFireAndForget(0);
        testInterfaceProxy
                .subscribeToAttributeFireAndForget(mockInt32SubscriptionListener, subscriptionQos)
                ->get(subscriptionIdFutureTimeoutMs, attributeFireAndForgetSubscriptionId);
        EXPECT_TRUE(onSubscribedSemaphore.waitFor(
                std::chrono::milliseconds(subscriptionIdFutureTimeoutMs)));
    });
    JOYNR_LOG_INFO(logger(),
                   "subscribeToAttributeFireAndForget - subscriptionId: " +
                           attributeFireAndForgetSubscriptionId);
    waitForAttributeFireAndForgetPublication();
}

void IltConsumerFireAndForgetMethodTest::unsubscribeAttributeFireAndForget()
{
    JOYNR_LOG_INFO(
            logger(), "unSubscribeAttributeFireAndForget: " + attributeFireAndForgetSubscriptionId);
    JOYNR_ASSERT_NO_THROW({
        testInterfaceProxy->unsubscribeFromAttributeFireAndForget(
                attributeFireAndForgetSubscriptionId);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    JOYNR_LOG_INFO(
            logger(),
            "unSubscribeAttributeFireAndForget: " + attributeFireAndForgetSubscriptionId + " - OK");
}

TEST_F(IltConsumerFireAndForgetMethodTest, callMethodFireAndForgetWithoutParameter)
{
    JOYNR_LOG_INFO(logger(),
                   "callMethodFireAndForgetWithoutParameter - subscribeToAttributeFireAndForget");
    subscribeToAttributeFireAndForget(*testInterfaceProxy);
    int32_t expectedValue = 1;
    EXPECT_CALL(*mockInt32SubscriptionListener, onReceive(expectedValue)).Times(1);
    EXPECT_CALL(*mockInt32SubscriptionListener, onError(_)).Times(0);
    JOYNR_EXPECT_NO_THROW({
        JOYNR_LOG_INFO(logger(), "callMethodFireAndForgetWithoutParameter");
        testInterfaceProxy->methodFireAndForgetWithoutParameter();
        waitForAttributeFireAndForgetPublication();
    });

    unsubscribeAttributeFireAndForget();
}

TEST_F(IltConsumerFireAndForgetMethodTest, callMethodFireAndForgetWithInputParameter)
{
    JOYNR_LOG_INFO(logger(),
                   "callMethodFireAndForgetWithInputParameter - subscribeToAttributeFireAndForget");
    subscribeToAttributeFireAndForget(*testInterfaceProxy);
    int32_t expectedValue = 4242;
    EXPECT_CALL(*mockInt32SubscriptionListener, onReceive(expectedValue)).Times(1);
    EXPECT_CALL(*mockInt32SubscriptionListener, onError(_)).Times(0);
    JOYNR_EXPECT_NO_THROW({
        JOYNR_LOG_INFO(logger(), "callMethodFireAndForgetWithInputParameter");
        testInterfaceProxy->methodFireAndForgetWithInputParameter(expectedValue);
        waitForAttributeFireAndForgetPublication();
    });
    unsubscribeAttributeFireAndForget();
}
