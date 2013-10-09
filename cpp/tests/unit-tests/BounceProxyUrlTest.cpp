/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "utils/TestQString.h"
#include "cluster-controller/http-communication-manager/BounceProxyUrl.h"

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
    EXPECT_QSTREQ(QString("http://localhost:8080/bounceproxy/channels/?ccid=testMcid"), createChannelUrl.toString());
    createChannelUrl = bounceProxyUrlWithTrailingSlash.getCreateChannelUrl("testMcid");
    EXPECT_QSTREQ(QString("http://localhost:8080/bounceproxy/channels/?ccid=testMcid"), createChannelUrl.toString());
}

TEST_F(BounceProxyUrlTest, getSendUrl) {
    QUrl sendUrl = bounceProxyUrl.getSendUrl("testMcid");
    EXPECT_QSTREQ(QString("http://localhost:8080/bounceproxy/channels/testMcid/message/"), sendUrl.toString());
    sendUrl = bounceProxyUrlWithTrailingSlash.getSendUrl("testMcid");
    EXPECT_QSTREQ(QString("http://localhost:8080/bounceproxy/channels/testMcid/message/"), sendUrl.toString());
}

TEST_F(BounceProxyUrlTest, getDeleteChannelUrl){
    QUrl deleteUrl = bounceProxyUrl.getDeleteChannelUrl("testMcid");
    EXPECT_QSTREQ(QString("http://localhost:8080/bounceproxy/channels/testMcid/"), deleteUrl.toString());
    deleteUrl = bounceProxyUrlWithTrailingSlash.getDeleteChannelUrl("testMcid");
    EXPECT_QSTREQ(QString("http://localhost:8080/bounceproxy/channels/testMcid/"), deleteUrl.toString());
}

TEST_F(BounceProxyUrlTest, getTimeCheckUrl){
    QUrl timeCheckUrl = bounceProxyUrl.getTimeCheckUrl();
    EXPECT_QSTREQ(QString("http://localhost:8080/bounceproxy/time/"), timeCheckUrl.toString());
    timeCheckUrl = bounceProxyUrlWithTrailingSlash.getTimeCheckUrl();
    EXPECT_QSTREQ(QString("http://localhost:8080/bounceproxy/time/"), timeCheckUrl.toString());
}
