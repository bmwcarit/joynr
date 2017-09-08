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
#include "joynr/SubscriptionManager.h"

#include <chrono>
#include <cstdint>
#include <mutex>

#include <boost/asio/io_service.hpp>

#include "joynr/IMessageRouter.h"
#include "joynr/ISubscriptionCallback.h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/MulticastReceiverDirectory.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/SingleThreadedDelayedScheduler.h"
#include "joynr/SubscriptionQos.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/SubscriptionUtil.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/SubscriptionException.h"

namespace joynr
{

class SubscriptionManager::Subscription
{
public:
    explicit Subscription(std::shared_ptr<ISubscriptionCallback> subscriptionCaller,
                          std::shared_ptr<ISubscriptionListenerBase> subscriptionListener);
    ~Subscription() = default;

    std::int64_t timeOfLastPublication;
    std::shared_ptr<ISubscriptionCallback> subscriptionCaller;
    std::shared_ptr<ISubscriptionListenerBase> subscriptionListener;
    std::recursive_mutex mutex;
    bool isStopped;
    std::uint32_t subscriptionEndRunnableHandle;
    std::uint32_t missedPublicationRunnableHandle;
    std::string multicastId;
    std::string subscriberParticipantId;
    std::string providerParticipantId;

private:
    DISALLOW_COPY_AND_ASSIGN(Subscription);
};

SubscriptionManager::~SubscriptionManager()
{
    JOYNR_LOG_TRACE(logger, "Destructing...");
    // check if all missed publication runnables are deleted before
    // deleting the missed publication scheduler

    missedPublicationScheduler->shutdown();
    delete missedPublicationScheduler;
    subscriptions.deleteAll();
}

INIT_LOGGER(SubscriptionManager);

SubscriptionManager::SubscriptionManager(boost::asio::io_service& ioService,
                                         std::shared_ptr<IMessageRouter> messageRouter)
        : ISubscriptionManager(),
          enable_shared_from_this<SubscriptionManager>(),
          subscriptions(),
          multicastSubscribers(),
          multicastSubscribersMutex(),
          messageRouter(messageRouter),
          missedPublicationScheduler(
                  new SingleThreadedDelayedScheduler("MissedPublications", ioService))
{
}

SubscriptionManager::SubscriptionManager(DelayedScheduler* scheduler,
                                         std::shared_ptr<IMessageRouter> messageRouter)
        : subscriptions(),
          multicastSubscribers(),
          multicastSubscribersMutex(),
          messageRouter(messageRouter),
          missedPublicationScheduler(scheduler)
{
}

void SubscriptionManager::shutdown()
{
    missedPublicationScheduler->shutdown();
}

void SubscriptionManager::registerSubscription(
        const std::string& subscribeToName,
        std::shared_ptr<ISubscriptionCallback> subscriptionCaller,
        std::shared_ptr<ISubscriptionListenerBase> subscriptionListener,
        std::shared_ptr<SubscriptionQos> qos,
        SubscriptionRequest& subscriptionRequest)
{
    // Register the subscription
    std::string subscriptionId = subscriptionRequest.getSubscriptionId();

    if (subscriptions.contains(subscriptionId)) {
        // pre-existing subscription: remove it first from the internal data structure
        unregisterSubscription(subscriptionId);
    }

    const std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::system_clock::now().time_since_epoch()).count();

    const std::int64_t subscriptionExpiryDateMs = qos->getExpiryDateMs();
    const std::int64_t alertAfterInterval = SubscriptionUtil::getAlertInterval(qos);
    const std::int64_t periodicPublicationInterval =
            SubscriptionUtil::getPeriodicPublicationInterval(qos);

    subscriptionRequest.setQos(qos);

    if (subscriptionExpiryDateMs != SubscriptionQos::NO_EXPIRY_DATE() &&
        subscriptionExpiryDateMs < now) {
        throw std::invalid_argument("Subscription ExpiryDate " +
                                    std::to_string(subscriptionExpiryDateMs) +
                                    " in the past. Now: " + std::to_string(now));
    }

    auto subscription = std::make_shared<Subscription>(subscriptionCaller, subscriptionListener);

    subscriptions.insert(subscriptionId, subscription);
    JOYNR_LOG_TRACE(logger, "Subscription registered. ID={}", subscriptionId);

