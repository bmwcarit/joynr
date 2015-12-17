/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include "joynr/PrivateCopyAssign.h"

#include "joynr/joynrlogging.h"

#include "joynr/JoynrExport.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/ISubscriptionCallback.h"
#include <string>
#include <memory>

#include "joynr/Variant.h"

namespace joynr
{

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
     * @brief Subscribe to an attribute. Modifies the subscription request to include all
     * necessary information (side effect). Takes ownership of the ISubscriptionCallback, i.e.
     * deletes the callback when no longer required.
     *
     * @param attributeName
     * @param attributeSubscriptionCaller
     * @param qos
     * @param subscriptionRequest
     */
    virtual void registerSubscription(const std::string& subscribeToName,
                                      std::shared_ptr<ISubscriptionCallback> subscriptionCaller,
                                      const Variant& qos,
                                      SubscriptionRequest& subscriptionRequest) = 0;

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
};
} // namespace joynr
#endif // ISUBSCRIPTIONMANAGER_H
