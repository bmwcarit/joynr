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
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"

using namespace joynr;

const qint64& OnChangeWithKeepAliveSubscriptionQos::MAX_MAX_INTERVAL() {
    static qint64 defaultMaxInterval = 2592000000UL;
    return defaultMaxInterval;
}

const qint64& OnChangeWithKeepAliveSubscriptionQos::MAX_ALERT_AFTER_INTERVAL(){
    static qint64 maxAlertAfterInterval = 2592000000UL;
    return maxAlertAfterInterval;
}

const qint64& OnChangeWithKeepAliveSubscriptionQos::DEFAULT_ALERT_AFTER_INTERVAL(){
    return NO_ALERT_AFTER_INTERVAL();
}

const qint64& OnChangeWithKeepAliveSubscriptionQos::NO_ALERT_AFTER_INTERVAL() {
    static qint64 noAlertAfterInterval = 0;
    return noAlertAfterInterval;
}

OnChangeWithKeepAliveSubscriptionQos::OnChangeWithKeepAliveSubscriptionQos() :
    OnChangeSubscriptionQos(),
    maxInterval(getMinInterval()),
    alertAfterInterval(DEFAULT_ALERT_AFTER_INTERVAL())
{
}

OnChangeWithKeepAliveSubscriptionQos::OnChangeWithKeepAliveSubscriptionQos(const qint64& validity, const qint64& minInterval, const qint64& maxInterval, const qint64& alertAfterInterval) :
    OnChangeSubscriptionQos(validity, minInterval),
    maxInterval(getMinInterval()),
    alertAfterInterval(DEFAULT_ALERT_AFTER_INTERVAL())
{
    setMaxInterval(maxInterval);
    setAlertAfterInterval(alertAfterInterval);
}

OnChangeWithKeepAliveSubscriptionQos::OnChangeWithKeepAliveSubscriptionQos(const OnChangeWithKeepAliveSubscriptionQos& other) :
    OnChangeSubscriptionQos(other),
    maxInterval(other.getMaxInterval()),
    alertAfterInterval(other.getAlertAfterInterval())
{
}

void OnChangeWithKeepAliveSubscriptionQos::setMaxInterval(const qint64& maxInterval){
    this->maxInterval = maxInterval;
    if(this->maxInterval < this->getMinInterval()) {
        this->maxInterval = this->minInterval;
    }
    if(this->maxInterval > MAX_MAX_INTERVAL()) {
        this->maxInterval = MAX_MAX_INTERVAL();
    }
    if(this->alertAfterInterval != 0 && this->alertAfterInterval < this->maxInterval) {
        this->alertAfterInterval = this->maxInterval;
    }
}

qint64 OnChangeWithKeepAliveSubscriptionQos::getMaxInterval() const {
    return this->maxInterval;
}

void OnChangeWithKeepAliveSubscriptionQos::setMinInterval(const qint64 &minInterval) {
    OnChangeSubscriptionQos::setMinInterval(minInterval);
    // corrects the maxinterval if minInterval changes
    setMaxInterval(this->maxInterval);
}

void OnChangeWithKeepAliveSubscriptionQos::setAlertAfterInterval(const qint64 &alertAfterInterval) {
    this->alertAfterInterval = alertAfterInterval;
    if(this->alertAfterInterval > MAX_ALERT_AFTER_INTERVAL()) {
        this->alertAfterInterval = MAX_ALERT_AFTER_INTERVAL();
    }
    if(this->alertAfterInterval != 0 && this->alertAfterInterval < this->getMaxInterval()) {
        this->alertAfterInterval = this->getMaxInterval();
    }
}

qint64 OnChangeWithKeepAliveSubscriptionQos::getAlertAfterInterval() const {
    return alertAfterInterval;
}

OnChangeWithKeepAliveSubscriptionQos& OnChangeWithKeepAliveSubscriptionQos::operator=(const OnChangeWithKeepAliveSubscriptionQos& other) {
    expiryDate = other.getExpiryDate();
    publicationTtl = other.getPublicationTtl();
    minInterval = other.getMinInterval();
    maxInterval = other.getMaxInterval();
    alertAfterInterval = other.getAlertAfterInterval();
    return *this;
}

bool OnChangeWithKeepAliveSubscriptionQos::operator==(const OnChangeWithKeepAliveSubscriptionQos& other) const {
    return
        expiryDate == other.getExpiryDate() &&
        publicationTtl == other.getPublicationTtl() &&
        minInterval == other.getMinInterval() &&
        maxInterval == other.getMaxInterval() &&
        alertAfterInterval == other.getAlertAfterInterval();
}

bool OnChangeWithKeepAliveSubscriptionQos::equals(const QObject &other) const {
    int typeThis = QMetaType::type(this->metaObject()->className());
    int typeOther = QMetaType::type(other.metaObject()->className());
    auto newOther = dynamic_cast<const OnChangeWithKeepAliveSubscriptionQos*>(&other);
    return typeThis == typeOther && *this == *newOther;
}
