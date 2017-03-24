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

#include "gtest/gtest.h"
#include "joynr/Url.h"

#include <vector>

using namespace joynr;

class UrlTest : public ::testing::Test {
public:
    UrlTest() :
        validTestData(),
        expected()
    {
        // Populate test data
        validTestData.push_back("http://bounceproxy.com/");
        expected.push_back(Url("http","bounceproxy.com",80,"/"));
        validTestData.push_back("http://bounceproxy.com:8080/");
        expected.push_back(Url("http","bounceproxy.com",8080,"/"));
        validTestData.push_back("http://bounceproxy.com/path");
        expected.push_back(Url("http","bounceproxy.com",80,"/path"));
        validTestData.push_back("ws://localhost");
        expected.push_back(Url("ws","localhost",80,"/"));
        validTestData.push_back("wss://localhost/some/path");
        expected.push_back(Url("wss","localhost",443,"/some/path"));
        validTestData.push_back("mqtt://localhost:1883");
        expected.push_back(Url("mqtt","localhost",1883,"/"));
        validTestData.push_back("mqtts://localhost:1883");
        expected.push_back(Url("mqtts","localhost",1883,"/"));
        validTestData.push_back("http://bounceproxy.com/script?query");
        expected.push_back(Url("http","","","bounceproxy.com",80,"/script","query",""));
        validTestData.push_back("https://bounceproxy.com/script#fragment");
        expected.push_back(Url("https","","","bounceproxy.com",443,"/script","","fragment"));
        validTestData.push_back("https://bounceproxy.com/script?query=someQuery#fragment");
        expected.push_back(Url("https","","","bounceproxy.com",443,"/script","query=someQuery","fragment"));
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
