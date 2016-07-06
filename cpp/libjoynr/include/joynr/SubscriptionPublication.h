/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include "joynr/BaseReply.h"
#include "joynr/JoynrExport.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/Variant.h"

namespace joynr
{

class JOYNR_EXPORT SubscriptionPublication : public BaseReply
{
public:
    SubscriptionPublication();

    explicit SubscriptionPublication(BaseReply&& reply);

    SubscriptionPublication(const SubscriptionPublication&) = default;
    SubscriptionPublication& operator=(const SubscriptionPublication&) = default;

    SubscriptionPublication(SubscriptionPublication&&) = default;
    SubscriptionPublication& operator=(SubscriptionPublication&&) = default;

    bool operator==(const SubscriptionPublication& other) const;
    bool operator!=(const SubscriptionPublication& other) const;

    const static SubscriptionPublication NULL_RESPONSE;

    std::string getSubscriptionId() const;
    void setSubscriptionId(const std::string& subscriptionId);

    std::vector<Variant> getResponseVariant() const;
    void setResponseVariant(const std::vector<Variant>& response);

    std::shared_ptr<exceptions::JoynrRuntimeException> getError() const;
    void setError(std::shared_ptr<exceptions::JoynrRuntimeException> error);

private:
    std::string subscriptionId;
    std::shared_ptr<exceptions::JoynrRuntimeException> error;
    std::vector<Variant> responseVariant;
};

} // namespace joynr

#endif // SUBSCRIPTIONPUBLICATION_H
