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
#ifndef SUBSCRIPTIONPUBLICATION_H
#define SUBSCRIPTIONPUBLICATION_H

#include <vector>

#include "joynr/JoynrExport.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/Variant.h"

namespace joynr
{

class JOYNR_EXPORT SubscriptionPublication
{
public:
    SubscriptionPublication();

    SubscriptionPublication(const SubscriptionPublication&) = default;
    SubscriptionPublication& operator=(const SubscriptionPublication&) = default;

    SubscriptionPublication(SubscriptionPublication&&) = default;
    SubscriptionPublication& operator=(SubscriptionPublication&&) = default;

    bool operator==(const SubscriptionPublication& other) const;
    bool operator!=(const SubscriptionPublication& other) const;

    const static SubscriptionPublication NULL_RESPONSE;

    std::string getSubscriptionId() const;
    void setSubscriptionId(const std::string& subscriptionId);

    std::vector<Variant> getResponse() const;
    void setResponse(const std::vector<Variant>& response);

    const Variant& getError() const;
    void setError(const Variant& error);

private:
    std::string subscriptionId;
    std::vector<Variant> response;
    Variant error;
};

} // namespace joynr

#endif // SUBSCRIPTIONPUBLICATION_H
