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
#ifndef MULTICASTSUBSCRIPTIONREQUEST_H
#define MULTICASTSUBSCRIPTIONREQUEST_H

#include <memory>
#include <string>

#include "joynr/SubscriptionRequest.h"

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

class SubscriptionQos;

/** @class MulticastSubscriptionRequest
 * @brief MulticastSubscriptionRequest stores the information that is necessary to store a
 * multicast
 * subscription request on subscriber side, while Arbitration is handled.
 */
class JOYNR_EXPORT MulticastSubscriptionRequest : public SubscriptionRequest
{
public:
    MulticastSubscriptionRequest();

    MulticastSubscriptionRequest(const MulticastSubscriptionRequest&) = default;
    MulticastSubscriptionRequest& operator=(const MulticastSubscriptionRequest&) = default;

    MulticastSubscriptionRequest(MulticastSubscriptionRequest&&) = default;
    MulticastSubscriptionRequest& operator=(MulticastSubscriptionRequest&&) = default;

    bool operator==(const MulticastSubscriptionRequest& subscriptionRequest) const;

    std::string toString() const;

    void setQos(std::shared_ptr<SubscriptionQos> qosLocal) override;

    const std::string& getMulticastId() const;
    void setMulticastId(const std::string& id);
    void setMulticastId(std::string&& id);

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(muesli::BaseClass<SubscriptionRequest>(this), MUESLI_NVP(multicastId));
    }

private:
    std::string multicastId;
};

} // namespace joynr

MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::MulticastSubscriptionRequest,
                                 joynr::SubscriptionRequest,
                                 "joynr.MulticastSubscriptionRequest")

#endif // MULTICASTSUBSCRIPTIONREQUEST_H
