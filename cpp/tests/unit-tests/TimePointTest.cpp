/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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

#include <cstdint>

#include "tests/utils/Gtest.h"

#include "joynr/TimePoint.h"

using namespace joynr;

TEST(TimePointTest, addition)
{
    const TimePoint now = TimePoint::now();
    const std::int64_t offset = 1000;
    const TimePoint tp = now + offset;
    EXPECT_EQ(now.toMilliseconds() + offset, tp.toMilliseconds());
    EXPECT_GT(tp, now);
}

TEST(TimePointTest, additionDoesNotOverflow)
{
    const TimePoint tp = TimePoint::max() + 1;
    EXPECT_EQ(TimePoint::max().toMilliseconds(), tp.toMilliseconds());
    EXPECT_EQ(TimePoint::max(), tp);
}

TEST(TimePointTest, substraction)
{
    const TimePoint now = TimePoint::now();
    const std::int64_t offset = 1000;
    const TimePoint tp = now - std::chrono::milliseconds(offset);
    EXPECT_EQ(now.toMilliseconds() - offset, tp.toMilliseconds());
    EXPECT_LT(tp, now);
}

TEST(TimePointTest, fromRelativeMs)
{
    const TimePoint now = TimePoint::now();
    const std::int64_t offset = 1000;
    const TimePoint tp = TimePoint::fromRelativeMs(offset);
    EXPECT_LE(now.toMilliseconds() + offset, tp.toMilliseconds());
}

TEST(TimePointTest, fromAbsoluteMs)
{
    const TimePoint now = TimePoint::now();
    const TimePoint tp = TimePoint::fromAbsoluteMs(now.toMilliseconds());
    EXPECT_EQ(now.toMilliseconds(), tp.toMilliseconds());
    EXPECT_EQ(now, tp);
}

TEST(TimePointTest, comparison)
{
    const TimePoint now = TimePoint::now();
    const std::int64_t offset = 1000;
    const TimePoint tp = now + offset;
    EXPECT_LT(now, tp);
    EXPECT_GT(tp, now);
    EXPECT_NE(tp, now);
}
