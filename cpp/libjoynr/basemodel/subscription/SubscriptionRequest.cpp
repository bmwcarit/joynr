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
#include "joynr/SubscriptionRequest.h"

#include "joynr/SubscriptionQos.h"
#include "joynr/Util.h"

namespace joynr
{

SubscriptionRequest::SubscriptionRequest() : subscriptionId(), subscribedToName(), qos()
{
    subscriptionId = util::createUuid();
}

const std::string& SubscriptionRequest::getSubscriptionId() const
{
    return subscriptionId;
}

const std::string& SubscriptionRequest::getSubscribeToName() const
{
    return subscribedToName;
}

bool SubscriptionRequest::operator==(const SubscriptionRequest& subscriptionRequest) const
{
    bool equal = *qos == *(subscriptionRequest.qos);
    return subscriptionId == subscriptionRequest.getSubscriptionId() &&
           subscribedToName == subscriptionRequest.getSubscribeToName() && equal;
}

void SubscriptionRequest::setSubscriptionId(const std::string& idLocal)
{
    this->subscriptionId = idLocal;
}

void SubscriptionRequest::setSubscriptionId(std::string&& idLocal)
{
    this->subscriptionId = std::move(idLocal);
}

void SubscriptionRequest::setSubscribeToName(const std::string& subscribedToNameLocal)
{
    this->subscribedToName = subscribedToNameLocal;
}

void SubscriptionRequest::setSubscribeToName(std::string&& subscribedToNameLocal)
{
    this->subscribedToName = std::move(subscribedToNameLocal);
}

std::string SubscriptionRequest::toString() const
{
    return joynr::serializer::serializeToJson(*this);
}

std::shared_ptr<SubscriptionQos> SubscriptionRequest::getQos() const
{
    return qos;
}

void SubscriptionRequest::setQos(std::shared_ptr<SubscriptionQos> qosLocal)
{
    this->qos = std::move(qosLocal);
}

} // namespace joynr