    {
        std::lock_guard<std::recursive_mutex> subscriptionLocker(subscription->mutex);
        if (alertAfterInterval > 0 && periodicPublicationInterval > 0) {
            JOYNR_LOG_TRACE(logger, "Will notify if updates are missed.");
            JoynrTimePoint expiryDate(std::chrono::milliseconds{subscriptionExpiryDateMs});
            if (subscriptionExpiryDateMs == SubscriptionQos::NO_EXPIRY_DATE()) {
                expiryDate = JoynrTimePoint(
                        std::chrono::milliseconds(std::numeric_limits<std::int64_t>::max()));
            }
            subscription->missedPublicationRunnableHandle = missedPublicationScheduler->schedule(
                    std::make_shared<MissedPublicationRunnable>(expiryDate,
                                                                periodicPublicationInterval,
                                                                subscriptionId,
                                                                subscription,
                                                                shared_from_this(),
                                                                alertAfterInterval),
                    std::chrono::milliseconds(alertAfterInterval));
        } else if (subscriptionExpiryDateMs != SubscriptionQos::NO_EXPIRY_DATE()) {
            const std::int64_t now =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count();

            subscription->subscriptionEndRunnableHandle = missedPublicationScheduler->schedule(
                    std::make_shared<SubscriptionEndRunnable>(subscriptionId, shared_from_this()),
                    std::chrono::milliseconds(subscriptionExpiryDateMs - now));
        }
    }
    subscriptionRequest.setSubscribeToName(subscribeToName);
}

