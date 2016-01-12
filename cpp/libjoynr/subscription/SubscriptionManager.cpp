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
#include "joynr/SubscriptionManager.h"

#include "joynr/SubscriptionUtil.h"
#include "joynr/SingleThreadedDelayedScheduler.h"

#include <mutex>

#include <cassert>
#include <chrono>
#include <cstdint>

namespace joynr
{

using namespace std::chrono;

class SubscriptionManager::Subscription
{
public:
    explicit Subscription(std::shared_ptr<ISubscriptionCallback> subscriptionCaller);
    ~Subscription() = default;

    int64_t timeOfLastPublication;
    std::shared_ptr<ISubscriptionCallback> subscriptionCaller;
    std::recursive_mutex mutex;
    bool isStopped;
    uint32_t subscriptionEndRunnableHandle;
    uint32_t missedPublicationRunnableHandle;

private:
    DISALLOW_COPY_AND_ASSIGN(Subscription);
};

SubscriptionManager::~SubscriptionManager()
{
    JOYNR_LOG_DEBUG(logger) << "Destructing...";
    // check if all missed publication runnables are deleted before
    // deleting the missed publication scheduler

    missedPublicationScheduler->shutdown();
    delete missedPublicationScheduler;
    subscriptions.deleteAll();
}

INIT_LOGGER(SubscriptionManager);

SubscriptionManager::SubscriptionManager()
        : subscriptions(),
          missedPublicationScheduler(new SingleThreadedDelayedScheduler("MissedPublications"))
{
}

SubscriptionManager::SubscriptionManager(DelayedScheduler* scheduler)
        : subscriptions(), missedPublicationScheduler(scheduler)
{
}

void SubscriptionManager::registerSubscription(
        const std::string& subscribeToName,
        std::shared_ptr<ISubscriptionCallback> subscriptionCaller,
        const Variant& qosVariant,
        SubscriptionRequest& subscriptionRequest)
{
    // Register the subscription
    std::string subscriptionId = subscriptionRequest.getSubscriptionId();
    JOYNR_LOG_DEBUG(logger) << "Subscription registered. ID=" << subscriptionId;

    if (subscriptions.contains(subscriptionId)) {
        // pre-existing subscription: remove it first from the internal data structure
        unregisterSubscription(subscriptionId);
    }

    int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    subscriptionRequest.setQos(qosVariant);
    const SubscriptionQos* qos = subscriptionRequest.getSubscriptionQosPtr();
    if (qos->getExpiryDate() != joynr::SubscriptionQos::NO_EXPIRY_DATE() &&
        qos->getExpiryDate() < now) {
        throw std::invalid_argument("Subscription ExpiryDate " +
                                    std::to_string(qos->getExpiryDate()) + " in the past. Now: " +
                                    std::to_string(now));
    }

    std::shared_ptr<Subscription> subscription(new Subscription(subscriptionCaller));

    subscriptions.insert(subscriptionId, subscription);

    {
        std::lock_guard<std::recursive_mutex> subscriptionLocker(subscription->mutex);
        if (SubscriptionUtil::getAlertInterval(qosVariant) > 0 &&
            SubscriptionUtil::getPeriodicPublicationInterval(qosVariant) > 0) {
            JOYNR_LOG_DEBUG(logger) << "Will notify if updates are missed.";
            int64_t alertAfterInterval = SubscriptionUtil::getAlertInterval(qosVariant);
            JoynrTimePoint expiryDate{milliseconds(qos->getExpiryDate())};
            int64_t periodicPublicationInterval =
                    SubscriptionUtil::getPeriodicPublicationInterval(qosVariant);

            if (expiryDate.time_since_epoch().count() == joynr::SubscriptionQos::NO_EXPIRY_DATE()) {
                expiryDate =
                        JoynrTimePoint{milliseconds(joynr::SubscriptionQos::NO_EXPIRY_DATE_TTL())};
            }

            subscription->missedPublicationRunnableHandle = missedPublicationScheduler->schedule(
                    new MissedPublicationRunnable(expiryDate,
                                                  periodicPublicationInterval,
                                                  subscriptionId,
                                                  subscription,
                                                  *this,
                                                  alertAfterInterval),
                    std::chrono::milliseconds(alertAfterInterval));
        } else if (qos->getExpiryDate() != joynr::SubscriptionQos::NO_EXPIRY_DATE()) {
            int64_t now =
                    duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            subscription->subscriptionEndRunnableHandle = missedPublicationScheduler->schedule(
                    new SubscriptionEndRunnable(subscriptionId, *this),
                    std::chrono::milliseconds(qos->getExpiryDate() - now));
        }
    }
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName(subscribeToName);
}

void SubscriptionManager::unregisterSubscription(const std::string& subscriptionId)
{
    if (subscriptions.contains(subscriptionId)) {
        std::shared_ptr<Subscription> subscription(subscriptions.take(subscriptionId));
        JOYNR_LOG_DEBUG(logger) << "Called unregister / unsubscribe on subscription id= "
                                << subscriptionId;
        std::lock_guard<std::recursive_mutex> subscriptionLocker(subscription->mutex);
        subscription->isStopped = true;
        if (subscription->subscriptionEndRunnableHandle !=
            DelayedScheduler::INVALID_RUNNABLE_HANDLE) {
            missedPublicationScheduler->unschedule(subscription->subscriptionEndRunnableHandle);
            subscription->subscriptionEndRunnableHandle = DelayedScheduler::INVALID_RUNNABLE_HANDLE;
        }
        if (subscription->missedPublicationRunnableHandle !=
            DelayedScheduler::INVALID_RUNNABLE_HANDLE) {
            missedPublicationScheduler->unschedule(subscription->missedPublicationRunnableHandle);
            subscription->missedPublicationRunnableHandle =
                    DelayedScheduler::INVALID_RUNNABLE_HANDLE;
        }
    } else {
        JOYNR_LOG_DEBUG(logger)
                << "Called unregister on a non/no longer existent subscription, used id= "
                << subscriptionId;
    }
}

#if 0
void SubscriptionManager::checkMissedPublication(
    const Timer::TimerId id)
{
    std::lock_guard<std::recursive_mutex> subscriptionLocker(&mutex);

    if (!isExpired() && !subscription->isStopped)
    {
        JOYNR_LOG_DEBUG(logger) << "Running MissedPublicationRunnable for subscription id= " << subscriptionId;
        int64_t delay = 0;
        int64_t now = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()).count();
        int64_t timeSinceLastPublication = now
            - subscription->timeOfLastPublication;
        bool publicationInTime = timeSinceLastPublication < alertAfterInterval;
        if (publicationInTime)
        {
            JOYNR_LOG_TRACE(logger) << "Publication in time!";
            delay = alertAfterInterval - timeSinceLastPublication;
        }
        else
        {
            JOYNR_LOG_DEBUG(logger) << "Publication missed!";
            std::shared_ptr<ISubscriptionCallback> callback =
                subscription->subscriptionCaller;

            callback->onError();
            delay = alertAfterInterval
                - timeSinceLastExpectedPublication(timeSinceLastPublication);
        }
        JOYNR_LOG_DEBUG(logger) << 
            "Resceduling MissedPublicationRunnable with delay: " << std::string::number(delay);
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
        JOYNR_LOG_DEBUG(logger) << "Publication expired / interrupted. Expiring on subscription id=" << subscriptionId;
    }
}
#endif

