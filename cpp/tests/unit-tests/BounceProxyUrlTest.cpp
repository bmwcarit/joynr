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
#include "joynr/BounceProxyUrl.h"

using namespace joynr;

class BounceProxyUrlTest : public ::testing::Test {
public:
    BounceProxyUrlTest() :
        bounceProxyUrl(BounceProxyUrl("http://localhost:8080/bounceproxy")),
        bounceProxyUrlWithTrailingSlash(BounceProxyUrl("http://localhost:8080/bounceproxy/"))
    {}

    void SetUp(){
    }
    void TearDown(){
    }
protected:
    BounceProxyUrl bounceProxyUrl;
    BounceProxyUrl bounceProxyUrlWithTrailingSlash;
};



TEST_F(BounceProxyUrlTest, getCreateChannelUrl) {
    QUrl createChannelUrl = bounceProxyUrl.getCreateChannelUrl("testMcid");
    EXPECT_STREQ("http://localhost:8080/bounceproxy/channels/?ccid=testMcid", createChannelUrl.toString().toStdString().c_str());
    createChannelUrl = bounceProxyUrlWithTrailingSlash.getCreateChannelUrl("testMcid");
    EXPECT_STREQ("http://localhost:8080/bounceproxy/channels/?ccid=testMcid", createChannelUrl.toString().toStdString().c_str());
}

TEST_F(BounceProxyUrlTest, getSendUrl) {
    QUrl sendUrl = bounceProxyUrl.getSendUrl("testMcid");
    EXPECT_STREQ("http://localhost:8080/bounceproxy/channels/testMcid/message/", sendUrl.toString().toStdString().c_str());
    sendUrl = bounceProxyUrlWithTrailingSlash.getSendUrl("testMcid");
    EXPECT_STREQ("http://localhost:8080/bounceproxy/channels/testMcid/message/", sendUrl.toString().toStdString().c_str());
}

TEST_F(BounceProxyUrlTest, getDeleteChannelUrl){
    QUrl deleteUrl = bounceProxyUrl.getDeleteChannelUrl("testMcid");
    EXPECT_STREQ("http://localhost:8080/bounceproxy/channels/testMcid/", deleteUrl.toString().toStdString().c_str());
    deleteUrl = bounceProxyUrlWithTrailingSlash.getDeleteChannelUrl("testMcid");
    EXPECT_STREQ("http://localhost:8080/bounceproxy/channels/testMcid/", deleteUrl.toString().toStdString().c_str());
}

TEST_F(BounceProxyUrlTest, getTimeCheckUrl){
    QUrl timeCheckUrl = bounceProxyUrl.getTimeCheckUrl();
    EXPECT_STREQ("http://localhost:8080/bounceproxy/time/", timeCheckUrl.toString().toStdString().c_str());
    timeCheckUrl = bounceProxyUrlWithTrailingSlash.getTimeCheckUrl();
    EXPECT_STREQ("http://localhost:8080/bounceproxy/time/", timeCheckUrl.toString().toStdString().c_str());
}
