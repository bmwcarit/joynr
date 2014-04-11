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
#include "tests/utils/MockObjects.h"
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "joynr/vehicle/GpsProxy.h"
#include "joynr/tests/TestProxy.h"
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
  * between the two Runtimes via HttpCommunicationManager
  *
  */

class End2EndPerformanceTest : public Test {
public:
    JoynrClusterControllerRuntime* runtime1;
    JoynrClusterControllerRuntime* runtime2;
    QSettings settings1;
    QSettings settings2;
    QString baseUuid;
    QString uuid;
    QString domain;

    End2EndPerformanceTest() :
        runtime1(NULL),
        runtime2(NULL),
        settings1("test-resources/SystemIntegrationTest1.settings", QSettings::IniFormat),
        settings2("test-resources/SystemIntegrationTest2.settings", QSettings::IniFormat),
        baseUuid(QUuid::createUuid().toString()),
        uuid( "_" + baseUuid.mid(1,baseUuid.length()-2 )),
        domain(QString("cppEnd2EndPerformancesTestDomain") + "_" + uuid)

    {
        QSettings* settings_1 = SettingsMerger::mergeSettings(QString("test-resources/SystemIntegrationTest1.settings"));
        SettingsMerger::mergeSettings(QString("test-resources/libjoynrSystemIntegration1.settings"), settings_1);
        runtime1 = new JoynrClusterControllerRuntime(NULL, settings_1);
        QSettings* settings_2 = SettingsMerger::mergeSettings(QString("test-resources/SystemIntegrationTest2.settings"));
        SettingsMerger::mergeSettings(QString("test-resources/libjoynrSystemIntegration2.settings"), settings_2);
        runtime2 = new JoynrClusterControllerRuntime(NULL, settings_2);
    }

    void SetUp() {
        runtime1->startMessaging();
        runtime1->waitForChannelCreation();
        runtime2->startMessaging();
        runtime2->waitForChannelCreation();
    }

    void TearDown() {
        runtime1->deleteChannel(); //cleanup the channels so they dont remain on the bp
        runtime2->deleteChannel(); //cleanup the channels so they dont remain on the bp
        runtime1->stopMessaging();
        runtime2->stopMessaging();
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
    QSharedPointer<tests::TestProvider> testProvider(new MockTestProvider(providerQos));

    runtime1->registerCapability<tests::TestProvider>(domain,testProvider, QString());

    QThreadSleep::msleep(2000);


    ProxyBuilder<tests::TestProxy>* testProxyBuilder = runtime2->getProxyBuilder<tests::TestProxy>(domain);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    qlonglong qosRoundTripTTL = 50000;
    qlonglong qosCacheDataFreshnessMs = 400000;

    // Send a message and expect to get a result
    QSharedPointer<tests::TestProxy> testProxy(testProxyBuilder
                                               ->setRuntimeQos(MessagingQos(qosRoundTripTTL))
                                               ->setProxyQos(ProxyQos(qosCacheDataFreshnessMs))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    QList<QSharedPointer<Future<int> > >testFutureList;
    int numberOfMessages = 150;
    int successFullMessages = 0;
    for (int i=0; i<numberOfMessages; i++){
        testFutureList.append(QSharedPointer<Future<int> >(new Future<int>() ) );
        QList<int> list;
        list.append(2);
        list.append(4);
        list.append(8);
        list.append(i);
        testProxy->sumInts(testFutureList.at(i), list);
    }

    for (int i=0; i<numberOfMessages; i++){
        testFutureList.at(i)->waitForFinished();
        int expectedValue = 2+4+8+i;
        if (testFutureList.at(i)->getStatus().successful()) {
            successFullMessages++;
            EXPECT_EQ(expectedValue, testFutureList.at(i)->getValue());
        }
    }
    qint64 stopTime = QDateTime::currentMSecsSinceEpoch();
    //check if all Messages were received:
    EXPECT_EQ(numberOfMessages, successFullMessages);
    Logger* logger = Logging::getInstance()->getLogger("TEST", "CombinedEnd2EndTest");
    LOG_INFO(logger,"Required Time for 1000 Messages: " + QString::number(stopTime - startTime));
}

