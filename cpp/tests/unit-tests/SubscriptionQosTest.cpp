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
#include <chrono>

#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/PeriodicSubscriptionQos.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"

using namespace joynr;

class SubscriptionQosTest : public ::testing::Test {
public:
    SubscriptionQosTest()
    {

    }

    std::int64_t nowPlusTimeSpan(const std::int64_t& timeSpan) {
        std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch()).count();
        return now + timeSpan;
    }
};


TEST_F(SubscriptionQosTest, createOnChangeSubscriptionQos)
{
    std::int64_t expiryDateMs = nowPlusTimeSpan(100000);
    std::int64_t publicationTtlMs = 2000;
    std::int64_t minIntervalMs = 100;
    OnChangeSubscriptionQos onChangeSubscriptionQos;
    onChangeSubscriptionQos.setExpiryDateMs(expiryDateMs);
    onChangeSubscriptionQos.setPublicationTtlMs(publicationTtlMs);
    onChangeSubscriptionQos.setMinIntervalMs(minIntervalMs);

    EXPECT_EQ(expiryDateMs, onChangeSubscriptionQos.getExpiryDateMs());
    EXPECT_EQ(publicationTtlMs, onChangeSubscriptionQos.getPublicationTtlMs());
    EXPECT_EQ(minIntervalMs, onChangeSubscriptionQos.getMinIntervalMs());
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TEST_F(SubscriptionQosTest, createOnChangeSubscriptionQosDeprecatedApi)
{
    std::int64_t expiryDateMs = nowPlusTimeSpan(100000);
    std::int64_t publicationTtlMs = 2000;
    std::int64_t minIntervalMs = 100;
    OnChangeSubscriptionQos onChangeSubscriptionQos;
    onChangeSubscriptionQos.setExpiryDate(expiryDateMs);
    onChangeSubscriptionQos.setPublicationTtl(publicationTtlMs);
    onChangeSubscriptionQos.setMinInterval(minIntervalMs);

    EXPECT_EQ(expiryDateMs, onChangeSubscriptionQos.getExpiryDate());
    EXPECT_EQ(publicationTtlMs, onChangeSubscriptionQos.getPublicationTtl());
    EXPECT_EQ(minIntervalMs, onChangeSubscriptionQos.getMinInterval());
}
#pragma GCC diagnostic pop

TEST_F(SubscriptionQosTest, createOnChangeSubscriptionQosWithValidity)
{
    std::int64_t validityMs = 100000;
    std::int64_t publicationTtlMs = 2000;
    std::int64_t minIntervalMs = 100;

    {
        std::int64_t expiryDateLowerBound = nowPlusTimeSpan(validityMs);
        OnChangeSubscriptionQos onChangeSubscriptionQos(validityMs, minIntervalMs);
        std::int64_t expiryDateUpperBound = nowPlusTimeSpan(validityMs);
        onChangeSubscriptionQos.setPublicationTtlMs(publicationTtlMs);

        EXPECT_LE(expiryDateLowerBound, onChangeSubscriptionQos.getExpiryDateMs());
        EXPECT_GE(expiryDateUpperBound, onChangeSubscriptionQos.getExpiryDateMs());
        EXPECT_EQ(publicationTtlMs, onChangeSubscriptionQos.getPublicationTtlMs());
        EXPECT_EQ(minIntervalMs, onChangeSubscriptionQos.getMinIntervalMs());
    }
    {
        OnChangeSubscriptionQos onChangeSubscriptionQos;
        std::int64_t expiryDateLowerBound = nowPlusTimeSpan(validityMs);
        onChangeSubscriptionQos.setValidityMs(validityMs);
        std::int64_t expiryDateUpperBound = nowPlusTimeSpan(validityMs);
        onChangeSubscriptionQos.setPublicationTtlMs(publicationTtlMs);
        onChangeSubscriptionQos.setMinIntervalMs(minIntervalMs);

        EXPECT_LE(expiryDateLowerBound, onChangeSubscriptionQos.getExpiryDateMs());
        EXPECT_GE(expiryDateUpperBound, onChangeSubscriptionQos.getExpiryDateMs());
        EXPECT_EQ(publicationTtlMs, onChangeSubscriptionQos.getPublicationTtlMs());
        EXPECT_EQ(minIntervalMs, onChangeSubscriptionQos.getMinIntervalMs());
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TEST_F(SubscriptionQosTest, createOnChangeSubscriptionQosWithValidityDeprecatedApi)
{
    std::int64_t validityMs = 100000;
    std::int64_t publicationTtlMs = 2000;
    std::int64_t minIntervalMs = 100;
    {
        std::int64_t expiryDateLowerBound = nowPlusTimeSpan(validityMs);
        OnChangeSubscriptionQos onChangeSubscriptionQos(validityMs, minIntervalMs);
        std::int64_t expiryDateUpperBound = nowPlusTimeSpan(validityMs);
        onChangeSubscriptionQos.setPublicationTtl(publicationTtlMs);

        EXPECT_LE(expiryDateLowerBound, onChangeSubscriptionQos.getExpiryDateMs());
        EXPECT_GE(expiryDateUpperBound, onChangeSubscriptionQos.getExpiryDateMs());
        EXPECT_EQ(publicationTtlMs, onChangeSubscriptionQos.getPublicationTtl());
        EXPECT_EQ(minIntervalMs, onChangeSubscriptionQos.getMinInterval());
    }
    {
        OnChangeSubscriptionQos onChangeSubscriptionQos;
        std::int64_t expiryDateLowerBound = nowPlusTimeSpan(validityMs);
        onChangeSubscriptionQos.setValidity(validityMs);
        std::int64_t expiryDateUpperBound = nowPlusTimeSpan(validityMs);
        onChangeSubscriptionQos.setPublicationTtl(publicationTtlMs);
        onChangeSubscriptionQos.setMinInterval(minIntervalMs);

        EXPECT_LE(expiryDateLowerBound, onChangeSubscriptionQos.getExpiryDateMs());
        EXPECT_GE(expiryDateUpperBound, onChangeSubscriptionQos.getExpiryDateMs());
        EXPECT_EQ(publicationTtlMs, onChangeSubscriptionQos.getPublicationTtl());
        EXPECT_EQ(minIntervalMs, onChangeSubscriptionQos.getMinInterval());
    }
}
#pragma GCC diagnostic pop

TEST_F(SubscriptionQosTest, createPeriodicSubscriptionQos)
{
    std::int64_t expiryDateMs = nowPlusTimeSpan(100000);
    std::int64_t publicationTtlMs = 2000;
    std::int64_t periodMs = 800;
    std::int64_t alertAfterIntervalMs = 3000;

    PeriodicSubscriptionQos periodicSubscriptionQos;
    periodicSubscriptionQos.setExpiryDateMs(expiryDateMs);
    periodicSubscriptionQos.setPublicationTtlMs(publicationTtlMs);
    periodicSubscriptionQos.setPeriodMs(periodMs);
    periodicSubscriptionQos.setAlertAfterIntervalMs(alertAfterIntervalMs);

    EXPECT_EQ(expiryDateMs, periodicSubscriptionQos.getExpiryDateMs());
    EXPECT_EQ(publicationTtlMs, periodicSubscriptionQos.getPublicationTtlMs());
    EXPECT_EQ(periodMs, periodicSubscriptionQos.getPeriodMs());
    EXPECT_EQ(alertAfterIntervalMs, periodicSubscriptionQos.getAlertAfterIntervalMs());
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TEST_F(SubscriptionQosTest, createPeriodicSubscriptionQosDeprecatedApi)
{
    std::int64_t expiryDateMs = nowPlusTimeSpan(100000);
    std::int64_t publicationTtlMs = 2000;
    std::int64_t periodMs = 800;
    std::int64_t alertAfterIntervalMs = 3000;

    PeriodicSubscriptionQos periodicSubscriptionQos;
    periodicSubscriptionQos.setExpiryDate(expiryDateMs);
    periodicSubscriptionQos.setPublicationTtl(publicationTtlMs);
    periodicSubscriptionQos.setPeriod(periodMs);
    periodicSubscriptionQos.setAlertAfterInterval(alertAfterIntervalMs);

    EXPECT_EQ(expiryDateMs, periodicSubscriptionQos.getExpiryDate());
    EXPECT_EQ(publicationTtlMs, periodicSubscriptionQos.getPublicationTtl());
    EXPECT_EQ(periodMs, periodicSubscriptionQos.getPeriod());
    EXPECT_EQ(alertAfterIntervalMs, periodicSubscriptionQos.getAlertAfterInterval());
}
#pragma GCC diagnostic pop

TEST_F(SubscriptionQosTest, createPeriodicSubscriptionQosWithValidity)
{
    std::int64_t validityMs = 1000;
    std::int64_t periodMs = 800;
    std::int64_t alertAfterIntervalMs = 3000;
    {
        std::int64_t expiryDateLowerBound = nowPlusTimeSpan(validityMs);
        PeriodicSubscriptionQos periodicSubscriptionQos(validityMs, periodMs, alertAfterIntervalMs);
        std::int64_t expiryDateUpperBound = nowPlusTimeSpan(validityMs);

        EXPECT_LE(expiryDateLowerBound, periodicSubscriptionQos.getExpiryDateMs());
        EXPECT_GE(expiryDateUpperBound, periodicSubscriptionQos.getExpiryDateMs());
        EXPECT_EQ(periodMs, periodicSubscriptionQos.getPeriodMs());
        EXPECT_EQ(alertAfterIntervalMs, periodicSubscriptionQos.getAlertAfterIntervalMs());
    }
    {
        PeriodicSubscriptionQos periodicSubscriptionQos;
        std::int64_t expiryDateLowerBound = nowPlusTimeSpan(validityMs);
        periodicSubscriptionQos.setValidityMs(validityMs);
        std::int64_t expiryDateUpperBound = nowPlusTimeSpan(validityMs);
        periodicSubscriptionQos.setPeriodMs(periodMs);
        periodicSubscriptionQos.setAlertAfterIntervalMs(alertAfterIntervalMs);

        EXPECT_LE(expiryDateLowerBound, periodicSubscriptionQos.getExpiryDateMs());
        EXPECT_GE(expiryDateUpperBound, periodicSubscriptionQos.getExpiryDateMs());
        EXPECT_EQ(periodMs, periodicSubscriptionQos.getPeriodMs());
        EXPECT_EQ(alertAfterIntervalMs, periodicSubscriptionQos.getAlertAfterIntervalMs());
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TEST_F(SubscriptionQosTest, createPeriodicSubscriptionQosWithValidityDeprecatedApi)
{
    std::int64_t validityMs = 1000;
    std::int64_t periodMs = 800;
    std::int64_t alertAfterIntervalMs = 3000;
    {
        std::int64_t expiryDateLowerBound = nowPlusTimeSpan(validityMs);
        PeriodicSubscriptionQos periodicSubscriptionQos(validityMs, periodMs, alertAfterIntervalMs);
        std::int64_t expiryDateUpperBound = nowPlusTimeSpan(validityMs);

        EXPECT_LE(expiryDateLowerBound, periodicSubscriptionQos.getExpiryDateMs());
        EXPECT_GE(expiryDateUpperBound, periodicSubscriptionQos.getExpiryDateMs());
        EXPECT_EQ(periodMs, periodicSubscriptionQos.getPeriod());
        EXPECT_EQ(alertAfterIntervalMs, periodicSubscriptionQos.getAlertAfterInterval());
    }
    {
        PeriodicSubscriptionQos periodicSubscriptionQos;
        std::int64_t expiryDateLowerBound = nowPlusTimeSpan(validityMs);
        periodicSubscriptionQos.setValidity(validityMs);
        std::int64_t expiryDateUpperBound = nowPlusTimeSpan(validityMs);
        periodicSubscriptionQos.setPeriod(periodMs);
        periodicSubscriptionQos.setAlertAfterInterval(alertAfterIntervalMs);

        EXPECT_LE(expiryDateLowerBound, periodicSubscriptionQos.getExpiryDateMs());
        EXPECT_GE(expiryDateUpperBound, periodicSubscriptionQos.getExpiryDateMs());
        EXPECT_EQ(periodMs, periodicSubscriptionQos.getPeriod());
        EXPECT_EQ(alertAfterIntervalMs, periodicSubscriptionQos.getAlertAfterInterval());
    }
}
#pragma GCC diagnostic pop

TEST_F(SubscriptionQosTest, createOnChangeWithKeepAliveSubscriptionQos)
{
    std::int64_t expiryDateMs = nowPlusTimeSpan(100000);
    std::int64_t publicationTtlMs = 2000;
    std::int64_t alertAfterIntervalMs = 4000;
    std::int64_t maxIntervalMs = 3000;
    std::int64_t minIntervalMs = 100;

    OnChangeWithKeepAliveSubscriptionQos onChangeWithKeepAliveSubscriptionQos;
    onChangeWithKeepAliveSubscriptionQos.setExpiryDateMs(expiryDateMs);
    onChangeWithKeepAliveSubscriptionQos.setPublicationTtlMs(publicationTtlMs);
    onChangeWithKeepAliveSubscriptionQos.setMaxIntervalMs(maxIntervalMs);
    onChangeWithKeepAliveSubscriptionQos.setMinIntervalMs(minIntervalMs);
    onChangeWithKeepAliveSubscriptionQos.setAlertAfterIntervalMs(alertAfterIntervalMs);

    EXPECT_EQ(expiryDateMs, onChangeWithKeepAliveSubscriptionQos.getExpiryDateMs());
    EXPECT_EQ(publicationTtlMs, onChangeWithKeepAliveSubscriptionQos.getPublicationTtlMs());
    EXPECT_EQ(maxIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMaxIntervalMs());
    EXPECT_EQ(minIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMinIntervalMs());
    EXPECT_EQ(alertAfterIntervalMs, onChangeWithKeepAliveSubscriptionQos.getAlertAfterIntervalMs());
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TEST_F(SubscriptionQosTest, createOnChangeWithKeepAliveSubscriptionQosDeprecatedApi)
{
    std::int64_t expiryDateMs = nowPlusTimeSpan(100000);
    std::int64_t publicationTtlMs = 2000;
    std::int64_t alertAfterIntervalMs = 4000;
    std::int64_t maxIntervalMs = 3000;
    std::int64_t minIntervalMs = 100;

    OnChangeWithKeepAliveSubscriptionQos onChangeWithKeepAliveSubscriptionQos;
    onChangeWithKeepAliveSubscriptionQos.setExpiryDate(expiryDateMs);
    onChangeWithKeepAliveSubscriptionQos.setPublicationTtl(publicationTtlMs);
    onChangeWithKeepAliveSubscriptionQos.setMaxInterval(maxIntervalMs);
    onChangeWithKeepAliveSubscriptionQos.setMinInterval(minIntervalMs);
    onChangeWithKeepAliveSubscriptionQos.setAlertAfterInterval(alertAfterIntervalMs);

    EXPECT_EQ(expiryDateMs, onChangeWithKeepAliveSubscriptionQos.getExpiryDate());
    EXPECT_EQ(publicationTtlMs, onChangeWithKeepAliveSubscriptionQos.getPublicationTtl());
    EXPECT_EQ(maxIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMaxInterval());
    EXPECT_EQ(minIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMinInterval());
    EXPECT_EQ(alertAfterIntervalMs, onChangeWithKeepAliveSubscriptionQos.getAlertAfterInterval());
}
#pragma GCC diagnostic pop

TEST_F(SubscriptionQosTest, alertAfterIntervalAdjustedIfSmallerThanPeriod)
{
    std::int64_t periodMs = 5000;
    std::int64_t alertAfterIntervalMs = 1000;

    PeriodicSubscriptionQos periodicSubscriptionQos;
    periodicSubscriptionQos.setPeriodMs(periodMs);
    periodicSubscriptionQos.setAlertAfterIntervalMs(alertAfterIntervalMs);

    EXPECT_EQ(periodMs, periodicSubscriptionQos.getPeriodMs());
    EXPECT_EQ(periodMs, periodicSubscriptionQos.getAlertAfterIntervalMs());
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TEST_F(SubscriptionQosTest, alertAfterIntervalAdjustedIfSmallerThanPeriodDeprecatedApi)
{
    std::int64_t periodMs = 5000;
    std::int64_t alertAfterIntervalMs = 1000;

    PeriodicSubscriptionQos periodicSubscriptionQos;
    periodicSubscriptionQos.setPeriod(periodMs);
    periodicSubscriptionQos.setAlertAfterInterval(alertAfterIntervalMs);

    EXPECT_EQ(periodMs, periodicSubscriptionQos.getPeriod());
    EXPECT_EQ(periodMs, periodicSubscriptionQos.getAlertAfterInterval());
}

TEST_F(SubscriptionQosTest, alertAfterIntervalAdjustedIfSmallerThanMaxInterval)
{
    std::int64_t alertAfterIntervalMs = 4000;
    std::int64_t maxIntervalMs = 5000;

    OnChangeWithKeepAliveSubscriptionQos onChangeWithKeepAliveSubscriptionQos;
    onChangeWithKeepAliveSubscriptionQos.setMaxIntervalMs(maxIntervalMs);
    onChangeWithKeepAliveSubscriptionQos.setAlertAfterIntervalMs(alertAfterIntervalMs);

    EXPECT_EQ(maxIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMaxIntervalMs());
    EXPECT_EQ(maxIntervalMs, onChangeWithKeepAliveSubscriptionQos.getAlertAfterIntervalMs());
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TEST_F(SubscriptionQosTest, alertAfterIntervalAdjustedIfSmallerThanMaxIntervalDeprecatedApi)
{
    std::int64_t alertAfterIntervalMs = 4000;
    std::int64_t maxIntervalMs = 5000;

    OnChangeWithKeepAliveSubscriptionQos onChangeWithKeepAliveSubscriptionQos;
    onChangeWithKeepAliveSubscriptionQos.setMaxInterval(maxIntervalMs);
    onChangeWithKeepAliveSubscriptionQos.setAlertAfterInterval(alertAfterIntervalMs);

    EXPECT_EQ(maxIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMaxInterval());
    EXPECT_EQ(maxIntervalMs, onChangeWithKeepAliveSubscriptionQos.getAlertAfterInterval());
}
#pragma GCC diagnostic pop

TEST_F(SubscriptionQosTest, maxIntervalAdjustedIfSmallerThanMinInterval)
{
    std::int64_t maxIntervalMs = 1000;
    std::int64_t minIntervalMs = 2000;

    OnChangeWithKeepAliveSubscriptionQos onChangeWithKeepAliveSubscriptionQos;
    onChangeWithKeepAliveSubscriptionQos.setMaxIntervalMs(maxIntervalMs);
    onChangeWithKeepAliveSubscriptionQos.setMinIntervalMs(minIntervalMs);

    EXPECT_EQ(minIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMaxIntervalMs());
    EXPECT_EQ(minIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMinIntervalMs());
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TEST_F(SubscriptionQosTest, maxIntervalAdjustedIfSmallerThanMinIntervalDeprecatedApi)
{
    std::int64_t maxIntervalMs = 1000;
    std::int64_t minIntervalMs = 2000;

    OnChangeWithKeepAliveSubscriptionQos onChangeWithKeepAliveSubscriptionQos;
    onChangeWithKeepAliveSubscriptionQos.setMaxInterval(maxIntervalMs);
    onChangeWithKeepAliveSubscriptionQos.setMinInterval(minIntervalMs);

    EXPECT_EQ(minIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMaxInterval());
    EXPECT_EQ(minIntervalMs, onChangeWithKeepAliveSubscriptionQos.getMinInterval());
}
#pragma GCC diagnostic pop
