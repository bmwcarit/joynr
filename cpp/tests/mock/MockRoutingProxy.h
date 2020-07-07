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
#ifndef TESTS_MOCK_MOCKROUTINGPROXY_H
#define TESTS_MOCK_MOCKROUTINGPROXY_H

#include <gmock/gmock.h>

#include "joynr/MessagingQos.h"
#include "joynr/system/RoutingProxy.h"

namespace joynr
{
class JoynrRuntimeImpl;
} // namespace joynr

class MockRoutingProxy : public virtual joynr::system::RoutingProxy {
public:
    MockRoutingProxy(std::weak_ptr<joynr::JoynrRuntimeImpl> runtime) :
        ProxyBase(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos()),
        RoutingProxyBase(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos()),
        RoutingSyncProxy(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos()),
        RoutingAsyncProxy(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos()),
        RoutingProxy(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos())
    { }

    std::shared_ptr<joynr::Future<void>> addNextHopAsync(
            const std::string& participantId,
            const joynr::system::RoutingTypes::WebSocketClientAddress& webSocketClientAddress,
            const bool& isGloballyVisible,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
            boost::optional<joynr::MessagingQos> qos
        ) noexcept override
    {
        return addNextHopAsyncMockWs(
                participantId,
                webSocketClientAddress,
                isGloballyVisible,
                std::move(onSuccess),
                std::move(onRuntimeError),
                std::move(qos));
    }
    MOCK_METHOD6(addNextHopAsyncMockWs, std::shared_ptr<joynr::Future<void>>(
            const std::string& participantId,
            const joynr::system::RoutingTypes::WebSocketClientAddress& webSocketClientAddress,
            const bool& isGloballyVisible,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
            boost::optional<joynr::MessagingQos> qos));

    std::shared_ptr<joynr::Future<void>> addNextHopAsync(
            const std::string& participantId,
            const joynr::system::RoutingTypes::UdsClientAddress& udsClientAddress,
            const bool& isGloballyVisible,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
            boost::optional<joynr::MessagingQos> qos
        ) noexcept override
    {
        return addNextHopAsyncMockUds(
                participantId,
                udsClientAddress,
                isGloballyVisible,
                std::move(onSuccess),
                std::move(onRuntimeError),
                std::move(qos));
    }
    MOCK_METHOD6(addNextHopAsyncMockUds, std::shared_ptr<joynr::Future<void>>(
            const std::string& participantId,
            const joynr::system::RoutingTypes::UdsClientAddress& udsClientAddress,
            const bool& isGloballyVisible,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
            boost::optional<joynr::MessagingQos> qos));

    std::shared_ptr<joynr::Future<bool>> resolveNextHopAsync(
             const std::string& participantId,
             std::function<void(const bool& resolved)> onSuccess,
             std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
             boost::optional<joynr::MessagingQos> qos
         ) noexcept override
    {
        return resolveNextHopAsyncMock(participantId, std::move(onSuccess), std::move(onRuntimeError), std::move(qos));
    }
    MOCK_METHOD4(resolveNextHopAsyncMock,
        std::shared_ptr<joynr::Future<bool>>(
            const std::string& participantId,
                     std::function<void(const bool& resolved)> onSuccess,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
                     boost::optional<joynr::MessagingQos> qos));

    std::shared_ptr<joynr::Future<void>> addMulticastReceiverAsync(
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
            boost::optional<joynr::MessagingQos> qos
        ) noexcept override
    {
        return addMulticastReceiverAsyncMock(
                multicastId,
                subscriberParticipantId,
                providerParticipantId,
                std::move(onSuccess),
                std::move(onRuntimeError),
                std::move(qos));
    }
    MOCK_METHOD6(addMulticastReceiverAsyncMock,
        std::shared_ptr<joynr::Future<void>> (
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
            boost::optional<joynr::MessagingQos> qos));

    std::shared_ptr<joynr::Future<void>> removeMulticastReceiverAsync(
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
            boost::optional<joynr::MessagingQos> qos
        ) noexcept override
    {
        return removeMulticastReceiverAsyncMock(
                multicastId,
                subscriberParticipantId,
                providerParticipantId,
                std::move(onSuccess),
                std::move(onRuntimeError),
                std::move(qos));
    }
    MOCK_METHOD6(removeMulticastReceiverAsyncMock,
        std::shared_ptr<joynr::Future<void>> (
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError,
            boost::optional<joynr::MessagingQos> qos));
};

#endif // TESTS_MOCK_MOCKROUTINGPROXY_H
