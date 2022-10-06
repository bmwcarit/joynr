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
#include "joynr/ISubscriptionManager.h"

#include <chrono>
#include <cstdint>
#include <limits>

#include "joynr/SubscriptionQos.h"

namespace joynr
{

std::int64_t ISubscriptionManager::convertExpiryDateIntoTtlMs(
        const SubscriptionQos& subscriptionQos)
{
    std::int64_t ttlMs = 0;

    if (subscriptionQos.getExpiryDateMs() == SubscriptionQos::NO_EXPIRY_DATE()) {
        ttlMs = std::numeric_limits<std::int64_t>::max(); // 2^63-1
    } else {
        std::int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::system_clock::now().time_since_epoch())
                                     .count();
        ttlMs = subscriptionQos.getExpiryDateMs() - nowMs;
    }

    return ttlMs;
}

} // namespace joynr
