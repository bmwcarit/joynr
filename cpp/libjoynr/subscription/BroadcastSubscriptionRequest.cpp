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
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/JsonSerializer.h"

#include <cassert>

namespace joynr {

using namespace joynr_logging;
Logger* BroadcastSubscriptionRequest::logger = Logging::getInstance()->getLogger("MSG", "BroadcastSubscriptionRequest");

BroadcastSubscriptionRequest::BroadcastSubscriptionRequest():
    filterParameters()
{
    qRegisterMetaType<BroadcastFilterParameters>("BroadcastFilterParameters");
    qRegisterMetaType<QSharedPointer<BroadcastFilterParameters>>();
}

BroadcastSubscriptionRequest::BroadcastSubscriptionRequest(const BroadcastSubscriptionRequest& subscriptionRequest) :
    SubscriptionRequest(subscriptionRequest),
    filterParameters(subscriptionRequest.getFilterParameters()) {
}


BroadcastSubscriptionRequest& BroadcastSubscriptionRequest::operator=(const BroadcastSubscriptionRequest& subscriptionRequest) {
    SubscriptionRequest::operator =(subscriptionRequest);
    filterParameters = subscriptionRequest.getFilterParameters();
    return *this;
}

bool BroadcastSubscriptionRequest::operator==(const BroadcastSubscriptionRequest& subscriptionRequest) const {

    bool equal = getQos()->equals(*subscriptionRequest.getQos())
            && getFilterParameters().equals(subscriptionRequest.getFilterParameters());
    return
            getSubscriptionId() == subscriptionRequest.getSubscriptionId()
            && getSubscribeToName() == subscriptionRequest.getSubscribeToName()
            && equal;
}

void BroadcastSubscriptionRequest::setFilterParametersData(QVariant filterParameters) {
    this->filterParameters = filterParameters.value<BroadcastFilterParameters>();
}

QString BroadcastSubscriptionRequest::toQString() const {
    return JsonSerializer::serialize(*this);
}

void BroadcastSubscriptionRequest::setQos(QSharedPointer<OnChangeSubscriptionQos> qos){
    SubscriptionRequest::setQos(qos);
}

QVariant BroadcastSubscriptionRequest::getFilterParametersData() const {
    return QVariant::fromValue(filterParameters);
}

BroadcastFilterParameters BroadcastSubscriptionRequest::getFilterParameters() const{
    return filterParameters;
}

void BroadcastSubscriptionRequest::setFilterParameters(const BroadcastFilterParameters &filterParameters){
    this->filterParameters = filterParameters;
}


} // namespace joynr
