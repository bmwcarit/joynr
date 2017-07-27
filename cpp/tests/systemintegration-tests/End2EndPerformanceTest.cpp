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
#include <memory>
#include <string>
#include <cstdint>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "tests/utils/MockObjects.h"
#include "joynr/JoynrClusterControllerRuntime.h"
#include "joynr/tests/testProxy.h"
#include "joynr/Future.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/PrivateCopyAssign.h"

using namespace ::testing;

using namespace joynr;


/*
  * This test tries to create two combined Runtimes and will test communication
  * between the two Runtimes via HttpReceiver
  *
  */

class End2EndPerformanceTest : public TestWithParam< std::tuple<std::string, std::string> > {
public:
    ADD_LOGGER(End2EndPerformanceTest);
    std::shared_ptr<JoynrClusterControllerRuntime> runtime1;
    std::shared_ptr<JoynrClusterControllerRuntime> runtime2;
    std::unique_ptr<Settings> settings1;
    std::unique_ptr<Settings> settings2;
    std::string baseUuid;
    std::string uuid;
    std::string domain;

    End2EndPerformanceTest() :
        runtime1(),
        runtime2(),
        settings1(std::make_unique<Settings>(std::get<0>(GetParam()))),
        settings2(std::make_unique<Settings>(std::get<1>(GetParam()))),
        baseUuid(util::createUuid()),
        uuid( "_" + baseUuid.substr(1, baseUuid.length()-2)),
        domain("cppEnd2EndPerformancesTestDomain" + uuid)
    {

        Settings integration1Settings{"test-resources/libjoynrSystemIntegration1.settings"};
        Settings::merge(integration1Settings, *settings1, false);
        runtime1 = std::make_shared<JoynrClusterControllerRuntime>(std::move(settings1));
        Settings integration2Settings{"test-resources/libjoynrSystemIntegration2.settings"};
        Settings::merge(integration2Settings, *settings2, false);
        runtime2 = std::make_shared<JoynrClusterControllerRuntime>(std::move(settings2));
    }

    void SetUp() {
        runtime1->start();
        runtime2->start();
    }

    void TearDown() {
        bool deleteChannel = true;
        runtime1->stop(deleteChannel);
        runtime2->stop(deleteChannel);

        // Delete persisted files
        std::remove(ClusterControllerSettings::DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_MESSAGE_ROUTER_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());
    }

    ~End2EndPerformanceTest() = default;

private:
    DISALLOW_COPY_AND_ASSIGN(End2EndPerformanceTest);

};

INIT_LOGGER(End2EndPerformanceTest);

TEST_P(End2EndPerformanceTest, sendManyRequests) {

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    auto testProvider = std::make_shared<MockTestProvider>();

    std::string participantId = runtime1->registerProvider<tests::testProvider>(domain, testProvider, providerQos);

    std::this_thread::sleep_for(std::chrono::seconds(2));


    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            runtime2->createProxyBuilder<tests::testProxy>(domain);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(3000);

    std::int64_t qosRoundTripTTL = 50000;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy = testProxyBuilder
                     ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                     ->setDiscoveryQos(discoveryQos)
                     ->build();
    std::uint64_t startTime = DispatcherUtils::nowInMilliseconds();
    std::vector<std::shared_ptr<Future<int> > >testFutureList;
    int numberOfRequests = 150;
    int successfulRequests = 0;
    for (int i=0; i<numberOfRequests; i++){
        std::vector<int> list;
        list.push_back(2);
        list.push_back(4);
        list.push_back(8);
        list.push_back(i);
        testFutureList.push_back(testProxy->sumIntsAsync(list));
    }

    for (int i=0; i<numberOfRequests; i++){
        testFutureList.at(i)->wait();
        int expectedValue = 2+4+8+i;
        if (testFutureList.at(i)->isOk()) {
            successfulRequests++;
            int actualValue;
            testFutureList.at(i)->get(actualValue);
            EXPECT_EQ(expectedValue, actualValue);
        }
    }
    std::uint64_t stopTime = DispatcherUtils::nowInMilliseconds();
    //check if all Requests were successful
    EXPECT_EQ(numberOfRequests, successfulRequests);
    JOYNR_LOG_INFO(logger, "Required Time for 1000 Requests: {}",(stopTime - startTime));

    runtime1->unregisterProvider(participantId);

    // to silence unused-variable compiler warnings
    std::ignore = startTime;
    std::ignore = stopTime;
    std::ignore = logger;
}
INSTANTIATE_TEST_CASE_P(DISABLED_Http,
        End2EndPerformanceTest,
        testing::Values(
            std::make_tuple("test-resources/HttpSystemIntegrationTest1.settings","test-resources/HttpSystemIntegrationTest2.settings")
        )
);

INSTANTIATE_TEST_CASE_P(Mqtt,
        End2EndPerformanceTest,
        testing::Values(
            std::make_tuple("test-resources/MqttSystemIntegrationTest1.settings","test-resources/MqttSystemIntegrationTest2.settings")
        )
);