void SubscriptionManager::touchSubscriptionState(const std::string& subscriptionId)
{
    JOYNR_LOG_DEBUG(logger) << "Touching subscription state for id=" << subscriptionId;
    if (!subscriptions.contains(subscriptionId)) {
        return;
    }
    std::shared_ptr<Subscription> subscription(subscriptions.value(subscriptionId));
    {
        int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        std::lock_guard<std::recursive_mutex> subscriptionLocker(subscription->mutex);
        subscription->timeOfLastPublication = now;
    }
}

std::shared_ptr<ISubscriptionCallback> SubscriptionManager::getSubscriptionCallback(
        const std::string& subscriptionId)
{
    JOYNR_LOG_DEBUG(logger) << "Getting subscription callback for subscription id="
                            << subscriptionId;
    if (!subscriptions.contains(subscriptionId)) {
        JOYNR_LOG_DEBUG(logger) << "Trying to acces a non existing subscription callback for id="
                                << subscriptionId;
        return std::shared_ptr<ISubscriptionCallback>();
    }

    std::shared_ptr<Subscription> subscription(subscriptions.value(subscriptionId));

    {
        std::lock_guard<std::recursive_mutex> subscriptionLockers(subscription->mutex);
        return subscription->subscriptionCaller;
    }
}

//------ SubscriptionManager::Subscription ---------------------------------------
SubscriptionManager::Subscription::Subscription(
        std::shared_ptr<ISubscriptionCallback> subscriptionCaller)
        : timeOfLastPublication(0),
          subscriptionCaller(subscriptionCaller),
          mutex(),
          isStopped(false),
          subscriptionEndRunnableHandle(),
          missedPublicationRunnableHandle()
{
}

