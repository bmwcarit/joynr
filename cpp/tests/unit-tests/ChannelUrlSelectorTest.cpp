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
#include <string>
#include <memory>
#include "cluster-controller/http-communication-manager/ChannelUrlSelector.h"
#include "joynr/BounceProxyUrl.h"
#include "tests/utils/MockObjects.h"
#include "joynr/Future.h"
#include "joynr/ThreadUtil.h"

using ::testing::A;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Between;
using ::testing::SetArgReferee;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::WithArgs;

using namespace joynr;

// global function used for calls to the MockChannelUrlSelectorProxy
std::shared_ptr<joynr::Future<joynr::types::ChannelUrlInformation>> pseudoGetChannelUrls(const std::string&  channelId, const qint64& timeout) {
    types::ChannelUrlInformation urlInformation;
    std::vector<std::string> urls = { "firstUrl", "secondUrl", "thirdUrl" };
    urlInformation.setUrls(urls);
    std::shared_ptr<joynr::Future<joynr::types::ChannelUrlInformation>> future(new joynr::Future<types::ChannelUrlInformation>());
    future->onSuccess(urlInformation);
    return future;
}


// No longer desired behavior!
TEST(ChannelUrlSelectorTest, DISABLED_usesBounceProxyUrlIfNotProvidedWithChannelUrlDir) {
    const QString bounceProxyBaseUrl = "http://www.urltest.org/pseudoBp";
    BounceProxyUrl bounceProxyUrl(bounceProxyBaseUrl);
    ChannelUrlSelector* urlCache = new ChannelUrlSelector(
                bounceProxyUrl,
                ChannelUrlSelector::TIME_FOR_ONE_RECOUPERATION(),
                ChannelUrlSelector::PUNISHMENT_FACTOR());
    RequestStatus* status = new RequestStatus();
    QString channelId = "testChannelId";
    QString url = urlCache->obtainUrl(channelId,*status, 20000);
    EXPECT_EQ("http://www.urltest.org/pseudoBp/testChannelId/message/", url);
    delete urlCache;
    delete status;
}


TEST(ChannelUrlSelectorTest, obtainUrlUsesLocalDirectory) {
    const QString bounceProxyBaseUrl = "http://www.UrlTest.org/pseudoBp";
    const std::string settingsFileName ("test-resources/ChannelUrlSelectorTest.settings");

    BounceProxyUrl bounceProxyUrl(bounceProxyBaseUrl);
    ChannelUrlSelector* urlCache = new ChannelUrlSelector(
                bounceProxyUrl,
                ChannelUrlSelector::TIME_FOR_ONE_RECOUPERATION(),
                ChannelUrlSelector::PUNISHMENT_FACTOR());

    MockLocalChannelUrlDirectory* mockDir = new MockLocalChannelUrlDirectory();
    std::shared_ptr<MockLocalChannelUrlDirectory>  mockDirectory(mockDir);
    Settings *baseSettings = new Settings(settingsFileName);
    MessagingSettings *settings = new MessagingSettings(*baseSettings);
    urlCache->init(
                mockDirectory,
                *settings);

    EXPECT_CALL(*mockDir, getUrlsForChannelAsync(
                    A<const std::string&>(),
                    A<const qint64&>(),
                    A<std::function<void(const types::ChannelUrlInformation& urls)>>(),
                    A<std::function<void(const exceptions::JoynrException& error)>>()))
            .WillOnce(WithArgs<0,1>(Invoke(pseudoGetChannelUrls)));

    RequestStatus* status = new RequestStatus();
    QString channelId = "testChannelId";

    QString url = urlCache->obtainUrl(channelId,*status, 20000);
    EXPECT_EQ("firstUrl/message/", url);

    delete status;
    delete urlCache;
    delete settings;
    delete baseSettings;
}


TEST(ChannelUrlSelectorTest, obtainUrlUsesFeedbackToChangeProviderUrl) {
    const QString bounceProxyBaseUrl = "http://www.UrlTest.org/pseudoBp";
    const std::string settingsFileName("test-resources/ChannelUrlSelectorTest.settings");

    BounceProxyUrl bounceProxyUrl(bounceProxyBaseUrl);
    ChannelUrlSelector* urlCache = new ChannelUrlSelector(
                bounceProxyUrl,
                ChannelUrlSelector::TIME_FOR_ONE_RECOUPERATION(),
                ChannelUrlSelector::PUNISHMENT_FACTOR());

    MockLocalChannelUrlDirectory* mockDir = new MockLocalChannelUrlDirectory();
    std::shared_ptr<ILocalChannelUrlDirectory> mockDirectory(mockDir);
    Settings *baseSettings = new Settings(settingsFileName);
    MessagingSettings *settings = new MessagingSettings(*baseSettings);
    urlCache->init(
                mockDirectory,
                *settings);

    EXPECT_CALL(*mockDir, getUrlsForChannelAsync(
                    A<const std::string&>(),
                    A<const qint64&>(),
                    A<std::function<void(const types::ChannelUrlInformation& urls)>>(),
                    A<std::function<void(const exceptions::JoynrException& error)>>()))
            .WillOnce(WithArgs<0,1>(Invoke(pseudoGetChannelUrls)));

    RequestStatus* status = new RequestStatus();
    QString channelId = "testChannelId";

    QString url = urlCache->obtainUrl(channelId,*status, 20000);
    EXPECT_EQ("firstUrl/message/", url);

    urlCache->feedback(false,channelId,url);
    url = urlCache->obtainUrl(channelId,*status, 20000);
    EXPECT_EQ("firstUrl/message/", url);

    urlCache->feedback(false,channelId,url);
    url = urlCache->obtainUrl(channelId,*status, 20000);
    EXPECT_EQ("firstUrl/message/", url);

    urlCache->feedback(false,channelId,url);
    url = urlCache->obtainUrl(channelId,*status, 20000);
    EXPECT_EQ("secondUrl/message/", url);

    delete status;
    delete urlCache;
    delete settings;
    delete baseSettings;
}


