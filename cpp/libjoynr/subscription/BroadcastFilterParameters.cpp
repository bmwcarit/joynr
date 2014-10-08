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
#include "joynr/BroadcastFilterParameters.h"

namespace joynr {

BroadcastFilterParameters::BroadcastFilterParameters() :
    filterParameters(QMap<QString, QVariant>()) {
}

BroadcastFilterParameters::BroadcastFilterParameters(const BroadcastFilterParameters &filterParameters) :
    QObject(),
    filterParameters(filterParameters.getFilterParameters()) {
}

BroadcastFilterParameters &BroadcastFilterParameters::operator=(const BroadcastFilterParameters &filterParameters){
    this->filterParameters = filterParameters.getFilterParameters();
    return *this;
}

BroadcastFilterParameters::~BroadcastFilterParameters() {
}

bool BroadcastFilterParameters::operator==(const BroadcastFilterParameters &filterParameters) const {
    bool equal = this->filterParameters.keys() == filterParameters.getFilterParameters().keys();

    QList<QVariant> values1 = this->filterParameters.values();
    QList<QVariant> values2 = this->filterParameters.values();

    if (equal){
        for (int i=0; i<values1.size(); i++){
            if (! (equal = equal && (values1[i].toString()) == values2[i].toString()))
                break;
        }
    }

    return equal;
}

void BroadcastFilterParameters::setFilterParameter(QString parameter, QString value) {
    filterParameters.insert(parameter, value);
}

QMap<QString, QVariant> BroadcastFilterParameters::getFilterParameters() const {
    return filterParameters;
}

QString BroadcastFilterParameters::getFilterParameter(QString parameter) const {
    if (filterParameters.contains(parameter)) {
        return filterParameters.value(parameter).value<QString>();
    }
    else {
        return QString();
    }
}

void BroadcastFilterParameters::setFilterParameters(const QMap<QString, QVariant> &value){
    filterParameters = value;
}

bool BroadcastFilterParameters::equals(const QObject& other) const {
    int typeThis = QMetaType::type(this->metaObject()->className());
    int typeOther = QMetaType::type(other.metaObject()->className());
    auto newOther = dynamic_cast<const BroadcastFilterParameters*>(&other);
    return typeThis == typeOther && *this == *newOther;
}

} // namespace joynr
