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

#include <assert.h>
#include <chrono>

namespace joynr
{

using namespace std::chrono;

class SubscriptionManager::Subscription
{
public:
    explicit Subscription(std::shared_ptr<ISubscriptionCallback> subscriptionCaller);
    ~Subscription();

    qint64 timeOfLastPublication;
    std::shared_ptr<ISubscriptionCallback> subscriptionCaller;
    std::recursive_mutex mutex;
    bool isStopped;
    quint32 subscriptionEndRunnableHandle;
    quint32 missedPublicationRunnableHandle;

private:
    DISALLOW_COPY_AND_ASSIGN(Subscription);
};

using namespace joynr_logging;
Logger* SubscriptionManager::logger =
        Logging::getInstance()->getLogger("MSG", "SubscriptionManager");

SubscriptionManager::~SubscriptionManager()
{
    LOG_DEBUG(logger, "Destructing...");
    // check if all missed publication runnables are deleted before
    // deleting the missed publication scheduler

    missedPublicationScheduler->shutdown();
    delete missedPublicationScheduler;
    subscriptions.deleteAll();
}

SubscriptionManager::SubscriptionManager()
        : subscriptions(),
          missedPublicationScheduler(new SingleThreadedDelayedScheduler("MissedPublications", 0))
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
    LOG_DEBUG(logger, FormatString("Subscription registered. ID=%1").arg(subscriptionId).str());

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
            LOG_DEBUG(logger, "Will notify if updates are missed.");
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
                    alertAfterInterval);
        } else if (qos->getExpiryDate() != joynr::SubscriptionQos::NO_EXPIRY_DATE()) {
            int64_t now =
                    duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            subscription->subscriptionEndRunnableHandle = missedPublicationScheduler->schedule(
                    new SubscriptionEndRunnable(subscriptionId, *this), qos->getExpiryDate() - now);
        }
    }
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName(subscribeToName);
}

void SubscriptionManager::unregisterSubscription(const std::string& subscriptionId)
{
    if (subscriptions.contains(subscriptionId)) {
        std::shared_ptr<Subscription> subscription(subscriptions.take(subscriptionId));
        LOG_DEBUG(logger,
                  FormatString("Called unregister / unsubscribe on subscription id= %1")
                          .arg(subscriptionId)
                          .str());
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
        LOG_DEBUG(logger,
                  FormatString(
                          "Called unregister on a non/no longer existent subscription, used id= %1")
                          .arg(subscriptionId)
                          .str());
    }
}

#if 0
void SubscriptionManager::checkMissedPublication(
    const Timer::TimerId id)
{
    std::lock_guard<std::recursive_mutex> subscriptionLocker(&mutex);

    if (!isExpired() && !subscription->isStopped)
    {
        LOG_DEBUG(
            logger,
            "Running MissedPublicationRunnable for subscription id= "
                + subscriptionId);
        qint64 delay = 0;
        int64_t now = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()).count();
        qint64 timeSinceLastPublication = now
            - subscription->timeOfLastPublication;
        bool publicationInTime = timeSinceLastPublication < alertAfterInterval;
        if (publicationInTime)
        {
            LOG_TRACE(logger, "Publication in time!");
            delay = alertAfterInterval - timeSinceLastPublication;
        }
        else
        {
            LOG_DEBUG(logger, "Publication missed!");
            std::shared_ptr<ISubscriptionCallback> callback =
                subscription->subscriptionCaller;

            callback->onError();
            delay = alertAfterInterval
                - timeSinceLastExpectedPublication(timeSinceLastPublication);
        }
        LOG_DEBUG(logger,
            "Resceduling MissedPublicationRunnable with delay: "
                + std::string::number(delay));
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
        LOG_DEBUG(
            logger,
            "Publication expired / interrupted. Expiring on subscription id="
                + subscriptionId);
    }
}
#endif

