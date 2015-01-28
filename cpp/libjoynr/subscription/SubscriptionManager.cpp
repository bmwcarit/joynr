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
#include "joynr/DelayedScheduler.h"
#include <QUuid>
#include <assert.h>

namespace joynr
{

class SubscriptionManager::Subscription
{
public:
    Subscription(QSharedPointer<ISubscriptionCallback> subscriptionCaller);
    ~Subscription();

    qint64 timeOfLastPublication;
    QSharedPointer<ISubscriptionCallback> subscriptionCaller;
    QMutex mutex;
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

    delete missedPublicationScheduler;
    subscriptions.clear();
}

SubscriptionManager::SubscriptionManager()
        : subscriptions(),
          subscriptionsLock(QReadWriteLock::RecursionMode::Recursive),
          missedPublicationScheduler(new SingleThreadedDelayedScheduler(
                  QString("SubscriptionManager-MissedPublicationScheduler")))
{
}

SubscriptionManager::SubscriptionManager(DelayedScheduler* scheduler)
        : subscriptions(),
          subscriptionsLock(QReadWriteLock::RecursionMode::Recursive),
          missedPublicationScheduler(scheduler)
{
}

void SubscriptionManager::registerSubscription(
        const QString& subscribeToName,
        QSharedPointer<ISubscriptionCallback> subscriptionCaller,
        QSharedPointer<SubscriptionQos> qos,
        SubscriptionRequest& subscriptionRequest)
{
    // Register the subscription
    QString subscriptionId = subscriptionRequest.getSubscriptionId();
    LOG_DEBUG(logger, "Subscription registered. ID=" + subscriptionId);

    // lock the access to the subscriptions data structure
    // we don't use a separate block for locking/unlocking, because the subscription object is
    // required after the subscription unlock
    QWriteLocker subscriptionsLocker(&subscriptionsLock);

    if (subscriptions.contains(subscriptionId)) {
        // pre-existing subscription: remove it first from the internal data structure
        unregisterSubscription(subscriptionId);
    }

    if (qos->getExpiryDate() != joynr::SubscriptionQos::NO_EXPIRY_DATE() &&
        qos->getExpiryDate() < QDateTime::currentMSecsSinceEpoch()) {
        LOG_DEBUG(logger, "Expiry date is in the past: no subscription created");
        return;
    }

    QSharedPointer<Subscription> subscription(new Subscription(subscriptionCaller));

    subscriptions.insert(subscriptionId, subscription);

    subscriptionsLocker.unlock();

    {
        QMutexLocker subscriptionLocker(&(subscription->mutex));
        if (SubscriptionUtil::getAlertInterval(qos.data()) > 0 &&
            SubscriptionUtil::getPeriodicPublicationInterval(qos.data()) > 0) {
            LOG_DEBUG(logger, "Will notify if updates are missed.");
            qint64 alertAfterInterval = SubscriptionUtil::getAlertInterval(qos.data());
            qint64 expiryDate = qos->getExpiryDate();
            qint64 periodicPublicationInterval =
                    SubscriptionUtil::getPeriodicPublicationInterval(qos.data());

            if (expiryDate == joynr::SubscriptionQos::NO_EXPIRY_DATE()) {
                expiryDate = joynr::SubscriptionQos::NO_EXPIRY_DATE_TTL();
            }

            subscription->missedPublicationRunnableHandle = missedPublicationScheduler->schedule(
                    new MissedPublicationRunnable(QDateTime::fromMSecsSinceEpoch(expiryDate),
                                                  periodicPublicationInterval,
                                                  subscriptionId,
                                                  subscription,
                                                  *this,
                                                  alertAfterInterval),
                    alertAfterInterval);
        } else if (qos->getExpiryDate() != joynr::SubscriptionQos::NO_EXPIRY_DATE()) {
            subscription->subscriptionEndRunnableHandle = missedPublicationScheduler->schedule(
                    new SubscriptionEndRunnable(subscriptionId, *this),
                    qos->getExpiryDate() - QDateTime::currentMSecsSinceEpoch());
        }
    }
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName(subscribeToName);
    subscriptionRequest.setQos(qos);
}

void SubscriptionManager::unregisterSubscription(const QString& subscriptionId)
{
    QWriteLocker subscriptionsLocker(&subscriptionsLock);
    if (subscriptions.contains(subscriptionId)) {
        QSharedPointer<Subscription> subscription = subscriptions.take(subscriptionId);
        LOG_DEBUG(logger, "Called unregister / unsubscribe on subscription id= " + subscriptionId);
        QMutexLocker subscriptionLocker(&(subscription->mutex));
        subscription->isStopped = true;
        if (subscription->subscriptionEndRunnableHandle !=
            DelayedScheduler::INVALID_RUNNABLE_HANDLE()) {
            missedPublicationScheduler->unschedule(subscription->subscriptionEndRunnableHandle);
            subscription->subscriptionEndRunnableHandle =
                    DelayedScheduler::INVALID_RUNNABLE_HANDLE();
        }
        if (subscription->missedPublicationRunnableHandle !=
            DelayedScheduler::INVALID_RUNNABLE_HANDLE()) {
            missedPublicationScheduler->unschedule(subscription->missedPublicationRunnableHandle);
            subscription->missedPublicationRunnableHandle =
                    DelayedScheduler::INVALID_RUNNABLE_HANDLE();
        }
    } else {
        LOG_DEBUG(logger,
                  "Called unregister on a non/no longer existent subscription, used id= " +
                          subscriptionId);
    }
}

void SubscriptionManager::touchSubscriptionState(const QString& subscriptionId)
{
    QReadLocker subscriptionsLocker(&subscriptionsLock);
    LOG_DEBUG(logger, "Touching subscription state for id=" + subscriptionId);
    if (!subscriptions.contains(subscriptionId)) {
        return;
    }
    QSharedPointer<Subscription> subscription = subscriptions.value(subscriptionId);
    {
        QMutexLocker subscriptionLocker(&(subscription->mutex));
        subscription->timeOfLastPublication = QDateTime::currentMSecsSinceEpoch();
    }
}

QSharedPointer<ISubscriptionCallback> SubscriptionManager::getSubscriptionCallback(
        const QString& subscriptionId)
{
    QReadLocker subscriptionsLocker(&subscriptionsLock);
    LOG_DEBUG(logger, "Getting subscription callback for subscription id=" + subscriptionId);
    if (!subscriptions.contains(subscriptionId)) {
        LOG_DEBUG(logger,
                  "Trying to acces a non existing subscription callback for id=" + subscriptionId);
    }

    QSharedPointer<Subscription> subscription(subscriptions.value(subscriptionId));

    {
        QMutexLocker subscriptionLockers(&(subscription->mutex));
        return subscription->subscriptionCaller;
    }
}

//------ SubscriptionManager::Subscription ---------------------------------------

SubscriptionManager::Subscription::~Subscription()
{
}

SubscriptionManager::Subscription::Subscription(
        QSharedPointer<ISubscriptionCallback> subscriptionCaller)
        : timeOfLastPublication(0),
          subscriptionCaller(subscriptionCaller),
          mutex(QMutex::RecursionMode::Recursive),
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
        const QDateTime& expiryDate,
        const qint64& expectedIntervalMSecs,
        const QString& subscriptionId,
        QSharedPointer<Subscription> subscription,
        SubscriptionManager& subscriptionManager,
        const qint64& alertAfterInterval)
        : ObjectWithDecayTime(expiryDate),
          expectedIntervalMSecs(expectedIntervalMSecs),
          subscription(subscription),
          subscriptionId(subscriptionId),
          alertAfterInterval(alertAfterInterval),
          subscriptionManager(subscriptionManager)
{
    setAutoDelete(true);
}

