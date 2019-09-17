/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

#include "joynr/BroadcastFilterParameters.h"
#include "joynr/OnChangeSubscriptionQos.h"

namespace joynr
{

BroadcastSubscriptionRequest::BroadcastSubscriptionRequest() : _filterParameters()
{
}

bool BroadcastSubscriptionRequest::operator==(
        const BroadcastSubscriptionRequest& subscriptionRequest) const
{

    bool equal = *(getQos()) == *(subscriptionRequest.getQos()) &&
                 getFilterParameters() == subscriptionRequest.getFilterParameters();
    return getSubscriptionId() == subscriptionRequest.getSubscriptionId() &&
           getSubscribeToName() == subscriptionRequest.getSubscribeToName() && equal;
}

std::string BroadcastSubscriptionRequest::toString() const
{
    return joynr::serializer::serializeToJson(*this);
}

const boost::optional<BroadcastFilterParameters>& BroadcastSubscriptionRequest::
        getFilterParameters() const
{
    return _filterParameters;
}

void BroadcastSubscriptionRequest::setFilterParameters(
        const BroadcastFilterParameters& filterParameters)
{
    this->_filterParameters = filterParameters;
}

void BroadcastSubscriptionRequest::setQos(std::shared_ptr<SubscriptionQos> qos)
{
    std::shared_ptr<OnChangeSubscriptionQos> onChangeQos =
            std::dynamic_pointer_cast<OnChangeSubscriptionQos>(qos);
    assert(onChangeQos);
    // force object slicing
    this->_qos = std::make_shared<OnChangeSubscriptionQos>(*onChangeQos);
}

} // namespace joynr
