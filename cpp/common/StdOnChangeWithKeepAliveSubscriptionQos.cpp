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
#include "joynr/StdOnChangeWithKeepAliveSubscriptionQos.h"

using namespace joynr;

const int64_t& StdOnChangeWithKeepAliveSubscriptionQos::MAX_MAX_INTERVAL()
{
    static int64_t defaultMaxInterval = 2592000000UL;
    return defaultMaxInterval;
}

const int64_t& StdOnChangeWithKeepAliveSubscriptionQos::MAX_ALERT_AFTER_INTERVAL()
{
    static int64_t maxAlertAfterInterval = 2592000000UL;
    return maxAlertAfterInterval;
}

const int64_t& StdOnChangeWithKeepAliveSubscriptionQos::DEFAULT_ALERT_AFTER_INTERVAL()
{
    return NO_ALERT_AFTER_INTERVAL();
}

const int64_t& StdOnChangeWithKeepAliveSubscriptionQos::NO_ALERT_AFTER_INTERVAL()
{
    static int64_t noAlertAfterInterval = 0;
    return noAlertAfterInterval;
}

StdOnChangeWithKeepAliveSubscriptionQos::StdOnChangeWithKeepAliveSubscriptionQos()
        : StdOnChangeSubscriptionQos(),
          maxInterval(getMinInterval()),
          alertAfterInterval(DEFAULT_ALERT_AFTER_INTERVAL())
{
}

StdOnChangeWithKeepAliveSubscriptionQos::StdOnChangeWithKeepAliveSubscriptionQos(
        const int64_t& validity,
        const int64_t& minInterval,
        const int64_t& maxInterval,
        const int64_t& alertAfterInterval)
        : StdOnChangeSubscriptionQos(validity, minInterval),
          maxInterval(getMinInterval()),
          alertAfterInterval(DEFAULT_ALERT_AFTER_INTERVAL())
{
    setMaxInterval(maxInterval);
    setAlertAfterInterval(alertAfterInterval);
}

StdOnChangeWithKeepAliveSubscriptionQos::StdOnChangeWithKeepAliveSubscriptionQos(
        const StdOnChangeWithKeepAliveSubscriptionQos& other)
        : StdOnChangeSubscriptionQos(other),
          maxInterval(other.getMaxInterval()),
          alertAfterInterval(other.getAlertAfterInterval())
{
}

void StdOnChangeWithKeepAliveSubscriptionQos::setMaxInterval(const int64_t& maxInterval)
{
    this->maxInterval = maxInterval;
    if (this->maxInterval < this->getMinInterval()) {
        this->maxInterval = this->minInterval;
    }
    if (this->maxInterval > MAX_MAX_INTERVAL()) {
        this->maxInterval = MAX_MAX_INTERVAL();
    }
    if (this->alertAfterInterval != 0 && this->alertAfterInterval < this->maxInterval) {
        this->alertAfterInterval = this->maxInterval;
    }
}

int64_t StdOnChangeWithKeepAliveSubscriptionQos::getMaxInterval() const
{
    return this->maxInterval;
}

void StdOnChangeWithKeepAliveSubscriptionQos::setMinInterval(const int64_t& minInterval)
{
    StdOnChangeSubscriptionQos::setMinInterval(minInterval);
    // corrects the maxinterval if minInterval changes
    setMaxInterval(this->maxInterval);
}

void StdOnChangeWithKeepAliveSubscriptionQos::setAlertAfterInterval(
        const int64_t& alertAfterInterval)
{
    this->alertAfterInterval = alertAfterInterval;
    if (this->alertAfterInterval > MAX_ALERT_AFTER_INTERVAL()) {
        this->alertAfterInterval = MAX_ALERT_AFTER_INTERVAL();
    }
    if (this->alertAfterInterval != 0 && this->alertAfterInterval < this->getMaxInterval()) {
        this->alertAfterInterval = this->getMaxInterval();
    }
}

int64_t StdOnChangeWithKeepAliveSubscriptionQos::getAlertAfterInterval() const
{
    return alertAfterInterval;
}

StdOnChangeWithKeepAliveSubscriptionQos& StdOnChangeWithKeepAliveSubscriptionQos::operator=(
        const StdOnChangeWithKeepAliveSubscriptionQos& other)
{
    expiryDate = other.getExpiryDate();
    publicationTtl = other.getPublicationTtl();
    minInterval = other.getMinInterval();
    maxInterval = other.getMaxInterval();
    alertAfterInterval = other.getAlertAfterInterval();
    return *this;
}

bool StdOnChangeWithKeepAliveSubscriptionQos::operator==(
        const StdOnChangeWithKeepAliveSubscriptionQos& other) const
{
    return expiryDate == other.getExpiryDate() && publicationTtl == other.getPublicationTtl() &&
           minInterval == other.getMinInterval() && maxInterval == other.getMaxInterval() &&
           alertAfterInterval == other.getAlertAfterInterval();
}
