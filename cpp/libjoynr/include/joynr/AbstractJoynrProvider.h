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
#ifndef ABSTRACTJOYNRPROVIDER_H
#define ABSTRACTJOYNRPROVIDER_H

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"
#include "joynr/IJoynrProvider.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/IBroadcastFilter.h"

#include <QReadWriteLock>
#include <QMap>
#include <QList>
#include <QVariant>
#include <string>

namespace joynr
{

class IAttributeListener;
class IBroadcastListener;

/**
 * Abstract class that specifies the interface providers need to implement
 * and contains functionality to support listening to onChange events
 */
class JOYNR_EXPORT AbstractJoynrProvider : public IJoynrProvider
{
public:
    AbstractJoynrProvider();
    virtual ~AbstractJoynrProvider();

    // --- Interface to be implemented by Providers ---
    virtual types::ProviderQos getProviderQos() const;

    // --- Support for listening to onChange events ---

    /**
     * Register an object that will be informed when the value of an attribute changes
     */
    virtual void registerAttributeListener(const std::string& attributeName,
                                           IAttributeListener* attributeListener);

    /**
     * Unregister and delete an attribute listener
     */
    virtual void unregisterAttributeListener(const std::string& attributeName,
                                             IAttributeListener* attributeListener);

    /**
     * Called by subclasses when the value of an attribute changes
     */
    void onAttributeValueChanged(const std::string& attributeName, const QVariant& value);

    /**
     * Register an object that will be informed when an event occurs
     */
    virtual void registerBroadcastListener(const std::string& broadcastName,
                                           IBroadcastListener* broadcastListener);

    /**
     * Unregister and delete a broadcast listener
     */
    virtual void unregisterBroadcastListener(const std::string& broadcastName,
                                             IBroadcastListener* broadcastListener);

    /**
     * Called by subclasses when a broadcast occurs
     */
    void fireBroadcast(const std::string& broadcastName, const QList<QVariant>& values);

    virtual void addBroadcastFilter(QSharedPointer<IBroadcastFilter> filter);

protected:
    types::ProviderQos providerQos;

private:
    DISALLOW_COPY_AND_ASSIGN(AbstractJoynrProvider);

    QReadWriteLock lock;
    QMap<std::string, QList<IAttributeListener*>> attributeListeners;
    QMap<std::string, QList<IBroadcastListener*>> broadcastListeners;
    QMap<std::string, QList<QSharedPointer<IBroadcastFilter>>> broadcastFilters;

    friend class End2EndBroadcastTest;
};

} // namespace joynr
#endif // ABSTRACTJOYNRPROVIDER_H
