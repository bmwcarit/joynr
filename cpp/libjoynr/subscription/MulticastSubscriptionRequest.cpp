/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#include "joynr/MulticastSubscriptionRequest.h"

#include "joynr/MulticastSubscriptionQos.h"

namespace joynr
{

MulticastSubscriptionRequest::MulticastSubscriptionRequest() : multicastId()
{
}

bool MulticastSubscriptionRequest::operator==(
        const MulticastSubscriptionRequest& subscriptionRequest) const
{
    bool equal = *(getQos()) == *(subscriptionRequest.getQos()) &&
                 getMulticastId() == subscriptionRequest.getMulticastId();
    return getSubscriptionId() == subscriptionRequest.getSubscriptionId() &&
           getSubscribeToName() == subscriptionRequest.getSubscribeToName() && equal;
}

std::string MulticastSubscriptionRequest::toString() const
{
    return joynr::serializer::serializeToJson(*this);
}

void MulticastSubscriptionRequest::setQos(std::shared_ptr<SubscriptionQos> qosLocal)
{
    std::shared_ptr<MulticastSubscriptionQos> onChangeQos =
            std::dynamic_pointer_cast<MulticastSubscriptionQos>(qosLocal);
    assert(onChangeQos);
    // force object slicing
    this->qos = std::make_shared<MulticastSubscriptionQos>(*onChangeQos);
}

const std::string& MulticastSubscriptionRequest::getMulticastId() const
{
    return multicastId;
}

void MulticastSubscriptionRequest::setMulticastId(const std::string& id)
{
    multicastId = id;
}

void MulticastSubscriptionRequest::setMulticastId(std::string&& id)
{
    multicastId = std::move(id);
}

} // namespace joynr
