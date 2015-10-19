#/*
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
#include "joynr/QtSubscriptionQos.h"
#include "joynr/QtPeriodicSubscriptionQos.h"
#include "joynr/QtOnChangeSubscriptionQos.h"
#include "joynr/QtOnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/DispatcherUtils.h"
#include <limits>
#include <chrono>
#include <stdint.h>

using namespace std::chrono;

namespace joynr
{

const qint64& QtSubscriptionQos::DEFAULT_PUBLICATION_TTL()
{
    static const qint64 defaultPublicationTtl = 10000;
    return defaultPublicationTtl;
}

const qint64& QtSubscriptionQos::MIN_PUBLICATION_TTL()
{
    static const qint64 minPublicationTtl = 100;
    return minPublicationTtl;
}

const qint64& QtSubscriptionQos::MAX_PUBLICATION_TTL()
{
    static const qint64 maxPublicationTtl = 2592000000UL;
    return maxPublicationTtl;
}

const qint64& QtSubscriptionQos::NO_EXPIRY_DATE_TTL()
{
    static const qint64 noExpiryDateTTL = std::numeric_limits<qint64>::max(); // 2^63-1
    return noExpiryDateTTL;
}

const qint64& QtSubscriptionQos::NO_EXPIRY_DATE()
{
    static qint64 noExpiryDate = 0;
    return noExpiryDate;
}

QtSubscriptionQos::QtSubscriptionQos() : expiryDate(-1), publicationTtl(DEFAULT_PUBLICATION_TTL())
{
    setValidity(1000);
}

QtSubscriptionQos::QtSubscriptionQos(const qint64& validity)
        : expiryDate(-1), publicationTtl(DEFAULT_PUBLICATION_TTL())
{
    setValidity(validity);
}

QtSubscriptionQos::QtSubscriptionQos(const QtSubscriptionQos& subscriptionQos)
        : QObject(),
          expiryDate(subscriptionQos.expiryDate),
          publicationTtl(subscriptionQos.publicationTtl)
{
}

QtSubscriptionQos::~QtSubscriptionQos()
{
}

qint64 QtSubscriptionQos::getPublicationTtl() const
{
    return publicationTtl;
}

void QtSubscriptionQos::setPublicationTtl(const qint64& publicationTtl)
{
    this->publicationTtl = publicationTtl;
    if (this->publicationTtl > MAX_PUBLICATION_TTL()) {
        this->publicationTtl = MAX_PUBLICATION_TTL();
    }
    if (this->publicationTtl < MIN_PUBLICATION_TTL()) {
        this->publicationTtl = MIN_PUBLICATION_TTL();
    }
}

qint64 QtSubscriptionQos::getExpiryDate() const
{
    return expiryDate;
}

void QtSubscriptionQos::setExpiryDate(const qint64& expiryDate)
{
    this->expiryDate = expiryDate;
    if (this->expiryDate < TypeUtil::toQt(DispatcherUtils::nowInMilliseconds())) {
        clearExpiryDate();
    }
}

void QtSubscriptionQos::clearExpiryDate()
{
    this->expiryDate = NO_EXPIRY_DATE();
}

void QtSubscriptionQos::setValidity(const qint64& validity)
{
    if (validity == -1) {
        setExpiryDate(joynr::QtSubscriptionQos::NO_EXPIRY_DATE());
    } else {
        setExpiryDate(TypeUtil::toQt(DispatcherUtils::nowInMilliseconds()) + validity);
    }
}

QtSubscriptionQos& QtSubscriptionQos::operator=(const QtSubscriptionQos& subscriptionQos)
{
    expiryDate = subscriptionQos.getExpiryDate();
    publicationTtl = subscriptionQos.getPublicationTtl();
    return *this;
}

bool QtSubscriptionQos::operator==(const QtSubscriptionQos& subscriptionQos) const
{
    return getExpiryDate() == subscriptionQos.getExpiryDate() &&
           publicationTtl == subscriptionQos.getPublicationTtl();
}

bool QtSubscriptionQos::equals(const QObject& other) const
{
    int typeThis = QMetaType::type(this->metaObject()->className());
    int typeOther = QMetaType::type(other.metaObject()->className());
    auto newOther = dynamic_cast<const QtSubscriptionQos*>(&other);
    return typeThis == typeOther && *this == *newOther;
}

QtSubscriptionQos* QtSubscriptionQos::createQt(const SubscriptionQos& from)
{
    QtSubscriptionQos* to;
    if (dynamic_cast<const OnChangeSubscriptionQos*>(&from) != NULL) {
        to = createQt(dynamic_cast<const OnChangeSubscriptionQos&>(from));
    } else if (dynamic_cast<const PeriodicSubscriptionQos*>(&from) != NULL) {
        to = new QtPeriodicSubscriptionQos();
        createQtInternal(dynamic_cast<const PeriodicSubscriptionQos&>(from), *to);
    } else {
        to = new QtSubscriptionQos();
        createQtInternal(from, *to);
    }
    return to;
}

QtOnChangeSubscriptionQos* QtSubscriptionQos::createQt(const OnChangeSubscriptionQos& from)
{
    if (dynamic_cast<const OnChangeWithKeepAliveSubscriptionQos*>(&from) != NULL) {
        QtOnChangeWithKeepAliveSubscriptionQos* to = new QtOnChangeWithKeepAliveSubscriptionQos();
        createQtInternal(dynamic_cast<const OnChangeWithKeepAliveSubscriptionQos&>(from), *to);
        return to;
    } else {
        QtOnChangeSubscriptionQos* to = new QtOnChangeSubscriptionQos();
        createQtInternal(from, *to);
        return to;
    }
}

void QtSubscriptionQos::createQtInternal(const SubscriptionQos& from, QtSubscriptionQos& to)
{
    to.setExpiryDate(TypeUtil::toQt(from.getExpiryDate()));
    to.setPublicationTtl(TypeUtil::toQt(from.getPublicationTtl()));
}

void QtSubscriptionQos::createQtInternal(const PeriodicSubscriptionQos& from,
                                         QtPeriodicSubscriptionQos& to)
{
    QtSubscriptionQos::createQtInternal(
            static_cast<const SubscriptionQos&>(from), static_cast<QtSubscriptionQos&>(to));
    to.setAlertAfterInterval(TypeUtil::toQt(from.getAlertAfterInterval()));
    to.setPeriod(TypeUtil::toQt(from.getPeriod()));
}

void QtSubscriptionQos::createQtInternal(const OnChangeSubscriptionQos& from,
                                         QtOnChangeSubscriptionQos& to)
{
    QtSubscriptionQos::createQtInternal(
            static_cast<const SubscriptionQos&>(from), static_cast<QtSubscriptionQos&>(to));
    to.setMinInterval(TypeUtil::toQt(from.getMinInterval()));
}

void QtSubscriptionQos::createQtInternal(const OnChangeWithKeepAliveSubscriptionQos& from,
                                         QtOnChangeWithKeepAliveSubscriptionQos& to)
{
    QtSubscriptionQos::createQtInternal(static_cast<const OnChangeSubscriptionQos&>(from),
                                        static_cast<QtOnChangeSubscriptionQos&>(to));
    to.setAlertAfterInterval(TypeUtil::toQt(from.getAlertAfterInterval()));
    to.setMaxInterval(TypeUtil::toQt(from.getMaxInterval()));
}

} // namespace joynr
