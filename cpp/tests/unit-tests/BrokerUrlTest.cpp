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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "joynr/BrokerUrl.h"
#include "joynr/Url.h"

using namespace joynr;

class BrokerUrlTest : public ::testing::Test {
public:
    BrokerUrlTest() :
        brokerUrl(BrokerUrl("http://localhost:8080/bounceproxy/"))
    {}

    void SetUp(){
    }
    void TearDown(){
    }
protected:
    BrokerUrl brokerUrl;
};



TEST_F(BrokerUrlTest, getCreateChannelUrl) {
    Url createChannelUrl = brokerUrl.getCreateChannelUrl("testMcid");
    EXPECT_EQ("http://localhost:8080/bounceproxy/channels/?ccid=testMcid", createChannelUrl.toString());
}

TEST_F(BrokerUrlTest, getSendUrl) {
    Url sendUrl = brokerUrl.getSendUrl("testMcid");
    EXPECT_EQ("http://localhost:8080/bounceproxy/channels/testMcid/message/", sendUrl.toString());
}

TEST_F(BrokerUrlTest, getDeleteChannelUrl){
    Url deleteUrl = brokerUrl.getDeleteChannelUrl("testMcid");
    EXPECT_EQ("http://localhost:8080/bounceproxy/channels/testMcid/", deleteUrl.toString());
}

TEST_F(BrokerUrlTest, getTimeCheckUrl){
    Url timeCheckUrl = brokerUrl.getTimeCheckUrl();
    EXPECT_EQ("http://localhost:8080/bounceproxy/time/", timeCheckUrl.toString());
}
