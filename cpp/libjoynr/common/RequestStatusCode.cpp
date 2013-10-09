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
#include "joynr/RequestStatusCode.h"

namespace joynr {

RequestStatusCode RequestStatusCode::OK = RequestStatusCode(0, "OK");
RequestStatusCode RequestStatusCode::NOT_STARTED = RequestStatusCode(1, "Not started");
RequestStatusCode RequestStatusCode::IN_PROGRESS = RequestStatusCode(2, "In progress");

RequestStatusCode RequestStatusCode::ERROR = RequestStatusCode(300, "Error");
RequestStatusCode RequestStatusCode::ERROR_TIME_OUT_ARBITRATION = RequestStatusCode(301, "Error timout waiting for arbitration");
RequestStatusCode RequestStatusCode::ERROR_TIME_OUT_WAITING_FOR_RESPONSE = RequestStatusCode(302, "Error timeout waiting for the json response");
RequestStatusCode RequestStatusCode::ERROR_REPLY_CALLER_CANNOT_CONVERT_RETURN_VALUE = RequestStatusCode(303, "Error in ReplyCaller when attempting to cast the return type to the desired type");


RequestStatusCode::RequestStatusCode(long id, QString description)
    : id(id),
      description(description){
}

QString RequestStatusCode::toString() const {
    QString result;
    result.append("[RequestStatusCode id: " + QString::number(id));
    result.append(" description: " + description + "]");
    return result;
}

long RequestStatusCode::getId() const {
    return id;
}

bool RequestStatusCode::operator==(const RequestStatusCode& requestStatusCode) const {
    return id == requestStatusCode.getId();
}

bool RequestStatusCode::operator!=(const RequestStatusCode& requestStatusCode) const {
    return !(*this==requestStatusCode);
}


} // namespace joynr
