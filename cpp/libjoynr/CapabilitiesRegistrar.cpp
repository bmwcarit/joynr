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

INIT_LOGGER(CapabilitiesRegistrar);

CapabilitiesRegistrar::CapabilitiesRegistrar(
        std::vector<IDispatcher*> dispatcherList,
        system::IDiscoveryAsync& discoveryProxy,
        std::shared_ptr<ParticipantIdStorage> participantIdStorage,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> dispatcherAddress,
        std::shared_ptr<IMessageRouter> messageRouter,
        std::int64_t defaultExpiryIntervalMs,
        PublicationManager& publicationManager)
        : dispatcherList(dispatcherList),
          discoveryProxy(discoveryProxy),
          participantIdStorage(participantIdStorage),
          dispatcherAddress(dispatcherAddress),
          messageRouter(messageRouter),
          defaultExpiryIntervalMs(defaultExpiryIntervalMs),
          publicationManager(publicationManager)
{
}

void CapabilitiesRegistrar::removeAsync(
        const std::string& participantId,
        std::function<void()> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException&)> onError)
{
    for (IDispatcher* currentDispatcher : dispatcherList) {
        currentDispatcher->removeRequestCaller(participantId);
    }

    auto onSuccessWrapper = [
        messageRouter = util::as_weak_ptr(messageRouter),
        participantId,
        onSuccess = std::move(onSuccess),
        onError
    ]
    {
        if (auto ptr = messageRouter.lock()) {
            ptr->removeNextHop(participantId, std::move(onSuccess), std::move(onError));
        }
    };

    discoveryProxy.removeAsync(participantId, std::move(onSuccessWrapper), std::move(onError));
}

void CapabilitiesRegistrar::addDispatcher(IDispatcher* dispatcher)
{
    dispatcherList.push_back(dispatcher);
}

void CapabilitiesRegistrar::removeDispatcher(IDispatcher* dispatcher)
{
    util::removeAll(dispatcherList, dispatcher);
}

} // namespace joynr
