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

#include "joynr/SubscriptionReply.h"

namespace joynr
{

const SubscriptionReply SubscriptionReply::NULL_RESPONSE = SubscriptionReply();

SubscriptionReply::SubscriptionReply() : subscriptionId()
{
}
SubscriptionReply::SubscriptionReply(const SubscriptionReply& other)
        : QObject(), subscriptionId(other.getSubscriptionId())
{
}

SubscriptionReply& SubscriptionReply::operator=(const SubscriptionReply& other)
{
    this->subscriptionId = other.getSubscriptionId();
    return *this;
}

QString SubscriptionReply::getSubscriptionId() const
{
    return subscriptionId;
}

void SubscriptionReply::setSubscriptionId(QString subscriptionId)
{
    this->subscriptionId = subscriptionId;
}

bool SubscriptionReply::operator==(const SubscriptionReply& other) const
{
    return subscriptionId == other.getSubscriptionId();
}

bool SubscriptionReply::operator!=(const SubscriptionReply& other) const
{
    return !(*this == other);
}

} // namespace joynr
