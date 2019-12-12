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

#ifndef ISUBSCRIPTIONMANAGER_H
#define ISUBSCRIPTIONMANAGER_H

#include <cstdint>
#include <forward_list>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "joynr/JoynrExport.h"

namespace joynr
{

class MulticastSubscriptionRequest;
class SubscriptionRequest;
class ISubscriptionCallback;
class ISubscriptionListenerBase;
class SubscriptionQos;

namespace exceptions
{
class ProviderRuntimeException;
} // namespace exceptions

/**
  * @class ISubscriptionManager
  * @brief The subscription manager is used by the proxy (via the appropriate connector)
  * to manage a subscription. This includes the registration and unregistration of attribute
  * subscriptions. In order to subscribe, a SubscriptionListener is passed in from the application
 * and
  * packaged into a callback by the connector.
  * This listener is notified (via the callback) when a subscription is missed or when a publication
  * arrives.
  */
class JOYNR_EXPORT ISubscriptionManager
{

public:
    virtual ~ISubscriptionManager() = default;

    /**
     * @brief Subscribe to an attribute or broadcast. Modifies the subscription request to include
     * all necessary information (side effect).
     *
     * @param subscribeToName
     * @param subscriptionCaller
     * @param qos
     * @param subscriptionRequest
     */
    virtual void registerSubscription(
            const std::string& subscribeToName,
            std::shared_ptr<ISubscriptionCallback> subscriptionCaller,
            std::shared_ptr<ISubscriptionListenerBase> subscriptionListener,
            std::shared_ptr<SubscriptionQos> qos,
            SubscriptionRequest& subscriptionRequest) = 0;

    /**
     * @brief Subscribe to a multicast. Modifies the subscription request to include
     * all necessary information (side effect).
     *
     * @param subscribeToName
     * @param subscriberParticipantId
     * @param providerParticipantId
     * @param partition
     * @param subscriptionCaller
     * @param qos
     * @param subscriptionRequest
     * @param onSuccess
     * @param onError
     */
    virtual void registerSubscription(
            const std::string& subscribeToName,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            const std::vector<std::string>& partitions,
            std::shared_ptr<ISubscriptionCallback> subscriptionCaller,
            std::shared_ptr<ISubscriptionListenerBase> subscriptionListener,
            std::shared_ptr<SubscriptionQos> qos,
            MulticastSubscriptionRequest& subscriptionRequest,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) = 0;

    /**
     * @brief Stop the subscription. Removes the callback and stops the notifications
     * on missed publications.
     *
     * @param subscriptionId
     */
    virtual void unregisterSubscription(const std::string& subscriptionId) = 0;

    /**
     * @brief Sets the time of last received publication (incoming attribute value) to the current
     *system time.
     *
     * @param subscriptionId
     */
    virtual void touchSubscriptionState(const std::string& subscriptionId) = 0;

    /**
     * @brief Get a shared pointer to the subscription callback. The shared pointer points to null
     * if the subscription ID does not exist.
     *
     * @param subscriptionId
     * @return std::shared_ptr<ISubscriptionCallback>
     */
    virtual std::shared_ptr<ISubscriptionCallback> getSubscriptionCallback(
            const std::string& subscriptionId) = 0;

    /**
     * @brief Get a shared pointer to the subscription callback. The shared pointer point to null
     * if the multicast ID does not exist.
     *
     * @param multicastId
     * @return <std::shared_ptr<ISubscriptionCallback>
     */
    virtual std::shared_ptr<ISubscriptionCallback> getMulticastSubscriptionCallback(
            const std::string& multicastId) = 0;

    /**
     * @brief Get a shared pointer to the subscription listener. The shared pointer points to null
     * if the subscription ID does not exist.
     *
     * @param subscriptionId
     * @return std::shared_ptr<ISubscriptionListenerBase>
     */
    virtual std::shared_ptr<ISubscriptionListenerBase> getSubscriptionListener(
            const std::string& subscriptionId) = 0;

    /**
     * @brief Get a list of shared pointers to the subscription listeners. The list is empty
     * if the multicast ID does not exist.
     *
     * @param multicastId
     * @return std::forward_list<std::shared_ptr<ISubscriptionListenerBase>>
     */
    virtual std::forward_list<std::shared_ptr<ISubscriptionListenerBase>>
    getMulticastSubscriptionListeners(const std::string& multicastId) = 0;

    virtual void shutdown() = 0;

    /**
     * @brief Converts the expiry date of a subscription into a TTL.
     * @param subscriptionQos the subscription QoS defining the subscription.
     * @return the TTL in millis.
     */
    static std::int64_t convertExpiryDateIntoTtlMs(const SubscriptionQos& subscriptionQos);
};
} // namespace joynr
#endif // ISUBSCRIPTIONMANAGER_H
