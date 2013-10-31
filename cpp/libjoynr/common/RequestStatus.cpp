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
#include "joynr/RequestStatus.h"

namespace joynr {

RequestStatus::RequestStatus()
    : code(RequestStatusCode::NOT_STARTED),
      description() {
}

RequestStatus::RequestStatus(RequestStatusCode requestStatus)
    : code(requestStatus),
      description() {
}

bool RequestStatus::successful() const {
    return code == RequestStatusCode::OK;
}

RequestStatusCode RequestStatus::getCode() const{
    return code;
}

void RequestStatus::setCode(const RequestStatusCode& code) {
    this->code = code;
}

QStringList RequestStatus::getDescription() {
    return description;
}

void RequestStatus::addDescription(const QString& description) {
    this->description.append(description);
}

QString RequestStatus::toString() const {
    QString desc;
    for (int i = 0; i < description.size(); i++) {
        if (i != 0) {
            desc.append("\n");
        }
        desc.append(i + ": ");
        desc.append(description.at(i));
    }


    return "[RequestStatus code: " + code.toString()
            + "description: " + desc + "]";
}

} // namespace joynr
