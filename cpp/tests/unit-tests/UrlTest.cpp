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
#include <vector>

#include "tests/utils/Gtest.h"

#include "joynr/Url.h"

using namespace joynr;

class UrlTest : public ::testing::Test
{
public:
    UrlTest() : validTestData(), expected()
    {
        // Populate test data
        validTestData.push_back("http://bounceproxy.com/");
        expected.push_back(Url("http", "bounceproxy.com", 80, "/"));
        validTestData.push_back("http://bounceproxy.com:8080/");
        expected.push_back(Url("http", "bounceproxy.com", 8080, "/"));
        validTestData.push_back("http://bounceproxy.com/path");
        expected.push_back(Url("http", "bounceproxy.com", 80, "/path"));
        validTestData.push_back("ws://localhost");
        expected.push_back(Url("ws", "localhost", 80, "/"));
        validTestData.push_back("wss://localhost/some/path");
        expected.push_back(Url("wss", "localhost", 443, "/some/path"));
        validTestData.push_back("mqtt://localhost:1883");
        expected.push_back(Url("mqtt", "localhost", 1883, "/"));
        validTestData.push_back("mqtts://localhost:1883");
        expected.push_back(Url("mqtts", "localhost", 1883, "/"));
        validTestData.push_back("http://bounceproxy.com/script?query");
        expected.push_back(Url("http", "", "", "bounceproxy.com", 80, "/script", "query", ""));
        validTestData.push_back("https://bounceproxy.com/script#fragment");
        expected.push_back(Url("https", "", "", "bounceproxy.com", 443, "/script", "", "fragment"));
        validTestData.push_back("https://bounceproxy.com/script?query=someQuery#fragment");
        expected.push_back(Url(
                "https", "", "", "bounceproxy.com", 443, "/script", "query=someQuery", "fragment"));

        // IPv6 with hexadecimal address
        validTestData.push_back("http://[abcd:dcba:0123:3210:4567:7654:3456:6543]/");
        expected.push_back(Url("http", "abcd:dcba:0123:3210:4567:7654:3456:6543", 80, "/"));

        validTestData.push_back("http://[abcd:dcba:0123:3210:4567:7654:3456:6543]:8080/");
        expected.push_back(Url("http", "abcd:dcba:0123:3210:4567:7654:3456:6543", 8080, "/"));

        validTestData.push_back("http://[abcd:dcba:0123:3210:4567:7654:3456:6543]/path");
        expected.push_back(Url("http", "abcd:dcba:0123:3210:4567:7654:3456:6543", 80, "/path"));

        validTestData.push_back("ws://[::1]");
        expected.push_back(Url("ws", "::1", 80, "/"));

        validTestData.push_back("wss://[::1]/some/path");
        expected.push_back(Url("wss", "::1", 443, "/some/path"));

        validTestData.push_back("mqtt://[::1]:1883");
        expected.push_back(Url("mqtt", "::1", 1883, "/"));

        validTestData.push_back("mqtts://[::1]:1883");
        expected.push_back(Url("mqtts", "::1", 1883, "/"));

        validTestData.push_back("http://[abcd:dcba:0123:3210:4567:7654:3456:6543]/script?query");
        expected.push_back(Url("http",
                               "",
                               "",
                               "abcd:dcba:0123:3210:4567:7654:3456:6543",
                               80,
                               "/script",
                               "query",
                               ""));

        validTestData.push_back(
                "https://[abcd:dcba:0123:3210:4567:7654:3456:6543]/script#fragment");
        expected.push_back(Url("https",
                               "",
                               "",
                               "abcd:dcba:0123:3210:4567:7654:3456:6543",
                               443,
                               "/script",
                               "",
                               "fragment"));

        validTestData.push_back("https://[abcd:dcba:0123:3210:4567:7654:3456:6543]/"
                                "script?query=someQuery#fragment");
        expected.push_back(Url("https",
                               "",
                               "",
                               "abcd:dcba:0123:3210:4567:7654:3456:6543",
                               443,
                               "/script",
                               "query=someQuery",
                               "fragment"));

        validTestData.push_back("https://"
                                "user:password@[abcd:dcba:0123:3210:4567:7654:3456:6543]:4040/"
                                "script?query=somequery#fragment");
        expected.push_back(Url("https",
                               "user",
                               "password",
                               "abcd:dcba:0123:3210:4567:7654:3456:6543",
                               4040,
                               "/script",
                               "query=somequery",
                               "fragment"));
    }

protected:
    std::vector<std::string> validTestData;
    std::vector<Url> expected;
};

// Test URL parsing
TEST_F(UrlTest, parseValid)
{
    for (std::size_t i = 0; i < validTestData.size(); i++) {

        std::string urlString = validTestData[i];
        Url expectedResult = expected[i];
        Url url{urlString};
        ASSERT_TRUE(url.isValid());

        ASSERT_EQ(expectedResult, url);
    }
}

TEST_F(UrlTest, parseEmpty)
{
    Url url{""};
    ASSERT_FALSE(url.isValid());
}
