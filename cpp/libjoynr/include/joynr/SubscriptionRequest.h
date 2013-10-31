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
#ifndef SUBSCRIPTIONREQUEST_H
#define SUBSCRIPTIONREQUEST_H


#include "joynr/JoynrExport.h"
#include "joynr/joynrlogging.h"
#include "joynr/MessagingQos.h"
#include "joynr/SubscriptionQos.h"

#include <QString>
#include <QSharedPointer>

namespace joynr {

/** \class SubscriptionRequest
  * \brief SubscriptionRequest stores the information that is necessary to store a subscription-Request on
  * subscriber side, while Aribtration is handled.
  */

class JOYNR_EXPORT SubscriptionRequest : public QObject{
    Q_OBJECT

    Q_PROPERTY(QString subscriptionId READ getSubscriptionId WRITE setSubscriptionId)
    Q_PROPERTY(QString attributeName READ getAttributeName WRITE setAttributeName)
    Q_PROPERTY(QVariant qos READ getQosData WRITE setQosData)

public:
    SubscriptionRequest();
    SubscriptionRequest(const SubscriptionRequest& subscriptionRequest);
    SubscriptionRequest& operator=(const SubscriptionRequest& subscriptionRequest);
    virtual ~SubscriptionRequest(){}
    bool operator==(const SubscriptionRequest& subscriptionRequest) const;

    QString getSubscriptionId() const;
    void setSubscriptionId(const QString& id);

    QString getAttributeName() const;
    void setAttributeName(const QString& attributeName);

    void setQos(QSharedPointer<SubscriptionQos> qos);
    QSharedPointer<SubscriptionQos> getQos() const;

    QString toQString() const;

protected:
    void setQosData(QVariant qos);
    QVariant getQosData() const;

private:
    /*
      SubscriptionRequest is used to store a subscription while Arbitration is still being done. To allow SubscriptionManager
      to notify about missedPublications for a subscription while offline, the SubscriptionId has to be determined when registering
      the subscription, and thus must be stored while waiting for arbitrations.
      */
    QString subscriptionId;
    QString attributeName;
    QSharedPointer<SubscriptionQos> qos;

    static joynr_logging::Logger* logger;
};


} // namespace joynr

Q_DECLARE_METATYPE(joynr::SubscriptionRequest)
Q_DECLARE_METATYPE(QSharedPointer<joynr::SubscriptionRequest>)
#endif // SUBSCRIPTIONREQUEST_H