void SubscriptionManager::touchSubscriptionState(const std::string& subscriptionId)
{
    LOG_DEBUG(logger,
              FormatString("Touching subscription state for id=%1").arg(subscriptionId).str());
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
    LOG_DEBUG(logger,
              FormatString("Getting subscription callback for subscription id=%1")
                      .arg(subscriptionId)
                      .str());
    if (!subscriptions.contains(subscriptionId)) {
        LOG_DEBUG(logger,
                  FormatString("Trying to acces a non existing subscription callback for id=%1")
                          .arg(subscriptionId)
                          .str());
        return std::shared_ptr<ISubscriptionCallback>();
    }

    std::shared_ptr<Subscription> subscription(subscriptions.value(subscriptionId));

    {
        std::lock_guard<std::recursive_mutex> subscriptionLockers(subscription->mutex);
        return subscription->subscriptionCaller;
    }
}

//------ SubscriptionManager::Subscription ---------------------------------------

SubscriptionManager::Subscription::~Subscription()
{
}

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
Logger* SubscriptionManager::MissedPublicationRunnable::logger =
        Logging::getInstance()->getLogger("MSG", "MissedPublicationRunnable");

SubscriptionManager::MissedPublicationRunnable::MissedPublicationRunnable(
        const JoynrTimePoint& expiryDate,
        const qint64& expectedIntervalMSecs,
        const std::string& subscriptionId,
        std::shared_ptr<Subscription> subscription,
        SubscriptionManager& subscriptionManager,
        const qint64& alertAfterInterval)
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
        LOG_DEBUG(logger,
                  FormatString("Running MissedPublicationRunnable for subscription id= %1")
                          .arg(subscriptionId)
                          .str());
        qint64 delay = 0;
        int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        qint64 timeSinceLastPublication = now - subscription->timeOfLastPublication;
        bool publicationInTime = timeSinceLastPublication < alertAfterInterval;
        if (publicationInTime) {
            LOG_TRACE(logger, "Publication in time!");
            delay = alertAfterInterval - timeSinceLastPublication;
        } else {
            LOG_DEBUG(logger, "Publication missed!");
            std::shared_ptr<ISubscriptionCallback> callback = subscription->subscriptionCaller;

            exceptions::PublicationMissedException error(subscriptionId);
            callback->onError(error);
            delay = alertAfterInterval - timeSinceLastExpectedPublication(timeSinceLastPublication);
        }
        LOG_DEBUG(logger,
                  FormatString("Rescheduling MissedPublicationRunnable with delay: %1")
                          .arg(delay)
                          .str());
        subscription->missedPublicationRunnableHandle =
                subscriptionManager.missedPublicationScheduler->schedule(
                        new MissedPublicationRunnable(decayTime,
                                                      expectedIntervalMSecs,
                                                      subscriptionId,
                                                      subscription,
                                                      subscriptionManager,
                                                      alertAfterInterval),
                        delay);
    } else {
        LOG_DEBUG(logger,
                  FormatString("Publication expired / interrupted. Expiring on subscription id=%1")
                          .arg(subscriptionId)
                          .str());
    }
}

qint64 SubscriptionManager::MissedPublicationRunnable::timeSinceLastExpectedPublication(
        const qint64& timeSinceLastPublication)
{
    return timeSinceLastPublication % expectedIntervalMSecs;
}

Logger* SubscriptionManager::SubscriptionEndRunnable::logger =
        Logging::getInstance()->getLogger("MSG", "SubscriptionEndRunnable");

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
    LOG_DEBUG(logger,
              FormatString("Running SubscriptionEndRunnable for subscription id= %1")
                      .arg(subscriptionId)
                      .str());
    LOG_DEBUG(logger,
              FormatString("Publication expired / interrupted. Expiring on subscription id=%1")
                      .arg(subscriptionId)
                      .str());
    subscriptionManager.unregisterSubscription(subscriptionId);
}

} // namespace joynr
