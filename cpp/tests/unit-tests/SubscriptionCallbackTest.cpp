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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/Logger.h"
#include "joynr/exceptions/SubscriptionException.h"
#include "joynr/Future.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/UnicastSubscriptionCallback.h"
#include "joynr/MulticastSubscriptionCallback.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionReply.h"
#include "tests/JoynrTest.h"
#include "tests/mock/MockSubscriptionManager.h"
#include "tests/mock/MockSubscriptionListener.h"
#include "tests/mock/MockMessageRouter.h"

using namespace joynr;
using namespace testing;

template <typename SubscriptionCallbackType>
class SubscriptionCallbackTest : public testing::Test
{
public:
    SubscriptionCallbackTest()
            : subscriptionId("testSubscriptionId"),
              singleThreadIOService(),
              mockMessageRouter(
                      std::make_shared<MockMessageRouter>(singleThreadIOService.getIOService())),
              mockSubscriptionManager(std::make_shared<MockSubscriptionManager>(
                      singleThreadIOService.getIOService(),
                      mockMessageRouter)),
              subscriptionIdFuture(std::make_shared<Future<std::string>>()),
              mockSubscriptionListener(
                      std::make_shared<MockSubscriptionListenerOneType<std::string>>()),
              subscriptionCallback(subscriptionId, subscriptionIdFuture, mockSubscriptionManager)
    {
        ON_CALL(*mockSubscriptionManager, getSubscriptionListener(subscriptionId))
                .WillByDefault(Return(mockSubscriptionListener));
        ON_CALL(*mockSubscriptionManager, getMulticastSubscriptionListeners(subscriptionId))
                .WillByDefault(
                        Return(std::forward_list<std::shared_ptr<joynr::ISubscriptionListenerBase>>{
                                mockSubscriptionListener}));
    }

protected:
    const std::string subscriptionId;
    SingleThreadedIOService singleThreadIOService;
    std::shared_ptr<MockMessageRouter> mockMessageRouter;
    std::shared_ptr<MockSubscriptionManager> mockSubscriptionManager;
    std::shared_ptr<Future<std::string>> subscriptionIdFuture;
    std::shared_ptr<MockSubscriptionListenerOneType<std::string>> mockSubscriptionListener;

    SubscriptionCallbackType subscriptionCallback;
};

typedef ::testing::Types<MulticastSubscriptionCallback<std::string>,
                         UnicastSubscriptionCallback<std::string>> SubscriptionCallbackTypes;

TYPED_TEST_SUITE(SubscriptionCallbackTest, SubscriptionCallbackTypes,);

TYPED_TEST(SubscriptionCallbackTest, forwardSubscriptionReplyToFutureAndListener)
{
    SubscriptionReply reply;
    reply.setSubscriptionId(this->subscriptionId);

    EXPECT_CALL(*(this->mockSubscriptionListener), onSubscribed(this->subscriptionId));

    this->subscriptionCallback.execute(reply);

    std::string subscriptionIdFromFuture;
    this->subscriptionIdFuture->get(100, subscriptionIdFromFuture);
    EXPECT_EQ(subscriptionIdFromFuture, this->subscriptionId);
}

TYPED_TEST(SubscriptionCallbackTest, forwardSubscriptionExceptionToFutureAndListener)
{
    SubscriptionReply reply;
    reply.setSubscriptionId(this->subscriptionId);
    auto expectedException =
            std::make_shared<exceptions::SubscriptionException>(this->subscriptionId);
    reply.setError(expectedException);

    EXPECT_CALL(*(this->mockSubscriptionManager), unregisterSubscription(this->subscriptionId));
    EXPECT_CALL(*(this->mockSubscriptionListener), onError(*expectedException)).Times(1);
    EXPECT_CALL(*(this->mockSubscriptionListener), onReceive(_)).Times(0);

    this->subscriptionCallback.execute(reply);

    try {
        std::string subscriptionIdFromFuture;
        this->subscriptionIdFuture->get(100, subscriptionIdFromFuture);
        ADD_FAILURE() << "expected SubscriptionException";
    } catch (const exceptions::SubscriptionException& error) {
        EXPECT_EQ(error, *expectedException);
    }
}

TYPED_TEST(SubscriptionCallbackTest, forwardSubscriptionPublicationToListener)
{
    const std::string response = "testResponse";
    SubscriptionPublication publication;
    publication.setSubscriptionId(this->subscriptionId);
    publication.setResponse(response);

    EXPECT_CALL(*(this->mockSubscriptionListener), onError(_)).Times(0);
    EXPECT_CALL(*(this->mockSubscriptionListener), onReceive(response)).Times(1);

    this->subscriptionCallback.execute(std::move(publication));
}

TYPED_TEST(SubscriptionCallbackTest, forwardSubscriptionPublicationErrorToListener)
{
    auto error = std::make_shared<exceptions::ProviderRuntimeException>("testException");
    SubscriptionPublication publication;
    publication.setSubscriptionId(this->subscriptionId);
    publication.setError(error);

    EXPECT_CALL(*(this->mockSubscriptionListener), onError(*error)).Times(1);
    EXPECT_CALL(*(this->mockSubscriptionListener), onReceive(_)).Times(0);

    this->subscriptionCallback.execute(std::move(publication));
}
