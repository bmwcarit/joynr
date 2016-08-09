/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

#include "joynr/AbstractJoynrProvider.h"

#include <cassert>

#include "joynr/Util.h"

namespace joynr
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations" // remove if providerQos is removed
AbstractJoynrProvider::AbstractJoynrProvider()
        : providerQos(), lock(), attributeListeners(), broadcastListeners()
{
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations" // remove if providerQos is removed
AbstractJoynrProvider::~AbstractJoynrProvider()
{
    // Delete all attribute listeners
    for (auto& mapEntry : attributeListeners) {
        const std::vector<SubscriptionAttributeListener*>& listeners(mapEntry.second);
        for (SubscriptionAttributeListener* listener : listeners) {
            delete listener;
        }
    }
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations" // remove if providerQos is removed
types::ProviderQos AbstractJoynrProvider::getProviderQos() const
{
    return providerQos;
}
#pragma GCC diagnostic pop

void AbstractJoynrProvider::registerAttributeListener(
        const std::string& attributeName,
        SubscriptionAttributeListener* attributeListener)
{
    WriteLocker locker(lock);
    attributeListeners[attributeName].push_back(attributeListener);
}

void AbstractJoynrProvider::unregisterAttributeListener(
        const std::string& attributeName,
        SubscriptionAttributeListener* attributeListener)
{
    WriteLocker locker(lock);
    std::vector<SubscriptionAttributeListener*>& listeners = attributeListeners[attributeName];

    auto listenerIt = std::find(listeners.cbegin(), listeners.cend(), attributeListener);
    assert(listenerIt != listeners.cend());
    delete *listenerIt;
    listeners.erase(listenerIt);

    if (listeners.empty()) {
        attributeListeners.erase(attributeName);
    }
}

void AbstractJoynrProvider::registerBroadcastListener(
        const std::string& broadcastName,
        SubscriptionBroadcastListener* broadcastListener)
{
    WriteLocker locker(lock);
    broadcastListeners[broadcastName].push_back(broadcastListener);
}

void AbstractJoynrProvider::unregisterBroadcastListener(
        const std::string& broadcastName,
        SubscriptionBroadcastListener* broadcastListener)
{
    WriteLocker locker(lock);
    std::vector<SubscriptionBroadcastListener*>& listeners = broadcastListeners[broadcastName];

    auto listenerIt = std::find(listeners.cbegin(), listeners.cend(), broadcastListener);
    assert(listenerIt != listeners.cend());
    delete *listenerIt;
    listeners.erase(listenerIt);

    if (listeners.empty()) {
        broadcastListeners.erase(broadcastName);
    }
}

} // namespace joynr
