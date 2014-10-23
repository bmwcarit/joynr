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
#include "joynr/PubSubState.h"

#include "joynr/SubscriptionUtil.h"

#include <QUuid>
#include <assert.h>

namespace joynr
{

using namespace joynr_logging;
Logger* SubscriptionManager::logger =
        Logging::getInstance()->getLogger("MSG", "SubscriptionManager");

SubscriptionManager::~SubscriptionManager()
{
    LOG_DEBUG(logger, "Destructing...");
    // check if all missed publication runnables are deleted before
    // deleting the missed publication scheduler

    delete missedPublicationScheduler;
    subscriptionStates->deleteAll(); // deletes and sets al elements to NULL
    delete subscriptionStates;
}

SubscriptionManager::SubscriptionManager()
        : subscriptionDirectory(QString("SubsriptionManager-SubscriptionDirectory")),
          subscriptionStates(new ThreadSafeMap<QString, PubSubState*>()),
          missedPublicationScheduler(new SingleThreadedDelayedScheduler(
                  QString("SubscriptionManager-MissedPublicationScheduler")))
{
}

SubscriptionManager::SubscriptionManager(DelayedScheduler* scheduler)
        : subscriptionDirectory(QString("SubsriptionManager-SubscriptionDirectory")),
          subscriptionStates(new ThreadSafeMap<QString, PubSubState*>()),
          missedPublicationScheduler(scheduler)
{
}

void SubscriptionManager::registerSubscription(
        const QString& subscribeToName,
        ISubscriptionCallback* subscriptionCaller, // SubMgr gets ownership of ptr
        QSharedPointer<SubscriptionQos> qos,
        SubscriptionRequest& subscriptionRequest)
{
    if (qos->getExpiryDate() != joynr::SubscriptionQos::NO_EXPIRY_DATE() &&
        qos->getExpiryDate() < QDateTime::currentMSecsSinceEpoch()) {
        LOG_DEBUG(logger, "Expiry date is in the past: no subscription created");
        return;
    }

    // Register the subscription
    QString subscriptionId = subscriptionRequest.getSubscriptionId();
    LOG_DEBUG(logger, "Subscription registered. ID=" + subscriptionId);
    subscriptionDirectory.add(subscriptionId, subscriptionCaller); // Owner: directory

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

        // Owner: SubscriptionManager. Delete triggered by MissedPublicationRunnable
        PubSubState* subState = new PubSubState();
        subscriptionStates->insert(subscriptionId, subState);
        MissedPublicationRunnable* processor =
                new MissedPublicationRunnable(QDateTime::fromMSecsSinceEpoch(expiryDate),
                                              periodicPublicationInterval,
                                              subscriptionId,
                                              *this,
                                              alertAfterInterval);
        missedPublicationScheduler->schedule(processor, alertAfterInterval);
    } else if (qos->getExpiryDate() != joynr::SubscriptionQos::NO_EXPIRY_DATE()) {
        missedPublicationScheduler->schedule(
                new ExpiredSubscriptionRunnable(subscriptionId, *this),
                qos->getExpiryDate() - QDateTime::currentMSecsSinceEpoch());
    }
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName(subscribeToName);
    subscriptionRequest.setQos(qos);
}

void SubscriptionManager::unregisterSubscription(const QString& subscriptionId)
{
    PubSubState* subscriptionState = subscriptionStates->value(subscriptionId);
    if (subscriptionState != NULL) {
        logger->log(DEBUG, "Called unregister / unsubscribe on subscription id= " + subscriptionId);
        subscriptionState->stop();
    } else {
        logger->log(DEBUG,
                    "Called unregister on a non/no longer existent subscription, used id= " +
                            subscriptionId);
    }
    // TM shouldnt the attributeSubscription be removed right here?
    // If the runnable is responsible for deleting it, and the PublicationManager is deleted
    // too early, the runnable will never be executed and the subscription will never be deleted.
    // attributeSubscriptionDirectory.remove(subscriptionId);
}

void SubscriptionManager::touchSubscriptionState(const QString& subscriptionId)
{
    LOG_DEBUG(logger, "Touching subscription state for id=" + subscriptionId);
    if (!subscriptionStates->contains(subscriptionId)) {
        return;
    }
    PubSubState* subscriptionState = subscriptionStates->value(subscriptionId);
    subscriptionState->setTimeOfLastPublication();
}

