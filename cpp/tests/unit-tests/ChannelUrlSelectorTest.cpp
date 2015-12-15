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
#include <chrono>
#include "cluster-controller/http-communication-manager/ChannelUrlSelector.h"
#include "joynr/BrokerUrl.h"
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
using ::testing::WithArgs;

using namespace joynr;

// global function used for calls to the MockChannelUrlSelectorProxy
std::shared_ptr<Future<joynr::types::ChannelUrlInformation>> pseudoGetChannelUrls(const std::string&  channelId, std::chrono::milliseconds timeout) {
    types::ChannelUrlInformation urlInformation;
    std::vector<std::string> urls = { "firstUrl", "secondUrl", "thirdUrl" };
    urlInformation.setUrls(urls);
    std::shared_ptr<Future<joynr::types::ChannelUrlInformation>> future(new Future<types::ChannelUrlInformation>());
    future->onSuccess(urlInformation);
    return future;
}


// No longer desired behavior!
TEST(ChannelUrlSelectorTest, DISABLED_usesBrokerUrlIfNotProvidedWithChannelUrlDir) {
    const std::string brokerBaseUrl = "http://www.urltest.org/pseudoBp";
    BrokerUrl brokerUrl(brokerBaseUrl);
    ChannelUrlSelector* urlCache = new ChannelUrlSelector(
                brokerUrl,
                ChannelUrlSelector::TIME_FOR_ONE_RECOUPERATION(),
                ChannelUrlSelector::PUNISHMENT_FACTOR());
    RequestStatus* status = new RequestStatus();
    std::string channelId = "testChannelId";
    std::string url = urlCache->obtainUrl(channelId,*status, std::chrono::seconds(20));
    EXPECT_EQ("http://www.urltest.org/pseudoBp/testChannelId/message/", url);
    delete urlCache;
    delete status;
}