TEST(ChannelUrlSelectorTest, obtainUrlRetriesUrlOfHigherPriority) {
    const QString bounceProxyBaseUrl = "http://www.UrlTest.org/pseudoBp";
    const std::string settingsFileName("test-resources/ChannelUrlSelectorTest.settings");
    qint64 timeForOneRecouperation = 1000; //half a minute
    double punishmentFactor = 0.4;//three punishments will lead to a try of the second Url
    BounceProxyUrl bounceProxyUrl(bounceProxyBaseUrl);

    ChannelUrlSelector* urlCache = new ChannelUrlSelector(
                bounceProxyUrl,
                timeForOneRecouperation,
                punishmentFactor);

    MockLocalChannelUrlDirectory* mockDir = new MockLocalChannelUrlDirectory();
    std::shared_ptr<ILocalChannelUrlDirectory>  mockDirectory(mockDir);
    Settings* baseSettings = new Settings(settingsFileName);
    MessagingSettings *settings = new MessagingSettings(*baseSettings);
    urlCache->init(
                mockDirectory,
                *settings);

    EXPECT_CALL(*mockDir, getUrlsForChannelAsync(
                    A<const std::string&>(),
                    A<const qint64&>(),
                    A<std::function<void(const types::ChannelUrlInformation& urls)>>(),
                    A<std::function<void(const exceptions::JoynrException& error)>>()))
            .WillOnce(WithArgs<0,1>(Invoke(pseudoGetChannelUrls)));

    RequestStatus* status = new RequestStatus();
    QString channelId = "testChannelId";
    QString url = urlCache->obtainUrl(channelId,*status, 20000);

    urlCache->feedback(false,channelId,url);
    urlCache->feedback(false,channelId,url);
    urlCache->feedback(false,channelId,url);
    url = urlCache->obtainUrl(channelId,*status, 20000);
    EXPECT_EQ("secondUrl/message/", url);

    ThreadUtil::sleepForMillis(timeForOneRecouperation + 100);
    url = urlCache->obtainUrl(channelId,*status, 20000);
    EXPECT_EQ("firstUrl/message/", url);

    delete status;
    delete urlCache;
    delete settings;
    delete baseSettings;
}


TEST(ChannelUrlSelectorTest, initFitnessTest) {
    types::ChannelUrlInformation urlInformation;
    std::vector<std::string> urls = {"firstUrl", "secondUrl", "thirdUrl"};
    urlInformation.setUrls(urls);
    ChannelUrlSelectorEntry* entry = new ChannelUrlSelectorEntry(
                urlInformation,
                ChannelUrlSelector::PUNISHMENT_FACTOR(),
                ChannelUrlSelector::TIME_FOR_ONE_RECOUPERATION());
    std::vector<double> fitness = entry->getFitness();
    EXPECT_EQ(3,fitness.size());
    EXPECT_EQ(3,fitness.at(0));
    EXPECT_EQ(2,fitness.at(1));
    EXPECT_EQ(1,fitness.at(2));
    delete entry;
}

TEST(ChannelUrlSelectorTest, punishTest) {
    types::ChannelUrlInformation urlInformation;
    std::vector<std::string> urls = {"firstUrl", "secondUrl", "thirdUrl"};
    urlInformation.setUrls(urls);
    double punishmentFactor = 0.4;
    ChannelUrlSelectorEntry* entry = new ChannelUrlSelectorEntry(
                urlInformation,
                ChannelUrlSelector::PUNISHMENT_FACTOR(),
                ChannelUrlSelector::TIME_FOR_ONE_RECOUPERATION());

    std::vector<double> fitness = entry->getFitness();
    EXPECT_EQ(3,fitness.at(0));
    entry->punish("firstUrl");
    fitness = entry->getFitness();
    EXPECT_EQ(3 - punishmentFactor,fitness.at(0));
    EXPECT_EQ(2,fitness.at(1));
    EXPECT_EQ(1,fitness.at(2));

    entry->punish("firstUrl");
    fitness = entry->getFitness();
    EXPECT_EQ(3 - punishmentFactor - punishmentFactor, fitness.at(0));
    EXPECT_EQ(2, fitness.at(1));
    EXPECT_EQ(1, fitness.at(2));

    entry->punish("secondUrl");
    fitness = entry->getFitness();
    EXPECT_EQ( 3 - punishmentFactor - punishmentFactor, fitness.at(0));
    EXPECT_EQ( 2 - punishmentFactor, fitness.at(1));
    EXPECT_EQ(1, fitness.at(2));

    entry->punish("thirdUrl");
    fitness = entry->getFitness();
    EXPECT_EQ(3 - punishmentFactor - punishmentFactor, fitness.at(0));
    EXPECT_EQ(2 - punishmentFactor, fitness.at(1));
    EXPECT_EQ(1 - punishmentFactor, fitness.at(2));

    delete entry;
}

