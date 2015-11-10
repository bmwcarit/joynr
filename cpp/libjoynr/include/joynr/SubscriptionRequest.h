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
#include <QObject>

#include <memory>
#include <string>

#include "joynr/Variant.h"

namespace joynr
{

/** @class SubscriptionRequest
  * @brief SubscriptionRequest stores the information that is necessary to store a
 * subscription-Request on
  * subscriber side, while Aribtration is handled.
  */

class JOYNR_EXPORT SubscriptionRequest : public QObject
{
    Q_OBJECT

    Q_PROPERTY(std::string subscriptionId READ getSubscriptionId WRITE setSubscriptionId)
    Q_PROPERTY(std::string subscribedToName READ getSubscribeToName WRITE setSubscribeToName)
    Q_PROPERTY(Variant qos READ getQos WRITE setQos)

public:
    SubscriptionRequest();
    SubscriptionRequest(const SubscriptionRequest& subscriptionRequest);
    SubscriptionRequest& operator=(const SubscriptionRequest& subscriptionRequest);
    virtual ~SubscriptionRequest()
    {
    }
    bool operator==(const SubscriptionRequest& subscriptionRequest) const;

    std::string getSubscriptionId() const;
    void setSubscriptionId(const std::string& id);

    std::string getSubscribeToName() const;
    void setSubscribeToName(const std::string& subscribedToName);

    void setQos(const Variant& qos);
    const Variant& getQos() const;

    const SubscriptionQos* getSubscriptionQosPtr();

    QString toQString() const;
    std::string toString() const;

private:
    /*
      SubscriptionRequest is used to store a subscription while Arbitration is still being done. To
      allow SubscriptionManager
      to notify about missedPublications for a subscription while offline, the SubscriptionId has to
      be determined when registering
      the subscription, and thus must be stored while waiting for arbitrations.
      */
    std::string subscriptionId;
    std::string subscribedToName;
    Variant qos;

    static joynr_logging::Logger* logger;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::SubscriptionRequest)
#endif // SUBSCRIPTIONREQUEST_H
