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
#include "joynr/PeriodicSubscriptionQos.h"

using namespace joynr;

const qint64& PeriodicSubscriptionQos::MIN_PERIOD(){
    static qint64 minPeriod = 50;
    return minPeriod;
}

const qint64& PeriodicSubscriptionQos::MAX_PERIOD(){
    static qint64 maxPeriod = 2592000000UL;
    return maxPeriod;
}

const qint64& PeriodicSubscriptionQos::MAX_ALERT_AFTER_INTERVAL(){
    static qint64 maxAlertAfterInterval = 2592000000UL;
    return maxAlertAfterInterval;
}

const qint64& PeriodicSubscriptionQos::DEFAULT_ALERT_AFTER_INTERVAL(){
    return NO_ALERT_AFTER_INTERVAL();
}

const qint64& PeriodicSubscriptionQos::NO_ALERT_AFTER_INTERVAL() {
    static qint64 noAlertAfterInterval = 0;
    return noAlertAfterInterval;
}

PeriodicSubscriptionQos::PeriodicSubscriptionQos() :
    SubscriptionQos(),
    period(-1),
    alterAfterInterval(DEFAULT_ALERT_AFTER_INTERVAL())
{
}

PeriodicSubscriptionQos::PeriodicSubscriptionQos(const qint64& validity, const qint64& period, const qint64& alertAfterInterval) :
    SubscriptionQos(validity),
    period(-1),
    alterAfterInterval(DEFAULT_ALERT_AFTER_INTERVAL())
{
    setPeriod(period);
    setAlertAfterInterval(alertAfterInterval);
}

PeriodicSubscriptionQos::PeriodicSubscriptionQos(const PeriodicSubscriptionQos& other) :
    SubscriptionQos(other),
    period(other.getPeriod()),
    alterAfterInterval(other.getAlertAfterInterval())
{
}

void PeriodicSubscriptionQos::setPeriod(const qint64& period){
    this->period = period;
    if(this->period > MAX_PERIOD()) {
        this->period = MAX_PERIOD();
    }
    if(this->period < MIN_PERIOD()) {
        this->period = MIN_PERIOD();
    }
    if(this->alterAfterInterval != 0 && this->alterAfterInterval < period) {
        this->alterAfterInterval = period;
    }
}

qint64 PeriodicSubscriptionQos::getPeriod() const {
    return this->period;
}

void PeriodicSubscriptionQos::setAlertAfterInterval(const qint64 &alertAfterInterval) {
    this->alterAfterInterval = alertAfterInterval;
    if(this->alterAfterInterval > MAX_ALERT_AFTER_INTERVAL()) {
        this->alterAfterInterval = MAX_ALERT_AFTER_INTERVAL();
    }
    if(this->alterAfterInterval != 0 && this->alterAfterInterval < period) {
        this->alterAfterInterval = period;
    }
}

qint64 PeriodicSubscriptionQos::getAlertAfterInterval() const {
    return alterAfterInterval;
}

void PeriodicSubscriptionQos::clearAlertAfterInterval() {
    this->alterAfterInterval = NO_ALERT_AFTER_INTERVAL();
}

PeriodicSubscriptionQos& PeriodicSubscriptionQos::operator=(const PeriodicSubscriptionQos& other) {
    expiryDate = other.getExpiryDate();
    publicationTtl = other.getPublicationTtl();
    period = other.getPeriod();
    alterAfterInterval = other.getAlertAfterInterval();
    return *this;
}

bool PeriodicSubscriptionQos::operator==(const PeriodicSubscriptionQos& other) const {
    return
        expiryDate == other.getExpiryDate() &&
        publicationTtl == other.getPublicationTtl() &&
        period == other.getPeriod() &&
        alterAfterInterval == other.getAlertAfterInterval();
}

bool PeriodicSubscriptionQos::equals(const QObject &other) const {
    int typeThis = QMetaType::type(this->metaObject()->className());
    int typeOther = QMetaType::type(other.metaObject()->className());
    auto newOther = dynamic_cast<const PeriodicSubscriptionQos*>(&other);
    return typeThis == typeOther && *this == *newOther;
}
