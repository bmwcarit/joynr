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

#include "joynr/SubscriptionPublication.h"

namespace joynr
{

static bool isSubscriptionPublicationRegistered =
        Variant::registerType<SubscriptionPublication>("joynr.SubscriptionPublication");

const SubscriptionPublication SubscriptionPublication::NULL_RESPONSE = SubscriptionPublication();

SubscriptionPublication::SubscriptionPublication() : subscriptionId(), response(), error(NULL)
{
}
SubscriptionPublication::SubscriptionPublication(const SubscriptionPublication& other)
        : QObject(),
          subscriptionId(other.getSubscriptionId()),
          response(other.response),
          error(other.error)
{
}

SubscriptionPublication& SubscriptionPublication::operator=(const SubscriptionPublication& other)
{
    this->subscriptionId = other.getSubscriptionId();
    this->response = other.response;
    this->error = other.error;
    return *this;
}

std::string SubscriptionPublication::getSubscriptionId() const
{
    return subscriptionId;
}

void SubscriptionPublication::setSubscriptionId(const std::string& subscriptionId)
{
    this->subscriptionId = subscriptionId;
}

std::vector<Variant> SubscriptionPublication::getResponse() const
{
    return response;
}

void SubscriptionPublication::setResponse(const std::vector<Variant>& response)
{
    this->response = response;
}

std::shared_ptr<exceptions::JoynrRuntimeException> SubscriptionPublication::getError() const
{
    return this->error;
}

void SubscriptionPublication::setError(std::shared_ptr<exceptions::JoynrRuntimeException> error)
{
    this->error = error;
}

bool SubscriptionPublication::operator==(const SubscriptionPublication& other) const
{
    return subscriptionId == other.getSubscriptionId() && error == other.error;
}

bool SubscriptionPublication::operator!=(const SubscriptionPublication& other) const
{
    return !(*this == other);
}

} // namespace joynr