TEST(ChannelUrlSelectorTest, obtainUrlUsesLocalDirectory) {
    const std::string brokerBaseUrl = "http://www.UrlTest.org/pseudoBp";
    const std::string settingsFileName ("test-resources/ChannelUrlSelectorTest.settings");

    BrokerUrl brokerUrl(brokerBaseUrl);
    ChannelUrlSelector* urlCache = new ChannelUrlSelector(
                brokerUrl,
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
                    A<std::chrono::milliseconds>(),
                    A<std::function<void(const types::ChannelUrlInformation& urls)>>(),
                    A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .WillOnce(WithArgs<0,1>(Invoke(pseudoGetChannelUrls)));

    RequestStatus* status = new RequestStatus();
    std::string channelId = "testChannelId";

    std::string url = urlCache->obtainUrl(channelId,*status, std::chrono::seconds(20));
    EXPECT_EQ("firstUrl/message/", url);

    delete status;
    delete urlCache;
    delete settings;
    delete baseSettings;
}


TEST(ChannelUrlSelectorTest, obtainUrlUsesFeedbackToChangeProviderUrl) {
    const std::string brokerBaseUrl = "http://www.UrlTest.org/pseudoBp";
    const std::string settingsFileName("test-resources/ChannelUrlSelectorTest.settings");

    BrokerUrl brokerUrl(brokerBaseUrl);
    ChannelUrlSelector* urlCache = new ChannelUrlSelector(
                brokerUrl,
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
                    A<std::chrono::milliseconds>(),
                    A<std::function<void(const types::ChannelUrlInformation& urls)>>(),
                    A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .WillOnce(WithArgs<0,1>(Invoke(pseudoGetChannelUrls)));

    RequestStatus* status = new RequestStatus();
    std::string channelId = "testChannelId";

    std::string url = urlCache->obtainUrl(channelId,*status, std::chrono::seconds(20));
    EXPECT_EQ("firstUrl/message/", url);

    urlCache->feedback(false,channelId,url);
    url = urlCache->obtainUrl(channelId,*status, std::chrono::seconds(20));
    EXPECT_EQ("firstUrl/message/", url);

    urlCache->feedback(false,channelId,url);
    url = urlCache->obtainUrl(channelId,*status, std::chrono::seconds(20));
    EXPECT_EQ("firstUrl/message/", url);

    urlCache->feedback(false,channelId,url);
    url = urlCache->obtainUrl(channelId,*status, std::chrono::seconds(20));
    EXPECT_EQ("secondUrl/message/", url);

    delete status;
    delete urlCache;
    delete settings;
    delete baseSettings;
}


TEST(ChannelUrlSelectorTest, obtainUrlRetriesUrlOfHigherPriority) {
    const std::string brokerBaseUrl = "http://www.UrlTest.org/pseudoBp";
    const std::string settingsFileName("test-resources/ChannelUrlSelectorTest.settings");
    std::chrono::milliseconds timeForOneRecouperation(1000);
    double punishmentFactor = 0.4;//three punishments will lead to a try of the second Url
    BrokerUrl brokerUrl(brokerBaseUrl);

    ChannelUrlSelector* urlCache = new ChannelUrlSelector(
                brokerUrl,
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
                    A<std::chrono::milliseconds>(),
                    A<std::function<void(const types::ChannelUrlInformation& urls)>>(),
                    A<std::function<void(const exceptions::JoynrRuntimeException& error)>>()))
            .WillOnce(WithArgs<0,1>(Invoke(pseudoGetChannelUrls)));

    RequestStatus* status = new RequestStatus();
    std::string channelId = "testChannelId";
    std::string url = urlCache->obtainUrl(channelId,*status, std::chrono::seconds(20));

    urlCache->feedback(false,channelId,url);
    urlCache->feedback(false,channelId,url);
    urlCache->feedback(false,channelId,url);
    url = urlCache->obtainUrl(channelId,*status, std::chrono::seconds(20));
    EXPECT_EQ("secondUrl/message/", url);

    std::this_thread::sleep_for(timeForOneRecouperation + std::chrono::milliseconds(100));
    url = urlCache->obtainUrl(channelId,*status, std::chrono::seconds(20));
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
    std::chrono::milliseconds timeForOneRecouperation(300);
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

    std::this_thread::sleep_for(std::chrono::milliseconds(timeForOneRecouperation.count() /3));
    entry->updateFitness();
    fitness = entry->getFitness();
    EXPECT_EQ(3 - punishmentFactor,fitness.at(0));
    EXPECT_EQ(2 - punishmentFactor,fitness.at(1));
    EXPECT_EQ(1 - punishmentFactor - punishmentFactor, fitness.at(2));

    std::this_thread::sleep_for(timeForOneRecouperation);
    entry->updateFitness();
    fitness = entry->getFitness();
    EXPECT_EQ(3,fitness.at(0));
    EXPECT_EQ(2,fitness.at(1));
    EXPECT_EQ(1 - punishmentFactor,fitness.at(2));

    std::this_thread::sleep_for(timeForOneRecouperation);
    entry->updateFitness();
    fitness = entry->getFitness();
    EXPECT_EQ(3,fitness.at(0));
    EXPECT_EQ(2,fitness.at(1));
    EXPECT_EQ(1,fitness.at(2));

    std::this_thread::sleep_for(timeForOneRecouperation);
    entry->updateFitness();
    fitness = entry->getFitness();
    EXPECT_EQ(3,fitness.at(0));
    EXPECT_EQ(2,fitness.at(1));
    EXPECT_EQ(1,fitness.at(2));


    entry->punish("firstUrl");
    entry->punish("secondUrl");
    entry->punish("thirdUrl");
    entry->punish("thirdUrl");

    std::this_thread::sleep_for(std::chrono::milliseconds( 2 * timeForOneRecouperation.count() + 10 ));
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
    std::chrono::milliseconds timeForOneRecouperation(300);
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
    std::this_thread::sleep_for(std::chrono::milliseconds(timeForOneRecouperation.count() + 100));
    EXPECT_EQ("secondUrl", entry->best());
    std::this_thread::sleep_for(std::chrono::milliseconds(timeForOneRecouperation.count() + 100));
    EXPECT_EQ("firstUrl", entry->best());

    delete entry;
}


