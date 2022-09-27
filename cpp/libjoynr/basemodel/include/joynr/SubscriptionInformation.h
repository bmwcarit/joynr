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
#ifndef SUBSCRIPTIONINFORMATION_H
#define SUBSCRIPTIONINFORMATION_H

#include <string>

#include "joynr/JoynrExport.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

/** @class SubscriptionInformation
 * @brief SubscriptionInformation stores information about the context in which a concrete
 * subscription request is used.
 */

class JOYNR_EXPORT SubscriptionInformation
{

public:
    SubscriptionInformation();
    SubscriptionInformation(const std::string& proxyParticipantId,
                            const std::string& providerParticipantId);
    SubscriptionInformation(const SubscriptionInformation& subscriptionInformation);
    SubscriptionInformation& operator=(const SubscriptionInformation& subscriptionInformation);
    virtual ~SubscriptionInformation() = default;
    bool operator==(const SubscriptionInformation& subscriptionInformation) const;

    const std::string& getProxyId() const;
    void setProxyId(const std::string& id);

    const std::string& getProviderId() const;
    void setProviderId(const std::string& id);

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(MUESLI_NVP(proxyId), MUESLI_NVP(providerId));
    }

private:
    std::string proxyId;
    std::string providerId;
};

} // namespace joynr

MUESLI_REGISTER_TYPE(joynr::SubscriptionInformation, "joynr.SubscriptionInformation")

#endif // SUBSCRIPTIONINFORMATION_H
