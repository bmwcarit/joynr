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
#include "gtest/gtest.h"

#include "joynr/joynrlogging.h"
#include "joynr/DispatcherUtils.h"
#include <chrono>
#include <stdint.h>

using namespace joynr;

using namespace std::chrono;

class DispatcherUtilsTest : public ::testing::Test {
public:
    DispatcherUtilsTest() :
            logger(joynr_logging::Logging::getInstance()->getLogger(QString("TEST"), QString("DispatcherUtilsTest")))
    {
    }

    void SetUp(){
    }
    void TearDown(){

    }

protected:
    joynr_logging::Logger* logger;
};

TEST_F(DispatcherUtilsTest, maxAbsoluteTimeIsValid) {
    JoynrTimePoint maxDate(DispatcherUtils::getMaxAbsoluteTime());
    LOG_DEBUG(
                logger,
                QString("date: %1 [%2]")
                .arg(QString::fromStdString(DispatcherUtils::convertAbsoluteTimeToTtlString(maxDate)))
                .arg(duration_cast<milliseconds>(maxDate.time_since_epoch()).count())
    );
    JoynrTimePoint now = time_point_cast<milliseconds>(system_clock::now());
    EXPECT_LT(now, maxDate);
}

TEST_F(DispatcherUtilsTest, convertTtlToAbsoluteTimeReturnsValidDateTime) {
    JoynrTimePoint ttl60s(DispatcherUtils::convertTtlToAbsoluteTime(60000));
    LOG_DEBUG(
                logger,
                QString("60s TTL: %1 [%2]")
                .arg(QString::fromStdString(DispatcherUtils::convertAbsoluteTimeToTtlString(ttl60s)))
                .arg(duration_cast<milliseconds>(ttl60s.time_since_epoch()).count())
    );
    int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    int64_t ttl60sMillis = ttl60s.time_since_epoch().count();
    EXPECT_LT(now + 59000, ttl60sMillis);
}

TEST_F(DispatcherUtilsTest, convertTtlToAbsoluteTimeDetectsPositiveOverflow) {
    JoynrTimePoint ttlMaxInt64(DispatcherUtils::getMaxAbsoluteTime());
    LOG_DEBUG(
                logger,
                QString("ttlMaxInt64: %1 [%2]")
                .arg(QString::fromStdString(DispatcherUtils::convertAbsoluteTimeToTtlString(ttlMaxInt64)))
                .arg(duration_cast<milliseconds>(ttlMaxInt64.time_since_epoch()).count())
    );
    JoynrTimePoint now = time_point_cast<milliseconds>(system_clock::now());
    EXPECT_LT(now, ttlMaxInt64);
}

TEST_F(DispatcherUtilsTest, convertTtlToAbsoluteTimeDetectsNegativeOverflow) {
    JoynrTimePoint ttlMinInt64(DispatcherUtils::getMinAbsoluteTime());
    LOG_DEBUG(
                logger,
                QString("ttlMinInt64: %1 [%2]")
                .arg(QString::fromStdString(DispatcherUtils::convertAbsoluteTimeToTtlString(ttlMinInt64)))
                .arg(duration_cast<milliseconds>(ttlMinInt64.time_since_epoch()).count())
    );
    JoynrTimePoint now = time_point_cast<milliseconds>(system_clock::now());
    EXPECT_GT(now, ttlMinInt64);
}

TEST_F(DispatcherUtilsTest, convertTtlToAbsoluteTimeHandelsNegativeTtl) {
    JoynrTimePoint ttlNegative(DispatcherUtils::convertTtlToAbsoluteTime(-1));
    LOG_DEBUG(
                logger,
                QString("ttlNegative: %1 [%2]")
                .arg(QString::fromStdString(DispatcherUtils::convertAbsoluteTimeToTtlString(ttlNegative)))
                .arg(duration_cast<milliseconds>(ttlNegative.time_since_epoch()).count())
    );
    JoynrTimePoint now = time_point_cast<milliseconds>(system_clock::now());
    EXPECT_GT(now, ttlNegative);
}

TEST_F(DispatcherUtilsTest, convertTtlToAbsoluteTimeHandelsZeroTtl) {
    JoynrTimePoint ttlZero(DispatcherUtils::convertTtlToAbsoluteTime(0));
    LOG_DEBUG(
                logger,
                QString("ttlZero: %1 [%2]")
                .arg(QString::fromStdString(DispatcherUtils::convertAbsoluteTimeToString(ttlZero)))
                .arg(duration_cast<milliseconds>(ttlZero.time_since_epoch()).count())
    );
    JoynrTimePoint now = time_point_cast<milliseconds>(system_clock::now());
    EXPECT_TRUE(duration_cast<milliseconds>(now - ttlZero).count() < 10);

}

TEST_F(DispatcherUtilsTest, testJoynrTimePointWithWithHugeNumbers) {
    uint64_t hugeNumber = 9007199254740991;
    uint64_t nowInMs = DispatcherUtils::nowInMilliseconds();
    uint64_t deltaInMs = hugeNumber - nowInMs;
    JoynrTimePoint now{std::chrono::milliseconds(nowInMs)};
    JoynrTimePoint fixture{std::chrono::milliseconds(hugeNumber)};
    JoynrTimePoint delta{std::chrono::milliseconds(deltaInMs)};
    LOG_DEBUG(
                logger,
                QString("time delta between %1 and %2: %3")
                .arg(QString::number(hugeNumber))
                .arg(QString::number(nowInMs))
                .arg(QString::number(deltaInMs))
    );

    EXPECT_EQ(deltaInMs, duration_cast<milliseconds>(fixture.time_since_epoch()).count() - duration_cast<milliseconds>(now.time_since_epoch()).count());
    JoynrTimePoint calculatedDelta{fixture -now};
    EXPECT_EQ(delta, calculatedDelta);
    EXPECT_GT(fixture, now);

}