void SubscriptionManager::MissedPublicationRunnable::run()
{
    QMutexLocker subscriptionLocker(&(subscription->mutex));

    if (!isExpired() && !subscription->isStopped) {
        LOG_DEBUG(
                logger, "Running MissedPublicationRunnable for subscription id= " + subscriptionId);
        qint64 delay = 0;
        qint64 timeSinceLastPublication =
                QDateTime::currentMSecsSinceEpoch() - subscription->timeOfLastPublication;
        bool publicationInTime = timeSinceLastPublication < alertAfterInterval;
        if (publicationInTime) {
            LOG_TRACE(logger, "Publication in time!");
            delay = alertAfterInterval - timeSinceLastPublication;
        } else {
            LOG_DEBUG(logger, "Publication missed!");
            QSharedPointer<ISubscriptionCallback> callback = subscription->subscriptionCaller;

            callback->publicationMissed();
            delay = alertAfterInterval - timeSinceLastExpectedPublication(timeSinceLastPublication);
        }
        LOG_DEBUG(logger,
                  "Resceduling MissedPublicationRunnable with delay: " + QString::number(delay));
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
        LOG_DEBUG(
                logger,
                "Publication expired / interrupted. Expiring on subscription id=" + subscriptionId);
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
        const QString& subscriptionId,
        SubscriptionManager& subscriptionManager)
        : subscriptionId(subscriptionId), subscriptionManager(subscriptionManager)
{
    setAutoDelete(true);
}

void SubscriptionManager::SubscriptionEndRunnable::run()
{
    LOG_DEBUG(logger, "Running SubscriptionEndRunnable for subscription id= " + subscriptionId);
    LOG_DEBUG(logger,
              "Publication expired / interrupted. Expiring on subscription id=" + subscriptionId);
    subscriptionManager.unregisterSubscription(subscriptionId);
}

} // namespace joynr
