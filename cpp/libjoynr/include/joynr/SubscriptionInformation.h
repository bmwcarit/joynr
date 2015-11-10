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
#ifndef SUBSCRIPTIONINFORMATION_H
#define SUBSCRIPTIONINFORMATION_H

#include "joynr/JoynrExport.h"
#include "joynr/joynrlogging.h"
#include "joynr/SubscriptionRequest.h"

#include <QString>

namespace joynr
{

/** @class SubscriptionInformation
  * @brief SubscriptionInformation stores information about the context in which a concrete
  * subscription request is used.
  */

class JOYNR_EXPORT SubscriptionInformation
{

public:
    SubscriptionInformation();
    SubscriptionInformation(const QString& proxyParticipantId,
                            const QString& providerParticipantId);
    SubscriptionInformation(const SubscriptionInformation& subscriptionInformation);
    SubscriptionInformation& operator=(const SubscriptionInformation& subscriptionInformation);
    virtual ~SubscriptionInformation()
    {
    }
    bool operator==(const SubscriptionInformation& subscriptionInformation) const;

    QString getProxyId() const;
    void setProxyId(const QString& id);

    QString getProviderId() const;
    void setProviderId(const QString& id);

private:
    QString proxyId;
    QString providerId;

    static joynr_logging::Logger* logger;
};

} // namespace joynr

#endif // SUBSCRIPTIONINFORMATION_H
