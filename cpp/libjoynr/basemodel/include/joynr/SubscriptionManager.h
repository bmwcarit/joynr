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

#ifndef SUBSCRIPTIONMANAGER_H
#define SUBSCRIPTIONMANAGER_H

#include <cstdint>
#include <forward_list>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "joynr/BoostIoserviceForwardDecl.h"
#include "joynr/ISubscriptionManager.h"

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/MulticastReceiverDirectory.h"
#include "joynr/ObjectWithDecayTime.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Runnable.h"
#include "joynr/ThreadSafeMap.h"

namespace joynr
{
class ISubscriptionCallback;
class IMessageRouter;
class DelayedScheduler;
class MulticastSubscriptionRequest;
class SubscriptionQos;
class SubscriptionRequest;

namespace exceptions
{
class ProviderRuntimeException;
} // namespace exceptions

/**
 * @class SubscriptionManager
 * @brief The subscription manager is used by the proxy (via the appropriate connector)
 * to manage a subscription. This includes the registration and unregistration of attribute
 * subscriptions. In order to subscribe, a SubscriptionListener is passed in from the application
 * and packaged into a callback by the connector.
 * This listener is notified (via the callback) when a subscription is missed or when a publication
 * arrives.
 */
class JOYNR_EXPORT SubscriptionManager : public ISubscriptionManager,
                                         public std::enable_shared_from_this<SubscriptionManager>
{

public:
    ~SubscriptionManager() override;

    SubscriptionManager(boost::asio::io_service& ioService,
                        std::shared_ptr<IMessageRouter> messageRouter);
    SubscriptionManager(std::shared_ptr<DelayedScheduler> scheduler,
                        std::shared_ptr<IMessageRouter> messageRouter);

    /**
     * @brief Subscribe to an attribute or broadcast. Modifies the subscription request to include
     * all necessary information (side effect). Takes ownership of the ISubscriptionCallback, i.e.
     * deletes the callback when no longer required.
     *
     * @param subscribeToName
     * @param subscriptionCaller
     * @param qos
     * @param subscriptionRequest
     */
    void registerSubscription(const std::string& subscribeToName,
                              std::shared_ptr<ISubscriptionCallback> subscriptionCaller,
                              std::shared_ptr<ISubscriptionListenerBase> subscriptionListener,
                              std::shared_ptr<SubscriptionQos> qos,
                              SubscriptionRequest& subscriptionRequest) override;

    /**
     * @brief Subscribe to a multicast. Modifies the subscription request to include
     * all necessary information (side effect). Takes ownership of the ISubscriptionCallback, i.e.
     * deletes the callback when no longer required.
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
    void registerSubscription(
            const std::string& subscribeToName,
            const std::string& subscriberParticipantId,
            const std::string& providerParticipantId,
            const std::vector<std::string>& partitions,
            std::shared_ptr<ISubscriptionCallback> subscriptionCaller,
            std::shared_ptr<ISubscriptionListenerBase> subscriptionListener,
            std::shared_ptr<SubscriptionQos> qos,
            MulticastSubscriptionRequest& subscriptionRequest,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;

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

    /**
     * @brief Get a shared pointer to the subscription callback. The shared pointer point to null
     * if the multicast ID does not exist.
     *
     * @param multicastId
     * @return <std::shared_ptr<ISubscriptionCallback>
     */
    std::shared_ptr<ISubscriptionCallback> getMulticastSubscriptionCallback(
            const std::string& multicastId) override;

    /**
     * @brief Get a shared pointer to the subscription listener. The shared pointer points to null
     * if the subscription ID does not exist.
     *
     * @param subscriptionId
     * @return std::shared_ptr<ISubscriptionListenerBase>
     */
    std::shared_ptr<ISubscriptionListenerBase> getSubscriptionListener(
            const std::string& subscriptionId) override;

    /**
     * @brief Get a list of shared pointers to the subscription listeners. The list is empty
     * if the multicast ID does not exist.
     *
     * @param multicastId
     * @return std::forward_list<std::shared_ptr<ISubscriptionListenerBase>>
     */
    std::forward_list<std::shared_ptr<ISubscriptionListenerBase>> getMulticastSubscriptionListeners(
            const std::string& multicastId) override;

    void shutdown() override;

private:
    //    void checkMissedPublication(const Timer::TimerId id);
    DISALLOW_COPY_AND_ASSIGN(SubscriptionManager);

    class Subscription;

    void stopSubscription(std::shared_ptr<Subscription> subscription);

    ThreadSafeMap<std::string, std::shared_ptr<Subscription>> _subscriptions;

    MulticastReceiverDirectory _multicastSubscribers;
    std::recursive_mutex _multicastSubscribersMutex;

    std::shared_ptr<IMessageRouter> _messageRouter;

    std::shared_ptr<DelayedScheduler> _missedPublicationScheduler;
    ADD_LOGGER(SubscriptionManager)
    /**
     * @class SubscriptionManager::MissedPublicationRunnable
     * @brief
     */
    class MissedPublicationRunnable : public Runnable, public ObjectWithDecayTime
    {
    public:
        MissedPublicationRunnable(const TimePoint& expiryDate,
                                  std::int64_t expectedIntervalMSecs,
                                  const std::string& subscriptionId,
                                  std::shared_ptr<Subscription> subscription,
                                  std::weak_ptr<SubscriptionManager> subscriptionManager,
                                  std::int64_t alertAfterInterval);

        void shutdown() override;

        /**
         * @brief Checks whether a publication arrived in time, whether it is expired or
         *interrupted.
         *
         */
        void run() override;

    private:
        DISALLOW_COPY_AND_ASSIGN(MissedPublicationRunnable);
        std::int64_t timeSinceLastExpectedPublication(std::int64_t timeSinceLastPublication);
        std::int64_t _expectedIntervalMSecs;
        std::shared_ptr<Subscription> _subscription;
        const std::string _subscriptionId;
        std::int64_t _alertAfterInterval;
        std::weak_ptr<SubscriptionManager> _subscriptionManager;
        ADD_LOGGER(MissedPublicationRunnable)
    };
    /**
     * @class SubscriptionManager::SubscriptionEndRunnable
     * @brief
     */
    class SubscriptionEndRunnable : public Runnable
    {
    public:
        SubscriptionEndRunnable(const std::string& subscriptionId,
                                std::weak_ptr<SubscriptionManager> subscriptionManager);

        void shutdown() override;
        /**
         * @brief removes subscription once running.
         *
         */
        void run() override;

    private:
        DISALLOW_COPY_AND_ASSIGN(SubscriptionEndRunnable);
        std::string _subscriptionId;
        std::weak_ptr<SubscriptionManager> _subscriptionManager;
        ADD_LOGGER(SubscriptionEndRunnable)
    };
};

} // namespace joynr
#endif // SUBSCRIPTIONMANAGER_H
