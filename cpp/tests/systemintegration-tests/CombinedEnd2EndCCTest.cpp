/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

#include "systemintegration-tests/CombinedEnd2EndTest.h"
#include "joynr/LocalChannelUrlDirectory.h"
#include "utils/QThreadSleep.h"

#include <QtConcurrent/QtConcurrent>

using namespace joynr;
using namespace joynr_logging;

TEST_F(CombinedEnd2EndTest, channelUrlProxyGetsNoUrlOnNonRegisteredChannel) {
    ProxyBuilder<infrastructure::ChannelUrlDirectoryProxy>* channelUrlDirectoryProxyBuilder =
            runtime1->createProxyBuilder<infrastructure::ChannelUrlDirectoryProxy>(
                messagingSettings1.getDiscoveryDirectoriesDomain()
            );

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    infrastructure::ChannelUrlDirectoryProxy* channelUrlDirectoryProxy
            = channelUrlDirectoryProxyBuilder
                ->setMessagingQos(MessagingQos(1000))
                ->setCached(true)
                ->setDiscoveryQos(discoveryQos)
                ->build();

    types::ChannelUrlInformation result;
    std::string channelId("test");
    EXPECT_THROW(channelUrlDirectoryProxy->getUrlsForChannel(result, channelId), exceptions::JoynrTimeOutException);
}

TEST_F(CombinedEnd2EndTest, channelUrlProxyRegistersUrlsCorrectly) {
    ProxyBuilder<infrastructure::ChannelUrlDirectoryProxy>* channelUrlDirectoryProxyBuilder =
            runtime1->createProxyBuilder<infrastructure::ChannelUrlDirectoryProxy>(
                messagingSettings1.getDiscoveryDirectoriesDomain()
            );

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    infrastructure::ChannelUrlDirectoryProxy* channelUrlDirectoryProxy
            = channelUrlDirectoryProxyBuilder
                ->setMessagingQos(MessagingQos(20000))
                ->setCached(true)
                ->setDiscoveryQos(discoveryQos)
                ->build();

    // There is a race condition where the actual channel url can be set AFTER the dummy data
    // used for testing. Pause for a short time so that the dummy data is always written
    // last
    QThreadSleep::msleep(2000);

    // Register new channel URLs
    std::string channelId = "bogus_1";
    types::ChannelUrlInformation channelUrlInformation;
    std::vector<std::string> urls = { "bogusTestUrl_1", "bogusTestUrl_2" };
    channelUrlInformation.setUrls(urls);
    try {
        channelUrlDirectoryProxy->registerChannelUrls(
                channelId,
                channelUrlInformation);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE() << "Registering Url was not successful";
    }

    types::ChannelUrlInformation result;
    try {
        channelUrlDirectoryProxy->getUrlsForChannel(result, channelId);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "Requesting Url was not successful";
    }
    EXPECT_EQ(channelUrlInformation,result) << "Returned Url did not match Expected Url";
}



// This test is disabled, because the feature is not yet implemented on the server.
TEST_F(CombinedEnd2EndTest, DISABLED_channelUrlProxyUnRegistersUrlsCorrectly) {
    ProxyBuilder<infrastructure::ChannelUrlDirectoryProxy>* channelUrlDirectoryProxyBuilder =
            runtime1->createProxyBuilder<infrastructure::ChannelUrlDirectoryProxy>(
                messagingSettings1.getDiscoveryDirectoriesDomain()
            );

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    infrastructure::ChannelUrlDirectoryProxy* channelUrlDirectoryProxy
            = channelUrlDirectoryProxyBuilder
                ->setMessagingQos(MessagingQos(10000))
                ->setCached(true)
                ->setDiscoveryQos(discoveryQos)
                ->build();

    std::string channelId = "bogus_3";
    types::ChannelUrlInformation channelUrlInformation;
    std::vector<std::string> urls = { "bogusTestUrl_1", "bogusTestUrl_2" };
    channelUrlInformation.setUrls(urls);
    try {
        channelUrlDirectoryProxy->registerChannelUrls(channelId, channelUrlInformation);
    }  catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "registerChannelUrls was not successful";
    }

    types::ChannelUrlInformation result;
    try {
        channelUrlDirectoryProxy->getUrlsForChannel(result, channelId);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "getUrlsForChannel was not successful";
    }
    EXPECT_EQ(channelUrlInformation,result);

    try {
        channelUrlDirectoryProxy->unregisterChannelUrls(channelId);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "unregisterChannelUrls was not successful";
    }

    types::ChannelUrlInformation result2;
    try {
        channelUrlDirectoryProxy->getUrlsForChannel(result2, channelId);
        ADD_FAILURE()<< "getUrlsForChannel was successful";
    } catch (exceptions::JoynrException& e) {
    }
    EXPECT_EQ(0,result2.getUrls().size());
}


