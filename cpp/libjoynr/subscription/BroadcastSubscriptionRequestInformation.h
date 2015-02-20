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
#ifndef BROADCASTSUBSCRIPTIONREQUESTINFORMATION_H
#define BROADCASTSUBSCRIPTIONREQUESTINFORMATION_H

#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/SubscriptionInformation.h"

#include <QString>
#include <QSharedPointer>

namespace joynr
{

class JOYNR_EXPORT BroadcastSubscriptionRequestInformation : public BroadcastSubscriptionRequest,
                                                             public SubscriptionInformation
{
    Q_OBJECT

    Q_PROPERTY(QString proxyId READ getProxyId WRITE setProxyId)
    Q_PROPERTY(QString providerId READ getProviderId WRITE setProviderId)

public:
    BroadcastSubscriptionRequestInformation();
    BroadcastSubscriptionRequestInformation(
            const QString& proxyParticipantId,
            const QString& providerParticipantId,
            const BroadcastSubscriptionRequest& subscriptionRequest);

    BroadcastSubscriptionRequestInformation(
            const BroadcastSubscriptionRequestInformation& subscriptionRequestInformation);
    virtual ~BroadcastSubscriptionRequestInformation()
    {
    }

    BroadcastSubscriptionRequestInformation& operator=(
            const BroadcastSubscriptionRequestInformation& subscriptionRequestInformation);
    bool operator==(
            const BroadcastSubscriptionRequestInformation& subscriptionRequestInformation) const;

    QString toQString() const;

private:
    static joynr_logging::Logger* logger;
};

} // namespace joynr

Q_DECLARE_METATYPE(joynr::BroadcastSubscriptionRequestInformation)
Q_DECLARE_METATYPE(QSharedPointer<joynr::BroadcastSubscriptionRequestInformation>)

#endif // BROADCASTSUBSCRIPTIONREQUESTINFORMATION_H
