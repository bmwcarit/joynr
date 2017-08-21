/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#include "joynr/BroadcastSubscriptionRequestInformation.h"

namespace joynr
{

BroadcastSubscriptionRequestInformation::BroadcastSubscriptionRequestInformation(
        const std::string& proxyParticipantId,
        const std::string& providerParticipantId,
        const BroadcastSubscriptionRequest& subscriptionRequest)
        : BroadcastSubscriptionRequest(subscriptionRequest),
          SubscriptionInformation(proxyParticipantId, providerParticipantId)
{
}

bool BroadcastSubscriptionRequestInformation::operator==(
        const BroadcastSubscriptionRequestInformation& subscriptionRequestInformation) const
{
    return BroadcastSubscriptionRequest::operator==(subscriptionRequestInformation) &&
           SubscriptionInformation::operator==(subscriptionRequestInformation);
}

std::string BroadcastSubscriptionRequestInformation::toString() const
{
    return joynr::serializer::serializeToJson(*this);
}

} // namespace joynr
