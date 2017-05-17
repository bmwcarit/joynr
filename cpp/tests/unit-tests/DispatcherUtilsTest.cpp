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
#include <chrono>
#include <cstdint>

#include <gtest/gtest.h>

#include "joynr/DispatcherUtils.h"

using namespace joynr;

class DispatcherUtilsTest : public ::testing::Test {
protected:
    ADD_LOGGER(DispatcherUtilsTest);
};

INIT_LOGGER(DispatcherUtilsTest);

TEST_F(DispatcherUtilsTest, maxAbsoluteTimeIsValid) {
    JoynrTimePoint maxDate(DispatcherUtils::getMaxAbsoluteTime());
    JOYNR_LOG_DEBUG(logger, "date: {}  [{}]",DispatcherUtils::convertAbsoluteTimeToTtlString(maxDate),std::chrono::duration_cast<std::chrono::milliseconds>(maxDate.time_since_epoch()).count());
    JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    EXPECT_LT(now, maxDate);
}

TEST_F(DispatcherUtilsTest, convertTtlToAbsoluteTimeReturnsValidDateTime) {
    JoynrTimePoint ttl60s(DispatcherUtils::convertTtlToAbsoluteTime(60000));
    JOYNR_LOG_DEBUG(logger, "60s TTL: {}  [{}]",DispatcherUtils::convertAbsoluteTimeToTtlString(ttl60s),std::chrono::duration_cast<std::chrono::milliseconds>(ttl60s.time_since_epoch()).count());
    std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::int64_t ttl60sMillis = ttl60s.time_since_epoch().count();
    EXPECT_LT(now + 59000, ttl60sMillis);
    EXPECT_GT(now + 61000, ttl60sMillis);
}

TEST_F(DispatcherUtilsTest, convertTtlToAbsoluteTimeDetectsPositiveOverflow) {
    JoynrTimePoint ttlMaxInt64(DispatcherUtils::getMaxAbsoluteTime());
    JOYNR_LOG_DEBUG(logger, "ttlMaxInt64: {}  [{}]",DispatcherUtils::convertAbsoluteTimeToTtlString(ttlMaxInt64),std::chrono::duration_cast<std::chrono::milliseconds>(ttlMaxInt64.time_since_epoch()).count());
    JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    EXPECT_LT(now, ttlMaxInt64);
}

TEST_F(DispatcherUtilsTest, convertTtlToAbsoluteTimeDetectsNegativeOverflow) {
    JoynrTimePoint ttlMinInt64(DispatcherUtils::getMinAbsoluteTime());
    JOYNR_LOG_DEBUG(logger, "ttlMinInt64: {}  [{}]",DispatcherUtils::convertAbsoluteTimeToTtlString(ttlMinInt64), std::chrono::duration_cast<std::chrono::milliseconds>(ttlMinInt64.time_since_epoch()).count());
    JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    EXPECT_GT(now, ttlMinInt64);
}

TEST_F(DispatcherUtilsTest, convertTtlToAbsoluteTimeHandelsNegativeTtl) {
    JoynrTimePoint ttlNegative(DispatcherUtils::convertTtlToAbsoluteTime(-1));
    JOYNR_LOG_DEBUG(logger, "ttlNegative: {}  [{}]",DispatcherUtils::convertAbsoluteTimeToTtlString(ttlNegative),std::chrono::duration_cast<std::chrono::milliseconds>(ttlNegative.time_since_epoch()).count());
    JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    EXPECT_GT(now, ttlNegative);
}

TEST_F(DispatcherUtilsTest, convertTtlToAbsoluteTimeHandelsZeroTtl) {
    JoynrTimePoint ttlZero(DispatcherUtils::convertTtlToAbsoluteTime(0));
    JOYNR_LOG_DEBUG(logger, "ttlZero: {}  [{}]",DispatcherUtils::convertAbsoluteTimeToString(ttlZero), std::chrono::duration_cast<std::chrono::milliseconds>(ttlZero.time_since_epoch()).count());
    JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    EXPECT_TRUE(std::chrono::duration_cast<std::chrono::milliseconds>(now - ttlZero).count() < 10);

}

TEST_F(DispatcherUtilsTest, testJoynrTimePointWithWithHugeNumbers) {
    std::uint64_t hugeNumber = 9007199254740991;
    std::uint64_t nowInMs = DispatcherUtils::nowInMilliseconds();
    std::uint64_t deltaInMs = hugeNumber - nowInMs;
    JoynrTimePoint now{std::chrono::milliseconds(nowInMs)};
    JoynrTimePoint fixture{std::chrono::milliseconds(hugeNumber)};
    JoynrTimePoint delta{std::chrono::milliseconds(deltaInMs)};
    JOYNR_LOG_DEBUG(logger, "time delta between {} and {}: {}",hugeNumber,nowInMs,deltaInMs);

    EXPECT_EQ(deltaInMs, std::chrono::duration_cast<std::chrono::milliseconds>(
                  fixture.time_since_epoch()).count() -std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
              );
    JoynrTimePoint calculatedDelta{fixture -now};
    EXPECT_EQ(delta, calculatedDelta);
    EXPECT_GT(fixture, now);

}
