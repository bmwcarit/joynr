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

class MockRoutingProxy : public virtual joynr::system::RoutingProxy {
public:
    MockRoutingProxy(std::weak_ptr<joynr::JoynrRuntime> runtime) :
        RoutingProxy(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos()),
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
                joynr::MessagingQos())
    { }

    MOCK_METHOD5(addNextHopAsync, std::shared_ptr<joynr::Future<void>>(
            const std::string& participantId,
            const joynr::system::RoutingTypes::WebSocketClientAddress& webSocketClientAddress,
            const bool& isGloballyVisible,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError));

    MOCK_METHOD3(resolveNextHopAsync,
                 std::shared_ptr<joynr::Future<bool>>(
                     const std::string& participantId,
                     std::function<void(const bool& resolved)> onSuccess,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError));

    MOCK_METHOD5(addMulticastReceiverAsync,
        std::shared_ptr<joynr::Future<void>> (
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError));

    MOCK_METHOD5(removeMulticastReceiverAsync,
        std::shared_ptr<joynr::Future<void>> (
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError));
};

#endif // TESTS_MOCK_MOCKROUTINGPROXY_H
