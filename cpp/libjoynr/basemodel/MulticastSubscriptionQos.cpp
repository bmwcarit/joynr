/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include "joynr/MulticastSubscriptionQos.h"

namespace joynr
{

MulticastSubscriptionQos::MulticastSubscriptionQos(std::int64_t validityMs)
        : SubscriptionQos(validityMs)
{
}

/** @brief Assignment operator */
MulticastSubscriptionQos& MulticastSubscriptionQos::operator=(
        const MulticastSubscriptionQos& subscriptionQos)
{
    setExpiryDateMs(subscriptionQos.getExpiryDateMs());
    return *this;
}

/** @brief Equality operator */
bool MulticastSubscriptionQos::operator==(const MulticastSubscriptionQos& subscriptionQos) const
{
    return subscriptionQos.getExpiryDateMs() == getExpiryDateMs();
}

} // namespace joynr
