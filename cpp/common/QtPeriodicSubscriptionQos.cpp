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
#include "joynr/QtPeriodicSubscriptionQos.h"

using namespace joynr;

const qint64& QtPeriodicSubscriptionQos::MIN_PERIOD()
{
    static qint64 minPeriod = 50;
    return minPeriod;
}

const qint64& QtPeriodicSubscriptionQos::MAX_PERIOD()
{
    static qint64 maxPeriod = 2592000000UL;
    return maxPeriod;
}

const qint64& QtPeriodicSubscriptionQos::MAX_ALERT_AFTER_INTERVAL()
{
    static qint64 maxAlertAfterInterval = 2592000000UL;
    return maxAlertAfterInterval;
}

const qint64& QtPeriodicSubscriptionQos::DEFAULT_ALERT_AFTER_INTERVAL()
{
    return NO_ALERT_AFTER_INTERVAL();
}

const qint64& QtPeriodicSubscriptionQos::NO_ALERT_AFTER_INTERVAL()
{
    static qint64 noAlertAfterInterval = 0;
    return noAlertAfterInterval;
}

QtPeriodicSubscriptionQos::QtPeriodicSubscriptionQos()
        : QtSubscriptionQos(), period(-1), alertAfterInterval(DEFAULT_ALERT_AFTER_INTERVAL())
{
}

QtPeriodicSubscriptionQos::QtPeriodicSubscriptionQos(const qint64& validity,
                                                     const qint64& period,
                                                     const qint64& alertAfterInterval)
        : QtSubscriptionQos(validity),
          period(-1),
          alertAfterInterval(DEFAULT_ALERT_AFTER_INTERVAL())
{
    setPeriod(period);
    setAlertAfterInterval(alertAfterInterval);
}

QtPeriodicSubscriptionQos::QtPeriodicSubscriptionQos(const QtPeriodicSubscriptionQos& other)
        : QtSubscriptionQos(other),
          period(other.getPeriod()),
          alertAfterInterval(other.getAlertAfterInterval())
{
}

void QtPeriodicSubscriptionQos::setPeriod(const qint64& period)
{
    this->period = period;
    if (this->period > MAX_PERIOD()) {
        this->period = MAX_PERIOD();
    }
    if (this->period < MIN_PERIOD()) {
        this->period = MIN_PERIOD();
    }
    if (this->alertAfterInterval != 0 && this->alertAfterInterval < period) {
        this->alertAfterInterval = period;
    }
}

qint64 QtPeriodicSubscriptionQos::getPeriod() const
{
    return this->period;
}

void QtPeriodicSubscriptionQos::setAlertAfterInterval(const qint64& alertAfterInterval)
{
    this->alertAfterInterval = alertAfterInterval;
    if (this->alertAfterInterval > MAX_ALERT_AFTER_INTERVAL()) {
        this->alertAfterInterval = MAX_ALERT_AFTER_INTERVAL();
    }
    if (this->alertAfterInterval != 0 && this->alertAfterInterval < period) {
        this->alertAfterInterval = period;
    }
}

qint64 QtPeriodicSubscriptionQos::getAlertAfterInterval() const
{
    return alertAfterInterval;
}

void QtPeriodicSubscriptionQos::clearAlertAfterInterval()
{
    this->alertAfterInterval = NO_ALERT_AFTER_INTERVAL();
}

QtPeriodicSubscriptionQos& QtPeriodicSubscriptionQos::operator=(
        const QtPeriodicSubscriptionQos& other)
{
    expiryDate = other.getExpiryDate();
    publicationTtl = other.getPublicationTtl();
    period = other.getPeriod();
    alertAfterInterval = other.getAlertAfterInterval();
    return *this;
}

bool QtPeriodicSubscriptionQos::operator==(const QtPeriodicSubscriptionQos& other) const
{
    return expiryDate == other.getExpiryDate() && publicationTtl == other.getPublicationTtl() &&
           period == other.getPeriod() && alertAfterInterval == other.getAlertAfterInterval();
}

bool QtPeriodicSubscriptionQos::equals(const QObject& other) const
{
    int typeThis = QMetaType::type(this->metaObject()->className());
    int typeOther = QMetaType::type(other.metaObject()->className());
    auto newOther = dynamic_cast<const QtPeriodicSubscriptionQos*>(&other);
    return typeThis == typeOther && *this == *newOther;
}
