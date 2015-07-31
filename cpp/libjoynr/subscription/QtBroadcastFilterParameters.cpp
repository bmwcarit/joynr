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
#include "joynr/QtBroadcastFilterParameters.h"
#include <QMapIterator>
#include "joynr/TypeUtil.h"

namespace joynr
{

QtBroadcastFilterParameters::QtBroadcastFilterParameters()
        : filterParameters(QMap<QString, QVariant>())
{
}

QtBroadcastFilterParameters::QtBroadcastFilterParameters(
        const QtBroadcastFilterParameters& filterParameters)
        : QObject(), filterParameters(filterParameters.filterParameters)
{
}

QtBroadcastFilterParameters& QtBroadcastFilterParameters::operator=(
        const QtBroadcastFilterParameters& filterParameters)
{
    this->filterParameters = filterParameters.filterParameters;
    return *this;
}

QtBroadcastFilterParameters::~QtBroadcastFilterParameters()
{
}

bool QtBroadcastFilterParameters::operator==(
        const QtBroadcastFilterParameters& filterParameters) const
{
    bool equal = this->filterParameters.keys() == filterParameters.filterParameters.keys();

    QList<QVariant> values1 = this->filterParameters.values();
    QList<QVariant> values2 = this->filterParameters.values();

    if (equal) {
        for (int i = 0; i < values1.size(); i++) {
            if (!(equal = equal && values1[i].toString() == values2[i].toString())) {
                break;
            }
        }
    }

    return equal;
}

void QtBroadcastFilterParameters::setFilterParameter(const QString& parameter, const QString& value)
{
    filterParameters.insert(parameter, value);
}

QMap<QString, QString> QtBroadcastFilterParameters::getFilterParameters() const
{
    QMap<QString, QString> stringParams;
    QMapIterator<QString, QVariant> i(filterParameters);
    while (i.hasNext()) {
        i.next();
        stringParams.insert(i.key(), i.value().toString());
    }
    return stringParams;
}

QString QtBroadcastFilterParameters::getFilterParameter(const QString& parameter) const
{
    if (filterParameters.contains(parameter)) {
        return filterParameters.value(parameter).toString();
    } else {
        return QString();
    }
}

void QtBroadcastFilterParameters::setFilterParameters(const QMap<QString, QString>& value)
{
    filterParameters.clear();
    QMapIterator<QString, QString> i(value);
    while (i.hasNext()) {
        i.next();
        filterParameters.insert(i.key(), QVariant(i.value()));
    }
}

bool QtBroadcastFilterParameters::equals(const QObject& other) const
{
    int typeThis = QMetaType::type(this->metaObject()->className());
    int typeOther = QMetaType::type(other.metaObject()->className());
    auto newOther = dynamic_cast<const QtBroadcastFilterParameters*>(&other);
    return typeThis == typeOther && *this == *newOther;
}

QtBroadcastFilterParameters QtBroadcastFilterParameters::createQt(
        const BroadcastFilterParameters& from)
{
    QtBroadcastFilterParameters to;
    std::map<std::string, std::string> filterParameters(from.getFilterParameters());
    for (std::map<std::string, std::string>::const_iterator iterator = filterParameters.begin();
         iterator != filterParameters.end();
         iterator++) {
        to.setFilterParameter(TypeUtil::toQt(iterator->first), TypeUtil::toQt(iterator->second));
    }
    return to;
}

BroadcastFilterParameters QtBroadcastFilterParameters::createStd(
        const QtBroadcastFilterParameters& from)
{
    BroadcastFilterParameters to;
    for (auto e : from.getFilterParameters().toStdMap()) {
        to.setFilterParameter(TypeUtil::toStd(e.first), TypeUtil::toStd(e.second));
    }
    return to;
}
} // namespace joynr
