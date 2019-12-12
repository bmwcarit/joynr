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
#include "joynr/SubscriptionRequestInformation.h"

namespace joynr
{

SubscriptionRequestInformation::SubscriptionRequestInformation(
        const std::string& proxyParticipantId,
        const std::string& providerParticipantId,
        const CallContext& callContextLocal,
        const SubscriptionRequest& subscriptionRequestLocal)
        : SubscriptionRequest(subscriptionRequestLocal),
          SubscriptionInformation(proxyParticipantId, providerParticipantId),
          callContext(callContextLocal)
{
}

const CallContext& SubscriptionRequestInformation::getCallContext() const
{
    return callContext;
}

void SubscriptionRequestInformation::setCallContext(const CallContext& callContextLocal)
{
    this->callContext = callContextLocal;
}

bool SubscriptionRequestInformation::operator==(
        const SubscriptionRequestInformation& subscriptionRequestInformation) const
{
    return SubscriptionRequest::operator==(subscriptionRequestInformation) &&
           SubscriptionInformation::operator==(subscriptionRequestInformation) &&
           callContext == subscriptionRequestInformation.getCallContext();
}

std::string SubscriptionRequestInformation::toString() const
{
    return joynr::serializer::serializeToJson(*this);
}

} // namespace joynr
