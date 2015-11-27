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
#include "joynr/JsonSerializer.h"
#include "joynr/Util.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/PeriodicSubscriptionQos.h"

#include <cassert>

namespace joynr
{

using namespace joynr_logging;
Logger* SubscriptionRequest::logger =
        Logging::getInstance()->getLogger("MSG", "SubscriptionRequest");

static const bool isSubscriptionRequestRegistered =
        Variant::registerType<SubscriptionRequest>("joynr.SubscriptionRequest");

SubscriptionRequest::SubscriptionRequest()
        : QObject(),
          subscriptionId(),
          subscribedToName(),
          qos(Variant::make<OnChangeSubscriptionQos>(OnChangeSubscriptionQos()))
{
    subscriptionId = Util::createUuid().toStdString();
}

SubscriptionRequest::SubscriptionRequest(const SubscriptionRequest& subscriptionRequest)
        : QObject(),
          subscriptionId(subscriptionRequest.getSubscriptionId()),
          subscribedToName(subscriptionRequest.getSubscribeToName()),
          qos(subscriptionRequest.getQos())
{
}

std::string SubscriptionRequest::getSubscriptionId() const
{
    return subscriptionId;
}

std::string SubscriptionRequest::getSubscribeToName() const
{
    return subscribedToName;
}

const Variant& SubscriptionRequest::getQos() const
{
    return qos;
}

const SubscriptionQos* SubscriptionRequest::getSubscriptionQosPtr()
{
    if (qos.is<OnChangeWithKeepAliveSubscriptionQos>()) {
        return &qos.get<OnChangeWithKeepAliveSubscriptionQos>();
    }
    if (qos.is<PeriodicSubscriptionQos>()) {
        return &qos.get<PeriodicSubscriptionQos>();
    }
    if (qos.is<OnChangeSubscriptionQos>()) {
        return &qos.get<OnChangeSubscriptionQos>();
    }
    if (qos.is<SubscriptionQos>()) {
        return &qos.get<SubscriptionQos>();
    }
}

SubscriptionRequest& SubscriptionRequest::operator=(const SubscriptionRequest& subscriptionRequest)
{
    subscriptionId = subscriptionRequest.getSubscriptionId();
    subscribedToName = subscriptionRequest.getSubscribeToName();
    qos = subscriptionRequest.getQos();
    return *this;
}

bool SubscriptionRequest::operator==(const SubscriptionRequest& subscriptionRequest) const
{
    bool equal = getQos() == subscriptionRequest.getQos();
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

void SubscriptionRequest::setQos(const Variant& qos)
{
    this->qos = qos;
}

QString SubscriptionRequest::toQString() const
{
    return JsonSerializer::serializeQObject(*this);
}

std::string SubscriptionRequest::toString() const
{
    return JsonSerializer::serialize(*this);
}

} // namespace joynr
