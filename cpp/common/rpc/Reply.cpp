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

#include "joynr/Reply.h"

namespace joynr {

const Reply Reply::NULL_RESPONSE = Reply();

Reply::Reply():
    requestReplyId(),
    response()
{

}
Reply::Reply(const Reply &other) :
    QObject(),
    requestReplyId(other.getRequestReplyId()),
    response(other.response)
{
}

Reply& Reply::operator=(const Reply &other){
    this->requestReplyId = other.getRequestReplyId();
    this->response = other.response;
    return *this;
}

QString Reply::getRequestReplyId() const {
    return requestReplyId;
}

void Reply::setRequestReplyId(QString requestReplyId){
    this->requestReplyId = requestReplyId;
}

QVariant Reply::getResponse() const {
    return response;
}

void Reply::setResponse(QVariant response){
    this->response = response;
}

bool Reply::operator==(const Reply& other) const {
    return requestReplyId == other.getRequestReplyId()
            && response == other.getResponse();
}

bool Reply::operator!=(const Reply& other) const {
    return !(*this==other);
}

} // namespace joynr
