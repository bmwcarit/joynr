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
#ifndef BROADCASTSUBSCRIPTIONREQUEST_H
#define BROADCASTSUBSCRIPTIONREQUEST_H

#include <string>
#include <memory>
#include <boost/type_index.hpp>

#include "joynr/SubscriptionRequest.h"
#include "joynr/BroadcastFilterParameters.h"
#include "joynr/OnChangeSubscriptionQos.h"

#include "joynr/Logger.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

/** @class BroadcastSubscriptionRequest
  * @brief SubscriptionRequest stores the information that is necessary to store a broadcast
  * subscription-Request on subscriber side, while Aribtration is handled.
  */

class JOYNR_EXPORT BroadcastSubscriptionRequest : public SubscriptionRequest
{
public:
    BroadcastSubscriptionRequest();

    BroadcastSubscriptionRequest(const BroadcastSubscriptionRequest&) = default;
    BroadcastSubscriptionRequest& operator=(const BroadcastSubscriptionRequest&) = default;

    BroadcastSubscriptionRequest(BroadcastSubscriptionRequest&&) = default;
    BroadcastSubscriptionRequest& operator=(BroadcastSubscriptionRequest&&) = default;

    bool operator==(const BroadcastSubscriptionRequest& subscriptionRequest) const;

    std::string toString() const;

    BroadcastFilterParameters getFilterParameters() const;
    void setFilterParameters(const BroadcastFilterParameters& filterParameters);

    void setQos(std::shared_ptr<SubscriptionQos> qos) override;

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(muesli::BaseClass<SubscriptionRequest>(this), MUESLI_NVP(filterParameters));
    }

private:
    BroadcastFilterParameters filterParameters;

    ADD_LOGGER(BroadcastSubscriptionRequest);
};

} // namespace joynr

MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::BroadcastSubscriptionRequest,
                                 joynr::SubscriptionRequest,
                                 "joynr.BroadcastSubscriptionRequest")

#endif // BROADCASTSUBSCRIPTIONREQUEST_H
