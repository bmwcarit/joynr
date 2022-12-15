/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
#include <memory>
#include <string>

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "joynr/Future.h"
#include "joynr/Logger.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/UnicastSubscriptionCallback.h"
#include "joynr/exceptions/SubscriptionException.h"
#include "tests/JoynrTest.h"
#include "tests/mock/MockMessageRouter.h"
#include "tests/mock/MockSubscriptionListener.h"
#include "tests/mock/MockSubscriptionManager.h"

using namespace joynr;
using namespace testing;

class UnicastSubscriptionCallbackTest : public testing::Test
{
public:
    UnicastSubscriptionCallbackTest()
            : singleThreadIOService(std::make_shared<SingleThreadedIOService>()),
              mockMessageRouter(
                      std::make_shared<MockMessageRouter>(singleThreadIOService->getIOService())),
              mockSubscriptionManager(std::make_shared<MockSubscriptionManager>(
                      singleThreadIOService->getIOService(),
                      mockMessageRouter))

    {
    }

    void setOnCalls() const
    {
        ON_CALL(*mockSubscriptionManager, getSubscriptionListener(subscriptionId))
                .WillByDefault(Return(mockSubscriptionListener));
    }
    void createSubscriptionCallback()
    {
        subscriptionCallback = std::make_shared<UnicastSubscriptionCallback<std::string>>(
                subscriptionId, subscriptionIdFuture, mockSubscriptionManager, nullptr);
    }

protected:
    const std::string subscriptionId{ "testSubscriptionId" };
    std::shared_ptr<SingleThreadedIOService> singleThreadIOService;
    std::shared_ptr<MockMessageRouter> mockMessageRouter;
    std::shared_ptr<MockSubscriptionManager> mockSubscriptionManager;
    std::shared_ptr<Future<std::string>> subscriptionIdFuture{ std::make_shared<Future<std::string>>() };
    std::shared_ptr<MockSubscriptionListenerOneType<std::string>> mockSubscriptionListener
        { std::make_shared<MockSubscriptionListenerOneType<std::string>>() };

    std::shared_ptr<UnicastSubscriptionCallback<std::string>> subscriptionCallback;
};

TEST_F(UnicastSubscriptionCallbackTest, forwardSubscriptionReplyToFutureAndListener)
{
    EXPECT_CALL(*(this->mockSubscriptionListener), onSubscribed(this->subscriptionId));
    this->setOnCalls();

    SubscriptionReply reply;
    reply.setSubscriptionId(this->subscriptionId);

    this->createSubscriptionCallback();
    this->subscriptionCallback->execute(reply);

    std::string subscriptionIdFromFuture;
    this->subscriptionIdFuture->get(100, subscriptionIdFromFuture);
    EXPECT_EQ(subscriptionIdFromFuture, this->subscriptionId);
}

TEST_F(UnicastSubscriptionCallbackTest, forwardSubscriptionExceptionToFutureAndListener)
{
    SubscriptionReply reply;
    reply.setSubscriptionId(this->subscriptionId);
    auto expectedException =
            std::make_shared<exceptions::SubscriptionException>(this->subscriptionId);
    reply.setError(expectedException);

    EXPECT_CALL(*(this->mockSubscriptionManager), unregisterSubscription(this->subscriptionId));
    EXPECT_CALL(*(this->mockSubscriptionListener), onError(*expectedException)).Times(1);
    EXPECT_CALL(*(this->mockSubscriptionListener), onReceive(_)).Times(0);
    this->setOnCalls();

    this->createSubscriptionCallback();
    this->subscriptionCallback->execute(reply);

    try {
        std::string subscriptionIdFromFuture;
        this->subscriptionIdFuture->get(100, subscriptionIdFromFuture);
        ADD_FAILURE() << "expected SubscriptionException";
    } catch (const exceptions::SubscriptionException& error) {
        EXPECT_EQ(error, *expectedException);
    }
}

TEST_F(UnicastSubscriptionCallbackTest, forwardPublicationToListener)
{
    const std::string response = "testResponse";

    EXPECT_CALL(*(this->mockSubscriptionListener), onError(_)).Times(0);
    EXPECT_CALL(*(this->mockSubscriptionListener), onReceive(response)).Times(1);
    this->setOnCalls();

    this->createSubscriptionCallback();
    SubscriptionPublication publication;
    publication.setResponse(response);
    this->subscriptionCallback->execute(std::move(publication));
}

TEST_F(UnicastSubscriptionCallbackTest, forwardPublicationErrorToListener)
{
    auto error = std::make_shared<exceptions::ProviderRuntimeException>("testException");

    EXPECT_CALL(*(this->mockSubscriptionListener), onError(*error)).Times(1);
    EXPECT_CALL(*(this->mockSubscriptionListener), onReceive(_)).Times(0);
    this->setOnCalls();

    this->createSubscriptionCallback();
    SubscriptionPublication publication;
    publication.setError(error);
    this->subscriptionCallback->execute(std::move(publication));
}
