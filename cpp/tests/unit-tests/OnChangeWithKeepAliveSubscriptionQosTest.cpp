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
#include <cstdint>

#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"

using namespace joynr;

class OnChangeWithKeepAliveSubscriptionQosTest : public ::testing::Test {
public:
    OnChangeWithKeepAliveSubscriptionQosTest()
    {
    }
};

TEST_F(OnChangeWithKeepAliveSubscriptionQosTest, clearAlertAfterInterval)
{
    std::int64_t alertAfterIntervalMs = 4000;

    OnChangeWithKeepAliveSubscriptionQos onChangeWithKeepAliveSubscriptionQos;
    onChangeWithKeepAliveSubscriptionQos.setAlertAfterIntervalMs(alertAfterIntervalMs);

    EXPECT_EQ(alertAfterIntervalMs, onChangeWithKeepAliveSubscriptionQos.getAlertAfterIntervalMs());

    onChangeWithKeepAliveSubscriptionQos.clearAlertAfterInterval();
    EXPECT_EQ(
            OnChangeWithKeepAliveSubscriptionQos::NO_ALERT_AFTER_INTERVAL(),
            onChangeWithKeepAliveSubscriptionQos.getAlertAfterIntervalMs()
    );
}
