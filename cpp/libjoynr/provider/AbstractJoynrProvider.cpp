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

#include "joynr/IAttributeListener.h"
#include "joynr/IBroadcastListener.h"

namespace joynr
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations" // remove if providerQos is removed
AbstractJoynrProvider::AbstractJoynrProvider()
        : providerQos(), lock(), attributeListeners(), broadcastListeners(), broadcastFilters()
{
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations" // remove if providerQos is removed
AbstractJoynrProvider::~AbstractJoynrProvider()
{
    // Delete all attribute listeners
    for (auto& mapEntry : attributeListeners) {
        const std::vector<IAttributeListener*>& listeners(mapEntry.second);
        for (IAttributeListener* listener : listeners) {
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

void AbstractJoynrProvider::registerAttributeListener(const std::string& attributeName,
                                                      IAttributeListener* attributeListener)
{
    WriteLocker locker(lock);
    attributeListeners[attributeName].push_back(attributeListener);
}

void AbstractJoynrProvider::unregisterAttributeListener(const std::string& attributeName,
                                                        IAttributeListener* attributeListener)
{
    WriteLocker locker(lock);
    std::vector<IAttributeListener*>& listeners = attributeListeners[attributeName];

    auto listenerIt = std::find(listeners.cbegin(), listeners.cend(), attributeListener);
    assert(listenerIt != listeners.cend());
    delete *listenerIt;
    listeners.erase(listenerIt);

    if (listeners.empty()) {
        attributeListeners.erase(attributeName);
    }
}

void AbstractJoynrProvider::onAttributeValueChanged(const std::string& attributeName,
                                                    const Variant& value)
{
    ReadLocker locker(lock);

    if (attributeListeners.find(attributeName) != attributeListeners.cend()) {
        const std::vector<IAttributeListener*>& listeners = attributeListeners[attributeName];

        // Inform all the attribute listeners for this attribute
        for (IAttributeListener* listener : listeners) {
            listener->attributeValueChanged(value);
        }
    }
}

void AbstractJoynrProvider::registerBroadcastListener(const std::string& broadcastName,
                                                      IBroadcastListener* broadcastListener)
{
    WriteLocker locker(lock);
    broadcastListeners[broadcastName].push_back(broadcastListener);
}

void AbstractJoynrProvider::unregisterBroadcastListener(const std::string& broadcastName,
                                                        IBroadcastListener* broadcastListener)
{
    WriteLocker locker(lock);
    std::vector<IBroadcastListener*>& listeners = broadcastListeners[broadcastName];

    auto listenerIt = std::find(listeners.cbegin(), listeners.cend(), broadcastListener);
    assert(listenerIt != listeners.cend());
    delete *listenerIt;
    listeners.erase(listenerIt);

    if (listeners.empty()) {
        broadcastListeners.erase(broadcastName);
    }
}

void AbstractJoynrProvider::fireBroadcast(const std::string& broadcastName,
                                          const std::vector<Variant>& values)
{
    ReadLocker locker(lock);

    const std::vector<IBroadcastListener*>& listeners = broadcastListeners[broadcastName];

    // Inform all the broadcast listeners for this broadcast
    for (IBroadcastListener* listener : listeners) {
        auto broadcastFiletersIterator = broadcastFilters.find(broadcastName);
        if (broadcastFiletersIterator != broadcastFilters.end()) {
            listener->broadcastOccurred(values, broadcastFiletersIterator->second);
        } else {
            listener->broadcastOccurred(values, std::vector<std::shared_ptr<IBroadcastFilter>>());
        }
    }
}

void AbstractJoynrProvider::addBroadcastFilter(std::shared_ptr<IBroadcastFilter> filter)
{
    std::map<std::string, std::vector<std::shared_ptr<IBroadcastFilter>>>::iterator it =
            broadcastFilters.find(filter->getName());

    if (it != broadcastFilters.end()) {
        it->second.push_back(filter);
    } else {
        broadcastFilters.insert(std::make_pair(
                filter->getName(), std::vector<std::shared_ptr<IBroadcastFilter>>({filter})));
    }
}

} // namespace joynr