TEST(ChannelUrlSelectorTest, updateTest) {

    types::ChannelUrlInformation urlInformation;
    std::vector<std::string> urls = {"firstUrl", "secondUrl", "thirdUrl"};
    urlInformation.setUrls(urls);
    double punishmentFactor = 0.4;
    qint64 timeForOneRecouperation = 300;
    ChannelUrlSelectorEntry* entry = new ChannelUrlSelectorEntry(
                urlInformation,
                punishmentFactor,
                timeForOneRecouperation);

    entry->punish("firstUrl");
    entry->punish("secondUrl");
    entry->punish("thirdUrl");
    entry->punish("thirdUrl");
    entry->updateFitness();
    std::vector<double> fitness = entry->getFitness();
    EXPECT_EQ(3 - punishmentFactor,fitness.at(0));
    EXPECT_EQ(2 - punishmentFactor,fitness.at(1));
    EXPECT_EQ(1 - punishmentFactor - punishmentFactor,fitness.at(2));

    ThreadUtil::sleepForMillis(timeForOneRecouperation /3);
    entry->updateFitness();
    fitness = entry->getFitness();
    EXPECT_EQ(3 - punishmentFactor,fitness.at(0));
    EXPECT_EQ(2 - punishmentFactor,fitness.at(1));
    EXPECT_EQ(1 - punishmentFactor - punishmentFactor, fitness.at(2));

    ThreadUtil::sleepForMillis(timeForOneRecouperation );
    entry->updateFitness();
    fitness = entry->getFitness();
    EXPECT_EQ(3,fitness.at(0));
    EXPECT_EQ(2,fitness.at(1));
    EXPECT_EQ(1 - punishmentFactor,fitness.at(2));

    ThreadUtil::sleepForMillis(timeForOneRecouperation);
    entry->updateFitness();
    fitness = entry->getFitness();
    EXPECT_EQ(3,fitness.at(0));
    EXPECT_EQ(2,fitness.at(1));
    EXPECT_EQ(1,fitness.at(2));

    ThreadUtil::sleepForMillis(timeForOneRecouperation );
    entry->updateFitness();
    fitness = entry->getFitness();
    EXPECT_EQ(3,fitness.at(0));
    EXPECT_EQ(2,fitness.at(1));
    EXPECT_EQ(1,fitness.at(2));


    entry->punish("firstUrl");
    entry->punish("secondUrl");
    entry->punish("thirdUrl");
    entry->punish("thirdUrl");

    ThreadUtil::sleepForMillis( 2 * timeForOneRecouperation + 10 );
    entry->updateFitness();
    fitness = entry->getFitness();
    EXPECT_EQ(3,fitness.at(0));
    EXPECT_EQ(2,fitness.at(1));
    EXPECT_EQ(1,fitness.at(2));

    delete entry;
}


TEST(ChannelUrlSelectorTest, bestTest) {

    types::ChannelUrlInformation urlInformation;
    std::vector<std::string> urls = {"firstUrl", "secondUrl", "thirdUrl"};
    urlInformation.setUrls(urls);
    double punishmentFactor = 0.4;
    qint64 timeForOneRecouperation = 300;
    ChannelUrlSelectorEntry* entry = new ChannelUrlSelectorEntry(
                urlInformation,
                punishmentFactor,
                timeForOneRecouperation);

    EXPECT_EQ("firstUrl", entry->best());
    entry->punish("firstUrl");
    EXPECT_EQ("firstUrl", entry->best());
    entry->punish("firstUrl");
    EXPECT_EQ("firstUrl", entry->best());
    entry->punish("firstUrl");
    EXPECT_EQ("secondUrl", entry->best());
    entry->punish("secondUrl");
    EXPECT_EQ("firstUrl", entry->best());
    entry->punish("firstUrl");
    EXPECT_EQ("secondUrl", entry->best());
    ThreadUtil::sleepForMillis(timeForOneRecouperation + 100);
    EXPECT_EQ("secondUrl", entry->best());
    ThreadUtil::sleepForMillis(timeForOneRecouperation + 100);
    EXPECT_EQ("firstUrl", entry->best());

    delete entry;
}


