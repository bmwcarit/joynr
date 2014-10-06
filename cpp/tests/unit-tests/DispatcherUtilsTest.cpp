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

#include <QtCore/QDateTime>

#include "joynr/joynrlogging.h"
#include "joynr/DispatcherUtils.h"


using namespace joynr;

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
    QDateTime date(DispatcherUtils::getMaxAbsoluteTime());
    LOG_DEBUG(
                logger,
                QString("date: %1 [%2]")
                    .arg(date.toString())
                    .arg(date.toMSecsSinceEpoch())
    );
    EXPECT_TRUE(date.isValid());
    EXPECT_LT(QDateTime::currentMSecsSinceEpoch(), date.toMSecsSinceEpoch());
}

TEST_F(DispatcherUtilsTest, convertTtlToAbsoluteTimeReturnsValidDateTime) {
    QDateTime ttl60s(DispatcherUtils::convertTtlToAbsoluteTime(60000));
    LOG_DEBUG(
                logger,
                QString("60s TTL: %1 [%2]")
                    .arg(ttl60s.toString())
                    .arg(ttl60s.toMSecsSinceEpoch())
    );
    EXPECT_TRUE(ttl60s.isValid());
    EXPECT_LT(QDateTime::currentMSecsSinceEpoch() + 59000, ttl60s.toMSecsSinceEpoch());
}

TEST_F(DispatcherUtilsTest, convertTtlToAbsoluteTimeDetectsPositiveOverflow) {
    QDateTime ttlMaxInt64(DispatcherUtils::convertTtlToAbsoluteTime(std::numeric_limits<qint64>::max()));
    LOG_DEBUG(
                logger,
                QString("ttlMaxInt64: %1 [%2]")
                    .arg(ttlMaxInt64.toString())
                    .arg(ttlMaxInt64.toMSecsSinceEpoch())
    );
    EXPECT_TRUE(ttlMaxInt64.isValid());
    EXPECT_LT(QDateTime::currentMSecsSinceEpoch(), ttlMaxInt64.toMSecsSinceEpoch());
}

TEST_F(DispatcherUtilsTest, convertTtlToAbsoluteTimeDetectsNegativeOverflow) {
    QDateTime ttlMinInt64(DispatcherUtils::convertTtlToAbsoluteTime(std::numeric_limits<qint64>::min()));
    LOG_DEBUG(
                logger,
                QString("ttlMaxInt64: %1 [%2]")
                    .arg(ttlMinInt64.toString())
                    .arg(ttlMinInt64.toMSecsSinceEpoch())
    );
    EXPECT_TRUE(ttlMinInt64.isValid());
    EXPECT_GT(QDateTime::currentMSecsSinceEpoch(), ttlMinInt64.toMSecsSinceEpoch());
}

TEST_F(DispatcherUtilsTest, convertTtlToAbsoluteTimeHandelsNegativeTtl) {
    QDateTime ttlNegative(DispatcherUtils::convertTtlToAbsoluteTime(-1));
    LOG_DEBUG(
                logger,
                QString("ttlNegative: %1 [%2]")
                    .arg(ttlNegative.toString())
                    .arg(ttlNegative.toMSecsSinceEpoch())
    );
    EXPECT_TRUE(ttlNegative.isValid());
    EXPECT_GT(QDateTime::currentMSecsSinceEpoch(), ttlNegative.toMSecsSinceEpoch());
}

TEST_F(DispatcherUtilsTest, convertTtlToAbsoluteTimeHandelsZeroTtl) {
    QDateTime ttlZero(DispatcherUtils::convertTtlToAbsoluteTime(0));
    LOG_DEBUG(
                logger,
                QString("ttlZero: %1 [%2]")
                    .arg(ttlZero.toString())
                    .arg(ttlZero.toMSecsSinceEpoch())
    );
    EXPECT_TRUE(ttlZero.isValid());
    EXPECT_TRUE(QDateTime::currentMSecsSinceEpoch() - ttlZero.toMSecsSinceEpoch() < 10);
}
