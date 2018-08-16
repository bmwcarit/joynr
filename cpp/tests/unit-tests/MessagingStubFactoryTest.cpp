/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/IMiddlewareMessagingStubFactory.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"

#include "tests/mock/MockMessagingStub.h"

using namespace joynr;
using namespace testing;

struct MockMiddlewareMessagingStubFactory : public joynr::IMiddlewareMessagingStubFactory
{
    MOCK_METHOD1(create,
                 std::shared_ptr<joynr::IMessagingStub>(
                         const joynr::system::RoutingTypes::Address& destAddress));
    MOCK_METHOD1(canCreate, bool(const joynr::system::RoutingTypes::Address& destAddress));
    MOCK_METHOD1(
            registerOnMessagingStubClosedCallback,
            void(std::function<void(std::shared_ptr<const joynr::system::RoutingTypes::Address>
                                            destinationAddress)> onMessagingStubClosedCallback));
};

class MessagingStubFactoryTest : public Test
{
public:
    MessagingStubFactoryTest()
            : messagingStubFactory(), expectedStub(std::make_shared<MockMessagingStub>())
    {
        auto mockMiddlewareMessagingStubFactory =
                std::make_shared<MockMiddlewareMessagingStubFactory>();
        // retain a pointer for later use in EXPECT_CALL, unique_ptr will be moved when registering
        this->mockMiddlewareMessagingStubFactory = mockMiddlewareMessagingStubFactory.get();
        ON_CALL(*(this->mockMiddlewareMessagingStubFactory), create(_))
                .WillByDefault(Return(expectedStub));
        ON_CALL(*(this->mockMiddlewareMessagingStubFactory), canCreate(_))
                .WillByDefault(Return(true));
        messagingStubFactory.registerStubFactory(mockMiddlewareMessagingStubFactory);

        std::string host = "test.domain";
        address = std::make_shared<joynr::system::RoutingTypes::WebSocketAddress>();
        address->setHost(host);
        addressCopy = std::make_shared<joynr::system::RoutingTypes::WebSocketAddress>();
        addressCopy->setHost(host);
    }

protected:
    MessagingStubFactory messagingStubFactory;
    std::shared_ptr<MockMessagingStub> expectedStub;
    std::shared_ptr<joynr::system::RoutingTypes::WebSocketAddress> address;
    std::shared_ptr<joynr::system::RoutingTypes::WebSocketAddress> addressCopy;
    MockMiddlewareMessagingStubFactory* mockMiddlewareMessagingStubFactory;
};

TEST_F(MessagingStubFactoryTest, emptyAtBegin)
{
    EXPECT_FALSE(messagingStubFactory.contains(address));
}

TEST_F(MessagingStubFactoryTest, createReturnsStub)
{
    std::shared_ptr<IMessagingStub> returnedStub = messagingStubFactory.create(address);
    EXPECT_EQ(expectedStub, returnedStub);
}

TEST_F(MessagingStubFactoryTest, repeatedCreateReturnsSameStub)
{
    EXPECT_CALL(*mockMiddlewareMessagingStubFactory, create(_)).Times(1);
    std::shared_ptr<IMessagingStub> stub1 = messagingStubFactory.create(address);
    ASSERT_TRUE(stub1 != nullptr);
    std::shared_ptr<IMessagingStub> stub2 = messagingStubFactory.create(address);
    EXPECT_TRUE(stub1 == stub2);
    std::shared_ptr<IMessagingStub> stub3 = messagingStubFactory.create(addressCopy);
    EXPECT_TRUE(stub1 == stub3);
}

TEST_F(MessagingStubFactoryTest, containsFindsStub)
{
    messagingStubFactory.create(address);
    EXPECT_TRUE(messagingStubFactory.contains(address));
    EXPECT_TRUE(messagingStubFactory.contains(addressCopy));
}

TEST_F(MessagingStubFactoryTest, nullptrIsNotInsertedIntoCache)
{
    EXPECT_CALL(*(this->mockMiddlewareMessagingStubFactory), create(_))
            .WillOnce(Return(std::shared_ptr<IMessagingStub>()));
    messagingStubFactory.create(address);
    EXPECT_FALSE(messagingStubFactory.contains(address));
    EXPECT_FALSE(messagingStubFactory.contains(addressCopy));
}

TEST_F(MessagingStubFactoryTest, emptyAfterRemove)
{
    messagingStubFactory.create(address);
    messagingStubFactory.remove(address);
    EXPECT_FALSE(messagingStubFactory.contains(address));
    EXPECT_FALSE(messagingStubFactory.contains(addressCopy));

    messagingStubFactory.create(address);
    messagingStubFactory.remove(addressCopy);
    EXPECT_FALSE(messagingStubFactory.contains(address));
    EXPECT_FALSE(messagingStubFactory.contains(addressCopy));
}
