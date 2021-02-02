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
            : brokerUrlHttp(BrokerUrl("http://localhost:8080/bounceproxy/")),
              brokerUrlMqtt(BrokerUrl("mqtt://localhost:1883/"))
    {
    }

    void createBrokerUrlWithEmptyString()
    {
        BrokerUrl brokerUrl("");
    }

protected:
    BrokerUrl brokerUrlHttp;
    BrokerUrl brokerUrlMqtt;
};

TEST_F(BrokerUrlTest, getCreateChannelUrl)
{
    Url createChannelUrlHttp = brokerUrlHttp.getCreateChannelUrl("testMcid");
    EXPECT_EQ("http://localhost:8080/bounceproxy/channels/?ccid=testMcid",
              createChannelUrlHttp.toString());
    Url createChannelUrlMqtt = brokerUrlMqtt.getCreateChannelUrl("testMcid");
    EXPECT_EQ("mqtt://localhost:1883/channels/?ccid=testMcid", createChannelUrlMqtt.toString());
}

TEST_F(BrokerUrlTest, getSendUrl)
{
    Url sendUrlHttp = brokerUrlHttp.getSendUrl("testMcid");
    EXPECT_EQ(
            "http://localhost:8080/bounceproxy/channels/testMcid/message/", sendUrlHttp.toString());
    Url sendUrlMqtt = brokerUrlMqtt.getSendUrl("testMcid");
    EXPECT_EQ("mqtt://localhost:1883/channels/testMcid/message/", sendUrlMqtt.toString());
}

TEST_F(BrokerUrlTest, getDeleteChannelUrl)
{
    Url deleteUrlHttp = brokerUrlHttp.getDeleteChannelUrl("testMcid");
    EXPECT_EQ("http://localhost:8080/bounceproxy/channels/testMcid/", deleteUrlHttp.toString());
    Url deleteUrlMqtt = brokerUrlMqtt.getDeleteChannelUrl("testMcid");
    EXPECT_EQ("mqtt://localhost:1883/channels/testMcid/", deleteUrlMqtt.toString());
}

TEST_F(BrokerUrlTest, getTimeCheckUrl)
{
    Url timeCheckUrlHttp = brokerUrlHttp.getTimeCheckUrl();
    EXPECT_EQ("http://localhost:8080/bounceproxy/time/", timeCheckUrlHttp.toString());
    Url timeCheckUrlMqtt = brokerUrlMqtt.getTimeCheckUrl();
    EXPECT_EQ("mqtt://localhost:1883/time/", timeCheckUrlMqtt.toString());
}

TEST_F(BrokerUrlTest, createEmptyBrokerUrlWithEmptyStringThrows)
{
    EXPECT_THROW(createBrokerUrlWithEmptyString(), std::invalid_argument);
}
