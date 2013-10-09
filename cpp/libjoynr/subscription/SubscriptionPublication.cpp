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

#include "joynr/SubscriptionPublication.h"

namespace joynr {

const SubscriptionPublication SubscriptionPublication::NULL_RESPONSE = SubscriptionPublication();

SubscriptionPublication::SubscriptionPublication():
    subscriptionId(),
    response()
{

}
SubscriptionPublication::SubscriptionPublication(const SubscriptionPublication &other) :
    QObject(),
    subscriptionId(other.getSubscriptionId()),
    response(other.response)
{
}

SubscriptionPublication& SubscriptionPublication::operator=(const SubscriptionPublication &other){
    this->subscriptionId = other.getSubscriptionId();
    this->response = other.response;
    return *this;
}

QString SubscriptionPublication::getSubscriptionId() const {
    return subscriptionId;
}

void SubscriptionPublication::setSubscriptionId(QString subscriptionId){
    this->subscriptionId = subscriptionId;
}

QVariant SubscriptionPublication::getResponse() const {
    return response;
}

void SubscriptionPublication::setResponse(QVariant response){
    this->response = response;
}

bool SubscriptionPublication::operator==(const SubscriptionPublication& other) const {
    return subscriptionId == other.getSubscriptionId()
            && response == other.getResponse();
}

bool SubscriptionPublication::operator!=(const SubscriptionPublication& other) const {
    return !(*this==other);
}

} // namespace joynr