// The subscription callback is shared by the dispatcher and subscription manager
QSharedPointer<ISubscriptionCallback> SubscriptionManager::getSubscriptionCallback(
        const QString& subscriptionId)
{
    LOG_DEBUG(logger, "Getting subscription callback for subscription id=" + subscriptionId);
    if (!subscriptionStates->contains(subscriptionId) ||
        !subscriptionDirectory.contains(subscriptionId)) {
        LOG_DEBUG(logger,
                  "Trying to acces a non existing subscription callback for id=" + subscriptionId);
    }
    QSharedPointer<ISubscriptionCallback> callback = subscriptionDirectory.lookup(subscriptionId);
    return callback;
}

void SubscriptionManager::subscriptionEnded(const QString& subscriptionId)
{
    LOG_DEBUG(logger, "Subscription " + subscriptionId + " ended.");
    LOG_DEBUG(logger, "Deleting subscription state, removing subscription callback.");
    PubSubState* state = subscriptionStates->value(subscriptionId);
    subscriptionStates->remove(subscriptionId);
    delete state;
    subscriptionDirectory.remove(subscriptionId);
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
        SubscriptionManager& subscriptionManager,
        const qint64& alertAfterInterval)
        : ObjectWithDecayTime(expiryDate),
          stoppedSemaphore(),
          expectedIntervalMSecs(expectedIntervalMSecs),
          subscriptionId(subscriptionId),
          alertAfterInterval(alertAfterInterval),
          subscriptionManager(subscriptionManager),
          state(subscriptionManager.subscriptionStates->value(subscriptionId))
{
    setAutoDelete(true);
}

void SubscriptionManager::MissedPublicationRunnable::run()
{
    assert(state != NULL);
    LOG_DEBUG(logger, "Running MissedPublicationRunnable for subscription id= " + subscriptionId);
    assert(subscriptionManager.subscriptionStates->contains(subscriptionId));

    if (!isExpired() && !state->isStopped()) {
        qint64 delay = 0;
        qint64 timeSinceLastPublication;
        timeSinceLastPublication =
                QDateTime::currentMSecsSinceEpoch() - state->getTimeOfLastPublication();
        bool publicationInTime = timeSinceLastPublication < alertAfterInterval;

        if (publicationInTime) {
            LOG_TRACE(logger, "Publication in time!");
            delay = alertAfterInterval - timeSinceLastPublication;
        } else {
            LOG_DEBUG(logger, "Publication missed!");
            QSharedPointer<ISubscriptionCallback> callback =
                    subscriptionManager.getSubscriptionCallback(subscriptionId);
            callback->publicationMissed();
            delay = alertAfterInterval - timeSinceLastExpectedPublication(timeSinceLastPublication);
        }
        LOG_DEBUG(logger,
                  "Resceduling MissedPublicationRunnable with delay: " + QString::number(delay));
        MissedPublicationRunnable* newRunnable =
                new MissedPublicationRunnable(decayTime,
                                              expectedIntervalMSecs,
                                              subscriptionId,
                                              subscriptionManager,
                                              alertAfterInterval);
        subscriptionManager.missedPublicationScheduler->schedule(newRunnable, delay);
    } else {
        LOG_DEBUG(
                logger,
                "Publication expired / interrupted. Expiring on subscription id=" + subscriptionId);
        subscriptionManager.subscriptionEnded(subscriptionId);
    }
}

qint64 SubscriptionManager::MissedPublicationRunnable::timeSinceLastExpectedPublication(
        const qint64& timeSinceLastPublication)
{
    return timeSinceLastPublication % expectedIntervalMSecs;
}

Logger* SubscriptionManager::ExpiredSubscriptionRunnable::logger =
        Logging::getInstance()->getLogger("MSG", "ExpiredSubscriptionRunnable");

SubscriptionManager::ExpiredSubscriptionRunnable::ExpiredSubscriptionRunnable(
        const QString& subscriptionId,
        SubscriptionManager& subscriptionManager)
        : subscriptionId(subscriptionId), subscriptionManager(subscriptionManager)
{
    setAutoDelete(true);
}

void SubscriptionManager::ExpiredSubscriptionRunnable::run()
{
    LOG_DEBUG(logger, "Running ExpiredSubscriptionRunnable for subscription id= " + subscriptionId);
    LOG_DEBUG(logger,
              "Publication expired / interrupted. Expiring on subscription id=" + subscriptionId);
    subscriptionManager.subscriptionEnded(subscriptionId);
}

} // namespace joynr
