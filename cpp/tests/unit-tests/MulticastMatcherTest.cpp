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

#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/MulticastMatcher.h"

TEST(MulticastMatcherTest, checkSingleLevelWildcard)
{
    std::string multicastId = "provider/broad/+";
    joynr::MulticastMatcher m(multicastId);

    
    EXPECT_TRUE(m.doesMatch("provider/broad/AnyPart"));
    EXPECT_FALSE(m.doesMatch("provider/broad"));
    EXPECT_FALSE(m.doesMatch("provider/broad/AnyPart/NoMatch"));
}

TEST(MulticastMatcherTest, checkMultipleSingleLevelWildcard)
{
    std::string multicastId = "provider/broad/+/part1/+";
    joynr::MulticastMatcher m(multicastId);

    EXPECT_TRUE(m.doesMatch("provider/broad/AnyPart/part1/AnyPart"));

    EXPECT_FALSE(m.doesMatch("provider/broad"));
    EXPECT_FALSE(m.doesMatch("provider/broad/AnyPart"));
    EXPECT_FALSE(m.doesMatch("provider/broad/AnyPart/part1"));
    EXPECT_FALSE(m.doesMatch("provider/broad/AnyPart/NoMatch/AnyPart"));
    EXPECT_FALSE(m.doesMatch("provider/broad/AnyPart/part1/AnyPart/NoMatch"));
}

TEST(MulticastMatcherTest, checkMultiLevelWildcard)
{
    std::string multicastId = "provider/broad/*";
    joynr::MulticastMatcher m(multicastId);

    EXPECT_TRUE(m.doesMatch("provider/broad/AnyPart"));
    EXPECT_TRUE(m.doesMatch("provider/broad"));
    EXPECT_TRUE(m.doesMatch("provider/broad/AnyPart/AnyPart/AnyPart/AnyPart"));
    EXPECT_FALSE(m.doesMatch("provider/NoMatch"));
}
