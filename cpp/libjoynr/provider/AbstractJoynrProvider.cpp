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
#include "joynr/IAttributeListener.h"
#include "joynr/IBroadcastListener.h"

#include <QVariant>
#include <QWriteLocker>
#include <QReadLocker>

namespace joynr
{

AbstractJoynrProvider::AbstractJoynrProvider()
        : providerQos(), lock(), attributeListeners(), broadcastListeners(), broadcastFilters()
{
}

AbstractJoynrProvider::~AbstractJoynrProvider()
{
    // Delete all attribute listeners
    for (auto& mapEntry : attributeListeners) {
        const QList<IAttributeListener*>& listeners(mapEntry.second);
        foreach (IAttributeListener* listener, listeners) {
            delete listener;
        }
    }
}

types::ProviderQos AbstractJoynrProvider::getProviderQos() const
{
    return providerQos;
}

void AbstractJoynrProvider::registerAttributeListener(const std::string& attributeName,
                                                      IAttributeListener* attributeListener)
{
    QWriteLocker locker(&lock);
    attributeListeners[attributeName].append(attributeListener);
}

void AbstractJoynrProvider::unregisterAttributeListener(const std::string& attributeName,
                                                        IAttributeListener* attributeListener)
{
    QWriteLocker locker(&lock);
    QList<IAttributeListener*>& listeners = attributeListeners[attributeName];

    // Find and delete the attribute listener
    for (int i = 0; i < listeners.length(); i++) {
        if (listeners[i] == attributeListener) {
            IAttributeListener* listener = listeners[i];
            listeners.removeAt(i);
            delete listener;
        }
    }
}

void AbstractJoynrProvider::onAttributeValueChanged(const std::string& attributeName,
                                                    const Variant& value)
{
    QReadLocker locker(&lock);

    const QList<IAttributeListener*>& listeners = attributeListeners[attributeName];

    // Inform all the attribute listeners for this attribute
    for (IAttributeListener* listener : listeners) {
        listener->attributeValueChanged(value);
    }
}

void AbstractJoynrProvider::registerBroadcastListener(const std::string& broadcastName,
                                                      IBroadcastListener* broadcastListener)
{
    QWriteLocker locker(&lock);
    broadcastListeners[broadcastName].append(broadcastListener);
}

void AbstractJoynrProvider::unregisterBroadcastListener(const std::string& broadcastName,
                                                        IBroadcastListener* broadcastListener)
{
    QWriteLocker locker(&lock);
    QList<IBroadcastListener*>& listeners = broadcastListeners[broadcastName];

    int listenerIndex = listeners.indexOf(broadcastListener);
    delete listeners.takeAt(listenerIndex);
}

void AbstractJoynrProvider::fireBroadcast(const std::string& broadcastName,
                                          const std::vector<Variant>& values)
{
    QReadLocker locker(&lock);

    const QList<IBroadcastListener*>& listeners = broadcastListeners[broadcastName];

    // Inform all the broadcast listeners for this broadcast
    for (IBroadcastListener* listener : listeners) {
        auto broadcastFiletersIterator = broadcastFilters.find(broadcastName);
        if (broadcastFiletersIterator != broadcastFilters.end()) {
            listener->broadcastOccurred(values, broadcastFiletersIterator->second);
        } else {
            listener->broadcastOccurred(values, QList<std::shared_ptr<IBroadcastFilter>>());
        }
    }
}

void AbstractJoynrProvider::addBroadcastFilter(std::shared_ptr<IBroadcastFilter> filter)
{
    std::map<std::string, QList<std::shared_ptr<IBroadcastFilter>>>::iterator it =
            broadcastFilters.find(filter->getName());

    if (it != broadcastFilters.end()) {
        it->second.append(filter);
    } else {
        broadcastFilters.insert(std::make_pair(
                filter->getName(), QList<std::shared_ptr<IBroadcastFilter>>({filter})));
    }
}

} // namespace joynr
