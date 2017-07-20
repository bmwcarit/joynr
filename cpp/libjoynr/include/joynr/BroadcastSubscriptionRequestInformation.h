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
#ifndef BROADCASTSUBSCRIPTIONREQUESTINFORMATION_H
#define BROADCASTSUBSCRIPTIONREQUESTINFORMATION_H

#include <string>

#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/SubscriptionInformation.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

class JOYNR_EXPORT BroadcastSubscriptionRequestInformation : public BroadcastSubscriptionRequest,
                                                             public SubscriptionInformation
{
public:
    BroadcastSubscriptionRequestInformation() = default;
    ~BroadcastSubscriptionRequestInformation() override = default;

    BroadcastSubscriptionRequestInformation(const BroadcastSubscriptionRequestInformation&) =
            default;
    BroadcastSubscriptionRequestInformation(BroadcastSubscriptionRequestInformation&&) = default;

    BroadcastSubscriptionRequestInformation(
            const std::string& proxyParticipantId,
            const std::string& providerParticipantId,
            const BroadcastSubscriptionRequest& subscriptionRequest);

    BroadcastSubscriptionRequestInformation& operator=(
            const BroadcastSubscriptionRequestInformation&) = default;

    BroadcastSubscriptionRequestInformation& operator=(BroadcastSubscriptionRequestInformation&&) =
            default;

    bool operator==(
            const BroadcastSubscriptionRequestInformation& subscriptionRequestInformation) const;

    std::string toString() const;

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(muesli::BaseClass<BroadcastSubscriptionRequest>(this),
                muesli::BaseClass<SubscriptionInformation>(this));
    }
};

} // namespace joynr

MUESLI_REGISTER_TYPE(joynr::BroadcastSubscriptionRequestInformation,
                     "joynr.BroadcastSubscriptionRequestInformation")

#endif // BROADCASTSUBSCRIPTIONREQUESTINFORMATION_H
