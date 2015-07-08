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
#include "joynr/vehicle/GpsProxy.h"
#include "joynr/tests/testProxy.h"
#include "joynr/types/Trip.h"
#include "joynr/types/GpsLocation.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "utils/QThreadSleep.h"
#include "PrettyPrint.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/Future.h"
#include "joynr/SettingsMerger.h"

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
    QSettings settings1;
    QSettings settings2;
    std::string baseUuid;
    std::string uuid;
    std::string domain;

    End2EndPerformanceTest() :
        runtime1(NULL),
        runtime2(NULL),
        settings1("test-resources/SystemIntegrationTest1.settings", QSettings::IniFormat),
        settings2("test-resources/SystemIntegrationTest2.settings", QSettings::IniFormat),
        baseUuid(TypeUtil::convertQStringtoStdString(QUuid::createUuid().toString())),
        uuid( "_" + baseUuid.substr(1, baseUuid.length()-2)),
        domain("cppEnd2EndPerformancesTestDomain" + uuid)
    {
        QSettings* settings_1 = SettingsMerger::mergeSettings(QString("test-resources/SystemIntegrationTest1.settings"));
        SettingsMerger::mergeSettings(QString("test-resources/libjoynrSystemIntegration1.settings"), settings_1);
        runtime1 = new JoynrClusterControllerRuntime(NULL, settings_1);
        QSettings* settings_2 = SettingsMerger::mergeSettings(QString("test-resources/SystemIntegrationTest2.settings"));
        SettingsMerger::mergeSettings(QString("test-resources/libjoynrSystemIntegration2.settings"), settings_2);
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
        QFile::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME());
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

    QThreadSleep::msleep(2000);


    ProxyBuilder<tests::testProxy>* testProxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domain);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    qlonglong qosRoundTripTTL = 50000;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    QList<QSharedPointer<Future<int> > >testFutureList;
    int numberOfMessages = 150;
    int successFullMessages = 0;
    for (int i=0; i<numberOfMessages; i++){
        QList<int> list;
        list.append(2);
        list.append(4);
        list.append(8);
        list.append(i);
        testFutureList.append(testProxy->sumInts(list));
    }

    for (int i=0; i<numberOfMessages; i++){
        testFutureList.at(i)->waitForFinished();
        int expectedValue = 2+4+8+i;
        if (testFutureList.at(i)->getStatus().successful()) {
            successFullMessages++;
            int actualValue;
            testFutureList.at(i)->getValues(actualValue);
            EXPECT_EQ(expectedValue, actualValue);
        }
    }
    qint64 stopTime = QDateTime::currentMSecsSinceEpoch();
    //check if all Messages were received:
    EXPECT_EQ(numberOfMessages, successFullMessages);
    Logger* logger = Logging::getInstance()->getLogger("TEST", "CombinedEnd2EndTest");
    LOG_INFO(logger,"Required Time for 1000 Messages: " + QString::number(stopTime - startTime));
}