/**
  *  SubscriptionManager::MissedPublicationRunnable
  */
INIT_LOGGER(SubscriptionManager::MissedPublicationRunnable);

SubscriptionManager::MissedPublicationRunnable::MissedPublicationRunnable(
        const JoynrTimePoint& expiryDate,
        const int64_t& expectedIntervalMSecs,
        const std::string& subscriptionId,
        std::shared_ptr<Subscription> subscription,
        SubscriptionManager& subscriptionManager,
        const int64_t& alertAfterInterval)
        : joynr::Runnable(true),
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
        JOYNR_LOG_DEBUG(logger) << "Running MissedPublicationRunnable for subscription id= "
                                << subscriptionId;
        int64_t delay = 0;
        int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        int64_t timeSinceLastPublication = now - subscription->timeOfLastPublication;
        bool publicationInTime = timeSinceLastPublication < alertAfterInterval;
        if (publicationInTime) {
            JOYNR_LOG_TRACE(logger) << "Publication in time!";
            delay = alertAfterInterval - timeSinceLastPublication;
        } else {
            JOYNR_LOG_DEBUG(logger) << "Publication missed!";
            std::shared_ptr<ISubscriptionCallback> callback = subscription->subscriptionCaller;

            exceptions::PublicationMissedException error(subscriptionId);
            callback->onError(error);
            delay = alertAfterInterval - timeSinceLastExpectedPublication(timeSinceLastPublication);
        }
        JOYNR_LOG_DEBUG(logger) << "Rescheduling MissedPublicationRunnable with delay: " << delay;
        subscription->missedPublicationRunnableHandle =
                subscriptionManager.missedPublicationScheduler->schedule(
                        new MissedPublicationRunnable(decayTime,
                                                      expectedIntervalMSecs,
                                                      subscriptionId,
                                                      subscription,
                                                      subscriptionManager,
                                                      alertAfterInterval),
                        std::chrono::milliseconds(delay));
    } else {
        JOYNR_LOG_DEBUG(logger) << "Publication expired / interrupted. Expiring on subscription id="
                                << subscriptionId;
    }
}

int64_t SubscriptionManager::MissedPublicationRunnable::timeSinceLastExpectedPublication(
        const int64_t& timeSinceLastPublication)
{
    return timeSinceLastPublication % expectedIntervalMSecs;
}

INIT_LOGGER(SubscriptionManager::SubscriptionEndRunnable);

SubscriptionManager::SubscriptionEndRunnable::SubscriptionEndRunnable(
        const std::string& subscriptionId,
        SubscriptionManager& subscriptionManager)
        : joynr::Runnable(true),
          subscriptionId(subscriptionId),
          subscriptionManager(subscriptionManager)
{
}

void SubscriptionManager::SubscriptionEndRunnable::shutdown()
{
}

void SubscriptionManager::SubscriptionEndRunnable::run()
{
    JOYNR_LOG_DEBUG(logger) << "Running SubscriptionEndRunnable for subscription id= "
                            << subscriptionId;
    JOYNR_LOG_DEBUG(logger) << "Publication expired / interrupted. Expiring on subscription id="
                            << subscriptionId;
    subscriptionManager.unregisterSubscription(subscriptionId);
}

} // namespace joynr
