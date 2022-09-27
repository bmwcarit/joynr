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

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"
#include <cstdint>

#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"

using namespace joynr;

class OnChangeWithKeepAliveSubscriptionQosTest : public ::testing::Test
{
public:
    OnChangeWithKeepAliveSubscriptionQosTest()
    {
    }
};

TEST_F(OnChangeWithKeepAliveSubscriptionQosTest, clearAlertAfterInterval)
{
    std::int64_t alertAfterIntervalMs =
            OnChangeWithKeepAliveSubscriptionQos::DEFAULT_MAX_INTERVAL_MS() + 4000;

    OnChangeWithKeepAliveSubscriptionQos onChangeWithKeepAliveSubscriptionQos;
    onChangeWithKeepAliveSubscriptionQos.setAlertAfterIntervalMs(alertAfterIntervalMs);

    EXPECT_EQ(alertAfterIntervalMs, onChangeWithKeepAliveSubscriptionQos.getAlertAfterIntervalMs());

    onChangeWithKeepAliveSubscriptionQos.clearAlertAfterInterval();
    EXPECT_EQ(OnChangeWithKeepAliveSubscriptionQos::NO_ALERT_AFTER_INTERVAL(),
              onChangeWithKeepAliveSubscriptionQos.getAlertAfterIntervalMs());
}

TEST_F(OnChangeWithKeepAliveSubscriptionQosTest, maxIntervalMsDefaultValueIsSetProperly)
{
    OnChangeWithKeepAliveSubscriptionQos onChangeWithKeepAliveSubscriptionQos;
    EXPECT_EQ(OnChangeWithKeepAliveSubscriptionQos::DEFAULT_MAX_INTERVAL_MS(),
              onChangeWithKeepAliveSubscriptionQos.getMaxIntervalMs());
}

TEST_F(OnChangeWithKeepAliveSubscriptionQosTest, maxIntervalMsMinimumValueIsSetProperly)
{
    std::int64_t validityMs = 100000;
    std::int64_t publicationTtlMs = 1000;
    std::int64_t alertAfterIntervalMs = 4000;
    std::int64_t tooSmallMaxIntervalMs =
            OnChangeWithKeepAliveSubscriptionQos::MIN_MAX_INTERVAL_MS() - 1;
    std::int64_t minIntervalMs = tooSmallMaxIntervalMs - 1;

    {
        OnChangeWithKeepAliveSubscriptionQos onChangeWithKeepAliveSubscriptionQos(
                validityMs,
                publicationTtlMs,
                minIntervalMs,
                tooSmallMaxIntervalMs,
                alertAfterIntervalMs);

        EXPECT_EQ(OnChangeWithKeepAliveSubscriptionQos::MIN_MAX_INTERVAL_MS(),
                  onChangeWithKeepAliveSubscriptionQos.getMaxIntervalMs());
    }
    {
        OnChangeWithKeepAliveSubscriptionQos onChangeWithKeepAliveSubscriptionQos;
        onChangeWithKeepAliveSubscriptionQos.setMaxIntervalMs(tooSmallMaxIntervalMs);

        EXPECT_EQ(OnChangeWithKeepAliveSubscriptionQos::MIN_MAX_INTERVAL_MS(),
                  onChangeWithKeepAliveSubscriptionQos.getMaxIntervalMs());
    }
}
