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

#include "joynr/SubscriptionPublication.h"

namespace joynr
{

SubscriptionPublication::SubscriptionPublication() : BasePublication(), subscriptionId()
{
}

SubscriptionPublication::SubscriptionPublication(BaseReply&& baseReply)
        : BasePublication(std::move(baseReply)), subscriptionId()
{
}

const std::string& SubscriptionPublication::getSubscriptionId() const
{
    return subscriptionId;
}

void SubscriptionPublication::setSubscriptionId(const std::string& subscriptionIdLocal)
{
    this->subscriptionId = subscriptionIdLocal;
}

void SubscriptionPublication::setSubscriptionId(std::string&& subscriptionIdLocal)
{
    this->subscriptionId = std::move(subscriptionIdLocal);
}

bool SubscriptionPublication::operator==(const SubscriptionPublication& other) const
{
    return subscriptionId == other.getSubscriptionId() && BasePublication::operator==(other);
}

bool SubscriptionPublication::operator!=(const SubscriptionPublication& other) const
{
    return !(*this == other);
}

// printing SubscriptionPublication with google-test and google-mock
void PrintTo(const SubscriptionPublication& subscriptionPublication, ::std::ostream* os)
{
    *os << "SubscriptionPublication{";
    *os << "subscriptionId:" << subscriptionPublication.subscriptionId;
    *os << ", ";
    *os << "error:" << subscriptionPublication.getError()
            ? "null"
            : subscriptionPublication.getError()->getMessage();
    *os << ", SKIPPED printing BaseReply";
    *os << "}";
}

} // namespace joynr
