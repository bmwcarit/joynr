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
#include "joynr/SubscriptionRequest.h"

#include "joynr/Util.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/PeriodicSubscriptionQos.h"

namespace joynr
{

static const bool isSubscriptionRequestRegistered =
        Variant::registerType<SubscriptionRequest>("joynr.SubscriptionRequest");

SubscriptionRequest::SubscriptionRequest()
        : subscriptionId(),
          subscribedToName(),
          qosVariant(Variant::make<OnChangeSubscriptionQos>(OnChangeSubscriptionQos()))
{
    subscriptionId = util::createUuid();
}

std::string SubscriptionRequest::getSubscriptionId() const
{
    return subscriptionId;
}

std::string SubscriptionRequest::getSubscribeToName() const
{
    return subscribedToName;
}

const Variant& SubscriptionRequest::getQosVariant() const
{
    return qosVariant;
}

const SubscriptionQos* SubscriptionRequest::getSubscriptionQosPtr()
{
    if (qosVariant.is<OnChangeWithKeepAliveSubscriptionQos>()) {
        return &qosVariant.get<OnChangeWithKeepAliveSubscriptionQos>();
    }
    if (qosVariant.is<PeriodicSubscriptionQos>()) {
        return &qosVariant.get<PeriodicSubscriptionQos>();
    }
    if (qosVariant.is<OnChangeSubscriptionQos>()) {
        return &qosVariant.get<OnChangeSubscriptionQos>();
    }
    if (qosVariant.is<SubscriptionQos>()) {
        return &qosVariant.get<SubscriptionQos>();
    }

    return nullptr;
}

bool SubscriptionRequest::operator==(const SubscriptionRequest& subscriptionRequest) const
{
    bool equal = getQosVariant() == subscriptionRequest.getQosVariant();
    return subscriptionId == subscriptionRequest.getSubscriptionId() &&
           subscribedToName == subscriptionRequest.getSubscribeToName() && equal;
}

void SubscriptionRequest::setSubscriptionId(const std::string& id)
{
    this->subscriptionId = id;
}

void SubscriptionRequest::setSubscribeToName(const std::string& attributeName)
{
    this->subscribedToName = attributeName;
}

void SubscriptionRequest::setQosVariant(const Variant& qos)
{
    this->qosVariant = qos;
}

std::string SubscriptionRequest::toString() const
{
    return joynr::serializer::serializeToJson(*this);
}

std::shared_ptr<SubscriptionQos> SubscriptionRequest::getQos() const
{
    return qos;
}

void SubscriptionRequest::setQos(std::shared_ptr<SubscriptionQos> qos)
{
    this->qos = std::move(qos);
}

} // namespace joynr
