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
#ifndef SUBSCRIPTIONPUBLICATION_H
#define SUBSCRIPTIONPUBLICATION_H

#include <vector>

#include "joynr/BasePublication.h"
#include "joynr/BaseReply.h"
#include "joynr/JoynrExport.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

class JOYNR_EXPORT SubscriptionPublication : public BasePublication
{
public:
    SubscriptionPublication();

    explicit SubscriptionPublication(BaseReply&& reply);

    SubscriptionPublication(SubscriptionPublication&&) = default;
    SubscriptionPublication& operator=(SubscriptionPublication&&) = default;

    bool operator==(const SubscriptionPublication& other) const;
    bool operator!=(const SubscriptionPublication& other) const;

    const std::string& getSubscriptionId() const;
    void setSubscriptionId(const std::string& subscriptionIdLocal);
    void setSubscriptionId(std::string&& subscriptionIdLocal);

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(muesli::BaseClass<BasePublication>(this), MUESLI_NVP(subscriptionId));
    }

private:
    // printing SubscriptionPublication with google-test and google-mock
    friend void PrintTo(const SubscriptionPublication& subscriptionPublication, ::std::ostream* os);
    std::string subscriptionId;
};

} // namespace joynr

MUESLI_REGISTER_TYPE(joynr::SubscriptionPublication, "joynr.SubscriptionPublication")

#endif // SUBSCRIPTIONPUBLICATION_H
