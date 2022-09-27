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
#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "joynr/IMiddlewareMessagingStubFactory.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"

#include "tests/mock/MockMessagingStub.h"

using namespace joynr;
using namespace testing;

struct MockMiddlewareMessagingStubFactory : public joynr::IMiddlewareMessagingStubFactory {
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
            : _messagingStubFactory(), _expectedStub(std::make_shared<MockMessagingStub>())
    {
        auto localMockMiddlewareMessagingStubFactory =
                std::make_shared<MockMiddlewareMessagingStubFactory>();
        // retain a pointer for later use in EXPECT_CALL, unique_ptr will be moved when registering
        this->_mockMiddlewareMessagingStubFactory = localMockMiddlewareMessagingStubFactory.get();
        ON_CALL(*(this->_mockMiddlewareMessagingStubFactory), create(_))
                .WillByDefault(Return(_expectedStub));
        ON_CALL(*(this->_mockMiddlewareMessagingStubFactory), canCreate(_))
                .WillByDefault(Return(true));
        _messagingStubFactory.registerStubFactory(localMockMiddlewareMessagingStubFactory);

        std::string host = "test.domain";
        _address = std::make_shared<joynr::system::RoutingTypes::WebSocketAddress>();
        _address->setHost(host);
        _addressCopy = std::make_shared<joynr::system::RoutingTypes::WebSocketAddress>();
        _addressCopy->setHost(host);
    }

protected:
    MessagingStubFactory _messagingStubFactory;
    std::shared_ptr<MockMessagingStub> _expectedStub;
    std::shared_ptr<joynr::system::RoutingTypes::WebSocketAddress> _address;
    std::shared_ptr<joynr::system::RoutingTypes::WebSocketAddress> _addressCopy;
    MockMiddlewareMessagingStubFactory* _mockMiddlewareMessagingStubFactory;
};

TEST_F(MessagingStubFactoryTest, emptyAtBegin)
{
    EXPECT_FALSE(_messagingStubFactory.contains(_address));
}

TEST_F(MessagingStubFactoryTest, createReturnsStub)
{
    std::shared_ptr<IMessagingStub> returnedStub = _messagingStubFactory.create(_address);
    EXPECT_EQ(_expectedStub, returnedStub);
}

TEST_F(MessagingStubFactoryTest, repeatedCreateReturnsSameStub)
{
    EXPECT_CALL(*_mockMiddlewareMessagingStubFactory, create(_)).Times(1);
    std::shared_ptr<IMessagingStub> stub1 = _messagingStubFactory.create(_address);
    ASSERT_TRUE(stub1 != nullptr);
    std::shared_ptr<IMessagingStub> stub2 = _messagingStubFactory.create(_address);
    EXPECT_TRUE(stub1 == stub2);
    std::shared_ptr<IMessagingStub> stub3 = _messagingStubFactory.create(_addressCopy);
    EXPECT_TRUE(stub1 == stub3);
}

TEST_F(MessagingStubFactoryTest, containsFindsStub)
{
    _messagingStubFactory.create(_address);
    EXPECT_TRUE(_messagingStubFactory.contains(_address));
    EXPECT_TRUE(_messagingStubFactory.contains(_addressCopy));
}

TEST_F(MessagingStubFactoryTest, nullptrIsNotInsertedIntoCache)
{
    EXPECT_CALL(*(this->_mockMiddlewareMessagingStubFactory), create(_))
            .WillOnce(Return(std::shared_ptr<IMessagingStub>()));
    _messagingStubFactory.create(_address);
    EXPECT_FALSE(_messagingStubFactory.contains(_address));
    EXPECT_FALSE(_messagingStubFactory.contains(_addressCopy));
}

TEST_F(MessagingStubFactoryTest, emptyAfterRemove)
{
    _messagingStubFactory.create(_address);
    _messagingStubFactory.remove(_address);
    EXPECT_FALSE(_messagingStubFactory.contains(_address));
    EXPECT_FALSE(_messagingStubFactory.contains(_addressCopy));

    _messagingStubFactory.create(_address);
    _messagingStubFactory.remove(_addressCopy);
    EXPECT_FALSE(_messagingStubFactory.contains(_address));
    EXPECT_FALSE(_messagingStubFactory.contains(_addressCopy));
}
