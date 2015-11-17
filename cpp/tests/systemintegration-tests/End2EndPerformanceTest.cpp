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
#include "joynr/PrivateCopyAssign.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include "tests/utils/MockObjects.h"
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "joynr/tests/testProxy.h"
#include "joynr/Future.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/ThreadUtil.h"
#include "joynr/LibjoynrSettings.h"

using namespace ::testing;

using namespace joynr;
using namespace joynr_logging;


/*
  * This test tries to create two combined Runtimes and will test communication
  * between the two Runtimes via HttpReceiver
  *
  */

class End2EndPerformanceTest : public Test {
public:
    JoynrClusterControllerRuntime* runtime1;
    JoynrClusterControllerRuntime* runtime2;
    Settings settings1;
    Settings settings2;
    std::string baseUuid;
    std::string uuid;
    std::string domain;

    End2EndPerformanceTest() :
        runtime1(NULL),
        runtime2(NULL),
        settings1("test-resources/SystemIntegrationTest1.settings"),
        settings2("test-resources/SystemIntegrationTest2.settings"),
        baseUuid(TypeUtil::toStd(QUuid::createUuid().toString())),
        uuid( "_" + baseUuid.substr(1, baseUuid.length()-2)),
        domain("cppEnd2EndPerformancesTestDomain" + uuid)
    {

        Settings* settings_1 = new Settings("test-resources/SystemIntegrationTest1.settings");
        Settings integration1Settings{"test-resources/libjoynrSystemIntegration1.settings"};
        Settings::merge(integration1Settings, *settings_1, false);
        runtime1 = new JoynrClusterControllerRuntime(NULL, settings_1);
        Settings* settings_2 = new Settings("test-resources/SystemIntegrationTest2.settings");
        Settings integration2Settings{"test-resources/libjoynrSystemIntegration2.settings"};
        Settings::merge(integration2Settings, *settings_2, false);
        runtime2 = new JoynrClusterControllerRuntime(NULL, settings_2);
    }

    void SetUp() {
        runtime1->start();
        runtime2->start();
    }

    void TearDown() {
        bool deleteChannel = true;
        runtime1->stop(deleteChannel);
        runtime2->stop(deleteChannel);

        // Remove participant id persistence file
        QFile::remove(TypeUtil::toQt(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME()));
    }

    ~End2EndPerformanceTest(){
        delete runtime1;
        delete runtime2;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(End2EndPerformanceTest);

};


TEST_F(End2EndPerformanceTest, sendManyRequests) {

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    std::shared_ptr<tests::testProvider> testProvider(new MockTestProvider(providerQos));

    runtime1->registerProvider<tests::testProvider>(domain, testProvider);

    ThreadUtil::sleepForMillis(2000);


    ProxyBuilder<tests::testProxy>* testProxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domain);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    qlonglong qosRoundTripTTL = 50000;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());
    uint64_t startTime = DispatcherUtils::nowInMilliseconds();
    QList<std::shared_ptr<Future<int> > >testFutureList;
    int numberOfMessages = 150;
    int successFullMessages = 0;
    for (int i=0; i<numberOfMessages; i++){
        std::vector<int> list;
        list.push_back(2);
        list.push_back(4);
        list.push_back(8);
        list.push_back(i);
        testFutureList.append(testProxy->sumIntsAsync(list));
    }

    for (int i=0; i<numberOfMessages; i++){
        testFutureList.at(i)->wait();
        int expectedValue = 2+4+8+i;
        if (testFutureList.at(i)->getStatus().successful()) {
            successFullMessages++;
            int actualValue;
            testFutureList.at(i)->get(actualValue);
            EXPECT_EQ(expectedValue, actualValue);
        }
    }
    uint64_t stopTime = DispatcherUtils::nowInMilliseconds();
    //check if all Messages were received:
    EXPECT_EQ(numberOfMessages, successFullMessages);
    Logger* logger = Logging::getInstance()->getLogger("TEST", "CombinedEnd2EndTest");
    LOG_INFO(logger,"Required Time for 1000 Messages: " + QString::number(TypeUtil::toQt(stopTime - startTime)));
}

