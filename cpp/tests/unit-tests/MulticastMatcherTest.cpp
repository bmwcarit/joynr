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

#include <string>

#include "tests/utils/Gtest.h"
#include "tests/utils/Gmock.h"

#include "joynr/MulticastMatcher.h"

TEST(MulticastMatcherTest, replaceLeadingSingleLevelWildcard)
{
    std::string multicastId = "+/one/two/three";
    joynr::MulticastMatcher matcher(multicastId);

    EXPECT_TRUE(matcher.doesMatch("anything/one/two/three"));
    EXPECT_TRUE(matcher.doesMatch("1/one/two/three"));
    EXPECT_TRUE(matcher.doesMatch("hello/one/two/three"));

    EXPECT_FALSE(matcher.doesMatch("one/two/three"));
    EXPECT_FALSE(matcher.doesMatch("one/any/two/three"));
    EXPECT_FALSE(matcher.doesMatch("/one/two/three"));
    EXPECT_FALSE(matcher.doesMatch("five/six/one/two/three"));
}

TEST(MulticastMatcherTest, singleLevelWildcardInMiddle)
{
    std::string multicastId = "one/+/three";
    joynr::MulticastMatcher matcher(multicastId);

    EXPECT_TRUE(matcher.doesMatch("one/anything/three"));
    EXPECT_TRUE(matcher.doesMatch("one/1/three"));
    EXPECT_TRUE(matcher.doesMatch("one/here/three"));

    EXPECT_FALSE(matcher.doesMatch("one/two/four/three"));
    EXPECT_FALSE(matcher.doesMatch("one/three"));
}

TEST(MulticastMatcherTest, singleLevelWildcardAtEnd)
{
    std::string multicastId = "one/two/+";
    joynr::MulticastMatcher matcher(multicastId);

    EXPECT_TRUE(matcher.doesMatch("one/two/anything"));
    EXPECT_TRUE(matcher.doesMatch("one/two/3"));
    EXPECT_TRUE(matcher.doesMatch("one/two/andAnotherPartition"));

    EXPECT_FALSE(matcher.doesMatch("one/two/three/four"));
    EXPECT_FALSE(matcher.doesMatch("one/two"));
}

TEST(MulticastMatcherTest, multiLevelWildcardAtEnd)
{
    std::string multicastId = "one/two/*";
    joynr::MulticastMatcher matcher(multicastId);

    EXPECT_TRUE(matcher.doesMatch("one/two/anything"));
    EXPECT_TRUE(matcher.doesMatch("one/two/3"));
    EXPECT_TRUE(matcher.doesMatch("one/two/andAnotherPartition"));
    EXPECT_TRUE(matcher.doesMatch("one/two/three/four"));
    EXPECT_TRUE(matcher.doesMatch("one/two"));

    EXPECT_FALSE(matcher.doesMatch("one/twothree"));
}
