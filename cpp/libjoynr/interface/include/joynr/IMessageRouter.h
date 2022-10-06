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
#ifndef IMESSAGEROUTER_H
#define IMESSAGEROUTER_H

#include <cstdint>
#include <functional>
#include <memory>

#include "joynr/JoynrExport.h"

namespace joynr
{
class IAccessController;
class ImmutableMessage;

namespace system
{
namespace RoutingTypes
{
class Address;
} // namespace RoutingTypes
} // namespace system

namespace exceptions
{
class ProviderRuntimeException;
class JoynrRuntimeException;
} // namespace exceptions

class JOYNR_EXPORT IMessageRouter
{
public:
    virtual ~IMessageRouter() = default;

    virtual void route(std::shared_ptr<ImmutableMessage> message, std::uint32_t tryCount = 0) = 0;

    virtual void addNextHop(
            const std::string& participantId,
            const std::shared_ptr<const joynr::system::RoutingTypes::Address>& address,
            bool isGloballyVisible,
            const std::int64_t expiryDateMs,
            const bool isSticky,
            std::function<void()> onSuccess = nullptr,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError =
                    nullptr) = 0;

    virtual void removeNextHop(
            const std::string& participantId,
            std::function<void()> onSuccess = nullptr,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError =
                    nullptr) = 0;

    virtual void addMulticastReceiver(
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::function<void()> onSuccess = nullptr,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError =
                    nullptr) = 0;

    virtual void removeMulticastReceiver(
            const std::string& multicastId,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            std::function<void()> onSuccess = nullptr,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError =
                    nullptr) = 0;

    virtual void sendQueuedMessages(
            std::shared_ptr<const joynr::system::RoutingTypes::Address> address) = 0;

    virtual void setToKnown(const std::string& participantId) = 0;
};

} // namespace joynr
#endif // IMESSAGEROUTER_H