void SubscriptionManager::registerSubscription(
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
{
    std::string multicastId =
            util::createMulticastId(providerParticipantId, subscribeToName, partitions);
    subscriptionRequest.setMulticastId(multicastId);
    {
        std::lock_guard<std::recursive_mutex> multicastSubscribersLocker(multicastSubscribersMutex);
        std::string subscriptionId = subscriptionRequest.getSubscriptionId();

        // remove pre-exisiting multicast subscription
        if (subscriptions.contains(subscriptionId)) {
            std::shared_ptr<Subscription> subscription(subscriptions.value(subscriptionId));
            std::lock_guard<std::recursive_mutex> subscriptionLocker(subscription->mutex);
            std::string oldMulticastId = subscription->multicastId;
            if (multicastId != oldMulticastId) {
                unregisterSubscription(subscriptionId);
            } else {
                // do not call message router if multicast id has not changed
                subscriptions.remove(subscriptionId);
                stopSubscription(subscription);
            }
        }

        // register multicast subscription
        registerSubscription(subscribeToName,
                             subscriptionCaller,
                             subscriptionListener,
                             qos,
                             subscriptionRequest);
        std::shared_ptr<Subscription> subscription(subscriptions.value(subscriptionId));
        {
            std::lock_guard<std::recursive_mutex> subscriptionLocker(subscription->mutex);
            subscription->multicastId = multicastId;
            subscription->subscriberParticipantId = subscriberParticipantId;
            subscription->providerParticipantId = providerParticipantId;
        }
        if (!multicastSubscribers.contains(multicastId, subscriptionId)) {
            messageRouter->addMulticastReceiver(multicastId,
                                                subscriberParticipantId,
                                                providerParticipantId,
                                                onSuccess,
                                                onError);
            multicastSubscribers.registerMulticastReceiver(multicastId, subscriptionId);
        } else {
            onSuccess();
        }
    }
}

void SubscriptionManager::stopSubscription(std::shared_ptr<Subscription> subscription)
{
    std::lock_guard<std::recursive_mutex> subscriptionLocker(subscription->mutex);
    subscription->isStopped = true;
    if (subscription->subscriptionEndRunnableHandle != DelayedScheduler::INVALID_RUNNABLE_HANDLE) {
        missedPublicationScheduler->unschedule(subscription->subscriptionEndRunnableHandle);
        subscription->subscriptionEndRunnableHandle = DelayedScheduler::INVALID_RUNNABLE_HANDLE;
    }
    if (subscription->missedPublicationRunnableHandle !=
        DelayedScheduler::INVALID_RUNNABLE_HANDLE) {
        missedPublicationScheduler->unschedule(subscription->missedPublicationRunnableHandle);
        subscription->missedPublicationRunnableHandle = DelayedScheduler::INVALID_RUNNABLE_HANDLE;
    }
}

void SubscriptionManager::unregisterSubscription(const std::string& subscriptionId)
{
    if (!subscriptions.contains(subscriptionId)) {
        JOYNR_LOG_TRACE(logger,
                        "Called unregister on a non/no longer existent subscription, used id= {}",
                        subscriptionId);
        return;
    }
    std::shared_ptr<Subscription> subscription(subscriptions.take(subscriptionId));
    std::lock_guard<std::recursive_mutex> subscriptionLocker(subscription->mutex);
    JOYNR_LOG_TRACE(
            logger, "Called unregister / unsubscribe on subscription id= {}", subscriptionId);
    {
        std::lock_guard<std::recursive_mutex> multicastSubscribersLocker(multicastSubscribersMutex);
        std::string multicastId = subscription->multicastId;
        if (!multicastId.empty()) {
            stopSubscription(subscription);
            // remove multicast subscriber
            if (!multicastSubscribers.unregisterMulticastReceiver(multicastId, subscriptionId)) {
                JOYNR_LOG_FATAL(
                        logger,
                        "No multicast subscriber found for subscriptionId={}, multicastId={}",
                        subscriptionId,
                        multicastId);
                return;
            }
            auto onSuccess = [subscriptionId, multicastId]() {
                JOYNR_LOG_TRACE(logger,
                                "Multicast receiver unregistered. ID={}, multicastId={}",
                                subscriptionId,
                                multicastId);
            };
            std::shared_ptr<ISubscriptionListenerBase> subscriptionListener =
                    subscription->subscriptionListener;
            auto onError = [subscriptionId, multicastId, subscriptionListener](
                    const joynr::exceptions::ProviderRuntimeException& error) {
                std::string message = "Unsubscribe from subscription (ID=" + subscriptionId +
                                      ", multicastId=" + multicastId +
                                      ") failed. Could not remove multicast receiver: " +
                                      error.getMessage();
                exceptions::SubscriptionException subscriptionException(message, subscriptionId);
                subscriptionListener->onError(subscriptionException);
            };
            messageRouter->removeMulticastReceiver(multicastId,
                                                   subscription->subscriberParticipantId,
                                                   subscription->providerParticipantId,
                                                   onSuccess,
                                                   onError);
            return;
        }
    }
    stopSubscription(subscription);
}

#if 0
void SubscriptionManager::checkMissedPublication(
    const Timer::TimerId id)
{
    std::lock_guard<std::recursive_mutex> subscriptionLocker(&mutex);

    if (!isExpired() && !subscription->isStopped)
    {
        JOYNR_LOG_TRACE(logger, "Running MissedPublicationRunnable for subscription id= {}",subscriptionId);
        std::int64_t delay = 0;
        std::int64_t now = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()).count();
        std::int64_t timeSinceLastPublication = now
            - subscription->timeOfLastPublication;
        bool publicationInTime = timeSinceLastPublication < alertAfterInterval;
        if (publicationInTime)
        {
            JOYNR_LOG_TRACE(logger, "Publication in time!");
            delay = alertAfterInterval - timeSinceLastPublication;
        }
        else
        {
            JOYNR_LOG_TRACE(logger, "Publication missed!");
            std::shared_ptr<ISubscriptionCallback> callback =
                subscription->subscriptionCaller;

            callback->onError();
            delay = alertAfterInterval
                - timeSinceLastExpectedPublication(timeSinceLastPublication);
        }
        JOYNR_LOG_TRACE(logger, "Resceduling MissedPublicationRunnable with delay: {}",std::string::number(delay));
        subscription->missedPublicationRunnableHandle =
            subscriptionManager.missedPublicationScheduler->schedule(
                new MissedPublicationRunnable(decayTime,
                    expectedIntervalMSecs,
                    subscriptionId,
                    subscription,
                    subscriptionManager,
                    alertAfterInterval),
                delay);
    }
    else
    {
        JOYNR_LOG_TRACE(logger, "Publication expired / interrupted. Expiring on subscription id={}",subscriptionId);
    }
}
#endif

