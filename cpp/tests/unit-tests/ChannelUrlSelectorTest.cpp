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
#include "cluster-controller/http-communication-manager/ChannelUrlSelector.h"
#include "joynr/BounceProxyUrl.h"
#include "utils/QThreadSleep.h"
#include "tests/utils/MockObjects.h"
#include "joynr/Future.h"
#include <string>
#include <memory>

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
    future->onSuccess(RequestStatus(RequestStatusCode::OK), urlInformation);
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
    const QString settingsFileName ("test-resources/ChannelUrlSelectorTest.settings");

    BounceProxyUrl bounceProxyUrl(bounceProxyBaseUrl);
    ChannelUrlSelector* urlCache = new ChannelUrlSelector(
                bounceProxyUrl,
                ChannelUrlSelector::TIME_FOR_ONE_RECOUPERATION(),
                ChannelUrlSelector::PUNISHMENT_FACTOR());

    MockLocalChannelUrlDirectory* mockDir = new MockLocalChannelUrlDirectory();
    QSharedPointer<MockLocalChannelUrlDirectory>  mockDirectory(mockDir);
    QSettings *qsettings = new QSettings(settingsFileName, QSettings::IniFormat);
    MessagingSettings *settings = new MessagingSettings(*qsettings);
    urlCache->init(
                mockDirectory,
                *settings);

    EXPECT_CALL(*mockDir, getUrlsForChannel(
                    A<const std::string&>(),
                    A<const qint64&>(),
                    A<std::function<void(
                        const RequestStatus& status,
                        const types::ChannelUrlInformation& urls)>>()))
            .WillOnce(WithArgs<0,1>(Invoke(pseudoGetChannelUrls)));

    RequestStatus* status = new RequestStatus();
    QString channelId = "testChannelId";

    QString url = urlCache->obtainUrl(channelId,*status, 20000);
    EXPECT_EQ("firstUrl/message/", url);

    delete status;
    delete urlCache;
    // MessageSettings requires a message loop to delete qsettings.
    // QSettings must still exist when the destructor of MessageSettings is called.
    // When unittesting we have to delete manually because there is no message loop.
    delete settings;
    delete qsettings;
}


TEST(ChannelUrlSelectorTest, obtainUrlUsesFeedbackToChangeProviderUrl) {
    const QString bounceProxyBaseUrl = "http://www.UrlTest.org/pseudoBp";
    const QString settingsFileName("test-resources/ChannelUrlSelectorTest.settings");

    BounceProxyUrl bounceProxyUrl(bounceProxyBaseUrl);
    ChannelUrlSelector* urlCache = new ChannelUrlSelector(
                bounceProxyUrl,
                ChannelUrlSelector::TIME_FOR_ONE_RECOUPERATION(),
                ChannelUrlSelector::PUNISHMENT_FACTOR());

    MockLocalChannelUrlDirectory* mockDir = new MockLocalChannelUrlDirectory();
    QSharedPointer<ILocalChannelUrlDirectory> mockDirectory(mockDir);
    QSettings *qsettings = new QSettings(settingsFileName, QSettings::IniFormat);
    MessagingSettings *settings = new MessagingSettings(*qsettings);
    urlCache->init(
                mockDirectory,
                *settings);

    EXPECT_CALL(*mockDir, getUrlsForChannel(
                    A<const std::string&>(),
                    A<const qint64&>(),
                    A<std::function<void(
                        const RequestStatus& status,
                        const types::ChannelUrlInformation& urls)>>()))
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
    // MessageSettings requires a message loop to delete qsettings.
    // QSettings must still exist when the destructor of MessageSettings is called.
    // When unittesting we have to delete manually because there is no message loop.
    delete settings;
    delete qsettings;
}


TEST(ChannelUrlSelectorTest, obtainUrlRetriesUrlOfHigherPriority) {
    const QString bounceProxyBaseUrl = "http://www.UrlTest.org/pseudoBp";
    const QString settingsFileName("test-resources/ChannelUrlSelectorTest.settings");
    qint64 timeForOneRecouperation = 1000; //half a minute
    double punishmentFactor = 0.4;//three punishments will lead to a try of the second Url
    BounceProxyUrl bounceProxyUrl(bounceProxyBaseUrl);

    ChannelUrlSelector* urlCache = new ChannelUrlSelector(
                bounceProxyUrl,
                timeForOneRecouperation,
                punishmentFactor);

    MockLocalChannelUrlDirectory* mockDir = new MockLocalChannelUrlDirectory();
    QSharedPointer<ILocalChannelUrlDirectory>  mockDirectory(mockDir);
    QSettings* qsettings = new QSettings(settingsFileName, QSettings::IniFormat);
    MessagingSettings *settings = new MessagingSettings(*qsettings);
    urlCache->init(
                mockDirectory,
                *settings);

    EXPECT_CALL(*mockDir, getUrlsForChannel(
                    A<const std::string&>(),
                    A<const qint64&>(),
                    A<std::function<void(
                        const RequestStatus& status,
                        const types::ChannelUrlInformation& urls)>>()))
            .WillOnce(WithArgs<0,1>(Invoke(pseudoGetChannelUrls)));

    RequestStatus* status = new RequestStatus();
    QString channelId = "testChannelId";
    QString url = urlCache->obtainUrl(channelId,*status, 20000);

    urlCache->feedback(false,channelId,url);
    urlCache->feedback(false,channelId,url);
    urlCache->feedback(false,channelId,url);
    url = urlCache->obtainUrl(channelId,*status, 20000);
    EXPECT_EQ("secondUrl/message/", url);

    QThreadSleep::msleep(timeForOneRecouperation + 100);
    url = urlCache->obtainUrl(channelId,*status, 20000);
    EXPECT_EQ("firstUrl/message/", url);

    delete status;
    delete urlCache;
    // MessageSettings requires a message loop to delete qsettings.
    // QSettings must still exist when the destructor of MessageSettings is called.
    // When unittesting we have to delete manually because there is no message loop.
    delete settings;
    delete qsettings;
}


