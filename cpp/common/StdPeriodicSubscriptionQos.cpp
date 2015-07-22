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
#include "joynr/StdPeriodicSubscriptionQos.h"

using namespace joynr;

const int64_t& StdPeriodicSubscriptionQos::MIN_PERIOD()
{
    static int64_t minPeriod = 50;
    return minPeriod;
}

const int64_t& StdPeriodicSubscriptionQos::MAX_PERIOD()
{
    static int64_t maxPeriod = 2592000000UL;
    return maxPeriod;
}

const int64_t& StdPeriodicSubscriptionQos::MAX_ALERT_AFTER_INTERVAL()
{
    static int64_t maxAlertAfterInterval = 2592000000UL;
    return maxAlertAfterInterval;
}

const int64_t& StdPeriodicSubscriptionQos::DEFAULT_ALERT_AFTER_INTERVAL()
{
    return NO_ALERT_AFTER_INTERVAL();
}

const int64_t& StdPeriodicSubscriptionQos::NO_ALERT_AFTER_INTERVAL()
{
    static int64_t noAlertAfterInterval = 0;
    return noAlertAfterInterval;
}

StdPeriodicSubscriptionQos::StdPeriodicSubscriptionQos()
        : StdSubscriptionQos(), period(-1), alertAfterInterval(DEFAULT_ALERT_AFTER_INTERVAL())
{
}

StdPeriodicSubscriptionQos::StdPeriodicSubscriptionQos(const int64_t& validity,
                                                       const int64_t& period,
                                                       const int64_t& alertAfterInterval)
        : StdSubscriptionQos(validity),
          period(-1),
          alertAfterInterval(DEFAULT_ALERT_AFTER_INTERVAL())
{
    setPeriod(period);
    setAlertAfterInterval(alertAfterInterval);
}

StdPeriodicSubscriptionQos::StdPeriodicSubscriptionQos(const StdPeriodicSubscriptionQos& other)
        : StdSubscriptionQos(other),
          period(other.getPeriod()),
          alertAfterInterval(other.getAlertAfterInterval())
{
}

void StdPeriodicSubscriptionQos::setPeriod(const int64_t& period)
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

int64_t StdPeriodicSubscriptionQos::getPeriod() const
{
    return this->period;
}

void StdPeriodicSubscriptionQos::setAlertAfterInterval(const int64_t& alertAfterInterval)
{
    this->alertAfterInterval = alertAfterInterval;
    if (this->alertAfterInterval > MAX_ALERT_AFTER_INTERVAL()) {
        this->alertAfterInterval = MAX_ALERT_AFTER_INTERVAL();
    }
    if (this->alertAfterInterval != 0 && this->alertAfterInterval < period) {
        this->alertAfterInterval = period;
    }
}

int64_t StdPeriodicSubscriptionQos::getAlertAfterInterval() const
{
    return alertAfterInterval;
}

void StdPeriodicSubscriptionQos::clearAlertAfterInterval()
{
    this->alertAfterInterval = NO_ALERT_AFTER_INTERVAL();
}

StdPeriodicSubscriptionQos& StdPeriodicSubscriptionQos::operator=(
        const StdPeriodicSubscriptionQos& other)
{
    expiryDate = other.getExpiryDate();
    publicationTtl = other.getPublicationTtl();
    period = other.getPeriod();
    alertAfterInterval = other.getAlertAfterInterval();
    return *this;
}

bool StdPeriodicSubscriptionQos::operator==(const StdPeriodicSubscriptionQos& other) const
{
    return expiryDate == other.getExpiryDate() && publicationTtl == other.getPublicationTtl() &&
           period == other.getPeriod() && alertAfterInterval == other.getAlertAfterInterval();
}
