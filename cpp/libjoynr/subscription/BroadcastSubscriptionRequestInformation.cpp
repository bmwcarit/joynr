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
#include "joynr/JsonSerializer.h"
#include "joynr/Util.h"
#include "libjoynr/subscription/BroadcastSubscriptionRequestInformation.h"

#include <cassert>

namespace joynr {

using namespace joynr_logging;
Logger* BroadcastSubscriptionRequestInformation::logger = Logging::getInstance()->getLogger("MSG", "BroadcastSubscriptionRequestInformation");

BroadcastSubscriptionRequestInformation::BroadcastSubscriptionRequestInformation():
    BroadcastSubscriptionRequest(),
    proxyId(),
    providerId()
{
}

BroadcastSubscriptionRequestInformation::BroadcastSubscriptionRequestInformation(
        const QString& proxyParticipantId,
        const QString& providerParticipantId,
        const BroadcastSubscriptionRequest& subscriptionRequest
) :
    BroadcastSubscriptionRequest(subscriptionRequest),
    proxyId(proxyParticipantId),
    providerId(providerParticipantId)
{
}

BroadcastSubscriptionRequestInformation::BroadcastSubscriptionRequestInformation(const BroadcastSubscriptionRequestInformation& subscriptionRequestInformation) :
    BroadcastSubscriptionRequest(subscriptionRequestInformation),
    proxyId(subscriptionRequestInformation.getProxyId()),
    providerId(subscriptionRequestInformation.getProviderId())
{
}

BroadcastSubscriptionRequestInformation& BroadcastSubscriptionRequestInformation::operator=(const BroadcastSubscriptionRequestInformation& subscriptionRequestInformation) {
    BroadcastSubscriptionRequest::operator=(subscriptionRequestInformation);
    proxyId = subscriptionRequestInformation.getProxyId();
    providerId = subscriptionRequestInformation.getProviderId();
    return *this;
}

bool BroadcastSubscriptionRequestInformation::operator==(const BroadcastSubscriptionRequestInformation& subscriptionRequestInformation) const {
    return
            BroadcastSubscriptionRequest::operator==(subscriptionRequestInformation)
            && proxyId == subscriptionRequestInformation.getProxyId()
            && providerId == subscriptionRequestInformation.getProviderId();
}

QString BroadcastSubscriptionRequestInformation::getProxyId() const
{
    return proxyId;
}

QString BroadcastSubscriptionRequestInformation::getProviderId() const
{
    return providerId;
}

void BroadcastSubscriptionRequestInformation::setProxyId(const QString &id)
{
    this->proxyId = id;
}

void BroadcastSubscriptionRequestInformation::setProviderId(const QString &id)
{
    this->providerId = id;
}


QString BroadcastSubscriptionRequestInformation::toQString() const {
    return JsonSerializer::serialize(*this);
}

} // namespace joynr
