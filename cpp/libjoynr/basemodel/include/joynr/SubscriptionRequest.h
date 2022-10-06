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
#ifndef SUBSCRIPTIONREQUEST_H
#define SUBSCRIPTIONREQUEST_H

#include <memory>
#include <string>

#include "joynr/JoynrExport.h"
#include "joynr/SubscriptionQos.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

/** @class SubscriptionRequest
 * @brief SubscriptionRequest stores the information that is necessary to store a
 * subscription request on
 * subscriber side, while Aribtration is handled.
 */
class JOYNR_EXPORT SubscriptionRequest
{
public:
    SubscriptionRequest();
    SubscriptionRequest(const SubscriptionRequest&) = default;
    SubscriptionRequest& operator=(const SubscriptionRequest&) = default;

    SubscriptionRequest(SubscriptionRequest&&) = default;
    SubscriptionRequest& operator=(SubscriptionRequest&&) = default;

    virtual ~SubscriptionRequest() = default;
    bool operator==(const SubscriptionRequest& subscriptionRequest) const;

    const std::string& getSubscriptionId() const;
    void setSubscriptionId(const std::string& idLocal);
    void setSubscriptionId(std::string&& idLocal);

    const std::string& getSubscribeToName() const;
    void setSubscribeToName(const std::string& subscribedToNameLocal);
    void setSubscribeToName(std::string&& subscribedToNameLocal);

    std::string toString() const;

    std::shared_ptr<SubscriptionQos> getQos() const;

    virtual void setQos(std::shared_ptr<SubscriptionQos> qosLocal);

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(MUESLI_NVP(subscriptionId), MUESLI_NVP(subscribedToName), MUESLI_NVP(qos));
    }

protected:
    /*
      SubscriptionRequest is used to store a subscription while Arbitration is still being done. To
      allow SubscriptionManager
      to notify about missedPublications for a subscription while offline, the SubscriptionId has to
      be determined when registering
      the subscription, and thus must be stored while waiting for arbitrations.
      */
    std::string subscriptionId;
    std::string subscribedToName;

    std::shared_ptr<SubscriptionQos> qos;
};

} // namespace joynr

MUESLI_REGISTER_TYPE(joynr::SubscriptionRequest, "joynr.SubscriptionRequest")

#endif // SUBSCRIPTIONREQUEST_H
