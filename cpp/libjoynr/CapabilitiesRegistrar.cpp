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
#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/ParticipantIdStorage.h"

namespace joynr
{

CapabilitiesRegistrar::CapabilitiesRegistrar(
        std::vector<std::shared_ptr<IDispatcher>> dispatcherList,
        std::shared_ptr<system::IDiscoveryAsync> discoveryProxy,
        std::shared_ptr<ParticipantIdStorage> participantIdStorage,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> dispatcherAddress,
        std::shared_ptr<IMessageRouter> messageRouter,
        std::int64_t defaultExpiryIntervalMs,
        std::weak_ptr<PublicationManager> publicationManager,
        const std::string& globalAddress)
        : dispatcherList(std::move(dispatcherList)),
          discoveryProxy(discoveryProxy),
          participantIdStorage(std::move(participantIdStorage)),
          dispatcherAddress(std::move(dispatcherAddress)),
          messageRouter(std::move(messageRouter)),
          defaultExpiryIntervalMs(defaultExpiryIntervalMs),
          publicationManager(std::move(publicationManager)),
          globalAddress(globalAddress)
{
}

void CapabilitiesRegistrar::removeAsync(
        const std::string& participantId,
        std::function<void()> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException&)> onError) noexcept
{
    auto onSuccessWrapper = [
        dispatcherList = this->dispatcherList,
        messageRouter = util::as_weak_ptr(messageRouter),
        participantId,
        onSuccess = std::move(onSuccess),
        onError
    ]()
    {
        for (std::shared_ptr<IDispatcher> currentDispatcher : dispatcherList) {
            currentDispatcher->removeRequestCaller(participantId);
        }

        if (auto ptr = messageRouter.lock()) {
            ptr->removeNextHop(participantId, std::move(onSuccess), std::move(onError));
        }
    };

    discoveryProxy->removeAsync(participantId, std::move(onSuccessWrapper), std::move(onError));
}

void CapabilitiesRegistrar::addDispatcher(std::shared_ptr<IDispatcher> dispatcher)
{
    dispatcherList.push_back(std::move(dispatcher));
}

void CapabilitiesRegistrar::removeDispatcher(std::shared_ptr<IDispatcher> dispatcher)
{
    util::removeAll(dispatcherList, dispatcher);
}

} // namespace joynr
