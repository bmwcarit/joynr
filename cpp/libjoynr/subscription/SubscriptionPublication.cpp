/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include "joynr/SubscriptionPublication.h"

namespace joynr
{

static bool isSubscriptionPublicationRegistered =
        Variant::registerType<SubscriptionPublication>("joynr.SubscriptionPublication");

const SubscriptionPublication SubscriptionPublication::NULL_RESPONSE = SubscriptionPublication();

SubscriptionPublication::SubscriptionPublication() : subscriptionId()
{
}

SubscriptionPublication::SubscriptionPublication(Reply&& reply)
        : response(std::move(reply.response)), error(std::move(reply.error))
{
}

std::string SubscriptionPublication::getSubscriptionId() const
{
    return subscriptionId;
}

void SubscriptionPublication::setSubscriptionId(const std::string& subscriptionId)
{
    this->subscriptionId = subscriptionId;
}

bool SubscriptionPublication::operator==(const SubscriptionPublication& other) const
{
    return subscriptionId == other.getSubscriptionId() && BaseReply::operator==(other);
    ;
}

bool SubscriptionPublication::operator!=(const SubscriptionPublication& other) const
{
    return !(*this == other);
}

} // namespace joynr
