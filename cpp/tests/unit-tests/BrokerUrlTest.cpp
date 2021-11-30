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
#include <gtest/gtest.h>

#include "joynr/BrokerUrl.h"
#include "joynr/Url.h"

using namespace joynr;

class BrokerUrlTest : public ::testing::Test
{
public:
    BrokerUrlTest()
            : brokerUrlMqtt(BrokerUrl("mqtt://localhost:1883/"))
    {
    }

    void createBrokerUrlWithEmptyString()
    {
        BrokerUrl brokerUrl("");
    }

protected:
    BrokerUrl brokerUrlMqtt;
};

TEST_F(BrokerUrlTest, getBrokerUrl)
{
    Url brokerUrl = brokerUrlMqtt.getBrokerBaseUrl();
    EXPECT_EQ("mqtt://localhost:1883/", brokerUrl.toString());
}

TEST_F(BrokerUrlTest, createEmptyBrokerUrlWithEmptyStringThrows)
{
    EXPECT_THROW(createBrokerUrlWithEmptyString(), std::invalid_argument);
}

TEST_F(BrokerUrlTest, invalidNonEmptyBrokerUrlsDoNotThrow)
{
    BrokerUrl invalidNonEmptyBrokerUrl_1("mqtt://127.0.0.1.1883");
    EXPECT_EQ("mqtt://127.0.0.1.1883", invalidNonEmptyBrokerUrl_1.toString());
    BrokerUrl invalidNonEmptyBrokerUrl_2("mqtts://127.0.0.1.1883");
    EXPECT_EQ("mqtts://127.0.0.1.1883", invalidNonEmptyBrokerUrl_2.toString());
    BrokerUrl invalidNonEmptyBrokerUrl_3("tcp://127.0.0.1.1883");
    EXPECT_EQ("tcp://127.0.0.1.1883", invalidNonEmptyBrokerUrl_3.toString());
}
