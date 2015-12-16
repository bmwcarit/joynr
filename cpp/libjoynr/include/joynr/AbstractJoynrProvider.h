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
#include "joynr/ReadWriteLock.h"

#include <string>
#include <memory>
#include <map>
#include <vector>

namespace joynr
{

class IAttributeListener;
class IBroadcastListener;

/**
 * @brief Abstract class that specifies the interface providers need to implement
 * and contains functionality to support listening to onChange events
 */
class JOYNR_EXPORT AbstractJoynrProvider : public virtual IJoynrProvider
{
public:
    /** @brief Default constructor */
    AbstractJoynrProvider();

    /** @brief Destructor */
    virtual ~AbstractJoynrProvider();

    // --- Interface to be implemented by Providers ---

    /**
     * @brief Get the provider quality of service settings
     * @return the provider quality of service settings
     */
    virtual types::ProviderQos getProviderQos() const;

    // --- Support for listening to onChange events ---

    /**
     * @brief Register an object that will be informed when the value of an attribute changes
     * @param attributeName The name of the attribute for which publications shall be done
     * @param attributeListener The listener object containing the callbacks for publications and
     * failures
     */
    virtual void registerAttributeListener(const std::string& attributeName,
                                           IAttributeListener* attributeListener);

    /**
     * @brief Unregister and delete an attribute listener
     * @param attributeName The name of the attribute for which publications shall be stopped
     * @param attributeListener The listener object to be unregisterd
     */
    virtual void unregisterAttributeListener(const std::string& attributeName,
                                             IAttributeListener* attributeListener);

    /**
     * @brief Register an object that will be informed when an event occurs
     * @param broadcastName The name of the broadcast for which publications shall be done
     * @param broadcastListener The listener object containing the callbacks for publications and
     * failures
     */
    virtual void registerBroadcastListener(const std::string& broadcastName,
                                           IBroadcastListener* broadcastListener);

    /**
     * @brief Unregister and delete a broadcast listener
     * @param broadcastName The name of the broadcast for which publications shall be done
     * @param broadcastListener The listener object containing the callbacks for publications and
     * failures
     */
    virtual void unregisterBroadcastListener(const std::string& broadcastName,
                                             IBroadcastListener* broadcastListener);

    /**
     * @brief Add a broadcast filter
     * @param filter The broadcast filter to be added
     */
    virtual void addBroadcastFilter(std::shared_ptr<IBroadcastFilter> filter);

protected:
    /**
     * @brief Called by subclasses when the value of an attribute changes
     * @param attributeName The name of the attribute whose value changes
     * @param value The new value of the attribute
     */
    void onAttributeValueChanged(const std::string& attributeName, const Variant& value);

    /**
     * @brief Called by subclasses when a broadcast occurs
     * @param broadcastName The name of the broadcast that occurred
     * @param values The output values of the broadcast
     */
    void fireBroadcast(const std::string& broadcastName, const std::vector<Variant>& values);

    /** @brief The provider quality settings */
    types::ProviderQos providerQos;

private:
    DISALLOW_COPY_AND_ASSIGN(AbstractJoynrProvider);

    ReadWriteLock lock;
    std::map<std::string, std::vector<IAttributeListener*>> attributeListeners;
    std::map<std::string, std::vector<IBroadcastListener*>> broadcastListeners;
    std::map<std::string, std::vector<std::shared_ptr<IBroadcastFilter>>> broadcastFilters;

    friend class End2EndBroadcastTest;
};

} // namespace joynr
#endif // ABSTRACTJOYNRPROVIDER_H