void SubscriptionManager::touchSubscriptionState(const std::string& subscriptionId)
{
    JOYNR_LOG_TRACE(logger, "Touching subscription state for id={}", subscriptionId);
    if (!subscriptions.contains(subscriptionId)) {
        return;
    }
    std::shared_ptr<Subscription> subscription(subscriptions.value(subscriptionId));
    {
        std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch()).count();
        std::lock_guard<std::recursive_mutex> subscriptionLocker(subscription->mutex);
        subscription->timeOfLastPublication = now;
    }
}

std::shared_ptr<ISubscriptionCallback> SubscriptionManager::getSubscriptionCallback(
        const std::string& subscriptionId)
{
    JOYNR_LOG_TRACE(logger, "Getting subscription callback for subscription id={}", subscriptionId);
    if (!subscriptions.contains(subscriptionId)) {
        JOYNR_LOG_TRACE(logger,
                        "Trying to access a non existing subscription callback for id={}",
                        subscriptionId);
        return std::shared_ptr<ISubscriptionCallback>();
    }

    std::shared_ptr<Subscription> subscription(subscriptions.value(subscriptionId));

    {
        std::lock_guard<std::recursive_mutex> subscriptionLockers(subscription->mutex);
        return subscription->subscriptionCaller;
    }
}

std::shared_ptr<ISubscriptionCallback> SubscriptionManager::getMulticastSubscriptionCallback(
        const std::string& multicastId)
{
    std::lock_guard<std::recursive_mutex> multicastSubscribersLocker(multicastSubscribersMutex);
    auto subscriptionIds = multicastSubscribers.getReceivers(multicastId);
    if (subscriptionIds.empty()) {
        JOYNR_LOG_WARN(logger,
                       "Trying to access a non existing subscription callback for multicast id={}",
                       multicastId);
        return std::shared_ptr<ISubscriptionCallback>();
    }
    return getSubscriptionCallback(*(subscriptionIds.begin()));
}

std::shared_ptr<ISubscriptionListenerBase> SubscriptionManager::getSubscriptionListener(
        const std::string& subscriptionId)
{
    JOYNR_LOG_TRACE(logger, "Getting subscription listener for subscription id={}", subscriptionId);
    if (!subscriptions.contains(subscriptionId)) {
        JOYNR_LOG_WARN(logger,
                       "Trying to access a non existing subscription listener for id={}",
                       subscriptionId);
        return std::shared_ptr<ISubscriptionListenerBase>();
    }

    std::shared_ptr<Subscription> subscription(subscriptions.value(subscriptionId));

    {
        std::lock_guard<std::recursive_mutex> subscriptionLockers(subscription->mutex);
        return subscription->subscriptionListener;
    }
}

std::forward_list<std::shared_ptr<ISubscriptionListenerBase>> SubscriptionManager::
        getMulticastSubscriptionListeners(const std::string& multicastId)
{
    std::forward_list<std::shared_ptr<ISubscriptionListenerBase>> listeners;
    {
        std::lock_guard<std::recursive_mutex> multicastSubscribersLocker(multicastSubscribersMutex);
        auto subscriptionIds = multicastSubscribers.getReceivers(multicastId);
        for (const auto& subscriptionId : subscriptionIds) {
            std::shared_ptr<ISubscriptionListenerBase> listener =
                    getSubscriptionListener(subscriptionId);
            if (listener) {
                listeners.push_front(listener);
            }
        }
    }
    return listeners;
}

//------ SubscriptionManager::Subscription ---------------------------------------
SubscriptionManager::Subscription::Subscription(
        std::shared_ptr<ISubscriptionCallback> subscriptionCaller,
        std::shared_ptr<ISubscriptionListenerBase> subscriptionListener)
        : timeOfLastPublication(0),
          subscriptionCaller(subscriptionCaller),
          subscriptionListener(subscriptionListener),
          mutex(),
          isStopped(false),
          subscriptionEndRunnableHandle(),
          missedPublicationRunnableHandle(),
          multicastId(),
          subscriberParticipantId(),
          providerParticipantId()
{
}

/**
  *  SubscriptionManager::MissedPublicationRunnable
  */
INIT_LOGGER(SubscriptionManager::MissedPublicationRunnable);

