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
#ifndef SUBSCRIPTIONSTOP_H
#define SUBSCRIPTIONSTOP_H

#include <string>

#include "joynr/JoynrExport.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

class JOYNR_EXPORT SubscriptionStop
{
public:
    SubscriptionStop();

    SubscriptionStop(const SubscriptionStop&) = default;
    SubscriptionStop& operator=(const SubscriptionStop&) = default;

    SubscriptionStop(SubscriptionStop&&) = default;
    SubscriptionStop& operator=(SubscriptionStop&& other) = default;

    bool operator==(const SubscriptionStop& other) const;
    bool operator!=(const SubscriptionStop& other) const;

    const std::string& getSubscriptionId() const;
    void setSubscriptionId(const std::string& subscriptionIdLocal);
    void setSubscriptionId(std::string&& subscriptionIdLocal);

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(MUESLI_NVP(subscriptionId));
    }

private:
    std::string subscriptionId;
};

} // namespace joynr

MUESLI_REGISTER_TYPE(joynr::SubscriptionStop, "joynr.SubscriptionStop")

#endif // SUBSCRIPTIONSTOP_H
