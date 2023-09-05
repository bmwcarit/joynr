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
#ifndef ABSTRACTJOYNRPROVIDER_H
#define ABSTRACTJOYNRPROVIDER_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "joynr/IJoynrProvider.h"
#include "joynr/JoynrExport.h"
#include "joynr/MulticastBroadcastListener.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ReadWriteLock.h"
#include "joynr/SubscriptionAttributeListener.h"
#include "joynr/UnicastBroadcastListener.h"

namespace joynr
{

class CallContext;

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
    ~AbstractJoynrProvider() override;

    // --- Interface to be implemented by Providers ---

    // --- Support for listening to onChange events ---

    /**
     * @brief Register an object that will be informed when the value of an attribute changes
     * @param attributeName The name of the attribute for which publications shall be done
     * @param attributeListener The listener object containing the callbacks for publications and
     * failures
     */
    void registerAttributeListener(
            const std::string& attributeName,
            std::shared_ptr<SubscriptionAttributeListener> attributeListener) override;

    /**
     * @brief Unregister and delete an attribute listener
     * @param attributeName The name of the attribute for which publications shall be stopped
     * @param attributeListener The listener object to be unregisterd
     */
    void unregisterAttributeListener(
            const std::string& attributeName,
            std::shared_ptr<SubscriptionAttributeListener> attributeListener) override;

    /**
     * @brief Register a listener for unicast broadcasts
     * @param broadcastName The name of the broadcast for which publications shall be done
     * @param broadcastListener The listener object containing the callbacks for publications and
     * failures
     */
    void registerBroadcastListener(
            const std::string& broadcastName,
            std::shared_ptr<UnicastBroadcastListener> broadcastListener) override;

    /**
     * @brief Register a listener for multicast broadcasts
     * @param broadcastListener The listener object containing the callbacks for publications and
     * failures
     */
    void registerBroadcastListener(
            std::shared_ptr<MulticastBroadcastListener> broadcastListener) override;

    /**
     * @brief Unregister and delete a broadcast listener
     * @param broadcastName The name of the broadcast for which publications shall be done
     * @param broadcastListener The listener object containing the callbacks for publications and
     * failures
     */
    void unregisterBroadcastListener(
            const std::string& broadcastName,
            std::shared_ptr<UnicastBroadcastListener> broadcastListener) override;

    /**
     * @brief Unregister and delete a listener for multicast broadcasts
     * @param broadcastName The name of the broadcast for which publications shall be done
     * @param broadcastListener The listener object containing the callbacks for publications and
     * failures
     */
    void unregisterBroadcastListener(
            std::shared_ptr<MulticastBroadcastListener> broadcastListener) override;

protected:
    /**
     * @brief Called by subclasses when the value of an attribute changes
     * @param attributeName The name of the attribute whose value changes
     * @param value The new value of the attribute
     */
    template <typename T>
    void onAttributeValueChanged(const std::string& attributeName, const T& value)
    {
        ReadLocker locker(_lockAttributeListeners);

        if (_attributeListeners.find(attributeName) != _attributeListeners.cend()) {
            std::vector<std::shared_ptr<SubscriptionAttributeListener>>& listeners =
                    _attributeListeners[attributeName];

            // Inform all the attribute listeners for this attribute
            for (std::shared_ptr<SubscriptionAttributeListener> listener : listeners) {
                listener->attributeValueChanged(value);
            }
        }
    }

    /**
     * @brief Called by subclasses when a selective broadcast occurs
     * @param broadcastName The name of the broadcast that occurred
     * @param filters vector containing the broadcast filters
     * @param values The output values of the broadcast
     */
    template <typename BroadcastFilter, typename... Ts>
    void fireSelectiveBroadcast(const std::string& broadcastName,
                                const std::vector<std::shared_ptr<BroadcastFilter>>& filters,
                                const Ts&... values)
    {

        ReadLocker locker(_lockSelectiveBroadcastListeners);
        const std::vector<std::shared_ptr<UnicastBroadcastListener>>& listeners =
                _selectiveBroadcastListeners[broadcastName];
        // Inform all the broadcast listeners for this broadcast
        for (std::shared_ptr<UnicastBroadcastListener> listener : listeners) {
            listener->selectiveBroadcastOccurred(filters, values...);
        }
    }

    /**
     * @brief Called by subclasses when a selective broadcast occurs
     * @param broadcastName The name of the broadcast that occurred
     * @param values The output values of the broadcastselectiveBroadcastListeners
     */
    template <typename... Ts>
    void fireBroadcast(const std::string& broadcastName,
                       const std::vector<std::string>& partitions,
                       const Ts&... values)
    {
        util::validatePartitions(partitions, false /*do not allow wildcard*/);

        ReadLocker locker(_lockBroadcastListeners);
        // Inform all the broadcast listeners for this broadcast
        for (std::shared_ptr<MulticastBroadcastListener> listener : _broadcastListeners) {
            listener->broadcastOccurred(broadcastName, partitions, values...);
        }
    }

    /**
     * @brief Returns a call context object including meta information (such as calling
     * principal) for the current provider call.
     *
     * Method calls on joynr providers are executed by joynr middleware threads. These threads
     * must not be blocked by the provider to ensure proper joynr function. Providers must
     * spawn threads to perform computation asynchronously and release the joynr thread. The
     * call context object is thread local and only valid in the context of the joynr middleware
     * thread and hence must be copied by the provider for asychronous or future usage.
     *
     * @return The current call context
     */
    const CallContext& getCallContext() const;

private:
    DISALLOW_COPY_AND_ASSIGN(AbstractJoynrProvider);

    ReadWriteLock _lockAttributeListeners;
    ReadWriteLock _lockBroadcastListeners;
    ReadWriteLock _lockSelectiveBroadcastListeners;
    std::map<std::string, std::vector<std::shared_ptr<SubscriptionAttributeListener>>>
            _attributeListeners;
    std::map<std::string, std::vector<std::shared_ptr<UnicastBroadcastListener>>>
            _selectiveBroadcastListeners;
    std::vector<std::shared_ptr<MulticastBroadcastListener>> _broadcastListeners;

    friend class End2EndBroadcastTest;
    friend class End2EndSubscriptionTest;
};

} // namespace joynr
#endif // ABSTRACTJOYNRPROVIDER_H
