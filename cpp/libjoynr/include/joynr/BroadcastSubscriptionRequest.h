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
#ifndef BROADCASTSUBSCRIPTIONREQUEST_H
#define BROADCASTSUBSCRIPTIONREQUEST_H

#include <memory>
#include <string>

#include <boost/optional.hpp>

#include "joynr/SubscriptionRequest.h"

#include "joynr/BroadcastFilterParameters.h"
#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

class SubscriptionQos;

/** @class BroadcastSubscriptionRequest
  * @brief BroadcastSubscriptionRequest stores the information that is necessary to store
  * a broadcast subscription request on subscriber side, while Arbitration is handled.
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

    const boost::optional<BroadcastFilterParameters>& getFilterParameters() const;
    void setFilterParameters(const BroadcastFilterParameters& filterParameters);

    void setQos(std::shared_ptr<SubscriptionQos> _qos) override;

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(muesli::BaseClass<SubscriptionRequest>(this), MUESLI_NVP(_filterParameters));
    }

private:
    boost::optional<BroadcastFilterParameters> _filterParameters;

    ADD_LOGGER(BroadcastSubscriptionRequest)
};

} // namespace joynr

MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::BroadcastSubscriptionRequest,
                                 joynr::SubscriptionRequest,
                                 "joynr.BroadcastSubscriptionRequest")

#endif // BROADCASTSUBSCRIPTIONREQUEST_H
