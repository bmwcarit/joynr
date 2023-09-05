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

#include "joynr/AbstractJoynrProvider.h"

#include <cassert>

#include "joynr/CallContextStorage.h"
#include "joynr/Util.h"

namespace joynr
{

AbstractJoynrProvider::AbstractJoynrProvider()
        : _lockAttributeListeners(),
          _lockBroadcastListeners(),
          _lockSelectiveBroadcastListeners(),
          _attributeListeners(),
          _broadcastListeners()
{
}

AbstractJoynrProvider::~AbstractJoynrProvider()
{
}

void AbstractJoynrProvider::registerAttributeListener(
        const std::string& attributeName,
        std::shared_ptr<SubscriptionAttributeListener> attributeListener)
{
    WriteLocker locker(_lockAttributeListeners);
    _attributeListeners[attributeName].push_back(std::move(attributeListener));
}

void AbstractJoynrProvider::unregisterAttributeListener(
        const std::string& attributeName,
        std::shared_ptr<SubscriptionAttributeListener> attributeListener)
{
    WriteLocker locker(_lockAttributeListeners);
    std::vector<std::shared_ptr<SubscriptionAttributeListener>>& listeners =
            _attributeListeners[attributeName];

    auto listenerIt = std::find(listeners.cbegin(), listeners.cend(), attributeListener);
    assert(listenerIt != listeners.cend());
    listeners.erase(listenerIt);

    if (listeners.empty()) {
        _attributeListeners.erase(attributeName);
    }
}

void AbstractJoynrProvider::registerBroadcastListener(
        const std::string& broadcastName,
        std::shared_ptr<UnicastBroadcastListener> broadcastListener)
{
    WriteLocker locker(_lockSelectiveBroadcastListeners);
    _selectiveBroadcastListeners[broadcastName].push_back(std::move(broadcastListener));
}

void AbstractJoynrProvider::registerBroadcastListener(
        std::shared_ptr<MulticastBroadcastListener> broadcastListener)
{
    WriteLocker locker(_lockBroadcastListeners);
    _broadcastListeners.push_back(std::move(broadcastListener));
}

void AbstractJoynrProvider::unregisterBroadcastListener(
        const std::string& broadcastName,
        std::shared_ptr<UnicastBroadcastListener> broadcastListener)
{
    WriteLocker locker(_lockSelectiveBroadcastListeners);
    std::vector<std::shared_ptr<UnicastBroadcastListener>>& listeners =
            _selectiveBroadcastListeners[broadcastName];

    auto listenerIt = std::find(listeners.cbegin(), listeners.cend(), broadcastListener);
    assert(listenerIt != listeners.cend());
    listeners.erase(listenerIt);

    if (listeners.empty()) {
        _selectiveBroadcastListeners.erase(broadcastName);
    }
}

void AbstractJoynrProvider::unregisterBroadcastListener(
        std::shared_ptr<MulticastBroadcastListener> broadcastListener)
{
    WriteLocker locker(_lockBroadcastListeners);
    auto listenerIt =
            std::find(_broadcastListeners.cbegin(), _broadcastListeners.cend(), broadcastListener);
    if (listenerIt != _broadcastListeners.cend()) {
        _broadcastListeners.erase(listenerIt);
    }
}

const CallContext& AbstractJoynrProvider::getCallContext() const
{
    return CallContextStorage::get();
}

} // namespace joynr
