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
#include <chrono>
#include <thread>
#include <stdint.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "joynr/ContentWithDecayTime.h"
#include "joynr/JoynrMessage.h"

using namespace joynr;
using namespace std::chrono;

TEST(ContentWithDecayTimeTest, messageWithDecayTime)
{
    JoynrMessage message;
    int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    JoynrTimePoint decaytime{milliseconds(now + 2000)};
    ContentWithDecayTime<JoynrMessage> mwdt =  ContentWithDecayTime<JoynrMessage>(message, decaytime);
    EXPECT_TRUE(!mwdt.isExpired());
    EXPECT_GT(mwdt.getRemainingTtl_ms(), 1500);
    EXPECT_LT(mwdt.getRemainingTtl_ms(), 2500);
    EXPECT_EQ(decaytime, mwdt.getDecayTime());
    EXPECT_EQ(message, mwdt.getContent());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_GT( mwdt.getRemainingTtl_ms(), 500);
    EXPECT_LT( mwdt.getRemainingTtl_ms(), 1500 );
    EXPECT_TRUE(!mwdt.isExpired());

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    EXPECT_TRUE(mwdt.isExpired());
    EXPECT_LT(mwdt.getRemainingTtl_ms(), 0 );
}
