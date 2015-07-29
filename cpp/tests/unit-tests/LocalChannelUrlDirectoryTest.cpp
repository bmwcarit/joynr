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
#include <QSettings>
#include <QFile>
#include <string>
#include "joynr/MessagingSettings.h"
#include "joynr/LocalChannelUrlDirectory.h"
#include "utils/QThreadSleep.h"
#include "tests/utils/MockObjects.h"
#include "joynr/Future.h"

using ::testing::A;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Between;
using ::testing::SetArgReferee;
using ::testing::Return;
using ::testing::Invoke;


using namespace joynr;

// global function used for calls to the MockChannelUrlSelectorProxy
std::shared_ptr<joynr::Future<joynr::types::ChannelUrlInformation>> localChannelUrlDirectoryTestPseudoGetChannelUrls(
        const std::string& channelId,
        std::function<void(
            RequestStatus& status,
            types::ChannelUrlInformation& urls)> callbackFct) {
    types::ChannelUrlInformation urlInformation;
    std::vector<std::string> urls = { "firstUrl", "secondUrl", "thirdUrl" };
    urlInformation.setUrls(urls);
    std::shared_ptr<joynr::Future<joynr::types::ChannelUrlInformation>> future(new joynr::Future<types::ChannelUrlInformation>());
    future->onSuccess(RequestStatus(RequestStatusCode::OK), urlInformation);
    RequestStatus status(RequestStatusCode::OK);
    callbackFct(status, urlInformation);
    return future;
}

class LocalChannelUrlDirectoryTest : public ::testing::Test {
public:
    LocalChannelUrlDirectoryTest() :
        settingsFileName("LocalChannelUrlDirectoryTest.settings"),
        settings(settingsFileName, QSettings::IniFormat),
        messagingSettings(settings)
    {}

    ~LocalChannelUrlDirectoryTest() {
        QFile::remove(settingsFileName);
    }

    void SetUp(){

    }

    void TearDown(){

    }

protected:
    QString settingsFileName;
    QSettings settings;
    MessagingSettings messagingSettings;
};



TEST_F(LocalChannelUrlDirectoryTest, getChannelUrlsUsesInternalProxy) {
    QSharedPointer<MockChannelUrlDirectoryProxy> mockChannelUrlDirectoryProxy(new MockChannelUrlDirectoryProxy());

    EXPECT_CALL(*mockChannelUrlDirectoryProxy, getUrlsForChannel(
                    A<const std::string&>(),
                    A<std::function<void(const RequestStatus& status, const types::ChannelUrlInformation& urls)>>()))
            .WillOnce(Invoke(localChannelUrlDirectoryTestPseudoGetChannelUrls));

    LocalChannelUrlDirectory localDirectory(messagingSettings, mockChannelUrlDirectoryProxy);
    std::shared_ptr<Future<types::ChannelUrlInformation> > futureUrls(
                localDirectory.getUrlsForChannel(
                    "pseudoChannelID",
                    20000,
                    [](const RequestStatus& status, const types::ChannelUrlInformation& url) {
                    }));

    EXPECT_EQ(true, futureUrls->getStatus().successful());

    types::ChannelUrlInformation channelInf;
    futureUrls->getValues(channelInf);
    types::ChannelUrlInformation urlInformation;
    std::vector<std::string> urls = { "firstUrl", "secondUrl", "thirdUrl" };
    urlInformation.setUrls(urls);

    EXPECT_EQ(urlInformation,channelInf);
}

TEST_F(LocalChannelUrlDirectoryTest, registerChannelUrls) {
    QSharedPointer<MockChannelUrlDirectoryProxy> mockChannelUrlDirectoryProxy(new MockChannelUrlDirectoryProxy());

    EXPECT_CALL(*mockChannelUrlDirectoryProxy, registerChannelUrls(
                    A<const std::string&>(),
                    _,
                    A<std::function<void(const RequestStatus& status)>>()))
            .Times(1)
            .WillOnce(Return(std::shared_ptr<joynr::Future<void>>(
                                 new joynr::Future<void>())));

    LocalChannelUrlDirectory localDirectory(messagingSettings, mockChannelUrlDirectoryProxy);

    types::ChannelUrlInformation urlInformation;
    localDirectory.registerChannelUrls("myChannelId", urlInformation);
}

TEST_F(LocalChannelUrlDirectoryTest, unregisterChannelUrls) {
    QSharedPointer<MockChannelUrlDirectoryProxy> mockChannelUrlDirectoryProxy(new MockChannelUrlDirectoryProxy());

    EXPECT_CALL(*mockChannelUrlDirectoryProxy, unregisterChannelUrls(
                    A<const std::string&>(),
                    A<std::function<void(const RequestStatus& status)>>()))
            .Times(1)
            .WillOnce(Return(std::shared_ptr<joynr::Future<void>>(new joynr::Future<void>())));

    LocalChannelUrlDirectory localDirectory(messagingSettings, mockChannelUrlDirectoryProxy);

    localDirectory.unregisterChannelUrls("pseudoChannelId");
}

















