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
#include "joynr/QtOnChangeSubscriptionQos.h"
#include "joynr/QtOnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/QtPeriodicSubscriptionQos.h"

#include <cassert>

namespace joynr
{

using namespace joynr_logging;
Logger* SubscriptionRequest::logger =
        Logging::getInstance()->getLogger("MSG", "SubscriptionRequest");

SubscriptionRequest::SubscriptionRequest()
        : QObject(),
          subscriptionId(),
          subscribedToName(),
          qos(std::shared_ptr<QtSubscriptionQos>(new QtOnChangeSubscriptionQos()))
{
    subscriptionId = Util::createUuid();
    qRegisterMetaType<QtSubscriptionQos>("QtSubscriptionQos");

    qRegisterMetaType<QtOnChangeSubscriptionQos>("QtOnChangeSubscriptionQos");

    qRegisterMetaType<QtOnChangeWithKeepAliveSubscriptionQos>(
            "QtOnChangeWithKeepAliveSubscriptionQos");

    qRegisterMetaType<QtPeriodicSubscriptionQos>("QtPeriodicSubscriptionQos");
}

SubscriptionRequest::SubscriptionRequest(const SubscriptionRequest& subscriptionRequest)
        : QObject(),
          subscriptionId(subscriptionRequest.getSubscriptionId()),
          subscribedToName(subscriptionRequest.getSubscribeToName()),
          qos(subscriptionRequest.getQos())
{
}

QString SubscriptionRequest::getSubscriptionId() const
{
    return subscriptionId;
}

QString SubscriptionRequest::getSubscribeToName() const
{
    return subscribedToName;
}

std::shared_ptr<QtSubscriptionQos> SubscriptionRequest::getQos() const
{
    return qos;
}

QVariant SubscriptionRequest::getQosData() const
{
    int typeId = QMetaType::type(qos->metaObject()->className());
    return QVariant(typeId, qos.get());
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
    bool equal = getQos()->equals(*subscriptionRequest.getQos());
    return subscriptionId == subscriptionRequest.getSubscriptionId() &&
           subscribedToName == subscriptionRequest.getSubscribeToName() && equal;
}

void SubscriptionRequest::setSubscriptionId(const QString& id)
{
    this->subscriptionId = id;
}

void SubscriptionRequest::setSubscribeToName(const QString& attributeName)
{
    this->subscribedToName = attributeName;
}

void SubscriptionRequest::setQos(std::shared_ptr<QtSubscriptionQos> qos)
{
    this->qos = qos;
}

void SubscriptionRequest::setQosData(QVariant qos)
{
    // copy the object
    QMetaType type(qos.userType());
    auto newQos = static_cast<QtSubscriptionQos*>(type.create(qos.constData()));
    this->qos = std::shared_ptr<QtSubscriptionQos>(newQos);
}

QString SubscriptionRequest::toQString() const
{
    return JsonSerializer::serializeQObject(*this);
}

} // namespace joynr
