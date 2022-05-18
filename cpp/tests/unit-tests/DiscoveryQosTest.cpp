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

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "joynr/DiscoveryQos.h"

TEST(SubscriptionQos, configureDiscoveryQos)
{
    joynr::DiscoveryQos qos;

    std::int64_t expectedDiscoveryTimeout = 3;
    std::int64_t expectedCacheMaxAge = 4;
    std::int64_t expectedRetryInterval = 5;

    qos.setDiscoveryTimeoutMs(expectedDiscoveryTimeout);
    qos.setCacheMaxAgeMs(expectedCacheMaxAge);
    qos.setRetryIntervalMs(expectedRetryInterval);

    ASSERT_EQ(expectedDiscoveryTimeout, qos.getDiscoveryTimeoutMs());
    ASSERT_EQ(expectedCacheMaxAge, qos.getCacheMaxAgeMs());
    ASSERT_EQ(expectedRetryInterval, qos.getRetryIntervalMs());
}
