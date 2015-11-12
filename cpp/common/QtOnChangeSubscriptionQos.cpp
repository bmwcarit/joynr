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
#include "joynr/QtOnChangeSubscriptionQos.h"

using namespace joynr;

const qint64& QtOnChangeSubscriptionQos::DEFAULT_MIN_INTERVAL()
{
    static qint64 defaultMinInterval = 1000;
    return defaultMinInterval;
}

const qint64& QtOnChangeSubscriptionQos::MIN_MIN_INTERVAL()
{
    static qint64 minMinInterval = 0;
    return minMinInterval;
}

const qint64& QtOnChangeSubscriptionQos::MAX_MIN_INTERVAL()
{
    static qint64 maxMinInterval = 2592000000UL;
    return maxMinInterval;
}

QtOnChangeSubscriptionQos::QtOnChangeSubscriptionQos()
        : QtSubscriptionQos(), minInterval(MIN_MIN_INTERVAL())
{
}

QtOnChangeSubscriptionQos::QtOnChangeSubscriptionQos(const qint64& validity,
                                                     const qint64& minInterval)
        : QtSubscriptionQos(validity), minInterval(DEFAULT_MIN_INTERVAL())
{
    setMinInterval(minInterval);
}

QtOnChangeSubscriptionQos::QtOnChangeSubscriptionQos(const QtOnChangeSubscriptionQos& other)
        : QtSubscriptionQos(other), minInterval(other.getMinInterval())
{
}

qint64 QtOnChangeSubscriptionQos::getMinInterval() const
{
    return minInterval;
}

void QtOnChangeSubscriptionQos::setMinInterval(const qint64& minInterval)
{
    this->minInterval = minInterval;
    if (this->minInterval < MIN_MIN_INTERVAL()) {
        this->minInterval = MIN_MIN_INTERVAL();
    }
    if (this->minInterval > MAX_MIN_INTERVAL()) {
        this->minInterval = MAX_MIN_INTERVAL();
    }
}

QtOnChangeSubscriptionQos& QtOnChangeSubscriptionQos::operator=(
        const QtOnChangeSubscriptionQos& other)
{
    expiryDate = other.getExpiryDate();
    publicationTtl = other.getPublicationTtl();
    minInterval = other.getMinInterval();
    return *this;
}

bool QtOnChangeSubscriptionQos::operator==(const QtOnChangeSubscriptionQos& other) const
{
    return expiryDate == other.getExpiryDate() && publicationTtl == other.getPublicationTtl() &&
           minInterval == other.getMinInterval();
}

bool QtOnChangeSubscriptionQos::equals(const QObject& other) const
{
    int typeThis = QMetaType::type(this->metaObject()->className());
    int typeOther = QMetaType::type(other.metaObject()->className());
    auto newOther = dynamic_cast<const QtOnChangeSubscriptionQos*>(&other);
    return typeThis == typeOther && *this == *newOther;
}
