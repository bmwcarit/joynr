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
#include "joynr/PrivateCopyAssign.h"

#include "joynr/joynrlogging.h"

#include "joynr/JoynrExport.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/ISubscriptionCallback.h"
#include "joynr/Directory.h"
#include "joynr/ObjectWithDecayTime.h"
#include "joynr/ThreadSafeMap.h"
#include "joynr/MessagingQos.h"

#include <QString>
#include <QSharedPointer>

namespace joynr {


class PubSubState;

/**
  * \class SubscriptionManager
  * \brief The subscription manager is used by the proxy (via the appropriate connector)
  * to manage a subscription. This includes the registration and unregistration of attribute
  * subscriptions. In order to subscribe, a SubscriptionListener is passed in from the application and
  * packaged into a callback by the connector.
  * This listener is notified (via the callback) when a subscription is missed or when a publication
  * arrives.
  */
class JOYNR_EXPORT SubscriptionManager {

public:
    ~SubscriptionManager();

    SubscriptionManager();

    SubscriptionManager(DelayedScheduler* scheduler);
    /**
     * @brief Subscribe to an attribute. Modifies the subscription request to include all
     * necessary information (side effect). Takes ownership of the ISubscriptionCallback, i.e.
     * deletes the callback when no longer required.
     *
     * @param proxyId
     * @param providerId
     * @param attributeName
     * @param attributeSubscriptionCaller
     * @param qos
     * @param subscriptionRequest
     */
    void registerAttributeSubscription(
            const QString &attributeName,
            ISubscriptionCallback * attributeSubscriptionCaller, // SubMgr gets ownership of ptr
            QSharedPointer<SubscriptionQos> qos,
            SubscriptionRequest& subscriptionRequest);

    /**
     * @brief Stop the subscription. Removes the callback and stops the notifications
     * on missed publications.
     *
     * @param subscriptionId
     */
    void unregisterAttributeSubscription(const QString& subscriptionId);

    /**
     * @brief Sets the time of last received publication (incoming attribute value) to the current system time.
     *
     * @param subscriptionId
     */
    void touchSubscriptionState(const QString& subscriptionId);

    /**
     * @brief Get a shared pointer to the subscription callback - ownership remains with the subscription manager.
     *
     * @param subscriptionId
     * @return QSharedPointer<ISubscriptionCallback>
     */
    QSharedPointer<ISubscriptionCallback> getSubscriptionCallback(const QString& subscriptionId);

private:
    DISALLOW_COPY_AND_ASSIGN(SubscriptionManager);
    void subscriptionEnded(const QString& subscriptionId);

    Directory<QString, ISubscriptionCallback> attributeSubscriptionDirectory;
    ThreadSafeMap<QString, PubSubState*>* subscriptionStates;
    DelayedScheduler* missedPublicationScheduler;
    static joynr_logging::Logger* logger;
    /**
      * \class SubscriptionManager::MissedPublicationRunnable
      * \brief
      */
    class MissedPublicationRunnable : public QRunnable, public ObjectWithDecayTime {
    public:
        MissedPublicationRunnable(const QDateTime& decayTime,
                                  const QString& subscriptionId,
                                  SubscriptionManager& subscriptionManager,
                                  const qint64& alertAfterInterval);

        /**
         * @brief Checks whether a publication arrived in time, whether it is expired or interrupted.
         *
         */
        void run();
    private:
        DISALLOW_COPY_AND_ASSIGN(MissedPublicationRunnable);
        QSemaphore stoppedSemaphore;
        QString subscriptionId;
        qint64 alertAfterInterval;
        SubscriptionManager& subscriptionManager;
        PubSubState* state;
        static joynr_logging::Logger* logger;
    };
    /**
      * \class SubscriptionManager::ExpiredSubscriptionRunnable
      * \brief
      */
    class ExpiredSubscriptionRunnable : public QRunnable {
    public:
        ExpiredSubscriptionRunnable(const QString& subscriptionId,
                                  SubscriptionManager& subscriptionManager);

        /**
         * @brief removes subscription once running.
         *
         */
        void run();
    private:
        DISALLOW_COPY_AND_ASSIGN(ExpiredSubscriptionRunnable);
        QString subscriptionId;
        SubscriptionManager& subscriptionManager;
        static joynr_logging::Logger* logger;
    };
};


} // namespace joynr
#endif // SUBSCRIPTIONMANAGER_H
