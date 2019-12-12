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

#include "joynr/SubscriptionStop.h"

namespace joynr
{

SubscriptionStop::SubscriptionStop() : subscriptionId()
{
}

const std::string& SubscriptionStop::getSubscriptionId() const
{
    return subscriptionId;
}

void SubscriptionStop::setSubscriptionId(const std::string& subscriptionIdLocal)
{
    this->subscriptionId = subscriptionIdLocal;
}

void SubscriptionStop::setSubscriptionId(std::string&& subscriptionIdLocal)
{
    this->subscriptionId = std::move(subscriptionIdLocal);
}

bool SubscriptionStop::operator==(const SubscriptionStop& other) const
{
    return subscriptionId == other.getSubscriptionId();
}

bool SubscriptionStop::operator!=(const SubscriptionStop& other) const
{
    return !(*this == other);
}

} // namespace joynr
