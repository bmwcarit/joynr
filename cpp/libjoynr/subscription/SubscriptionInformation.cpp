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
#include "joynr/SubscriptionInformation.h"
#include "joynr/JsonSerializer.h"

namespace joynr {

using namespace joynr_logging;
Logger* SubscriptionInformation::logger = Logging::getInstance()->getLogger("MSG", "SubscriptionInformation");

SubscriptionInformation::SubscriptionInformation():
    proxyId(),
    providerId()
{
}

SubscriptionInformation::SubscriptionInformation(
        const QString& proxyParticipantId,
        const QString& providerParticipantId
) :
    proxyId(proxyParticipantId),
    providerId(providerParticipantId)
{
}

SubscriptionInformation::SubscriptionInformation(const SubscriptionInformation& subscriptionInformation) :
    proxyId(subscriptionInformation.getProxyId()),
    providerId(subscriptionInformation.getProviderId())
{
}

SubscriptionInformation& SubscriptionInformation::operator=(const SubscriptionInformation& subscriptionInformation) {
    proxyId = subscriptionInformation.getProxyId();
    providerId = subscriptionInformation.getProviderId();
    return *this;
}

bool SubscriptionInformation::operator==(const SubscriptionInformation& subscriptionInformation) const {
    return proxyId == subscriptionInformation.getProxyId()
            && providerId == subscriptionInformation.getProviderId();
}

QString SubscriptionInformation::getProxyId() const
{
    return proxyId;
}

QString SubscriptionInformation::getProviderId() const
{
    return providerId;
}

void SubscriptionInformation::setProxyId(const QString &id)
{
    this->proxyId = id;
}

void SubscriptionInformation::setProviderId(const QString &id)
{
    this->providerId = id;
}


} // namespace joynr
