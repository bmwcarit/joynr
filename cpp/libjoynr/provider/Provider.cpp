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

namespace joynr {

Provider::Provider()
    : lock(),
      attributeListeners(),
      broadcastListeners()
{
}

Provider::~Provider()
{
    // Delete all attribute listeners
    foreach (const QList<IAttributeListener *>& listeners, attributeListeners) {
        foreach (IAttributeListener *listener, listeners) {
            delete listener;
        }
    }
}

void Provider::registerAttributeListener(const QString& attributeName, IAttributeListener* attributeListener)
{
    QWriteLocker locker(&lock);
    attributeListeners[attributeName].append(attributeListener);
}


void Provider::unregisterAttributeListener(const QString& attributeName, IAttributeListener* attributeListener)
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


void Provider::onAttributeValueChanged(const QString& attributeName, const QVariant& value)
{
    QReadLocker locker(&lock);

    const QList<IAttributeListener*>& listeners = attributeListeners[attributeName];

    // Inform all the attribute listeners for this attribute
    foreach (IAttributeListener* listener, listeners) {
        listener->attributeValueChanged(value);
    }
}

void Provider::registerBroadcastListener(const QString &broadcastName, IBroadcastListener *broadcastListener)
{
    QWriteLocker locker(&lock);
    broadcastListeners[broadcastName].append(broadcastListener);
}

void Provider::unregisterBroadcastListener(const QString &broadcastName, IBroadcastListener *broadcastListener)
{
    QWriteLocker locker(&lock);
    QList<IBroadcastListener*>& listeners = broadcastListeners[broadcastName];

    int listenerIndex = listeners.indexOf(broadcastListener);
    delete listeners.takeAt(listenerIndex);
}

void Provider::onEventOccured(const QString &broadcastName, const QVariant &values)
{
    QReadLocker locker(&lock);

    const QList<IBroadcastListener*>& listeners = broadcastListeners[broadcastName];

    // Inform all the broadcast listeners for this attribute
    foreach (IBroadcastListener* listener, listeners) {
        listener->eventOccured(values);
    }
}

} // namespace joynr
