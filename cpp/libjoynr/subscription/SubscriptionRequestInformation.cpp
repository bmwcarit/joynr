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
#include "libjoynr/subscription/SubscriptionRequestInformation.h"

namespace joynr
{

static bool isSubscriptionRequestInformationRegistered =
        Variant::registerType<SubscriptionRequestInformation>(
                "joynr.SubscriptionRequestInformation");

using namespace joynr_logging;
Logger* SubscriptionRequestInformation::logger =
        Logging::getInstance()->getLogger("MSG", "SubscriptionRequestInformation");

SubscriptionRequestInformation::SubscriptionRequestInformation()
{
}

SubscriptionRequestInformation::SubscriptionRequestInformation(
        const std::string& proxyParticipantId,
        const std::string& providerParticipantId,
        const SubscriptionRequest& subscriptionRequest)
        : SubscriptionRequest(subscriptionRequest),
          SubscriptionInformation(proxyParticipantId, providerParticipantId)
{
}

SubscriptionRequestInformation::SubscriptionRequestInformation(
        const SubscriptionRequestInformation& subscriptionRequestInformation)
        : SubscriptionRequest(subscriptionRequestInformation),
          SubscriptionInformation(subscriptionRequestInformation.getProxyId(),
                                  subscriptionRequestInformation.getProviderId())
{
}

bool SubscriptionRequestInformation::operator==(
        const SubscriptionRequestInformation& subscriptionRequestInformation) const
{
    return SubscriptionRequest::operator==(subscriptionRequestInformation) &&
           SubscriptionInformation::operator==(subscriptionRequestInformation);
}

std::string SubscriptionRequestInformation::toString() const
{
    return JsonSerializer::serialize<SubscriptionRequestInformation>(*this);
}

} // namespace joynr
