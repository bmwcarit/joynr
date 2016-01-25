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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include "joynr/MessagingSettings.h"
#include "joynr/LocalChannelUrlDirectory.h"
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
std::shared_ptr<Future<joynr::types::ChannelUrlInformation>> localChannelUrlDirectoryTestPseudoGetChannelUrls(
        const std::string& channelId,
        std::function<void(types::ChannelUrlInformation& urls)> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onError) {
    std::ignore = channelId;
    std::ignore = onError;
    types::ChannelUrlInformation urlInformation;
    std::vector<std::string> urls = { "firstUrl", "secondUrl", "thirdUrl" };
    urlInformation.setUrls(urls);
    std::shared_ptr<Future<joynr::types::ChannelUrlInformation>> future(new Future<types::ChannelUrlInformation>());
    future->onSuccess(urlInformation);
    onSuccess(urlInformation);
    return future;
}

class LocalChannelUrlDirectoryTest : public ::testing::Test {
public:
    LocalChannelUrlDirectoryTest() :
        settingsFileName("LocalChannelUrlDirectoryTest.settings"),
        settings(settingsFileName),
        messagingSettings(settings)
    {}

    ~LocalChannelUrlDirectoryTest() {
        std::remove(settingsFileName.c_str());
    }

    void SetUp(){

    }

    void TearDown(){

    }

protected:
    std::string settingsFileName;
    Settings settings;
    MessagingSettings messagingSettings;
};



TEST_F(LocalChannelUrlDirectoryTest, getChannelUrlsUsesInternalProxy) {
    std::shared_ptr<MockChannelUrlDirectoryProxy> mockChannelUrlDirectoryProxy = std::make_shared<MockChannelUrlDirectoryProxy>();

    EXPECT_CALL(*mockChannelUrlDirectoryProxy, getUrlsForChannelAsync(
                    A<const std::string&>(),
                    A<std::function<void(const types::ChannelUrlInformation& urls)>>(),
                    A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .WillOnce(Invoke(localChannelUrlDirectoryTestPseudoGetChannelUrls));

    LocalChannelUrlDirectory localDirectory(messagingSettings, mockChannelUrlDirectoryProxy);
    std::shared_ptr<Future<types::ChannelUrlInformation> > futureUrls(
                localDirectory.getUrlsForChannelAsync(
                    "pseudoChannelID",
                    std::chrono::seconds(20),
                    [](const types::ChannelUrlInformation& url) {},
                    [](const exceptions::JoynrException& error) {}));

    EXPECT_EQ(StatusCode::SUCCESS, futureUrls->getStatus());

    types::ChannelUrlInformation channelInf;
    futureUrls->get(channelInf);
    types::ChannelUrlInformation urlInformation;
    std::vector<std::string> urls = { "firstUrl", "secondUrl", "thirdUrl" };
    urlInformation.setUrls(urls);

    EXPECT_EQ(urlInformation,channelInf);
}

TEST_F(LocalChannelUrlDirectoryTest, registerChannelUrls) {
    std::shared_ptr<MockChannelUrlDirectoryProxy> mockChannelUrlDirectoryProxy(new MockChannelUrlDirectoryProxy());

    EXPECT_CALL(*mockChannelUrlDirectoryProxy, registerChannelUrlsAsync(
                    A<const std::string&>(),
                    _,
                    A<std::function<void(void)>>(),
                    A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1)
            .WillOnce(Return(std::shared_ptr<Future<void>>(
                                 new Future<void>())));

    LocalChannelUrlDirectory localDirectory(messagingSettings, mockChannelUrlDirectoryProxy);

    types::ChannelUrlInformation urlInformation;
    localDirectory.registerChannelUrlsAsync("myChannelId", urlInformation);
}

TEST_F(LocalChannelUrlDirectoryTest, unregisterChannelUrls) {
    std::shared_ptr<MockChannelUrlDirectoryProxy> mockChannelUrlDirectoryProxy(new MockChannelUrlDirectoryProxy());

    EXPECT_CALL(*mockChannelUrlDirectoryProxy, unregisterChannelUrlsAsync(
                    A<const std::string&>(),
                    A<std::function<void()>>(),
                    A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .Times(1)
            .WillOnce(Return(std::shared_ptr<Future<void>>(new Future<void>())));

    LocalChannelUrlDirectory localDirectory(messagingSettings, mockChannelUrlDirectoryProxy);

    localDirectory.unregisterChannelUrlsAsync("pseudoChannelId");
}

















