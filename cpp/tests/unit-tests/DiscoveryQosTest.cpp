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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/DiscoveryQos.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TEST(SubscriptionQos, Constants_DeprecatedApi)
{
    ASSERT_EQ(joynr::DiscoveryQos::DEFAULT_DISCOVERYTIMEOUT_MS(), joynr::DiscoveryQos::DEFAULT_DISCOVERYTIMEOUT());
    ASSERT_EQ(joynr::DiscoveryQos::DEFAULT_RETRYINTERVAL_MS(), joynr::DiscoveryQos::DEFAULT_RETRYINTERVAL());
    ASSERT_EQ(joynr::DiscoveryQos::DEFAULT_CACHEMAXAGE_MS(), joynr::DiscoveryQos::DEFAULT_CACHEMAXAGE());
}
#pragma GCC diagnostic pop

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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TEST(SubscriptionQos, configureDiscoveryQos_forwardToNewAPI)
{
    joynr::DiscoveryQos qos;

    std::int64_t expectedDiscoveryTimeout = 3;
    std::int64_t expectedCacheMaxAge = 4;
    std::int64_t expectedRetryInterval = 5;

    qos.setDiscoveryTimeout(expectedDiscoveryTimeout);
    qos.setCacheMaxAge(expectedCacheMaxAge);
    qos.setRetryInterval(expectedRetryInterval);

    ASSERT_EQ(expectedDiscoveryTimeout, qos.getDiscoveryTimeoutMs());
    ASSERT_EQ(expectedCacheMaxAge, qos.getCacheMaxAgeMs());
    ASSERT_EQ(expectedRetryInterval, qos.getRetryIntervalMs());
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TEST(SubscriptionQos, configureDiscoveryQos_DeprecatedApi)
{
    joynr::DiscoveryQos qos;

    std::int64_t expectedDiscoveryTimeout = 3;
    std::int64_t expectedCacheMaxAge = 4;
    std::int64_t expectedRetryInterval = 5;

    qos.setDiscoveryTimeout(expectedDiscoveryTimeout);
    qos.setCacheMaxAge(expectedCacheMaxAge);
    qos.setRetryInterval(expectedRetryInterval);

    ASSERT_EQ(expectedDiscoveryTimeout, qos.getDiscoveryTimeout());
    ASSERT_EQ(expectedCacheMaxAge, qos.getCacheMaxAge());
    ASSERT_EQ(expectedRetryInterval, qos.getRetryInterval());
}
#pragma GCC diagnostic pop
