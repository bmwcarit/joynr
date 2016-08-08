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

#ifndef SUBSCRIPTIONMANAGER_H
#define SUBSCRIPTIONMANAGER_H

#include <string>
#include <memory>
#include <cstdint>

#include "joynr/ISubscriptionCallback.h"
#include "joynr/ISubscriptionManager.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/ObjectWithDecayTime.h"
#include "joynr/MessagingQos.h"
#include "joynr/Runnable.h"
#include "joynr/ReadWriteLock.h"
#include "joynr/ThreadSafeMap.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"

namespace boost
{
namespace asio
{
class io_service;
} // namespace asio
} // namespace boost

namespace joynr
{
class SubscriptionQos;
class DelayedScheduler;
/**
  * @class SubscriptionManager
  * @brief The subscription manager is used by the proxy (via the appropriate connector)
  * to manage a subscription. This includes the registration and unregistration of attribute
  * subscriptions. In order to subscribe, a SubscriptionListener is passed in from the application
 * and
  * packaged into a callback by the connector.
  * This listener is notified (via the callback) when a subscription is missed or when a publication
  * arrives.
  */
class JOYNR_EXPORT SubscriptionManager : public ISubscriptionManager
{

public:
    ~SubscriptionManager() override;

    explicit SubscriptionManager(boost::asio::io_service& ioService);
    explicit SubscriptionManager(DelayedScheduler* scheduler);
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
    void registerSubscription(const std::string& subscribeToName,
                              std::shared_ptr<ISubscriptionCallback> subscriptionCaller,
                              std::shared_ptr<SubscriptionQos> qos,
                              SubscriptionRequest& subscriptionRequest) override;

    /**
     * @brief Stop the subscription. Removes the callback and stops the notifications
     * on missed publications.
     *
     * @param subscriptionId
     */
    void unregisterSubscription(const std::string& subscriptionId) override;

    /**
     * @brief Sets the time of last received publication (incoming attribute value) to the current
     *system time.
     *
     * @param subscriptionId
     */
    void touchSubscriptionState(const std::string& subscriptionId) override;

    /**
     * @brief Get a shared pointer to the subscription callback. The shared pointer points to null
     * if the subscription ID does not exist.
     *
     * @param subscriptionId
     * @return std::shared_ptr<ISubscriptionCallback>
     */
    std::shared_ptr<ISubscriptionCallback> getSubscriptionCallback(
            const std::string& subscriptionId) override;

private:
    //    void checkMissedPublication(const Timer::TimerId id);

    DISALLOW_COPY_AND_ASSIGN(SubscriptionManager);
    class Subscription;

    ThreadSafeMap<std::string, std::shared_ptr<Subscription>> subscriptions;

    DelayedScheduler* missedPublicationScheduler;
    ADD_LOGGER(SubscriptionManager);
    /**
      * @class SubscriptionManager::MissedPublicationRunnable
      * @brief
      */
    class MissedPublicationRunnable : public Runnable, public ObjectWithDecayTime
    {
    public:
        MissedPublicationRunnable(const JoynrTimePoint& expiryDate,
                                  const std::int64_t& expectedIntervalMSecs,
                                  const std::string& subscriptionId,
                                  std::shared_ptr<Subscription> subscription,
                                  SubscriptionManager& subscriptionManager,
                                  const std::int64_t& alertAfterInterval);

        void shutdown() override;

        /**
         * @brief Checks whether a publication arrived in time, whether it is expired or
         *interrupted.
         *
         */
        void run() override;

    private:
        DISALLOW_COPY_AND_ASSIGN(MissedPublicationRunnable);
        std::int64_t timeSinceLastExpectedPublication(const std::int64_t& timeSinceLastPublication);
        std::int64_t expectedIntervalMSecs;
        std::shared_ptr<Subscription> subscription;
        const std::string subscriptionId;
        std::int64_t alertAfterInterval;
        SubscriptionManager& subscriptionManager;
        ADD_LOGGER(MissedPublicationRunnable);
    };
    /**
      * @class SubscriptionManager::SubscriptionEndRunnable
      * @brief
      */
    class SubscriptionEndRunnable : public Runnable
    {
    public:
        SubscriptionEndRunnable(const std::string& subscriptionId,
                                SubscriptionManager& subscriptionManager);

        void shutdown() override;
        /**
         * @brief removes subscription once running.
         *
         */
        void run() override;

    private:
        DISALLOW_COPY_AND_ASSIGN(SubscriptionEndRunnable);
        std::string subscriptionId;
        SubscriptionManager& subscriptionManager;
        ADD_LOGGER(SubscriptionEndRunnable);
    };
};

} // namespace joynr
#endif // SUBSCRIPTIONMANAGER_H
