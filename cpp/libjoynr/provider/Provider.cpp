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

#include "joynr/Provider.h"
#include "joynr/IAttributeListener.h"
#include "joynr/IBroadcastListener.h"

#include <QVariant>
#include <QWriteLocker>
#include <QReadLocker>

namespace joynr
{

Provider::Provider() : lock(), attributeListeners(), broadcastListeners(), broadcastFilters()
{
}

Provider::~Provider()
{
    // Delete all attribute listeners
    foreach (const QList<IAttributeListener*>& listeners, attributeListeners) {
        foreach (IAttributeListener* listener, listeners) {
            delete listener;
        }
    }
}

void Provider::registerAttributeListener(const std::string& attributeName,
                                         IAttributeListener* attributeListener)
{
    QWriteLocker locker(&lock);
    attributeListeners[attributeName].append(attributeListener);
}

void Provider::unregisterAttributeListener(const std::string& attributeName,
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

void Provider::onAttributeValueChanged(const std::string& attributeName, const QVariant& value)
{
    QReadLocker locker(&lock);

    const QList<IAttributeListener*>& listeners = attributeListeners[attributeName];

    // Inform all the attribute listeners for this attribute
    foreach (IAttributeListener* listener, listeners) {
        listener->attributeValueChanged(value);
    }
}

void Provider::registerBroadcastListener(const std::string& broadcastName,
                                         IBroadcastListener* broadcastListener)
{
    QWriteLocker locker(&lock);
    broadcastListeners[broadcastName].append(broadcastListener);
}

void Provider::unregisterBroadcastListener(const std::string& broadcastName,
                                           IBroadcastListener* broadcastListener)
{
    QWriteLocker locker(&lock);
    QList<IBroadcastListener*>& listeners = broadcastListeners[broadcastName];

    int listenerIndex = listeners.indexOf(broadcastListener);
    delete listeners.takeAt(listenerIndex);
}

void Provider::fireBroadcast(const std::string& broadcastName, const QList<QVariant>& values)
{
    QReadLocker locker(&lock);

    const QList<IBroadcastListener*>& listeners = broadcastListeners[broadcastName];

    // Inform all the broadcast listeners for this broadcast
    foreach (IBroadcastListener* listener, listeners) {
        listener->broadcastOccurred(values, broadcastFilters.value(broadcastName));
    }
}

void Provider::addBroadcastFilter(QSharedPointer<IBroadcastFilter> filter)
{
    QMap<std::string, QList<QSharedPointer<IBroadcastFilter>>>::iterator it =
            broadcastFilters.find(filter->getName());

    if (it != broadcastFilters.end()) {
        it.value().append(filter);
    } else {
        broadcastFilters.insert(
                filter->getName(), QList<QSharedPointer<IBroadcastFilter>>({filter}));
    }
}

} // namespace joynr
