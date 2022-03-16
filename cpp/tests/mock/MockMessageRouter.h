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
#ifndef TESTS_MOCK_MOCKMESSAGEROUTER_H
#define TESTS_MOCK_MOCKMESSAGEROUTER_H

#include <cstdint>

#include <gmock/gmock.h>
#include <boost/asio.hpp>

#include "joynr/IMessageRouter.h"
#include "joynr/ImmutableMessage.h"

class MockMessageRouter : public joynr::IMessageRouter {
public:
    void invokeAddNextHopOnSuccessFct(const std::string& /*participantId*/,
            const std::shared_ptr<const joynr::system::RoutingTypes::Address>& /*inprocessAddress*/,
            const bool& /*isGloballyVisible*/,
            const std::int64_t /*expiryDateMs*/,
            const bool /*isSticky*/,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> /*onError*/) {
        if (onSuccess) {
            onSuccess();
        }
    }
    void invokeRemoveNextHopOnSuccessFct(const std::string& /*participantId*/,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> /*onError*/) {
        if (onSuccess) {
            onSuccess();
        }
    }

    MockMessageRouter(boost::asio::io_service& ioService)
    {
        std::ignore = ioService;
        using ::testing::_;
        ON_CALL(
                *this,
                addNextHop(_,_,_,_,_,_,_)
        )
                .WillByDefault(testing::Invoke(this, &MockMessageRouter::invokeAddNextHopOnSuccessFct));
        ON_CALL(
                *this,
                removeNextHop(_,_,_)
        )
                .WillByDefault(testing::Invoke(this, &MockMessageRouter::invokeRemoveNextHopOnSuccessFct));
    }

    MOCK_METHOD2(route, void(std::shared_ptr<joynr::ImmutableMessage> message, std::uint32_t tryCount));

    MOCK_METHOD1(publishToGlobal, bool(const joynr::ImmutableMessage& message));

    MOCK_METHOD6(registerMulticastReceiver, void(const std::string& multicastId,
                                                 const std::string& subscriberParticipantId,
                                                 const std::string& providerParticipantId,
                                                 std::shared_ptr<const joynr::system::RoutingTypes::Address> providerAddress,
                                                 std::function<void()> onSuccess,
                                                 std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError));

    MOCK_METHOD7(addNextHop, void(
            const std::string& participantId,
            const std::shared_ptr<const joynr::system::RoutingTypes::Address>& inprocessAddress,
            bool isGloballyVisible,
            const std::int64_t expiryDateMs,
            const bool isSticky,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError));

    MOCK_METHOD3(removeNextHop, void(
            const std::string& participantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError));

    MOCK_METHOD5(addMulticastReceiver, void(
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError));

    MOCK_METHOD5(removeMulticastReceiver, void(
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError));

    MOCK_METHOD1(queueMessage, void(std::shared_ptr<joynr::ImmutableMessage> message));
    MOCK_METHOD1(sendQueuedMessages, void(std::shared_ptr<const joynr::system::RoutingTypes::Address> address));
    MOCK_METHOD1(setToKnown, void(const std::string& participantId));
};

#endif // TESTS_MOCK_MOCKMESSAGEROUTER_H