TEST(ChannelUrlSelectorTest, initFitnessTest) {
    types::QtChannelUrlInformation urlInformation;
    QList<QString> urls;
    urls << "firstUrl" << "secondUrl" << "thirdUrl";
    urlInformation.setUrls(urls);
    ChannelUrlSelectorEntry* entry = new ChannelUrlSelectorEntry(
                urlInformation,
                ChannelUrlSelector::PUNISHMENT_FACTOR(),
                ChannelUrlSelector::TIME_FOR_ONE_RECOUPERATION());
    QList<double> fitness = entry->getFitness();
    EXPECT_EQ(3,fitness.size());
    EXPECT_EQ(3,fitness.at(0));
    EXPECT_EQ(2,fitness.at(1));
    EXPECT_EQ(1,fitness.at(2));
    delete entry;
}

TEST(ChannelUrlSelectorTest, punishTest) {
    types::QtChannelUrlInformation urlInformation;
    QList<QString> urls;
    urls << "firstUrl" << "secondUrl" << "thirdUrl";
    urlInformation.setUrls(urls);
    double punishmentFactor = 0.4;
    ChannelUrlSelectorEntry* entry = new ChannelUrlSelectorEntry(
                urlInformation,
                ChannelUrlSelector::PUNISHMENT_FACTOR(),
                ChannelUrlSelector::TIME_FOR_ONE_RECOUPERATION());

    QList<double> fitness = entry->getFitness();
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

    types::QtChannelUrlInformation urlInformation;
    QList<QString> urls;
    urls << "firstUrl" << "secondUrl" << "thirdUrl";
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
    QList<double> fitness = entry->getFitness();
    EXPECT_EQ(3 - punishmentFactor,fitness.at(0));
    EXPECT_EQ(2 - punishmentFactor,fitness.at(1));
    EXPECT_EQ(1 - punishmentFactor - punishmentFactor,fitness.at(2));

    QThreadSleep::msleep(timeForOneRecouperation /3);
    entry->updateFitness();
    fitness = entry->getFitness();
    EXPECT_EQ(3 - punishmentFactor,fitness.at(0));
    EXPECT_EQ(2 - punishmentFactor,fitness.at(1));
    EXPECT_EQ(1 - punishmentFactor - punishmentFactor, fitness.at(2));

    QThreadSleep::msleep(timeForOneRecouperation );
    entry->updateFitness();
    fitness = entry->getFitness();
    EXPECT_EQ(3,fitness.at(0));
    EXPECT_EQ(2,fitness.at(1));
    EXPECT_EQ(1 - punishmentFactor,fitness.at(2));

    QThreadSleep::msleep(timeForOneRecouperation);
    entry->updateFitness();
    fitness = entry->getFitness();
    EXPECT_EQ(3,fitness.at(0));
    EXPECT_EQ(2,fitness.at(1));
    EXPECT_EQ(1,fitness.at(2));

    QThreadSleep::msleep(timeForOneRecouperation );
    entry->updateFitness();
    fitness = entry->getFitness();
    EXPECT_EQ(3,fitness.at(0));
    EXPECT_EQ(2,fitness.at(1));
    EXPECT_EQ(1,fitness.at(2));


    entry->punish("firstUrl");
    entry->punish("secondUrl");
    entry->punish("thirdUrl");
    entry->punish("thirdUrl");

    QThreadSleep::msleep( 2 * timeForOneRecouperation + 10 );
    entry->updateFitness();
    fitness = entry->getFitness();
    EXPECT_EQ(3,fitness.at(0));
    EXPECT_EQ(2,fitness.at(1));
    EXPECT_EQ(1,fitness.at(2));

    delete entry;
}


TEST(ChannelUrlSelectorTest, bestTest) {

    types::QtChannelUrlInformation urlInformation;
    QList<QString> urls;
    urls << "firstUrl" << "secondUrl" << "thirdUrl";
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
    QThreadSleep::msleep(timeForOneRecouperation + 100);
    EXPECT_EQ("secondUrl", entry->best());
    QThreadSleep::msleep(timeForOneRecouperation + 100);
    EXPECT_EQ("firstUrl", entry->best());

    delete entry;
}


