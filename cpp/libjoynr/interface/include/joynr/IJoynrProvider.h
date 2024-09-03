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
#ifndef IJOYNRPROVIDER_H
#define IJOYNRPROVIDER_H

#include <memory>
#include <string>

namespace joynr
{
class SubscriptionAttributeListener;
class UnicastBroadcastListener;
class MulticastBroadcastListener;
class IBroadcastListener;

namespace types
{
class ProviderQos;
} // namespace types

/** @brief Interface for the provider */
class IJoynrProvider
{
public:
    /** @brief Destructor */
    virtual ~IJoynrProvider() = default;

    /**
     * @brief Register an object that will be informed when the value of an attribute changes
     * @param attributeName The name of the attribute for which publications shall be done
     * @param attributeListener The listener object containing the callbacks for publications and
     * failures
     */
    virtual void registerAttributeListener(
            const std::string& attributeName,
            std::shared_ptr<SubscriptionAttributeListener> attributeListener) = 0;
    /**
     * @brief Unregister and delete an attribute listener
     * @param attributeName The name of the attribute for which publications shall be stopped
     * @param attributeListener The listener object to be unregisterd
     */
    virtual void unregisterAttributeListener(
            const std::string& attributeName,
            std::shared_ptr<SubscriptionAttributeListener> attributeListener) = 0;

    /**
     * @brief Register a listener for unicast broadcasts
     * @param broadcastName The name of the broadcast for which publications shall be done
     * @param broadcastListener The listener object containing the callbacks for publications and
     * failures
     */
    virtual void registerBroadcastListener(
            const std::string& broadcastName,
            std::shared_ptr<UnicastBroadcastListener> broadcastListener) = 0;

    /**
     * @brief Register a listener for multicast broadcasts
     * @param broadcastListener The listener object containing the callbacks for publications and
     * failures
     */
    virtual void registerBroadcastListener(
            std::shared_ptr<MulticastBroadcastListener> broadcastListener) = 0;

    /**
     * @brief Unregister and delete a broadcast listener
     * @param broadcastName The name of the broadcast for which publications shall be done
     * @param broadcastListener The listener object containing the callbacks for publications and
     * failures
     */
    virtual void unregisterBroadcastListener(
            const std::string& broadcastName,
            std::shared_ptr<UnicastBroadcastListener> broadcastListener) = 0;
    /**
     * @brief Unregister and delete a listener for multicast broadcasts
     * @param broadcastName The name of the broadcast for which publications shall be done
     * @param broadcastListener The listener object containing the callbacks for publications and
     * failures
     */
    virtual void unregisterBroadcastListener(
            std::shared_ptr<MulticastBroadcastListener> broadcastListener) = 0;

    /**
     * @brief Gets the interface name
     * @return the interface name
     */
    virtual const std::string& getInterfaceName() const = 0;
};

} // namespace joynr
#endif // IJOYNRPROVIDER_H
