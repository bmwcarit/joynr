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
#ifndef BROADCASTSUBSCRIPTIONREQUEST_H
#define BROADCASTSUBSCRIPTIONREQUEST_H

#include "joynr/SubscriptionRequest.h"
#include "joynr/QtBroadcastFilterParameters.h"
#include "joynr/OnChangeSubscriptionQos.h"

#include <QString>
#include <memory>

#include "joynr/Variant.h"

namespace joynr
{

/** @class BroadcastSubscriptionRequest
  * @brief SubscriptionRequest stores the information that is necessary to store a broadcast
  * subscription-Request on subscriber side, while Aribtration is handled.
  */

class JOYNR_EXPORT BroadcastSubscriptionRequest : public SubscriptionRequest
{
    Q_OBJECT

    Q_PROPERTY(BroadcastFilterParameters filterParameters READ getFilterParameters WRITE
                       setFilterParameters)

public:
    BroadcastSubscriptionRequest();
    BroadcastSubscriptionRequest(const BroadcastSubscriptionRequest& subscriptionRequest);
    BroadcastSubscriptionRequest& operator=(
            const BroadcastSubscriptionRequest& subscriptionRequest);
    bool operator==(const BroadcastSubscriptionRequest& subscriptionRequest) const;

    QString toQString() const;

    // Make sure that broadcast subscriptions are only used with on change qos.
    // Method from base class is hidden. See below in private section.
    void setQos(const OnChangeSubscriptionQos& qos);

    BroadcastFilterParameters getFilterParameters() const;
    void setFilterParameters(const BroadcastFilterParameters& filterParameters);

private:
    // Hide method for setting all kinds of QOS derived from base class
    void setQos(const Variant& qos);

    BroadcastFilterParameters filterParameters;

    static joynr_logging::Logger* logger;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::BroadcastFilterParameters)
Q_DECLARE_METATYPE(joynr::BroadcastSubscriptionRequest)
#endif // BROADCASTSUBSCRIPTIONREQUEST_H
