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
#include <string>
#include <thread>

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "joynr/ContentWithDecayTime.h"
#include "joynr/TimePoint.h"

using namespace joynr;

TEST(ContentWithDecayTimeTest, messageWithDecayTime)
{
    using namespace std::chrono_literals;
    std::string message = "test-message";
    TimePoint decayTime = TimePoint::fromRelativeMs(2000);
    ContentWithDecayTime<std::string> mwdt(message, decayTime);
    EXPECT_TRUE(!mwdt.isExpired());
    EXPECT_GT(mwdt.getRemainingTtl(), 1500ms);
    EXPECT_LT(mwdt.getRemainingTtl(), 2500ms);
    EXPECT_EQ(decayTime, mwdt.getDecayTime());
    EXPECT_EQ(message, mwdt.getContent());
    std::this_thread::sleep_for(1s);
    EXPECT_GT(mwdt.getRemainingTtl(), 500ms);
    EXPECT_LT(mwdt.getRemainingTtl(), 1500ms);
    EXPECT_TRUE(!mwdt.isExpired());

    std::this_thread::sleep_for(1500ms);
    EXPECT_TRUE(mwdt.isExpired());
    EXPECT_LT(mwdt.getRemainingTtl(), 0ms);
}
