/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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
#include "libjoynr/subscription/SubscriptionRequestInformation.h"

#include <cassert>

namespace joynr {

using namespace joynr_logging;
Logger* SubscriptionRequestInformation::logger = Logging::getInstance()->getLogger("MSG", "SubscriptionRequestInformation");

SubscriptionRequestInformation::SubscriptionRequestInformation():
    SubscriptionRequest(),
    proxyId(),
    providerId()
{
}

SubscriptionRequestInformation::SubscriptionRequestInformation(
        const QString& proxyParticipantId,
        const QString& providerParticipantId,
        const SubscriptionRequest& subscriptionRequest
) :
    SubscriptionRequest(subscriptionRequest),
    proxyId(proxyParticipantId),
    providerId(providerParticipantId)
{
}

SubscriptionRequestInformation::SubscriptionRequestInformation(const SubscriptionRequestInformation& subscriptionRequestInformation) :
    SubscriptionRequest(subscriptionRequestInformation),
    proxyId(subscriptionRequestInformation.getProxyId()),
    providerId(subscriptionRequestInformation.getProviderId())
{

}

QString SubscriptionRequestInformation::getProxyId() const
{
    return proxyId;
}

QString SubscriptionRequestInformation::getProviderId() const
{
    return providerId;
}

SubscriptionRequestInformation& SubscriptionRequestInformation::operator=(const SubscriptionRequestInformation& subscriptionRequestInformation) {
    SubscriptionRequest::operator=(subscriptionRequestInformation);
    proxyId = subscriptionRequestInformation.getProxyId();
    providerId = subscriptionRequestInformation.getProviderId();
    return *this;
}

bool SubscriptionRequestInformation::operator==(const SubscriptionRequestInformation& subscriptionRequestInformation) const {
    return
            SubscriptionRequest::operator==(subscriptionRequestInformation)
            && proxyId == subscriptionRequestInformation.getProxyId()
            && providerId == subscriptionRequestInformation.getProviderId();
}

void SubscriptionRequestInformation::setProxyId(const QString &id)
{
    this->proxyId = id;
}

void SubscriptionRequestInformation::setProviderId(const QString &id)
{
    this->providerId = id;
}

QString SubscriptionRequestInformation::toQString() const
{
    return JsonSerializer::serialize(*this);
}



} // namespace joynr