SubscriptionManager::MissedPublicationRunnable::MissedPublicationRunnable(
        const JoynrTimePoint& expiryDate,
        std::int64_t expectedIntervalMSecs,
        const std::string& subscriptionId,
        std::shared_ptr<Subscription> subscription,
        std::weak_ptr<SubscriptionManager> subscriptionManager,
        std::int64_t alertAfterInterval)
        : Runnable(),
          ObjectWithDecayTime(expiryDate),
          expectedIntervalMSecs(expectedIntervalMSecs),
          subscription(subscription),
          subscriptionId(subscriptionId),
          alertAfterInterval(alertAfterInterval),
          subscriptionManager(subscriptionManager)
{
}

void SubscriptionManager::MissedPublicationRunnable::shutdown()
{
}

void SubscriptionManager::MissedPublicationRunnable::run()
{
    std::lock_guard<std::recursive_mutex> subscriptionLocker(subscription->mutex);

    if (!isExpired() && !subscription->isStopped) {
        std::int64_t delay = 0;
        std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch()).count();
        std::int64_t timeSinceLastPublication = now - subscription->timeOfLastPublication;
        bool publicationInTime = timeSinceLastPublication < alertAfterInterval;
        if (publicationInTime) {
            JOYNR_LOG_TRACE(logger, "Publication in time for subscription id={}", subscriptionId);
            delay = alertAfterInterval - timeSinceLastPublication;
        } else {
            JOYNR_LOG_TRACE(logger, "Publication missed for subscription id={}", subscriptionId);
            std::shared_ptr<ISubscriptionListenerBase> listener =
                    subscription->subscriptionListener;

            exceptions::PublicationMissedException error(subscriptionId);
            listener->onError(error);
            delay = alertAfterInterval - timeSinceLastExpectedPublication(timeSinceLastPublication);
        }

        if (auto subscriptionManagerSharedPtr = subscriptionManager.lock()) {
            JOYNR_LOG_TRACE(logger, "Rescheduling MissedPublicationRunnable with delay: {}", delay);
            subscription->missedPublicationRunnableHandle =
                    subscriptionManagerSharedPtr->missedPublicationScheduler->schedule(
                            std::make_shared<MissedPublicationRunnable>(decayTime,
                                                                        expectedIntervalMSecs,
                                                                        subscriptionId,
                                                                        subscription,
                                                                        subscriptionManager,
                                                                        alertAfterInterval),
                            std::chrono::milliseconds(delay));
        } else {
            JOYNR_LOG_ERROR(logger,
                            "Error: Failed to rescheduling MissedPublicationRunnable with delay: "
                            "{} because SubscriptionManager is not available",
                            delay);
        }
    } else {
        JOYNR_LOG_TRACE(logger,
                        "Publication expired / interrupted. Expiring on subscription id={}",
                        subscriptionId);
    }
}

std::int64_t SubscriptionManager::MissedPublicationRunnable::timeSinceLastExpectedPublication(
        std::int64_t timeSinceLastPublication)
{
    return timeSinceLastPublication % expectedIntervalMSecs;
}

INIT_LOGGER(SubscriptionManager::SubscriptionEndRunnable);

SubscriptionManager::SubscriptionEndRunnable::SubscriptionEndRunnable(
        const std::string& subscriptionId,
        std::weak_ptr<SubscriptionManager> subscriptionManager)
        : Runnable(), subscriptionId(subscriptionId), subscriptionManager(subscriptionManager)
{
}

void SubscriptionManager::SubscriptionEndRunnable::shutdown()
{
}

void SubscriptionManager::SubscriptionEndRunnable::run()
{
    if (auto subscriptionManagerSharedPtr = subscriptionManager.lock()) {
        JOYNR_LOG_TRACE(
                logger, "Running SubscriptionEndRunnable for subscription id= {}", subscriptionId);
        JOYNR_LOG_TRACE(logger,
                        "Publication expired / interrupted. Expiring on subscription id={}",
                        subscriptionId);
        subscriptionManagerSharedPtr->unregisterSubscription(subscriptionId);
    } else {
        JOYNR_LOG_ERROR(logger,
                        "Error running SubscriptionEndRunnable for subscription id= {} because "
                        "subscriptionManager is not available",
                        subscriptionId);
    }
}

} // namespace joynr
