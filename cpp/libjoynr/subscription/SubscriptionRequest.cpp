/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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

namespace joynr {

using namespace joynr_logging;
Logger* SubscriptionRequest::logger = Logging::getInstance()->getLogger("MSG", "SubscriptionRequest");

SubscriptionRequest::SubscriptionRequest():
    QObject(),
    subscriptionId(),
    attributeName(),
    qos(QSharedPointer<SubscriptionQos>(new OnChangeSubscriptionQos()))
{
    subscriptionId = Util::createUuid();
    qRegisterMetaType<SubscriptionQos>("SubscriptionQos");
    qRegisterMetaType<QSharedPointer<SubscriptionQos>>();

    qRegisterMetaType<OnChangeSubscriptionQos>("OnChangeSubscriptionQos");
    qRegisterMetaType<QSharedPointer<OnChangeSubscriptionQos>>();

    qRegisterMetaType<OnChangeWithKeepAliveSubscriptionQos>("OnChangeWithKeepAliveSubscriptionQos");
    qRegisterMetaType<QSharedPointer<OnChangeWithKeepAliveSubscriptionQos>>();

    qRegisterMetaType<PeriodicSubscriptionQos>("PeriodicSubscriptionQos");
    qRegisterMetaType<QSharedPointer<PeriodicSubscriptionQos>>();
}

SubscriptionRequest::SubscriptionRequest(const SubscriptionRequest& subscriptionRequest) :
    QObject(),
    subscriptionId(subscriptionRequest.getSubscriptionId()),
    attributeName(subscriptionRequest.getAttributeName()),
    qos(subscriptionRequest.getQos())
{

}

QString SubscriptionRequest::getSubscriptionId() const {
    return subscriptionId;
}

QString SubscriptionRequest::getAttributeName() const {
    return attributeName;
}

QSharedPointer<SubscriptionQos> SubscriptionRequest::getQos() const {
    return qos;
}

QVariant SubscriptionRequest::getQosData() const {
    int typeId = QMetaType::type(qos->metaObject()->className());
    return QVariant(typeId, qos.data());
}

SubscriptionRequest& SubscriptionRequest::operator=(const SubscriptionRequest& subscriptionRequest) {
    subscriptionId = subscriptionRequest.getSubscriptionId();
    attributeName = subscriptionRequest.getAttributeName();
    qos = subscriptionRequest.getQos();
    return *this;
}

bool SubscriptionRequest::operator==(const SubscriptionRequest& subscriptionRequest) const {
    bool equal = getQos()->equals(*subscriptionRequest.getQos());
    return
            subscriptionId == subscriptionRequest.getSubscriptionId()
            && attributeName == subscriptionRequest.getAttributeName()
            && equal;
}

void SubscriptionRequest::setSubscriptionId(const QString &id) {
    this->subscriptionId = id;
}

void SubscriptionRequest::setAttributeName(const QString &attributeName) {
    this->attributeName = attributeName;
}

void SubscriptionRequest::setQos(QSharedPointer<SubscriptionQos> qos) {
    this->qos = qos;
}

void SubscriptionRequest::setQosData(QVariant qos) {
    // copy the object
    QMetaType type(qos.userType());
    auto newQos = static_cast<SubscriptionQos*>(type.create(qos.constData()));
    this->qos = QSharedPointer<SubscriptionQos>(newQos);
}

QString SubscriptionRequest::toQString() const {
    return JsonSerializer::serialize(*this);
}

} // namespace joynr
